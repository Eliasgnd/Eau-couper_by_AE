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
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

CustomDrawArea::CustomDrawArea(QWidget *parent)
    : QWidget(parent)
    , m_shapeManager(new ShapeManager(this))
    , m_renderer(new ShapeRenderer(this))
    , m_modeManager(new DrawModeManager(this))
    , m_historyManager(new HistoryManager(m_shapeManager, this))
    , m_transformer(new ViewTransformer(this))
    , m_mouseHandler(new MouseInteractionHandler(m_shapeManager, m_modeManager, m_transformer, this))
    , m_eraserTool(new EraserTool(m_shapeManager, this))
    , m_textTool(new TextTool(this))
{
    setMouseTracking(true);
    setAutoFillBackground(true);

    connect(m_shapeManager, &ShapeManager::shapesChanged, this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_shapeManager, &ShapeManager::selectionChanged, this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_mouseHandler, &MouseInteractionHandler::requestUpdate, this, QOverload<>::of(&CustomDrawArea::update));

    connect(m_modeManager, &DrawModeManager::drawModeChanged, this, [this](DrawModeManager::DrawMode mode) {
        emit drawModeChanged(mode);
        update();
    });
    connect(m_modeManager, &DrawModeManager::closeModeChanged, this, &CustomDrawArea::closeModeChanged);
    connect(m_modeManager, &DrawModeManager::deplacerModeChanged, this, &CustomDrawArea::deplacerModeChanged);
    connect(m_modeManager, &DrawModeManager::supprimerModeChanged, this, &CustomDrawArea::supprimerModeChanged);
    connect(m_modeManager, &DrawModeManager::gommeModeChanged, this, &CustomDrawArea::gommeModeChanged);

    connect(m_transformer, &ViewTransformer::zoomChanged, this, &CustomDrawArea::zoomChanged);
    connect(m_transformer, &ViewTransformer::viewTransformed, this, QOverload<>::of(&CustomDrawArea::update));

    m_historyManager->pushState();
}

void CustomDrawArea::setDrawMode(DrawMode mode)
{
    if (m_selectMode) cancelSelection();
    if (m_closeMode) cancelCloseMode();
    m_gommeErasing = false;
    m_modeManager->setDrawMode(mode);
}
CustomDrawArea::DrawMode CustomDrawArea::getDrawMode() const { return m_modeManager->drawMode(); }
void CustomDrawArea::restorePreviousMode() { m_modeManager->restorePreviousMode(); }

QList<QPolygonF> CustomDrawArea::getCustomShapes() const
{
    QList<QPolygonF> polygons;
    polygons.reserve(m_shapeManager->shapes().size());
    for (const auto &shape : m_shapeManager->shapes()) polygons.append(shape.path.toFillPolygon());
    return polygons;
}

void CustomDrawArea::clearDrawing() { m_shapeManager->clearShapes(); m_historyManager->pushState(); }
void CustomDrawArea::undoLastAction() { m_historyManager->undoLastAction(); }
void CustomDrawArea::setEraserRadius(qreal radius) { m_eraserTool->setEraserRadius(radius); }
void CustomDrawArea::addImportedLogo(const QPainterPath &logoPath) { m_shapeManager->addImportedLogo(logoPath, m_nextShapeId++); }
void CustomDrawArea::addImportedLogoSubpath(const QPainterPath &subpath) { m_shapeManager->addImportedLogoSubpath(subpath, m_nextShapeId++); }
void CustomDrawArea::setTextFont(const QFont &font) { m_textTool->setTextFont(font); }
QFont CustomDrawArea::getTextFont() const { return m_textTool->getTextFont(); }

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
bool CustomDrawArea::hasSelection() const { return !m_shapeManager->selectedShapes().isEmpty(); }

void CustomDrawArea::deleteSelectedShapes()
{
    auto selected = m_shapeManager->selectedShapes();
    std::sort(selected.begin(), selected.end(), std::greater<int>());
    for (int index : selected) m_shapeManager->removeShape(index);
    m_shapeManager->clearSelection();
    if (m_selectMode && !m_connectSelectionMode) emit multiSelectionModeChanged(false);
    m_selectMode = false;
    m_connectSelectionMode = false;
    m_historyManager->pushState();
}

void CustomDrawArea::copySelectedShapes() { m_shapeManager->copySelectedShapes(); }
void CustomDrawArea::enablePasteMode() { m_pasteMode = true; }

void CustomDrawArea::pasteCopiedShapes(const QPointF &dest)
{
    const auto pasted = m_shapeManager->pastedShapes(dest);
    for (const auto &shape : pasted) m_shapeManager->addShape(shape.path, m_nextShapeId++);
    m_historyManager->pushState();
}

