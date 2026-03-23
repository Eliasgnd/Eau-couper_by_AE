#include "CustomDrawArea.h"

#include "drawing/DrawModeManager.h"
#include "drawing/EraserTool.h"
#include "drawing/HistoryManager.h"
#include "drawing/MouseInteractionHandler.h"
#include "drawing/PathGenerator.h"
#include "drawing/ShapeManager.h"
#include "drawing/ShapeRenderer.h"
#include "drawing/TextTool.h"
#include "drawing/ViewTransformer.h"

#include <QInputDialog>
#include <QLineF>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QWheelEvent>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

CustomDrawArea::CustomDrawArea(QWidget *parent)
    : QWidget(parent)
    , m_shapeManager(std::make_unique<ShapeManager>(this))
    , m_renderer(std::make_unique<ShapeRenderer>(this))
    , m_modeManager(std::make_unique<DrawModeManager>(this))
    , m_historyManager(std::make_unique<HistoryManager>(m_shapeManager.get(), this))
    , m_transformer(std::make_unique<ViewTransformer>(this))
    , m_mouseHandler(std::make_unique<MouseInteractionHandler>(
          m_shapeManager.get(), m_modeManager.get(), m_transformer.get(), this))
    , m_eraserTool(std::make_unique<EraserTool>(m_shapeManager.get(), this))
    , m_textTool(std::make_unique<TextTool>(this))
{
    setMouseTracking(true);
    setAutoFillBackground(true);

    connect(m_shapeManager.get(), &ShapeManager::shapesChanged, this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_shapeManager.get(), &ShapeManager::selectionChanged, this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_mouseHandler.get(), &MouseInteractionHandler::requestUpdate, this, QOverload<>::of(&CustomDrawArea::update));

    connect(m_modeManager.get(), &DrawModeManager::drawModeChanged, this, &CustomDrawArea::onDrawModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::closeModeChanged, this, &CustomDrawArea::closeModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::deplacerModeChanged, this, &CustomDrawArea::deplacerModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::supprimerModeChanged, this, &CustomDrawArea::supprimerModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::gommeModeChanged, this, &CustomDrawArea::gommeModeChanged);

    connect(m_transformer.get(), &ViewTransformer::zoomChanged, this, &CustomDrawArea::zoomChanged);
    connect(m_transformer.get(), &ViewTransformer::viewTransformed, this, QOverload<>::of(&CustomDrawArea::update));

    Q_ASSERT(m_historyManager != nullptr);
    m_historyManager->pushState();
}

void CustomDrawArea::setDrawMode(DrawMode mode)
{
    Q_ASSERT(m_modeManager != nullptr);
    if (m_selectMode) cancelSelection();
    if (m_closeMode) cancelCloseMode();
    m_gommeErasing = false;
    m_pointByPointPoints.clear();
    m_drawing = false;
    m_modeManager->setDrawMode(mode);
}
CustomDrawArea::DrawMode CustomDrawArea::getDrawMode() const
{
    Q_ASSERT(m_modeManager != nullptr);
    return m_modeManager->drawMode();
}
void CustomDrawArea::restorePreviousMode()
{
    Q_ASSERT(m_modeManager != nullptr);
    m_modeManager->restorePreviousMode();
}

QList<QPolygonF> CustomDrawArea::getCustomShapes() const
{
    Q_ASSERT(m_shapeManager != nullptr);
    QList<QPolygonF> polygons;
    polygons.reserve(m_shapeManager->shapes().size());
    for (const auto &shape : m_shapeManager->shapes()) polygons.append(shape.path.toFillPolygon());
    return polygons;
}

void CustomDrawArea::clearDrawing()
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_historyManager != nullptr);
    m_shapeManager->clearShapes();
    m_historyManager->pushState();
}
void CustomDrawArea::undoLastAction()
{
    Q_ASSERT(m_historyManager != nullptr);
    m_historyManager->undoLastAction();
}
void CustomDrawArea::setEraserRadius(qreal radius)
{
    Q_ASSERT(m_eraserTool != nullptr);
    m_eraserTool->setEraserRadius(radius);
}
void CustomDrawArea::addImportedLogo(const QPainterPath &logoPath)
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_historyManager != nullptr);
    m_shapeManager->addImportedLogo(logoPath, m_nextShapeId++);
    m_historyManager->pushState();
    update();
}
void CustomDrawArea::addImportedLogoSubpath(const QPainterPath &subpath)
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_historyManager != nullptr);
    m_shapeManager->addImportedLogoSubpath(subpath, m_nextShapeId++);
    m_historyManager->pushState();
    update();
}
void CustomDrawArea::setTextFont(const QFont &font)
{
    Q_ASSERT(m_textTool != nullptr);
    m_textTool->setTextFont(font);
}
QFont CustomDrawArea::getTextFont() const
{
    Q_ASSERT(m_textTool != nullptr);
    return m_textTool->getTextFont();
}

