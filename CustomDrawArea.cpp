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

void CustomDrawArea::setDrawMode(DrawMode mode) { m_modeManager->setDrawMode(mode); }
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

void CustomDrawArea::startShapeSelection() { emit shapeSelection(true); }
void CustomDrawArea::cancelSelection() { m_shapeManager->clearSelection(); emit shapeSelection(false); }
void CustomDrawArea::toggleMultiSelectMode() { m_multiSelect = !m_multiSelect; emit multiSelectionModeChanged(m_multiSelect); }
bool CustomDrawArea::hasSelection() const { return !m_shapeManager->selectedShapes().isEmpty(); }

void CustomDrawArea::deleteSelectedShapes()
{
    auto selected = m_shapeManager->selectedShapes();
    std::sort(selected.begin(), selected.end(), std::greater<int>());
    for (int index : selected) m_shapeManager->removeShape(index);
    m_shapeManager->clearSelection();
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
bool CustomDrawArea::isConnectMode() const { return false; }

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

void CustomDrawArea::startCloseMode() { m_modeManager->startCloseMode(); }
void CustomDrawArea::cancelCloseMode() { m_modeManager->cancelCloseMode(); }
void CustomDrawArea::startDeplacerMode() { m_modeManager->startDeplacerMode(); }
void CustomDrawArea::cancelDeplacerMode() { m_modeManager->cancelDeplacerMode(); }
void CustomDrawArea::startSupprimerMode() { m_modeManager->startSupprimerMode(); }
void CustomDrawArea::cancelSupprimerMode() { m_modeManager->cancelSupprimerMode(); }
void CustomDrawArea::startGommeMode() { m_modeManager->startGommeMode(); }
void CustomDrawArea::cancelGommeMode() { m_modeManager->cancelGommeMode(); }
void CustomDrawArea::setSmoothingLevel(int level) { m_smoothingLevel = qMax(0, level); emit smoothingLevelChanged(m_smoothingLevel); }
void CustomDrawArea::setTwoFingersOn(bool active) { m_twoFingersOn = active; }

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

void CustomDrawArea::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.translate(m_transformer->offset());
    painter.scale(m_transformer->scale(), m_transformer->scale());

    const QRectF visibleArea = QRectF(toLogical(QPointF(0, 0)), toLogical(QPointF(width(), height()))).normalized();
    m_renderer->render(painter, *m_shapeManager, visibleArea);

    if (m_drawing && m_strokePoints.size() > 1) {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        painter.drawPath(PathGenerator::generateBezierPath(m_strokePoints));
    }
}

void CustomDrawArea::mousePressEvent(QMouseEvent *event)
{
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
    m_mouseHandler->handleMousePress(event, logical);

    if (event->button() == Qt::RightButton && m_pasteMode) {
        pasteCopiedShapes(logical);
        m_pasteMode = false;
        return;
    }

    m_drawing = true;
    m_startPoint = logical;
    m_currentPoint = logical;
    m_strokePoints = {logical};

    if (getDrawMode() == DrawMode::Gomme) {
        m_eraserTool->applyEraserAt(logical);
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
        m_eraserTool->eraseAlong(m_strokePoints.last(), logical);
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
        m_historyManager->pushState();
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