void CustomDrawArea::setSnapToGridEnabled(bool enabled) { m_renderer->setSnapToGridEnabled(enabled); update(); }
bool CustomDrawArea::isSnapToGridEnabled() const { return m_renderer->isSnapToGridEnabled(); }
void CustomDrawArea::setGridVisible(bool visible) { m_renderer->setGridVisible(visible); update(); }
bool CustomDrawArea::isGridVisible() const { return m_renderer->isGridVisible(); }
void CustomDrawArea::setGridSpacing(int px) { m_renderer->setGridSpacing(px); update(); }
int CustomDrawArea::gridSpacing() const { return m_renderer->gridSpacing(); }

bool CustomDrawArea::isDeplacerMode() const { return getDrawMode() == DrawMode::Deplacer; }
bool CustomDrawArea::isSupprimerMode() const { return getDrawMode() == DrawMode::Supprimer; }
bool CustomDrawArea::isGommeMode() const { return getDrawMode() == DrawMode::Gomme; }
bool CustomDrawArea::isConnectMode() const { return m_selectMode && m_connectSelectionMode; }

QList<QPainterPath> CustomDrawArea::separateIntoSubpaths(const QPainterPath &path)
{
    QList<QPainterPath> subpaths;
    QPainterPath current;
    for (int i = 0; i < path.elementCount(); ++i) {
        const auto e = path.elementAt(i);
        if (e.isMoveTo()) {
            if (!current.isEmpty()) subpaths.append(current);
            current = QPainterPath();
            current.moveTo(e.x, e.y);
        } else {
            current.lineTo(e.x, e.y);
        }
    }
    if (!current.isEmpty()) subpaths.append(current);
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
    return (widgetPoint - m_transformer->offset()) / m_transformer->scale();
}

QPointF CustomDrawArea::snapToGridIfNeeded(const QPointF &logicalPoint) const
{
    if (!m_renderer->isSnapToGridEnabled()) return logicalPoint;
    const int spacing = qMax(1, m_renderer->gridSpacing());
    return QPointF(std::round(logicalPoint.x() / spacing) * spacing,
                   std::round(logicalPoint.y() / spacing) * spacing);
}

int CustomDrawArea::hitTestShape(const QPointF &logicalPoint, qreal tolerance) const
{
    const auto &shapes = m_shapeManager->shapes();
    QPainterPathStroker stroker;
    stroker.setWidth(tolerance);
    for (int i = shapes.size() - 1; i >= 0; --i) {
        if (stroker.createStroke(shapes[i].path).contains(logicalPoint)) return i;
    }
    return -1;
}

void CustomDrawArea::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.translate(m_transformer->offset());
    painter.scale(m_transformer->scale(), m_transformer->scale());

    const QRectF visibleArea = QRectF(toLogical(QPointF(0, 0)), toLogical(QPointF(width(), height()))).normalized();
    m_renderer->render(painter, *m_shapeManager, visibleArea);

    if (!m_shapeManager->selectedShapes().isEmpty()) {
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

    if (m_drawing && m_strokePoints.size() > 1) {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        painter.drawPath(PathGenerator::generateBezierPath(m_strokePoints));
    }

    if (getDrawMode() == DrawMode::Gomme && m_gommeErasing) {
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(m_gommeCenter, m_eraserTool->eraserRadius(), m_eraserTool->eraserRadius());
    }
}

void CustomDrawArea::mousePressEvent(QMouseEvent *event)
{
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));

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

    if (m_selectMode && getDrawMode() != DrawMode::Deplacer) {
        const int hit = hitTestShape(logical);
        if (hit >= 0) {
            m_lastSelectClick = logical;
            QVector<int> selected = m_shapeManager->selectedShapes();
            const bool alreadySelected = selected.contains(hit);
            if (alreadySelected) selected.removeAll(hit);
            else selected.append(hit);
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

    m_mouseHandler->handleMousePress(event, logical);

    m_drawing = (event->button() == Qt::LeftButton);
    m_startPoint = logical;
    m_currentPoint = logical;
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

void CustomDrawArea::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
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
    } else {
        m_strokePoints.append(logical);
    }
    update();
}

void CustomDrawArea::mouseReleaseEvent(QMouseEvent *event)
{
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
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
    } else if (getDrawMode() == DrawMode::Text || getDrawMode() == DrawMode::ThinText) {
        bool ok = false;
        const QString text = QInputDialog::getText(this, tr("Texte"), tr("Saisir le texte"), QLineEdit::Normal,
                                                   m_textTool->getCurrentText(), &ok);
        if (ok && !text.isEmpty()) {
            m_textTool->setCurrentText(text);
            path.addText(logical, m_textTool->getTextFont(), text);
        }
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
    const qreal factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    const QPointF cursor = event->position();
    const QPointF logicalAtCursor = toLogical(cursor);

    m_transformer->setScale(m_transformer->scale() * factor);
    m_transformer->setOffset(cursor - logicalAtCursor * m_transformer->scale());
    event->accept();
}