void CustomDrawArea::startShapeSelection()
{
    cancelCloseMode();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_selectMode = true;
    m_connectSelectionMode = true;
    m_shapeManager->clearSelection();
    emit shapeSelection(true);
    update();
}

void CustomDrawArea::cancelSelection()
{
    if (!m_selectMode) return;
    m_selectMode = false;
    m_shapeManager->clearSelection();
    if (m_connectSelectionMode) emit shapeSelection(false);
    else emit multiSelectionModeChanged(false);
    m_connectSelectionMode = false;
    update();
}

void CustomDrawArea::toggleMultiSelectMode()
{
    if (!m_selectMode) {
        cancelCloseMode();
        cancelSupprimerMode();
        cancelDeplacerMode();
        cancelGommeMode();
        m_selectMode = true;
        m_connectSelectionMode = false;
        m_shapeManager->clearSelection();
        emit multiSelectionModeChanged(true);
    } else if (!m_connectSelectionMode) {
        cancelSelection();
    }
    update();
}
bool CustomDrawArea::hasSelection() const
{
    Q_ASSERT(m_shapeManager != nullptr);
    return !m_shapeManager->selectedShapes().empty();
}

void CustomDrawArea::deleteSelectedShapes()
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_historyManager != nullptr);
    auto selected = m_shapeManager->selectedShapes();
    std::sort(selected.begin(), selected.end(), std::greater<int>());
    for (int index : selected) m_shapeManager->removeShape(index);
    m_shapeManager->clearSelection();
    if (m_selectMode && !m_connectSelectionMode) emit multiSelectionModeChanged(false);
    m_selectMode = false;
    m_connectSelectionMode = false;
    m_historyManager->pushState();
}

void CustomDrawArea::copySelectedShapes()
{
    Q_ASSERT(m_shapeManager != nullptr);
    m_shapeManager->copySelectedShapes();
}
void CustomDrawArea::enablePasteMode() { m_pasteMode = true; }

void CustomDrawArea::pasteCopiedShapes(const QPointF &dest)
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_historyManager != nullptr);
    const auto pasted = m_shapeManager->pastedShapes(dest);
    for (const auto &shape : pasted) m_shapeManager->addShape(shape.path, m_nextShapeId++);
    m_historyManager->pushState();
}

void CustomDrawArea::setSnapToGridEnabled(bool enabled) { Q_ASSERT(m_renderer != nullptr); m_renderer->setSnapToGridEnabled(enabled); update(); }
bool CustomDrawArea::isSnapToGridEnabled() const { Q_ASSERT(m_renderer != nullptr); return m_renderer->isSnapToGridEnabled(); }
void CustomDrawArea::setGridVisible(bool visible) { Q_ASSERT(m_renderer != nullptr); m_renderer->setGridVisible(visible); update(); }
bool CustomDrawArea::isGridVisible() const { Q_ASSERT(m_renderer != nullptr); return m_renderer->isGridVisible(); }
void CustomDrawArea::setGridSpacing(int px) { Q_ASSERT(m_renderer != nullptr); m_renderer->setGridSpacing(px); update(); }
int CustomDrawArea::gridSpacing() const { Q_ASSERT(m_renderer != nullptr); return m_renderer->gridSpacing(); }

bool CustomDrawArea::isDeplacerMode() const { return getDrawMode() == DrawMode::Deplacer; }
bool CustomDrawArea::isSupprimerMode() const { return getDrawMode() == DrawMode::Supprimer; }
bool CustomDrawArea::isGommeMode() const { return getDrawMode() == DrawMode::Gomme; }
bool CustomDrawArea::isConnectMode() const { return m_selectMode && m_connectSelectionMode; }

void CustomDrawArea::onDrawModeChanged(DrawModeManager::DrawMode mode)
{
    emit drawModeChanged(mode);
    update();
}

QList<QPainterPath> CustomDrawArea::separateIntoSubpaths(const QPainterPath &path)
{
    QList<QPainterPath> subpaths;
    const QList<QPolygonF> polygons = path.toSubpathPolygons();
    subpaths.reserve(polygons.size());

    for (const QPolygonF &poly : polygons) {
        if (poly.isEmpty()) continue;

        QPainterPath subpath;
        subpath.addPolygon(poly);
        subpaths.append(subpath.simplified());
    }

    return subpaths;
}

void CustomDrawArea::startCloseMode()
{
    cancelSelection();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_closeMode = true;
    m_modeManager->startCloseMode();
}
void CustomDrawArea::cancelCloseMode()
{
    if (!m_closeMode) return;
    m_closeMode = false;
    m_modeManager->cancelCloseMode();
}
void CustomDrawArea::startDeplacerMode()
{
    cancelSelection();
    cancelCloseMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_modeManager->startDeplacerMode();
}
void CustomDrawArea::cancelDeplacerMode() { m_modeManager->cancelDeplacerMode(); }
void CustomDrawArea::startSupprimerMode()
{
    cancelSelection();
    cancelCloseMode();
    cancelDeplacerMode();
    cancelGommeMode();
    m_modeManager->startSupprimerMode();
}
void CustomDrawArea::cancelSupprimerMode() { m_modeManager->cancelSupprimerMode(); }
void CustomDrawArea::startGommeMode()
{
    cancelSelection();
    cancelCloseMode();
    cancelDeplacerMode();
    cancelSupprimerMode();
    m_modeManager->startGommeMode();
}
void CustomDrawArea::cancelGommeMode() { m_modeManager->cancelGommeMode(); }
void CustomDrawArea::setSmoothingLevel(int level) { m_smoothingLevel = qMax(0, level); emit smoothingLevelChanged(m_smoothingLevel); }
void CustomDrawArea::setTwoFingersOn(bool active)
{
    m_twoFingersOn = active;
    if (active) m_gommeErasing = false;
}

void CustomDrawArea::handlePinchZoom(QPointF center, qreal factor)
{
    const qreal oldScale = m_transformer->scale();
    const QPointF logicalCenter = toLogical(center);
    m_transformer->setScale(oldScale * factor);
    m_transformer->setOffset(center - logicalCenter * m_transformer->scale());
}

QPointF CustomDrawArea::toLogical(const QPointF &widgetPoint) const
{
    Q_ASSERT(m_transformer != nullptr);
    return (widgetPoint - m_transformer->offset()) / m_transformer->scale();
}

QPointF CustomDrawArea::snapToGridIfNeeded(const QPointF &logicalPoint) const
{
    Q_ASSERT(m_renderer != nullptr);
    if (!m_renderer->isSnapToGridEnabled()) return logicalPoint;
    const int spacing = qMax(1, m_renderer->gridSpacing());
    return QPointF(std::round(logicalPoint.x() / spacing) * spacing,
                   std::round(logicalPoint.y() / spacing) * spacing);
}

int CustomDrawArea::hitTestShape(const QPointF &logicalPoint, qreal tolerance) const
{
    Q_ASSERT(m_shapeManager != nullptr);
    const auto &shapes = m_shapeManager->shapes();
    QPainterPathStroker stroker;
    stroker.setWidth(tolerance);
    for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i) {
        if (stroker.createStroke(shapes[i].path).contains(logicalPoint)) return i;
    }
    return -1;
}

void CustomDrawArea::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_renderer != nullptr);
    Q_ASSERT(m_transformer != nullptr);
    Q_ASSERT(m_eraserTool != nullptr);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.translate(m_transformer->offset());
    painter.scale(m_transformer->scale(), m_transformer->scale());

    const QRectF visibleArea = QRectF(toLogical(QPointF(0, 0)), toLogical(QPointF(width(), height()))).normalized();
    m_renderer->render(painter, *m_shapeManager, visibleArea);

    if (!m_shapeManager->selectedShapes().empty()) {
        QPen halo(Qt::cyan, 6);
        halo.setCosmetic(true);
        QPen normal(Qt::black, 2);
        normal.setCosmetic(true);
        for (int idx : m_shapeManager->selectedShapes()) {
            if (idx < 0 || idx >= m_shapeManager->shapes().size()) continue;
            const QPainterPath &path = m_shapeManager->shapes()[idx].path;
            painter.setPen(halo);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
            painter.setPen(normal);
            painter.drawPath(path);
        }
    }

    if (getDrawMode() == DrawMode::Freehand && m_drawing && m_strokePoints.size() > 1) {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        painter.drawPath(PathGenerator::generateBezierPath(m_strokePoints));
    }

    if (m_drawing && (getDrawMode() == DrawMode::Line || getDrawMode() == DrawMode::Rectangle ||
                      getDrawMode() == DrawMode::Circle)) {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        if (getDrawMode() == DrawMode::Line) {
            painter.drawLine(m_startPoint, m_currentPoint);
        } else if (getDrawMode() == DrawMode::Rectangle) {
            painter.drawRect(QRectF(m_startPoint, m_currentPoint).normalized());
        } else if (getDrawMode() == DrawMode::Circle) {
            painter.drawEllipse(QRectF(m_startPoint, m_currentPoint).normalized());
        }
    }

    if (getDrawMode() == DrawMode::PointParPoint && !m_pointByPointPoints.isEmpty()) {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        QPainterPath previewPath;
        previewPath.moveTo(m_pointByPointPoints.first());
        for (int i = 1; i < m_pointByPointPoints.size(); ++i) previewPath.lineTo(m_pointByPointPoints[i]);
        previewPath.lineTo(m_currentPoint);
        painter.drawPath(previewPath);
    }

    if (getDrawMode() == DrawMode::Gomme && m_gommeErasing) {
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(m_gommeCenter, m_eraserTool->eraserRadius(), m_eraserTool->eraserRadius());
    }
}

void CustomDrawArea::mousePressEvent(QMouseEvent *event)
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_historyManager != nullptr);
    Q_ASSERT(m_mouseHandler != nullptr);
    Q_ASSERT(m_textTool != nullptr);
    Q_ASSERT(m_eraserTool != nullptr);
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
    m_currentPoint = logical;

    if (m_closeMode) {
        const int hit = hitTestShape(logical);
        if (hit >= 0) {
            auto shapes = m_shapeManager->shapes();
            QPainterPath &path = shapes[hit].path;
            if (path.elementCount() > 1) {
                const auto first = path.elementAt(0);
                path.lineTo(QPointF(first.x, first.y));
                m_historyManager->pushState();
                m_shapeManager->setShapes(shapes);
            }
        }
        m_closeMode = false;
        emit closeModeChanged(false);
        update();
        return;
    }

    if (event->button() == Qt::RightButton && m_pasteMode) {
        pasteCopiedShapes(logical);
        m_pasteMode = false;
        return;
    }

    if (event->button() == Qt::RightButton && getDrawMode() == DrawMode::PointParPoint) {
        m_pointByPointPoints.clear();
        m_drawing = false;
        update();
        return;
    }

    if (m_selectMode && getDrawMode() != DrawMode::Deplacer) {
        const int hit = hitTestShape(logical);
        if (hit >= 0) {
            m_lastSelectClick = logical;
            std::vector<int> selected = m_shapeManager->selectedShapes();
            const bool alreadySelected = std::find(selected.begin(), selected.end(), hit) != selected.end();
            if (alreadySelected) {
                selected.erase(std::remove(selected.begin(), selected.end(), hit), selected.end());
            } else {
                selected.push_back(hit);
            }
            m_shapeManager->setSelectedShapes(selected);

            if (m_connectSelectionMode && selected.size() == 2) {
                m_shapeManager->connectNearestEndpoints(selected[0], selected[1]);
                m_shapeManager->mergeShapesAndConnector(selected[0], selected[1]);
                m_selectMode = false;
                m_connectSelectionMode = false;
                m_shapeManager->clearSelection();
                emit shapeSelection(false);
            } else if (!m_connectSelectionMode) {
                emit multiSelectionModeChanged(true);
            }
            update();
        }
        return;
    }

    if (event->button() == Qt::LeftButton &&
        (getDrawMode() == DrawMode::Text || getDrawMode() == DrawMode::ThinText)) {
        bool ok = false;
        const QString text = QInputDialog::getText(this, tr("Texte"), tr("Saisir le texte"), QLineEdit::Normal,
                                                   m_textTool->getCurrentText(), &ok);
        if (ok && !text.isEmpty()) {
            m_textTool->setCurrentText(text);
            QPainterPath textPath;
            textPath.addText(logical, m_textTool->getTextFont(), text);
            m_shapeManager->addShape(textPath, m_nextShapeId++);
            m_historyManager->pushState();
        }
        return;
    }

    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::PointParPoint) {
        m_pointByPointPoints.append(logical);
        m_drawing = true;
        update();
        return;
    }

    m_mouseHandler->handleMousePress(event, logical);

    m_drawing = (event->button() == Qt::LeftButton);
    m_startPoint = logical;
    m_strokePoints = {logical};

    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::Gomme) {
        m_historyManager->pushState();
        m_gommeErasing = true;
        m_gommeCenter = logical;
        m_lastEraserPos = logical;
        m_eraserTool->applyEraserAt(logical);
        update();
    }
}

void CustomDrawArea::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_ASSERT(m_shapeManager != nullptr);
    Q_ASSERT(m_historyManager != nullptr);
    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::PointParPoint) {
        const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
        if (m_pointByPointPoints.isEmpty()) return;

        const qreal minDistance = 0.5;
        if (QLineF(m_pointByPointPoints.last(), logical).length() > minDistance) {
            m_pointByPointPoints.append(logical);
        }

        if (m_pointByPointPoints.size() >= 2) {
            QPainterPath path;
            path.moveTo(m_pointByPointPoints.first());
            for (int i = 1; i < m_pointByPointPoints.size(); ++i) path.lineTo(m_pointByPointPoints[i]);
            m_shapeManager->addShape(path, m_nextShapeId++);
            m_historyManager->pushState();
        }

        m_pointByPointPoints.clear();
        m_drawing = false;
        update();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void CustomDrawArea::mouseMoveEvent(QMouseEvent *event)
{
    Q_ASSERT(m_mouseHandler != nullptr);
    Q_ASSERT(m_eraserTool != nullptr);
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));

    if (getDrawMode() == DrawMode::PointParPoint && !m_pointByPointPoints.isEmpty()) {
        m_currentPoint = logical;
        update();
    }

    m_mouseHandler->handleMouseMove(event, logical);
    if (!m_drawing) return;

    if (getDrawMode() == DrawMode::Deplacer || getDrawMode() == DrawMode::Supprimer ||
        getDrawMode() == DrawMode::Pan) {
        return;
    }

    m_currentPoint = logical;
    if (getDrawMode() == DrawMode::Gomme) {
        if (!m_gommeErasing) return;
        m_gommeCenter = logical;
        m_eraserTool->eraseAlong(m_lastEraserPos, logical);
        m_lastEraserPos = logical;
    } else if (getDrawMode() == DrawMode::Freehand) {
        m_strokePoints.append(logical);
    }
    update();
}

void CustomDrawArea::mouseReleaseEvent(QMouseEvent *event)
{
    Q_ASSERT(m_mouseHandler != nullptr);
    Q_ASSERT(m_historyManager != nullptr);
    Q_ASSERT(m_shapeManager != nullptr);
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));

    if (getDrawMode() == DrawMode::PointParPoint) {
        m_currentPoint = logical;
        update();
        return;
    }

    m_mouseHandler->handleMouseRelease(event, logical);
    if (!m_drawing) return;

    m_drawing = false;
    if (getDrawMode() == DrawMode::Deplacer || getDrawMode() == DrawMode::Supprimer ||
        getDrawMode() == DrawMode::Pan) {
        m_historyManager->pushState();
        return;
    }

    if (getDrawMode() == DrawMode::Gomme) {
        m_gommeErasing = false;
        return;
    }

    QPainterPath path;
    if (getDrawMode() == DrawMode::Line) {
        path.moveTo(m_startPoint);
        path.lineTo(logical);
    } else if (getDrawMode() == DrawMode::Rectangle) {
        path.addRect(QRectF(m_startPoint, logical).normalized());
    } else if (getDrawMode() == DrawMode::Circle) {
        path.addEllipse(QRectF(m_startPoint, logical).normalized());
    } else {
        path = PathGenerator::generateBezierPath(
            PathGenerator::applyChaikinAlgorithm(m_strokePoints, m_smoothingLevel));
    }

    if (!path.isEmpty()) {
        m_shapeManager->addShape(path, m_nextShapeId++);
        m_historyManager->pushState();
    }
}

void CustomDrawArea::wheelEvent(QWheelEvent *event)
{
    Q_ASSERT(m_transformer != nullptr);
    const qreal factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    const QPointF cursor = event->position();
    const QPointF logicalAtCursor = toLogical(cursor);

    m_transformer->setScale(m_transformer->scale() * factor);
    m_transformer->setOffset(cursor - logicalAtCursor * m_transformer->scale());
    event->accept();
}
