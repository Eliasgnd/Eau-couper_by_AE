#include "CustomDrawArea.h"

#include "DrawModeManager.h"
#include "DrawingState.h"
#include "EraserTool.h"
#include "HistoryManager.h"
#include "MouseInteractionHandler.h"
#include "PlacementAssist.h"
#include "ShapeRenderer.h"
#include "TextTool.h"
#include "ViewTransformer.h"
#include "domain/geometry/GeometryUtils.h"
#include "domain/shapes/PathGenerator.h"
#include "domain/shapes/ShapeManager.h"

#include <QInputDialog>
#include <QFile>
#include <QLineEdit>
#include <QLineF>
#include <QMouseEvent>
#include <QIODevice>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPathStroker>
#include <QPen>
#include <QStringList>
#include <QSizeF>
#include <QTransform>
#include <QVector>
#include <QWheelEvent>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace {
QString modeToString(CustomDrawArea::DrawMode mode)
{
    using DM = CustomDrawArea::DrawMode;
    switch (mode) {
    case DM::Freehand:      return QObject::tr("Trace libre");
    case DM::PointParPoint: return QObject::tr("Point par point");
    case DM::Line:          return QObject::tr("Ligne droite");
    case DM::Rectangle:     return QObject::tr("Rectangle");
    case DM::Circle:        return QObject::tr("Cercle");
    case DM::Text:          return QObject::tr("Texte");
    case DM::ThinText:      return QObject::tr("Texte fin");
    case DM::Deplacer:      return QObject::tr("Deplacer");
    case DM::Gomme:         return QObject::tr("Gomme");
    case DM::Supprimer:     return QObject::tr("Supprimer");
    case DM::Pan:           return QObject::tr("Panoramique");
    default:                return QObject::tr("Dessin");
    }
}

QPointF pathStart(const QPainterPath &path)
{
    if (path.elementCount() == 0) return {};
    const auto e = path.elementAt(0);
    return {e.x, e.y};
}

QPointF pathEnd(const QPainterPath &path)
{
    if (path.elementCount() == 0) return {};
    const auto e = path.elementAt(path.elementCount() - 1);
    return {e.x, e.y};
}


PerformanceMode defaultPerformanceMode()
{
#ifdef Q_OS_LINUX
    QFile modelFile(QStringLiteral("/proc/device-tree/model"));
    if (modelFile.open(QIODevice::ReadOnly)) {
        const QByteArray model = modelFile.readAll().toLower();
        if (model.contains("raspberry"))
            return PerformanceMode::LowEnd;
    }
#endif
    return PerformanceMode::Balanced;
}
}

CustomDrawArea::CustomDrawArea(CanvasViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
    , m_renderer(std::make_unique<ShapeRenderer>(this))
    , m_modeManager(std::make_unique<DrawModeManager>(this))
    , m_transformer(std::make_unique<ViewTransformer>(this))
    , m_drawingState(std::make_unique<DrawingState>())
    , m_mouseHandler(std::make_unique<MouseInteractionHandler>(
          m_viewModel->shapeManager(), m_modeManager.get(), m_transformer.get(),
          m_drawingState.get(), this))
    , m_eraserTool(std::make_unique<EraserTool>(m_viewModel->shapeManager(), this))
    , m_textTool(std::make_unique<TextTool>(this))
{
    setMouseTracking(true);
    setAutoFillBackground(true);

    connect(m_viewModel, &CanvasViewModel::shapesChanged,
            this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_viewModel, &CanvasViewModel::selectionChanged,
            this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_viewModel, &CanvasViewModel::selectionChanged,
            this, &CustomDrawArea::emitSelectionState);
    connect(m_viewModel, &CanvasViewModel::selectionChanged,
            this, &CustomDrawArea::emitCanvasStatus);
    connect(m_mouseHandler.get(), &MouseInteractionHandler::requestUpdate,
            this, QOverload<>::of(&CustomDrawArea::update));

    connect(m_modeManager.get(), &DrawModeManager::drawModeChanged,
            this, &CustomDrawArea::onDrawModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::closeModeChanged,
            this, &CustomDrawArea::closeModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::closeModeChanged,
            this, &CustomDrawArea::emitCanvasStatus);
    connect(m_modeManager.get(), &DrawModeManager::deplacerModeChanged,
            this, &CustomDrawArea::deplacerModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::deplacerModeChanged,
            this, &CustomDrawArea::emitCanvasStatus);
    connect(m_modeManager.get(), &DrawModeManager::supprimerModeChanged,
            this, &CustomDrawArea::supprimerModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::supprimerModeChanged,
            this, &CustomDrawArea::emitCanvasStatus);
    connect(m_modeManager.get(), &DrawModeManager::gommeModeChanged,
            this, &CustomDrawArea::gommeModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::gommeModeChanged,
            this, &CustomDrawArea::emitCanvasStatus);
    connect(m_modeManager.get(), &DrawModeManager::shapeSelectionChanged,
            this, &CustomDrawArea::shapeSelection);
    connect(m_modeManager.get(), &DrawModeManager::shapeSelectionChanged,
            this, &CustomDrawArea::emitCanvasStatus);
    connect(m_modeManager.get(), &DrawModeManager::multiSelectionChanged,
            this, &CustomDrawArea::multiSelectionModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::multiSelectionChanged,
            this, &CustomDrawArea::emitCanvasStatus);

    connect(m_transformer.get(), &ViewTransformer::zoomChanged,
            this, &CustomDrawArea::zoomChanged);
    connect(m_transformer.get(), &ViewTransformer::viewTransformed,
            this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_transformer.get(), &ViewTransformer::viewTransformed,
            this, &CustomDrawArea::emitCanvasStatus);

    
    emitSelectionState();
    emitCanvasStatus();
    emitHistoryState();
}

CustomDrawArea::~CustomDrawArea() = default;

void CustomDrawArea::setDrawMode(DrawMode mode)
{
    if (m_modeManager->isSelectMode()) cancelSelection();
    if (m_modeManager->isCloseMode())  cancelCloseMode();
    m_drawingState->gommeErasing = false;
    m_drawingState->pointByPointPoints.clear();
    resetPointByPointResumeState();
    m_hoveredEndpoint = {};
    m_drawing = false;
    m_modeManager->setDrawMode(mode);
    emitCanvasStatus();
}

CustomDrawArea::DrawMode CustomDrawArea::getDrawMode() const
{
    return m_modeManager->drawMode();
}

void CustomDrawArea::restorePreviousMode()
{
    m_modeManager->restorePreviousMode();
    emitCanvasStatus();
}

QList<QPolygonF> CustomDrawArea::getCustomShapes() const
{
    QList<QPolygonF> polygons;
    for (const auto &shape : m_viewModel->shapes())
        polygons.append(shape.path.toSubpathPolygons());
    return polygons;
}




void CustomDrawArea::setEraserRadius(qreal radius)
{
    m_eraserTool->setEraserRadius(radius);
}





void CustomDrawArea::clearDrawing() { m_viewModel->clearDrawing(); }
void CustomDrawArea::undoLastAction() { m_viewModel->undo(); }
void CustomDrawArea::redoLastAction() { m_viewModel->redo(); }
void CustomDrawArea::addImportedLogo(const QPainterPath &logoPath) { m_viewModel->addImportedLogo(logoPath); }
void CustomDrawArea::addImportedLogoSubpath(const QPainterPath &subpath) { m_viewModel->addImportedLogoSubpath(subpath); }
void CustomDrawArea::addImportedLogoSubpaths(const QList<QPainterPath> &subpaths) { m_viewModel->addImportedLogoSubpaths(subpaths); }

void CustomDrawArea::deleteSelectedShapes() { m_viewModel->deleteSelectedShapes(); }
void CustomDrawArea::duplicateSelectedShapes() { m_viewModel->duplicateSelectedShapes(); }
void CustomDrawArea::resizeSelectedShapes(qreal targetWidth, qreal targetHeight) { m_viewModel->resizeSelectedShapes(targetWidth, targetHeight); }
void CustomDrawArea::rotateSelectedShapes(qreal angleDegrees) { m_viewModel->rotateSelectedShapes(angleDegrees); }
void CustomDrawArea::moveSelectedShapes(qreal dx, qreal dy, const QString &label) { m_viewModel->moveSelectedShapes(dx, dy, label); }
void CustomDrawArea::setSelectedShapesPosition(qreal x, qreal y) { m_viewModel->setSelectedShapesPosition(x, y); }
void CustomDrawArea::alignSelectedLeft() { m_viewModel->alignSelectedLeft(); }
void CustomDrawArea::alignSelectedHCenter() { m_viewModel->alignSelectedHCenter(); }
void CustomDrawArea::alignSelectedTop() { m_viewModel->alignSelectedTop(); }
void CustomDrawArea::alignSelectedVCenter() { m_viewModel->alignSelectedVCenter(); }
void CustomDrawArea::distributeSelectedHorizontally() { m_viewModel->distributeSelectedHorizontally(); }
void CustomDrawArea::distributeSelectedVertically() { m_viewModel->distributeSelectedVertically(); }
void CustomDrawArea::duplicateSelectedShapesLinear(int copies, const QPointF &step) { m_viewModel->duplicateSelectedShapesLinear(copies, step); }
void CustomDrawArea::commitSelectedTransform(const std::vector<ShapeManager::Shape> &updated, const QString &label) { m_viewModel->commitSelectedTransform(updated, label); }

bool CustomDrawArea::hasValidationIssues() const { return m_viewModel->hasValidationIssues(); }
QString CustomDrawArea::validationSummary() const { return m_viewModel->validationSummary(); }
std::vector<bool> CustomDrawArea::getCollisionFlags(int *pairCount) const { return m_viewModel->getCollisionFlags(pairCount); }

// Fix performance mode
void CustomDrawArea::setPerformanceMode(PerformanceMode mode) { 
    m_viewModel->setPerformanceMode(mode); 
    setLowEndMode(mode == PerformanceMode::LowEnd);
    emitCanvasStatus();
    update();
}
PerformanceMode CustomDrawArea::performanceMode() const { return m_viewModel->performanceMode(); }

// other accessors
void CustomDrawArea::setSmoothingLevel(int level) { 
    m_viewModel->setSmoothingLevel(level); 
    m_drawingState->freehandPreviewPointCount = -1;
    emit smoothingLevelChanged(level); 
    emitCanvasStatus(); 
}
void CustomDrawArea::setPrecisionConstraintEnabled(bool enabled) { m_viewModel->setPrecisionConstraintEnabled(enabled); emitCanvasStatus(); }
bool CustomDrawArea::isPrecisionConstraintEnabled() const { return m_viewModel->isPrecisionConstraintEnabled(); }
void CustomDrawArea::setSegmentStatusVisible(bool visible) { m_viewModel->setSegmentStatusVisible(visible); emitCanvasStatus(); }
bool CustomDrawArea::isSegmentStatusVisible() const { return m_viewModel->isSegmentStatusVisible(); }

void CustomDrawArea::setTextFont(const QFont &font)
{
    m_textTool->setTextFont(font);
}

QFont CustomDrawArea::getTextFont() const
{
    return m_textTool->getTextFont();
}

void CustomDrawArea::startShapeSelection()
{
    cancelCloseMode();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_viewModel->shapeManager()->clearSelection();
    m_modeManager->startSelectConnect();
    emitCanvasStatus();
    update();
}

void CustomDrawArea::cancelSelection()
{
    if (!m_modeManager->isSelectMode()) return;
    m_viewModel->shapeManager()->clearSelection();
    m_modeManager->cancelAnySelection();
    emitCanvasStatus();
    update();
}

void CustomDrawArea::toggleMultiSelectMode()
{
    if (!m_modeManager->isSelectMode()) {
        cancelCloseMode();
        cancelSupprimerMode();
        cancelDeplacerMode();
        cancelGommeMode();
        m_viewModel->shapeManager()->clearSelection();
        m_modeManager->startSelectMulti();
    } else if (m_modeManager->isMultiSelectMode()) {
        cancelSelection();
    }
    emitCanvasStatus();
    update();
}

bool CustomDrawArea::hasSelection() const
{
    return !m_viewModel->selectedShapes().empty();
}



QRectF CustomDrawArea::selectedShapesBounds() const
{
    return m_viewModel->selectedShapesBounds();
}

int CustomDrawArea::selectedShapesCount() const
{
    return static_cast<int>(m_viewModel->selectedShapes().size());
}

qreal CustomDrawArea::selectedRotationAngle() const
{
    return currentSelectionAngle();
}



void CustomDrawArea::setSelectedRotation(qreal angleDegrees)
{
    if (!hasSelection()) return;
    rotateSelectedShapes(angleDegrees - currentSelectionAngle());
}









void CustomDrawArea::centerSelectionInViewport()
{
    if (!hasSelection()) return;
    const QRectF visibleArea = QRectF(toLogical(QPointF(0, 0)),
                                      toLogical(QPointF(width(), height()))).normalized();
    const QPointF delta = visibleArea.center() - selectedShapesBounds().center();
    moveSelectedShapes(delta.x(), delta.y(), tr("Centrer la selection"));
}


void CustomDrawArea::zoomToSelection()
{
    if (!hasSelection()) return;

    const QRectF bounds = selectedShapesBounds().adjusted(-30.0, -30.0, 30.0, 30.0);
    if (bounds.width() <= 0.0 || bounds.height() <= 0.0) return;

    const qreal scaleX = width() / bounds.width();
    const qreal scaleY = height() / bounds.height();
    const qreal scale = qBound<qreal>(0.2, qMin(scaleX, scaleY), 8.0);
    m_transformer->setScale(scale);
    m_transformer->setOffset(QPointF(width() * 0.5, height() * 0.5) - bounds.center() * scale);
    emitCanvasStatus();
    update();
}

void CustomDrawArea::fitAllShapesInView()
{
    const auto &shapes = m_viewModel->shapes();
    if (shapes.empty()) return;

    QRectF bounds;
    bool initialized = false;
    for (const auto &shape : shapes) {
        const QRectF shapeBounds = shape.bounds;
        bounds = initialized ? bounds.united(shapeBounds) : shapeBounds;
        initialized = true;
    }
    bounds = bounds.adjusted(-40.0, -40.0, 40.0, 40.0);
    if (bounds.width() <= 0.0 || bounds.height() <= 0.0) return;

    const qreal scaleX = width() / bounds.width();
    const qreal scaleY = height() / bounds.height();
    const qreal scale = qBound<qreal>(0.2, qMin(scaleX, scaleY), 8.0);
    m_transformer->setScale(scale);
    m_transformer->setOffset(QPointF(width() * 0.5, height() * 0.5) - bounds.center() * scale);
    emitCanvasStatus();
    update();
}

void CustomDrawArea::finishPointByPointShape()
{
    if (getDrawMode() != DrawMode::PointParPoint || m_drawingState->pointByPointPoints.size() < 2)
        return;

    QList<QPointF> points = m_drawingState->pointByPointPoints;
    const bool shouldClose = points.size() >= 3
        && QLineF(points.first(), points.last()).length() <= endpointTouchTolerance();
    if (shouldClose)
        points.last() = points.first();

    QPainterPath path = buildPointByPointPath(points, shouldClose);

    if (m_drawingState->extendingExistingPath) {
        const auto oldState = m_viewModel->getCurrentState();
        if (m_drawingState->extendingShapeIndex >= 0
            && m_drawingState->extendingShapeIndex < static_cast<int>(oldState.size())) {
            auto updated = oldState;
            updated[m_drawingState->extendingShapeIndex].path = path;
            m_viewModel->shapeManager()->setShapes(updated);
            m_viewModel->commitSnapshot(oldState, updated, tr("Prolonger trace"));
        }
    } else {
        const int id = m_viewModel->nextShapeId();
        m_viewModel->commitAddShape(path, id, tr("Trace point-par-point"));
    }
    m_drawingState->pointByPointPoints.clear();
    resetPointByPointResumeState();
    m_hoveredEndpoint = {};
    m_drawing = false;
    emitHistoryState();
    emitCanvasStatus();
    update();
}

void CustomDrawArea::undoPointByPointPoint()
{
    if (getDrawMode() != DrawMode::PointParPoint || m_drawingState->pointByPointPoints.isEmpty())
        return;

    if (m_drawingState->extendingExistingPath && m_drawingState->pointByPointPoints.size() <= 1) {
        m_drawingState->pointByPointPoints.clear();
        resetPointByPointResumeState();
        m_drawing = false;
        emitCanvasStatus();
        update();
        return;
    }

    m_drawingState->pointByPointPoints.removeLast();
    m_drawing = !m_drawingState->pointByPointPoints.isEmpty();
    emitCanvasStatus();
    update();
}

void CustomDrawArea::undoPointByPointSegment()
{
    undoPointByPointPoint();
}

void CustomDrawArea::setSnapToGridEnabled(bool enabled)
{
    m_renderer->setSnapToGridEnabled(enabled);
    emitCanvasStatus();
    update();
}

bool CustomDrawArea::isSnapToGridEnabled() const { return m_renderer->isSnapToGridEnabled(); }

void CustomDrawArea::setPlacementAssistEnabled(bool enabled)
{
    m_placementAssistEnabled = enabled;
    if (!enabled)
        m_activePlacementGuides.clear();
    emitCanvasStatus();
    update();
}

bool CustomDrawArea::isPlacementAssistEnabled() const { return m_placementAssistEnabled; }

void CustomDrawArea::setPlacementMagnetEnabled(bool enabled)
{
    m_placementMagnetEnabled = enabled;
    emitCanvasStatus();
    update();
}

bool CustomDrawArea::isPlacementMagnetEnabled() const { return m_placementMagnetEnabled; }

void CustomDrawArea::setGridVisible(bool visible)
{
    m_renderer->setGridVisible(visible);
    emitCanvasStatus();
    update();
}

bool CustomDrawArea::isGridVisible() const { return m_renderer->isGridVisible(); }

void CustomDrawArea::setGridSpacing(int px)
{
    m_renderer->setGridSpacing(px);
    emitCanvasStatus();
    update();
}

int CustomDrawArea::gridSpacing() const { return m_renderer->gridSpacing(); }


bool CustomDrawArea::hasActiveSpecialMode() const
{
    return m_modeManager->isCloseMode()
        || m_modeManager->isSelectMode()
        || getDrawMode() == DrawMode::Deplacer
        || getDrawMode() == DrawMode::Supprimer
        || getDrawMode() == DrawMode::Gomme;
}

void CustomDrawArea::cancelActiveModes()
{
    cancelSelection();
    cancelCloseMode();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_drawingState->pointByPointPoints.clear();
    resetPointByPointResumeState();
    m_hoveredEndpoint = {};
    m_drawingState->gommeErasing = false;
    m_drawing = false;
    emitCanvasStatus();
    update();
}

bool CustomDrawArea::isDeplacerMode() const { return getDrawMode() == DrawMode::Deplacer; }
bool CustomDrawArea::isSupprimerMode() const { return getDrawMode() == DrawMode::Supprimer; }
bool CustomDrawArea::isGommeMode() const { return getDrawMode() == DrawMode::Gomme; }
bool CustomDrawArea::isConnectMode() const { return m_modeManager->isConnectMode(); }




QString CustomDrawArea::buildSelectionSummary() const
{
    if (!hasSelection()) return tr("Aucune selection");
    const QRectF bounds = selectedShapesBounds();
    return tr("%1 forme(s)  |  %2 x %3 px  |  angle %4 deg  |  X %5  |  Y %6")
        .arg(m_viewModel->selectedShapes().size())
        .arg(qRound(bounds.width()))
        .arg(qRound(bounds.height()))
        .arg(qRound(currentSelectionAngle()))
        .arg(qRound(bounds.left()))
        .arg(qRound(bounds.top()));
}

void CustomDrawArea::startCloseMode()
{
    cancelSelection();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_modeManager->startCloseMode();
    emitCanvasStatus();
}

void CustomDrawArea::cancelCloseMode()
{
    m_modeManager->cancelCloseMode();
    emitCanvasStatus();
}

void CustomDrawArea::startDeplacerMode()
{
    cancelSelection();
    cancelCloseMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_modeManager->startDeplacerMode();
    emitCanvasStatus();
}

void CustomDrawArea::cancelDeplacerMode()
{
    m_modeManager->cancelDeplacerMode();
    emitCanvasStatus();
}

void CustomDrawArea::startSupprimerMode()
{
    cancelSelection();
    cancelCloseMode();
    cancelDeplacerMode();
    cancelGommeMode();
    m_modeManager->startSupprimerMode();
    emitCanvasStatus();
}

void CustomDrawArea::cancelSupprimerMode()
{
    m_modeManager->cancelSupprimerMode();
    emitCanvasStatus();
}

void CustomDrawArea::startGommeMode()
{
    cancelSelection();
    cancelCloseMode();
    cancelDeplacerMode();
    cancelSupprimerMode();
    m_modeManager->startGommeMode();
    emitCanvasStatus();
}

void CustomDrawArea::cancelGommeMode()
{
    m_modeManager->cancelGommeMode();
    emitCanvasStatus();
}


void CustomDrawArea::setTwoFingersOn(bool active)
{
    m_twoFingersOn = active;
    if (active) m_drawingState->gommeErasing = false;
    emitCanvasStatus();
}

void CustomDrawArea::handlePinchZoom(QPointF center, qreal factor)
{
    const qreal oldScale = m_transformer->scale();
    const QPointF logicalCenter = toLogical(center);
    m_transformer->setScale(oldScale * factor);
    m_transformer->setOffset(center - logicalCenter * m_transformer->scale());
}

void CustomDrawArea::onDrawModeChanged(DrawModeManager::DrawMode mode)
{
    emit drawModeChanged(mode);
    emitCanvasStatus();
    update();
}

QPointF CustomDrawArea::toLogical(const QPointF &widgetPoint) const
{
    return (widgetPoint - m_transformer->offset()) / m_transformer->scale();
}

QPointF CustomDrawArea::toWidget(const QPointF &logicalPoint) const
{
    return logicalPoint * m_transformer->scale() + m_transformer->offset();
}

qreal CustomDrawArea::endpointTouchTolerance() const
{
    return qMax<qreal>(18.0, 30.0 / qMax<qreal>(0.25, m_transformer->scale()));
}

CustomDrawArea::EndpointHit CustomDrawArea::hitTestOpenEndpoint(const QPointF &logicalPoint,
                                                                int ignoredShapeIndex) const
{
    EndpointHit best;
    qreal bestDistance = std::numeric_limits<qreal>::max();
    const qreal tolerance = endpointTouchTolerance();
    const auto &shapes = m_viewModel->shapes();

    for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i) {
        if (i == ignoredShapeIndex)
            continue;
        const QPainterPath &path = shapes[i].path;
        if (path.elementCount() < 2 || m_viewModel->isPathClosed(path))
            continue;

        const QPointF start = pathStart(path);
        const QPointF end = pathEnd(path);
        const qreal startDistance = QLineF(logicalPoint, start).length();
        if (startDistance <= tolerance && startDistance < bestDistance) {
            best = {i, EndpointKind::Start, start};
            bestDistance = startDistance;
        }

        const qreal endDistance = QLineF(logicalPoint, end).length();
        if (endDistance <= tolerance && endDistance < bestDistance) {
            best = {i, EndpointKind::End, end};
            bestDistance = endDistance;
        }
    }

    return best;
}

bool CustomDrawArea::supportsEndpointResume(DrawMode mode) const
{
    return mode == DrawMode::PointParPoint
        || mode == DrawMode::Freehand
        || mode == DrawMode::Line;
}

QPointF CustomDrawArea::snapToDrawingAid(const QPointF &logicalPoint) const
{
    m_hoveredEndpoint = {};
    if (!supportsEndpointResume(getDrawMode()))
        return logicalPoint;

    if (getDrawMode() == DrawMode::PointParPoint && !m_drawingState->pointByPointPoints.isEmpty()) {
        const QPointF first = m_drawingState->pointByPointPoints.first();
        if (m_drawingState->pointByPointPoints.size() >= 3
            && QLineF(first, logicalPoint).length() <= endpointTouchTolerance()) {
            return first;
        }

        const QPointF last = m_drawingState->pointByPointPoints.last();
        if (QLineF(last, logicalPoint).length() <= endpointTouchTolerance() * 0.6)
            return last;
    }

    if (m_drawing && getDrawMode() == DrawMode::Freehand)
        return logicalPoint;

    const int ignoredIndex = m_drawingState->extendingExistingPath
        ? m_drawingState->extendingShapeIndex
        : -1;
    const EndpointHit hit = hitTestOpenEndpoint(logicalPoint, ignoredIndex);
    if (hit.isValid()) {
        m_hoveredEndpoint = hit;
        return hit.point;
    }

    return logicalPoint;
}

void CustomDrawArea::resetPointByPointResumeState()
{
    m_drawingState->extendingExistingPath = false;
    m_drawingState->extendingShapeIndex = -1;
    m_drawingState->extendingFromStart = false;
    m_drawingState->extendingAnchor = {};
}

QPainterPath CustomDrawArea::buildPointByPointPath(const QList<QPointF> &points, bool shouldClose) const
{
    QPainterPath path;
    if (points.isEmpty())
        return path;

    if (m_drawingState->extendingExistingPath) {
        const auto &shapes = m_viewModel->shapes();
        if (m_drawingState->extendingShapeIndex >= 0
            && m_drawingState->extendingShapeIndex < static_cast<int>(shapes.size())) {
            path = shapes[m_drawingState->extendingShapeIndex].path;
            if (m_drawingState->extendingFromStart)
                path = path.toReversed();
        }
    }

    if (path.isEmpty()) {
        path.moveTo(points.first());
    } else if (QLineF(pathEnd(path), points.first()).length() > 0.01) {
        path.lineTo(points.first());
    }

    for (int i = 1; i < points.size(); ++i)
        path.lineTo(points[i]);
    if (shouldClose)
        path.closeSubpath();
    return path;
}

QPainterPath CustomDrawArea::buildResumedPath(const QPainterPath &extensionPath) const
{
    if (!m_drawingState->extendingExistingPath || extensionPath.isEmpty())
        return extensionPath;

    const auto &shapes = m_viewModel->shapes();
    if (m_drawingState->extendingShapeIndex < 0
        || m_drawingState->extendingShapeIndex >= static_cast<int>(shapes.size()))
        return extensionPath;

    QPainterPath path = shapes[m_drawingState->extendingShapeIndex].path;
    if (m_drawingState->extendingFromStart)
        path = path.toReversed();

    path.connectPath(extensionPath);
    return path;
}

bool CustomDrawArea::beginEndpointResumeIfNeeded(const QPointF &logicalPoint)
{
    if (!supportsEndpointResume(getDrawMode()))
        return false;

    EndpointHit endpoint = m_hoveredEndpoint;
    if (!endpoint.isValid())
        endpoint = hitTestOpenEndpoint(logicalPoint);
    if (!endpoint.isValid())
        return false;

    resetPointByPointResumeState();
    m_drawingState->extendingExistingPath = true;
    m_drawingState->extendingShapeIndex = endpoint.shapeIndex;
    m_drawingState->extendingFromStart = endpoint.kind == EndpointKind::Start;
    m_drawingState->extendingAnchor = endpoint.point;
    m_drawingState->currentPoint = endpoint.point;
    m_drawingState->startPoint = endpoint.point;
    m_drawingState->strokePoints = {endpoint.point};
    if (getDrawMode() == DrawMode::PointParPoint)
        m_drawingState->pointByPointPoints = {endpoint.point};
    m_hoveredEndpoint = endpoint;
    m_drawing = true;
    Q_UNUSED(logicalPoint)
    emitCanvasStatus();
    update();
    return true;
}

QPointF CustomDrawArea::snapToGridIfNeeded(const QPointF &logicalPoint) const
{
    if (!m_renderer->isSnapToGridEnabled()) return logicalPoint;
    const int spacing = qMax(1, m_renderer->gridSpacing());
    return QPointF(std::round(logicalPoint.x() / spacing) * spacing,
                   std::round(logicalPoint.y() / spacing) * spacing);
}

QPointF CustomDrawArea::constrainedPoint(const QPointF &logicalPoint) const
{
    if (!m_viewModel->isPrecisionConstraintEnabled() || !m_drawing) return logicalPoint;

    if (getDrawMode() == DrawMode::Line) {
        const QPointF delta = logicalPoint - m_drawingState->startPoint;
        if (std::abs(delta.x()) >= std::abs(delta.y()))
            return QPointF(logicalPoint.x(), m_drawingState->startPoint.y());
        return QPointF(m_drawingState->startPoint.x(), logicalPoint.y());
    }

    if (getDrawMode() == DrawMode::Rectangle || getDrawMode() == DrawMode::Circle) {
        const QPointF delta = logicalPoint - m_drawingState->startPoint;
        const qreal size = qMax(std::abs(delta.x()), std::abs(delta.y()));
        return QPointF(m_drawingState->startPoint.x() + (delta.x() < 0 ? -size : size),
                       m_drawingState->startPoint.y() + (delta.y() < 0 ? -size : size));
    }

    return logicalPoint;
}

QRectF CustomDrawArea::visibleLogicalArea() const
{
    return QRectF(toLogical(QPointF(0, 0)),
                  toLogical(QPointF(width(), height()))).normalized();
}

QVector<QRectF> CustomDrawArea::placementTargetBounds(bool excludeSelection) const
{
    QVector<QRectF> targets;
    const auto &shapes = m_viewModel->shapes();
    const auto selected = m_viewModel->selectedShapes();

    for (int i = 0; i < static_cast<int>(shapes.size()); ++i) {
        if (excludeSelection && std::find(selected.begin(), selected.end(), i) != selected.end())
            continue;
        if (m_drawingState->extendingExistingPath && i == m_drawingState->extendingShapeIndex)
            continue;
        if (!shapes[i].bounds.isValid() && shapes[i].bounds.isNull())
            continue;
        targets.push_back(shapes[i].bounds);
    }
    return targets;
}

QPointF CustomDrawArea::applyPlacementAssistToPoint(const QPointF &logicalPoint) const
{
    m_activePlacementGuides.clear();
    if (!m_placementAssistEnabled)
        return logicalPoint;

    const bool supportedMode = getDrawMode() == DrawMode::PointParPoint
        || getDrawMode() == DrawMode::Line
        || getDrawMode() == DrawMode::Rectangle
        || getDrawMode() == DrawMode::Circle;
    if (!supportedMode)
        return logicalPoint;

    const qreal scale = qMax<qreal>(0.25, m_transformer->scale());
    PlacementAssist::Options options;
    options.enabled = true;
    options.magnetEnabled = m_placementMagnetEnabled;
    options.guideTolerance = 18.0 / scale;
    options.magnetTolerance = 12.0 / scale;

    const QRectF pointRect(logicalPoint, QSizeF(0.0, 0.0));
    const auto result = PlacementAssist::resolve(pointRect,
                                                 placementTargetBounds(false),
                                                 visibleLogicalArea(),
                                                 options);
    m_activePlacementGuides = result.guides;
    return result.correctedPoint(logicalPoint);
}

void CustomDrawArea::applyPlacementAssistToSelection()
{
    m_activePlacementGuides.clear();
    if (!m_placementAssistEnabled || !hasSelection())
        return;

    const QRectF selection = selectedShapesBounds();
    if (selection.isNull())
        return;

    const qreal scale = qMax<qreal>(0.25, m_transformer->scale());
    PlacementAssist::Options options;
    options.enabled = true;
    options.magnetEnabled = m_placementMagnetEnabled;
    options.guideTolerance = 18.0 / scale;
    options.magnetTolerance = 12.0 / scale;

    const auto result = PlacementAssist::resolve(selection,
                                                 placementTargetBounds(true),
                                                 visibleLogicalArea(),
                                                 options);
    m_activePlacementGuides = result.guides;
    if (result.hasSnap())
        m_viewModel->shapeManager()->translateSelectedShapes(result.correction);
}

QPointF CustomDrawArea::applyDrawingAids(const QPointF &logicalPoint) const
{
    m_activePlacementGuides.clear();
    const QPointF constrained = constrainedPoint(logicalPoint);
    const QPointF endpointSnapped = snapToDrawingAid(constrained);
    if (endpointSnapped != constrained)
        return endpointSnapped;
    const QPointF placementSnapped = applyPlacementAssistToPoint(constrained);
    if (placementSnapped != constrained)
        return placementSnapped;
    return snapToGridIfNeeded(constrained);
}

int CustomDrawArea::hitTestShape(const QPointF &logicalPoint, qreal tolerance) const
{
    const auto &shapes = m_viewModel->shapes();
    const qreal desiredPixels = 18.0 + (m_transformer->scale() < 1.0 ? (1.0 - m_transformer->scale()) * 12.0 : 0.0);
    const qreal adjustedTolerance = qMax<qreal>(tolerance * 0.25, desiredPixels / qMax<qreal>(0.25, m_transformer->scale()));

    QPainterPathStroker stroker;
    stroker.setWidth(adjustedTolerance);
    for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i) {
        if (!shapes[i].bounds.adjusted(-adjustedTolerance, -adjustedTolerance,
                                       adjustedTolerance, adjustedTolerance).contains(logicalPoint)) {
            continue;
        }
        if (stroker.createStroke(shapes[i].path).contains(logicalPoint))
            return i;
    }
    return -1;
}

CustomDrawArea::ResizeHandle CustomDrawArea::hitTestSelectionHandle(const QPointF &logicalPoint) const
{
    if (!hasSelection()) return ResizeHandle::None;

    const QPointF localPoint = selectionInverseRotationTransform().map(logicalPoint);
    const QRectF bounds = selectionOverlayBounds();
    const qreal tolerance = 18.0 / qMax<qreal>(0.25, m_transformer->scale());
    const auto isNear = [&](const QPointF &handlePoint) {
        return QLineF(localPoint, handlePoint).length() <= tolerance;
    };

    if (isNear(bounds.topLeft())) return ResizeHandle::TopLeft;
    if (isNear(bounds.topRight())) return ResizeHandle::TopRight;
    if (isNear(bounds.bottomLeft())) return ResizeHandle::BottomLeft;
    if (isNear(bounds.bottomRight())) return ResizeHandle::BottomRight;
    return ResizeHandle::None;
}

QRectF CustomDrawArea::resizeRectFromHandle(const QPointF &logicalPoint) const
{
    const QPointF localPoint = selectionInverseRotationTransform().map(logicalPoint);
    const QRectF bounds = m_transformStartBounds;
    const qreal minSize = 12.0;
    qreal left = bounds.left();
    qreal right = bounds.right();
    qreal top = bounds.top();
    qreal bottom = bounds.bottom();

    switch (m_activeResizeHandle) {
    case ResizeHandle::TopLeft:
        left = qMin(localPoint.x(), right - minSize);
        top = qMin(localPoint.y(), bottom - minSize);
        break;
    case ResizeHandle::TopRight:
        right = qMax(localPoint.x(), left + minSize);
        top = qMin(localPoint.y(), bottom - minSize);
        break;
    case ResizeHandle::BottomLeft:
        left = qMin(localPoint.x(), right - minSize);
        bottom = qMax(localPoint.y(), top + minSize);
        break;
    case ResizeHandle::BottomRight:
        right = qMax(localPoint.x(), left + minSize);
        bottom = qMax(localPoint.y(), top + minSize);
        break;
    case ResizeHandle::None:
        break;
    }

    return QRectF(QPointF(left, top), QPointF(right, bottom)).normalized();
}

QRectF CustomDrawArea::selectionOverlayBounds() const
{
    if (!hasSelection()) return {};

    const auto &shapes = m_viewModel->shapes();
    const auto &selected = m_viewModel->selectedShapes();
    const QTransform inverse = selectionInverseRotationTransform();

    QRectF bounds;
    bool initialized = false;
    for (int idx : selected) {
        if (idx < 0 || idx >= static_cast<int>(shapes.size())) continue;
        const QRectF mappedBounds = inverse.map(shapes[idx].path).boundingRect();
        bounds = initialized ? bounds.united(mappedBounds) : mappedBounds;
        initialized = true;
    }
    return bounds;
}

void CustomDrawArea::emitSelectionState()
{
    emit selectionStateChanged(hasSelection(), buildSelectionSummary());
}

void CustomDrawArea::emitCanvasStatus()
{
    emit canvasStatusChanged(currentModeLabel(), currentModeHint(), currentDetailText());
}

void CustomDrawArea::emitCanvasStatusThrottled()
{
    if (!m_canvasStatusThrottleStarted) {
        m_canvasStatusThrottle.start();
        m_canvasStatusThrottleStarted = true;
        emitCanvasStatus();
        return;
    }

    const int intervalMs = m_viewModel->performanceMode() == PerformanceMode::LowEnd ? 120 : 60;
    if (m_canvasStatusThrottle.elapsed() >= intervalMs) {
        m_canvasStatusThrottle.restart();
        emitCanvasStatus();
    }
}

void CustomDrawArea::emitHistoryState()
{
    emit historyStateChanged(m_viewModel->canUndo(),
                             m_viewModel->undoText(),
                             m_viewModel->canRedo(),
                             m_viewModel->redoText());
}

QString CustomDrawArea::currentModeLabel() const
{
    if (m_modeManager->isConnectMode()) return tr("Mode : Relier");
    if (m_modeManager->isMultiSelectMode()) return tr("Mode : Selection multiple");
    if (m_modeManager->isCloseMode()) return tr("Mode : Fermer forme");
    return tr("Mode : %1").arg(modeToString(getDrawMode()));
}

QString CustomDrawArea::currentModeHint() const
{
    if (m_modeManager->isConnectMode()) return tr("Touchez deux formes pour relier leurs extremites.");
    if (m_modeManager->isMultiSelectMode()) return tr("Touchez pour ajouter/retirer une forme, puis utilisez les actions rapides.");
    if (m_modeManager->isCloseMode()) return tr("Touchez une forme ouverte pour fermer son contour.");
    if (supportsEndpointResume(getDrawMode()) && m_drawingState->extendingExistingPath)
        return tr("Trace reprise : touchez pour ajouter des segments, puis terminez la trace.");
    if (supportsEndpointResume(getDrawMode()) && m_hoveredEndpoint.isValid())
        return tr("Touchez cette extremite pour reprendre la trace.");
    switch (getDrawMode()) {
    case DrawMode::Freehand:      return tr("Glissez le doigt pour dessiner librement, ou touchez une extremite pour reprendre.");
    case DrawMode::PointParPoint: return tr("Touchez pour poser des points, ou une extremite ouverte pour reprendre une trace.");
    case DrawMode::Line:          return tr("Glissez pour tracer une ligne, ou repartez d'une extremite ouverte.");
    case DrawMode::Rectangle:     return tr("Glissez pour tracer un rectangle. La contrainte force le carre.");
    case DrawMode::Circle:        return tr("Glissez pour tracer une ellipse. La contrainte force le cercle.");
    case DrawMode::Text:          return tr("Touchez pour inserer un texte contour.");
    case DrawMode::ThinText:      return tr("Touchez pour inserer un texte fin.");
    case DrawMode::Deplacer:      return tr("Glissez une selection pour la deplacer.");
    case DrawMode::Gomme:         return tr("Glissez pour effacer le trait.");
    case DrawMode::Supprimer:     return tr("Touchez une forme pour la supprimer.");
    case DrawMode::Pan:           return tr("Deplacez la vue.");
    default:                      return tr("Dessinez directement sur le canevas.");
    }
}

QString CustomDrawArea::currentDetailText() const
{
    QStringList parts;
    parts << (isGridVisible() ? tr("Grille visible") : tr("Grille masquee"));
    parts << (isSnapToGridEnabled() ? tr("Aimant ON") : tr("Aimant OFF"));
    parts << (m_placementAssistEnabled ? tr("Guides ON") : tr("Guides OFF"));
    parts << (m_placementMagnetEnabled ? tr("Aimant guides ON") : tr("Aimant guides OFF"));
    parts << (m_viewModel->isPrecisionConstraintEnabled() ? tr("Contrainte ON") : tr("Contrainte OFF"));
    parts << (m_viewModel->isSegmentStatusVisible() ? tr("Segments ON") : tr("Segments OFF"));
    if (hasSelection())
        parts << buildSelectionSummary();
    else {
        const QString preview = livePreviewMetrics();
        if (!preview.isEmpty())
            parts << preview;
    }
    return parts.join("  |  ");
}

QString CustomDrawArea::livePreviewMetrics() const
{
    if (m_drawing && getDrawMode() == DrawMode::Line) {
        const QLineF line(m_drawingState->startPoint, m_drawingState->currentPoint);
        return tr("Longueur %1 px  |  Angle %2 deg")
            .arg(qRound(line.length()))
            .arg(qRound(-line.angle()));
    }

    if (m_drawing && (getDrawMode() == DrawMode::Rectangle || getDrawMode() == DrawMode::Circle)) {
        const QRectF rect(m_drawingState->startPoint, m_drawingState->currentPoint);
        const QRectF normalized = rect.normalized();
        QString metrics = tr("%1 x %2 px  |  Centre X %3  |  Y %4")
            .arg(qRound(normalized.width()))
            .arg(qRound(normalized.height()))
            .arg(qRound(normalized.center().x()))
            .arg(qRound(normalized.center().y()));
        if (getDrawMode() == DrawMode::Circle) {
            metrics += tr("  |  Rayon ~ %1").arg(qRound(qMin(normalized.width(), normalized.height()) / 2.0));
        }
        return metrics;
    }

    if (getDrawMode() == DrawMode::PointParPoint && !m_drawingState->pointByPointPoints.isEmpty()) {
        const int extraPoints = m_drawingState->extendingExistingPath
            ? qMax(0, m_drawingState->pointByPointPoints.size() - 1)
            : m_drawingState->pointByPointPoints.size();
        if (m_drawingState->extendingExistingPath)
            return tr("Reprise active  |  %1 nouveau(x) point(s)").arg(extraPoints);
        return tr("%1 point(s) places").arg(m_drawingState->pointByPointPoints.size());
    }

    if (supportsEndpointResume(getDrawMode()) && m_hoveredEndpoint.isValid())
        return tr("Extremite detectee : touchez pour repartir de ce segment");

    if (getDrawMode() == DrawMode::Gomme && m_drawingState->gommeErasing)
        return tr("Diametre gomme : %1 px").arg(qRound(m_eraserTool->eraserRadius() * 2.0));

    return {};
}

bool CustomDrawArea::isPointInsideSelectedShape(const QPointF &logicalPoint) const
{
    if (!hasSelection())
        return false;

    const auto &shapes = m_viewModel->shapes();
    const auto &selected = m_viewModel->selectedShapes();
    QPainterPathStroker stroker;
    stroker.setWidth(26.0);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);

    for (auto it = selected.rbegin(); it != selected.rend(); ++it) {
        const int idx = *it;
        if (idx < 0 || idx >= static_cast<int>(shapes.size()))
            continue;
        const QPainterPath &path = shapes[idx].path;
        if (!shapes[idx].bounds.adjusted(-26.0, -26.0, 26.0, 26.0).contains(logicalPoint))
            continue;
        if (path.contains(logicalPoint) || stroker.createStroke(path).contains(logicalPoint))
            return true;
    }

    return false;
}


QPointF CustomDrawArea::rotationHandlePoint() const
{
    const QRectF bounds = selectionOverlayBounds();
    const qreal offset = 28.0 / qMax<qreal>(0.25, m_transformer->scale());
    const QPointF localPoint(bounds.center().x(), bounds.top() - offset);
    return selectionRotationTransform().map(localPoint);
}

bool CustomDrawArea::hitTestRotationHandle(const QPointF &logicalPoint) const
{
    if (!hasSelection()) return false;
    const qreal tolerance = 18.0 / qMax<qreal>(0.25, m_transformer->scale());
    return QLineF(logicalPoint, rotationHandlePoint()).length() <= tolerance;
}

qreal CustomDrawArea::currentSelectionAngle() const
{
    if (!hasSelection()) return 0.0;
    const auto &shapes = m_viewModel->shapes();
    const auto &selected = m_viewModel->selectedShapes();
    if (selected.empty()) return 0.0;

    qreal sum = 0.0;
    int count = 0;
    for (int idx : selected) {
        if (idx < 0 || idx >= static_cast<int>(shapes.size())) continue;
        sum += shapes[idx].rotationAngle;
        ++count;
    }
    if (count == 0) return 0.0;

    qreal angle = std::fmod(sum / count, 360.0);
    if (angle < 0.0) angle += 360.0;
    return angle;
}

QTransform CustomDrawArea::selectionRotationTransform() const
{
    const QRectF bounds = selectedShapesBounds();
    const QPointF center = bounds.center();
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(currentSelectionAngle());
    transform.translate(-center.x(), -center.y());
    return transform;
}

QTransform CustomDrawArea::selectionInverseRotationTransform() const
{
    const QRectF bounds = selectedShapesBounds();
    const QPointF center = bounds.center();
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(-currentSelectionAngle());
    transform.translate(-center.x(), -center.y());
    return transform;
}

void CustomDrawArea::drawMachinePreview(QPainter &painter) const
{
    for (const auto &shape : m_viewModel->shapes()) {
        // Ajout d'une opacité (ex: 120 sur 255) aux couleurs du stylo
        QColor pathColor = m_viewModel->isPathClosed(shape.path) ? QColor(14, 159, 110, 120) : QColor(220, 38, 38, 120);
        QPen pen(pathColor, 2.6);
        pen.setCosmetic(true);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(shape.path);

        const QPointF start = pathStart(shape.path);
        const QPointF end = pathEnd(shape.path);

        // Ajout d'une opacité aux points de début et de fin
        painter.setBrush(QColor(14, 159, 110, 150)); // Vert un peu transparent
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(start, 5.0 / qMax<qreal>(0.25, m_transformer->scale()),
                            5.0 / qMax<qreal>(0.25, m_transformer->scale()));

        painter.setBrush(QColor(220, 38, 38, 150)); // Rouge un peu transparent
        painter.drawEllipse(end, 5.0 / qMax<qreal>(0.25, m_transformer->scale()),
                            5.0 / qMax<qreal>(0.25, m_transformer->scale()));
    }
}

void CustomDrawArea::drawCanvasHud(QPainter &painter) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // SUPPRIMÉ : Le gros bloc sombre (statusRect) qui affichait les textes en double.

    // ON GARDE UNIQUEMENT LA BULLE D'APERÇU EN DIRECT (suivi du curseur)
    QFont bodyFont = painter.font();
    bodyFont.setBold(false);
    bodyFont.setPointSize(qMax(9, bodyFont.pointSize() - 1));
    painter.setFont(bodyFont);

    const QString preview = livePreviewMetrics();
    if (!preview.isEmpty()) {
        const QPointF anchor = toWidget(m_drawingState->currentPoint) + QPointF(18.0, -18.0);
        const QFontMetrics fm(bodyFont);
        const QRect textRect = fm.boundingRect(QRect(0, 0, 360, 80), Qt::TextWordWrap, preview).adjusted(-10, -8, 10, 8);
        QRect bubbleRect(anchor.x(), anchor.y() - textRect.height(), textRect.width(), textRect.height());
        bubbleRect.moveLeft(qMin(bubbleRect.left(), width() - bubbleRect.width() - 12));
        bubbleRect.moveTop(qMax(110, bubbleRect.top()));

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 235));
        painter.drawRoundedRect(bubbleRect, 12, 12);

        painter.setPen(QColor(15, 23, 42));
        painter.drawText(bubbleRect.adjusted(10, 8, -10, -8), Qt::TextWordWrap, preview);
    }

    painter.restore();
}

void CustomDrawArea::drawValidationOverlay(QPainter &painter) const
{
    if (!m_viewModel->isSegmentStatusVisible())
        return;

    const auto &shapes = m_viewModel->shapes();
    if (shapes.empty()) return;

    const auto collision = getCollisionFlags();

    painter.save();
    painter.setBrush(Qt::NoBrush);
    for (int i = 0; i < static_cast<int>(shapes.size()); ++i) {
        const QRectF bounds = shapes[i].bounds;
        QColor color;
        QString label;
        if (!m_viewModel->isPathClosed(shapes[i].path)) {
            color = QColor(220, 38, 38);
            label = tr("Ouvert");
        } else if (bounds.width() < 12.0 || bounds.height() < 12.0) {
            color = QColor(245, 158, 11);
            label = tr("Petit");
        } else if (collision[i]) {
            color = QColor(234, 88, 12);
            label = tr("Proche");
        } else {
            continue;
        }

        QPen pen(color, 3);
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawPath(shapes[i].path);

        const qreal markerRadius = 4.0 / qMax<qreal>(0.25, m_transformer->scale());
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(pathStart(shapes[i].path), markerRadius, markerRadius);
        painter.drawEllipse(pathEnd(shapes[i].path), markerRadius, markerRadius);

        painter.setPen(color);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(bounds.topLeft() + QPointF(0.0, -8.0 / qMax<qreal>(0.25, m_transformer->scale())), label);
    }
    painter.restore();
}

void CustomDrawArea::drawSmartGuides(QPainter &painter) const
{
    if (!m_placementAssistEnabled || m_activePlacementGuides.isEmpty())
        return;

    painter.save();
    QPen guidePen(QColor(14, 165, 233), 1.2, Qt::DashLine);
    guidePen.setCosmetic(true);
    painter.setPen(guidePen);
    painter.setBrush(Qt::NoBrush);

    const QRectF visibleArea = QRectF(toLogical(QPointF(0, 0)),
                                      toLogical(QPointF(width(), height()))).normalized();
    const qreal scale = qMax<qreal>(0.25, m_transformer->scale());
    auto drawVGuide = [&](qreal x, const QString &label, bool snapped) {
        QPen pen(snapped ? QColor(16, 185, 129) : QColor(14, 165, 233),
                 snapped ? 1.8 : 1.2,
                 Qt::DashLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.drawLine(QPointF(x, visibleArea.top()), QPointF(x, visibleArea.bottom()));
        if (!label.isEmpty())
            painter.drawText(QPointF(x + 8.0 / scale, visibleArea.top() + 20.0 / scale), label);
    };
    auto drawHGuide = [&](qreal y, const QString &label, bool snapped) {
        QPen pen(snapped ? QColor(16, 185, 129) : QColor(14, 165, 233),
                 snapped ? 1.8 : 1.2,
                 Qt::DashLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.drawLine(QPointF(visibleArea.left(), y), QPointF(visibleArea.right(), y));
        if (!label.isEmpty())
            painter.drawText(QPointF(visibleArea.left() + 8.0 / scale, y - 8.0 / scale), label);
    };

    for (const PlacementAssist::Guide &guide : m_activePlacementGuides) {
        if (guide.orientation == PlacementAssist::Orientation::Vertical)
            drawVGuide(guide.position, guide.label, guide.snapped);
        else
            drawHGuide(guide.position, guide.label, guide.snapped);
    }

    painter.restore();
}

void CustomDrawArea::drawOpenEndpointHandles(QPainter &painter) const
{
    if (!supportsEndpointResume(getDrawMode()))
        return;

    const auto &shapes = m_viewModel->shapes();
    if (shapes.empty())
        return;

    painter.save();
    const qreal scale = qMax<qreal>(0.25, m_transformer->scale());
    const qreal outerRadius = 9.0 / scale;
    const qreal innerRadius = 4.0 / scale;

    for (int i = 0; i < static_cast<int>(shapes.size()); ++i) {
        if (m_drawingState->extendingExistingPath && i == m_drawingState->extendingShapeIndex)
            continue;
        const QPainterPath &path = shapes[i].path;
        if (path.elementCount() < 2 || m_viewModel->isPathClosed(path))
            continue;

        const QPointF endpoints[2] = {pathStart(path), pathEnd(path)};
        for (const QPointF &endpoint : endpoints) {
            const bool hovered = m_hoveredEndpoint.isValid()
                && m_hoveredEndpoint.shapeIndex == i
                && QLineF(m_hoveredEndpoint.point, endpoint).length() <= 0.01;
            painter.setPen(Qt::NoPen);
            painter.setBrush(hovered ? QColor(16, 185, 129, 70) : QColor(14, 165, 233, 34));
            painter.drawEllipse(endpoint, outerRadius, outerRadius);

            QPen ringPen(hovered ? QColor(16, 185, 129) : QColor(14, 165, 233), 1.8);
            ringPen.setCosmetic(true);
            painter.setPen(ringPen);
            painter.setBrush(Qt::white);
            painter.drawEllipse(endpoint, innerRadius, innerRadius);
        }
    }

    if (m_hoveredEndpoint.isValid()) {
        QPen labelPen(QColor(16, 185, 129), 1.4);
        labelPen.setCosmetic(true);
        painter.setPen(labelPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(m_hoveredEndpoint.point + QPointF(10.0 / scale, -10.0 / scale),
                         tr("Reprendre"));
    }
    painter.restore();
}

void CustomDrawArea::drawPointByPointAids(QPainter &painter) const
{
    if (getDrawMode() != DrawMode::PointParPoint || m_drawingState->pointByPointPoints.isEmpty())
        return;

    painter.save();
    QPen pen(QColor(14, 165, 233), 1.6);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(QColor(14, 165, 233));

    const qreal r = 4.5 / qMax<qreal>(0.25, m_transformer->scale());
    for (const QPointF &point : m_drawingState->pointByPointPoints)
        painter.drawEllipse(point, r, r);

    if (m_drawingState->extendingExistingPath) {
        QPen resumePen(QColor(16, 185, 129), 2.0, Qt::DashLine);
        resumePen.setCosmetic(true);
        painter.setPen(resumePen);
        painter.setBrush(QColor(16, 185, 129, 46));
        painter.drawEllipse(m_drawingState->extendingAnchor,
                            12.0 / qMax<qreal>(0.25, m_transformer->scale()),
                            12.0 / qMax<qreal>(0.25, m_transformer->scale()));
        painter.drawText(m_drawingState->extendingAnchor + QPointF(8.0, -8.0), tr("Reprise"));
        painter.setPen(pen);
        painter.setBrush(QColor(14, 165, 233));
    }

    if (m_drawingState->pointByPointPoints.size() >= 3) {
        const QPointF first = m_drawingState->pointByPointPoints.first();
        const qreal closeDistance = QLineF(first, m_drawingState->currentPoint).length();
        if (closeDistance <= endpointTouchTolerance()) {
            QPen closePen(QColor(16, 185, 129), 2.2);
            closePen.setCosmetic(true);
            painter.setPen(closePen);
            painter.setBrush(QColor(16, 185, 129, 40));
            painter.drawEllipse(first,
                                14.0 / qMax<qreal>(0.25, m_transformer->scale()),
                                14.0 / qMax<qreal>(0.25, m_transformer->scale()));
            painter.drawText(first + QPointF(8.0, -8.0), tr("Fermer"));
        }
    }

    painter.restore();
}

void CustomDrawArea::updateFreehandPreviewCache()
{
    if (getDrawMode() != DrawMode::Freehand || !m_drawing || m_drawingState->strokePoints.size() <= 1)
        return;

    if (m_drawingState->freehandPreviewPointCount == m_drawingState->strokePoints.size()
        && m_drawingState->freehandPreviewSmoothingLevel == m_viewModel->smoothingLevel()) {
        return;
    }

    QPainterPath previewPath;
    if (m_viewModel->smoothingLevel() <= 0) {
        previewPath.moveTo(m_drawingState->strokePoints.first());
        for (int i = 1; i < m_drawingState->strokePoints.size(); ++i)
            previewPath.lineTo(m_drawingState->strokePoints[i]);
    } else {
        previewPath = PathGenerator::generateBezierPath(
            PathGenerator::applyChaikinAlgorithm(m_drawingState->strokePoints, m_viewModel->smoothingLevel()));
    }

    m_drawingState->freehandPreviewPath = previewPath;
    m_drawingState->freehandPreviewPointCount = m_drawingState->strokePoints.size();
    m_drawingState->freehandPreviewSmoothingLevel = m_viewModel->smoothingLevel();
}

bool CustomDrawArea::isInteractiveRenderingActive() const
{
    if (m_viewModel->performanceMode() == PerformanceMode::Quality)
        return false;
    return m_drawing
        || m_moveInProgress
        || m_resizeInProgress
        || m_rotateInProgress
        || m_mouseHandler->isSelectionDragActive();
}


void CustomDrawArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    const bool interactive = isInteractiveRenderingActive();
    painter.setRenderHint(QPainter::Antialiasing,
                          m_viewModel->performanceMode() != PerformanceMode::LowEnd || !interactive);
    painter.setBrush(Qt::NoBrush);
    painter.translate(m_transformer->offset());
    painter.scale(m_transformer->scale(), m_transformer->scale());

    const QRectF paintRect = event ? event->rect() : rect();
    const QRectF visibleArea = QRectF(toLogical(paintRect.topLeft()),
                                      toLogical(paintRect.bottomRight())).normalized();
    const auto quality = interactive
        ? ShapeRenderer::RenderQuality::Interactive
        : ShapeRenderer::RenderQuality::Full;
    m_renderer->render(painter, *(m_viewModel->shapeManager()), visibleArea, quality);

    drawValidationOverlay(painter);
    drawOpenEndpointHandles(painter);
    drawSmartGuides(painter);

    if (!m_viewModel->selectedShapes().empty()) {
        QPen halo(QColor(72, 187, 255), 10); halo.setCosmetic(true);
        QPen normal(Qt::black, 2); normal.setCosmetic(true);
        for (int idx : m_viewModel->selectedShapes()) {
            if (idx < 0 || idx >= static_cast<int>(m_viewModel->shapes().size())) continue;
            const QPainterPath &path = m_viewModel->shapes()[idx].path;
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
            painter.setPen(halo);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
            painter.setPen(normal);
            painter.drawPath(path);
        }

        const QRectF bounds = selectionOverlayBounds();
        const QTransform overlayTransform = selectionRotationTransform();
        const QPointF topLeft = overlayTransform.map(bounds.topLeft());
        const QPointF topRight = overlayTransform.map(bounds.topRight());
        const QPointF bottomLeft = overlayTransform.map(bounds.bottomLeft());
        const QPointF bottomRight = overlayTransform.map(bounds.bottomRight());
        const QPointF topCenter = overlayTransform.map(QPointF(bounds.center().x(), bounds.top()));
        const QPointF handlePoint = rotationHandlePoint();
        painter.setPen(QPen(QColor(43, 122, 255), 2));
        painter.setBrush(QColor(43, 122, 255, 26));
        painter.drawPolygon(QPolygonF() << topLeft << topRight << bottomRight << bottomLeft);
        painter.setBrush(QColor(43, 122, 255));
        painter.drawEllipse(topLeft, 9.0, 9.0);
        painter.drawEllipse(topRight, 9.0, 9.0);
        painter.drawEllipse(bottomLeft, 9.0, 9.0);
        painter.drawEllipse(bottomRight, 9.0, 9.0);
        painter.drawLine(topCenter, handlePoint);
        painter.setBrush(Qt::white);
        painter.drawEllipse(handlePoint, 11.0, 11.0);
        painter.setBrush(QColor(43, 122, 255));
        const QLineF arrowLine(topCenter, handlePoint);
        if (arrowLine.length() > 0.0) {
            const qreal headLength = 14.0 / qMax<qreal>(0.25, m_transformer->scale());
            const qreal headWidth = 7.0 / qMax<qreal>(0.25, m_transformer->scale());
            QLineF shaft = arrowLine;
            shaft.setP1(handlePoint);
            shaft.setLength(headLength);
            const QPointF arrowBase = shaft.p2();
            QPointF unit = handlePoint - arrowBase;
            const qreal unitLen = std::hypot(unit.x(), unit.y());
            if (unitLen > 0.0) {
                unit /= unitLen;
                const QPointF perp(-unit.y(), unit.x());
                QPolygonF arrowHead;
                arrowHead << handlePoint
                          << (arrowBase + perp * headWidth)
                          << (arrowBase - perp * headWidth);
                painter.drawPolygon(arrowHead);
                painter.drawArc(QRectF(handlePoint.x() - 16.0, handlePoint.y() - 16.0, 32.0, 32.0), 40 * 16, 220 * 16);
            }
        }
    }

    if (getDrawMode() == DrawMode::Freehand && m_drawing &&
        m_drawingState->strokePoints.size() > 1) {
        QPen pen(QColor(71, 85, 105), 1.2, Qt::DashLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        updateFreehandPreviewCache();
        painter.drawPath(m_drawingState->freehandPreviewPath);
    }

    if (m_drawing && (getDrawMode() == DrawMode::Line ||
                      getDrawMode() == DrawMode::Rectangle ||
                      getDrawMode() == DrawMode::Circle)) {
        QPen pen(QColor(71, 85, 105), 1.2, Qt::DashLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        const QRectF rect(m_drawingState->startPoint, m_drawingState->currentPoint);
        if (getDrawMode() == DrawMode::Line)
            painter.drawLine(m_drawingState->startPoint, m_drawingState->currentPoint);
        else if (getDrawMode() == DrawMode::Rectangle)
            painter.drawRect(rect.normalized());
        else if (getDrawMode() == DrawMode::Circle)
            painter.drawEllipse(rect.normalized());
    }

    if (getDrawMode() == DrawMode::PointParPoint &&
        !m_drawingState->pointByPointPoints.isEmpty()) {
        QPen pen(QColor(71, 85, 105), 1.2, Qt::DashLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        QPainterPath previewPath;
        previewPath.moveTo(m_drawingState->pointByPointPoints.first());
        for (int i = 1; i < m_drawingState->pointByPointPoints.size(); ++i)
            previewPath.lineTo(m_drawingState->pointByPointPoints[i]);
        previewPath.lineTo(m_drawingState->currentPoint);
        painter.drawPath(previewPath);
    }

    drawPointByPointAids(painter);

    if (getDrawMode() == DrawMode::Gomme && m_drawingState->gommeErasing) {
        QPen pen(QColor(220, 38, 38), 2, Qt::DashLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.drawEllipse(m_drawingState->gommeCenter,
                            m_eraserTool->eraserRadius(), m_eraserTool->eraserRadius());
    }

    painter.resetTransform();
    drawCanvasHud(painter);
}

void CustomDrawArea::mousePressEvent(QMouseEvent *event)
{
    m_canvasStatusThrottleStarted = false;
    const QPointF rawLogical = toLogical(event->position());
    const QPointF logical = applyDrawingAids(rawLogical);
    m_lastRawPointerLogical = rawLogical;
    m_lastPointerLogical = logical;
    m_hasPointer = true;
    m_drawingState->currentPoint = logical;

    if (event->button() == Qt::LeftButton && hasSelection()) {
        if (hitTestRotationHandle(logical)) {
            m_rotateInProgress = true;
            m_transformStartBounds = selectionOverlayBounds();
            m_transformStartState = m_viewModel->getCurrentState();
            m_rotateStartPointerAngle = std::atan2(logical.y() - m_transformStartBounds.center().y(),
                                                   logical.x() - m_transformStartBounds.center().x());
            m_drawing = false;
            emitCanvasStatus();
            return;
        }

        const ResizeHandle handle = hitTestSelectionHandle(logical);
        if (handle != ResizeHandle::None) {
            m_activeResizeHandle = handle;
            m_resizeInProgress = true;
            m_transformStartBounds = selectionOverlayBounds();
            m_transformStartState = m_viewModel->getCurrentState();
            m_drawing = false;
            emitCanvasStatus();
            return;
        }
    }

    if (m_modeManager->isCloseMode()) {
        const int hit = hitTestShape(logical);
        if (hit >= 0) {
            auto oldState = m_viewModel->getCurrentState();
            auto shapes = m_viewModel->shapes();
            QPainterPath &path = shapes[hit].path;
            if (path.elementCount() > 1 && !m_viewModel->isPathClosed(path)) {
                path.closeSubpath();
                m_viewModel->shapeManager()->setShapes(shapes);
                m_viewModel->commitSnapshot(oldState, shapes, tr("Fermer trace"));
                emitHistoryState();
            }
        }
        m_modeManager->cancelCloseMode();
        emitCanvasStatus();
        update();
        return;
    }

    if (event->button() == Qt::RightButton && getDrawMode() == DrawMode::PointParPoint) {
        m_drawingState->pointByPointPoints.clear();
        resetPointByPointResumeState();
        m_drawing = false;
        emitCanvasStatus();
        update();
        return;
    }

    const bool canDragSelectedShape =
        event->button() == Qt::LeftButton &&
        m_modeManager->isMultiSelectMode() &&
        isPointInsideSelectedShape(logical);

    if (m_modeManager->isSelectMode() && getDrawMode() != DrawMode::Deplacer && !canDragSelectedShape) {
        const int hit = hitTestShape(logical);
        if (hit >= 0) {
            m_lastSelectClick = logical;
            std::vector<int> selected = m_viewModel->selectedShapes();
            const bool alreadySelected = std::find(selected.begin(), selected.end(), hit) != selected.end();
            if (alreadySelected)
                selected.erase(std::remove(selected.begin(), selected.end(), hit), selected.end());
            else
                selected.push_back(hit);
            m_viewModel->shapeManager()->setSelectedShapes(selected);

            if (m_modeManager->isConnectMode() && selected.size() == 2) {
                auto old = m_viewModel->getCurrentState();
                m_viewModel->shapeManager()->mergeShapesByNearestEndpoints(selected[0], selected[1]);
                m_viewModel->commitSnapshot(old, m_viewModel->shapes(), tr("Connecter formes"));
                m_viewModel->shapeManager()->clearSelection();
                m_modeManager->cancelSelectConnect();
                emitHistoryState();
            } else if (m_modeManager->isMultiSelectMode()) {
                emit multiSelectionModeChanged(true);
            }
            emitCanvasStatus();
            update();
        }
        return;
    }

    if (event->button() == Qt::LeftButton && (getDrawMode() == DrawMode::Text || getDrawMode() == DrawMode::ThinText)) {
        bool ok = false;
        const QString text = QInputDialog::getText(this, tr("Texte"), tr("Saisir le texte"),
                                                   QLineEdit::Normal, m_textTool->getCurrentText(), &ok);
        if (ok && !text.isEmpty()) {
            m_textTool->setCurrentText(text);

            QPainterPath rawTextPath;
            rawTextPath.addText(logical, m_textTool->getTextFont(), text);

            QPainterPath hollowPath;
            for (const QPolygonF &poly : rawTextPath.toSubpathPolygons()) {
                if (poly.isEmpty()) continue;
                hollowPath.moveTo(poly.first());
                for (int i = 1; i < poly.size(); ++i)
                    hollowPath.lineTo(poly[i]);
            }

            const int id = m_viewModel->nextShapeId();
            m_viewModel->commitAddShape(hollowPath, id, tr("Ajouter texte"));
            emitHistoryState();
            emitCanvasStatus();
        }
        return;
    }

    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::PointParPoint) {
        if (m_drawingState->pointByPointPoints.isEmpty()
            && beginEndpointResumeIfNeeded(logical)) {
            return;
        }

        if (m_drawingState->pointByPointPoints.size() >= 3 &&
            QLineF(m_drawingState->pointByPointPoints.first(), logical).length() <= endpointTouchTolerance()) {
            m_drawingState->pointByPointPoints.append(m_drawingState->pointByPointPoints.first());
            finishPointByPointShape();
            return;
        }
        m_drawingState->pointByPointPoints.append(logical);
        m_drawingState->currentPoint = logical;
        m_drawing = true;
        emitCanvasStatus();
        update();
        return;
    }

    if (event->button() == Qt::LeftButton
        && (getDrawMode() == DrawMode::Freehand || getDrawMode() == DrawMode::Line)
        && beginEndpointResumeIfNeeded(logical)) {
        m_moveInProgress = false;
        return;
    }

    m_mouseHandler->handleMousePress(event, logical);
    const bool selectionDragActive = m_mouseHandler->isSelectionDragActive();
    m_drawing = (event->button() == Qt::LeftButton) && !selectionDragActive;
    if (event->button() == Qt::LeftButton &&
        (getDrawMode() == DrawMode::Deplacer || selectionDragActive)) {
        m_moveStartState = m_viewModel->getCurrentState();
        m_moveInProgress = true;
        m_drawing = false;
        emitCanvasStatus();
        update();
        return;
    } else {
        m_moveInProgress = false;
    }
    m_drawingState->startPoint = logical;
    m_drawingState->strokePoints = {logical};
    m_drawingState->freehandPreviewPath = QPainterPath();
    m_drawingState->freehandPreviewPointCount = -1;
    m_drawingState->freehandPreviewSmoothingLevel = -1;

    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::Gomme) {
        m_drawingState->gommeErasing = true;
        m_drawingState->gommeCenter = logical;
        m_drawingState->lastEraserPos = logical;
        m_eraserTool->applyEraserAt(logical);
        emitCanvasStatus();
        update();
    }
}

void CustomDrawArea::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::PointParPoint) {
        finishPointByPointShape();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CustomDrawArea::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF rawLogical = toLogical(event->position());
    const QPointF logical = applyDrawingAids(rawLogical);
    m_lastRawPointerLogical = rawLogical;
    m_lastPointerLogical = logical;
    m_hasPointer = true;

    if (getDrawMode() == DrawMode::PointParPoint && !m_drawingState->pointByPointPoints.isEmpty()) {
        m_drawingState->currentPoint = logical;
        emitCanvasStatusThrottled();
        update();
    }

    const QPointF freehandLogical = logical;

    m_mouseHandler->handleMouseMove(event, logical);

    if (m_mouseHandler->isSelectionDragActive()
        || (m_moveInProgress && getDrawMode() == DrawMode::Deplacer)) {
        m_drawing = false;
        applyPlacementAssistToSelection();
        emitCanvasStatusThrottled();
        update();
        return;
    }

    if (m_rotateInProgress) {
        const QPointF center = m_transformStartBounds.center();
        const qreal currentPointerAngle = std::atan2(logical.y() - center.y(), logical.x() - center.x());
        qreal deltaDegrees = (currentPointerAngle - m_rotateStartPointerAngle) * 180.0 / M_PI;
        if (m_viewModel->isPrecisionConstraintEnabled())
            deltaDegrees = std::round(deltaDegrees / 15.0) * 15.0;

        auto updated = m_transformStartState;
        const auto selected = m_viewModel->selectedShapes();
        QTransform transform;
        transform.translate(center.x(), center.y());
        transform.rotate(deltaDegrees);
        transform.translate(-center.x(), -center.y());

        for (int idx : selected) {
            if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
            updated[idx].path = transform.map(m_transformStartState[idx].path);
            updated[idx].rotationAngle = m_transformStartState[idx].rotationAngle + deltaDegrees;
        }

        m_viewModel->shapeManager()->setShapes(updated);
        m_viewModel->shapeManager()->setSelectedShapes(selected);
        emitCanvasStatusThrottled();
        return;
    }

    if (m_resizeInProgress && m_activeResizeHandle != ResizeHandle::None) {
        const QRectF targetRect = resizeRectFromHandle(logical);
        if (m_transformStartBounds.width() <= 0.0 || m_transformStartBounds.height() <= 0.0) return;

        auto updated = m_transformStartState;
        const auto selected = m_viewModel->selectedShapes();
        const QPointF center = m_transformStartBounds.center();
        QTransform rotateToWorld;
        rotateToWorld.translate(center.x(), center.y());
        rotateToWorld.rotate(currentSelectionAngle());
        rotateToWorld.translate(-center.x(), -center.y());

        QRectF finalRect = targetRect;
        if (m_viewModel->isPrecisionConstraintEnabled()) {
            const qreal side = qMax(finalRect.width(), finalRect.height());
            if (m_activeResizeHandle == ResizeHandle::TopLeft)
                finalRect = QRectF(finalRect.bottomRight() - QPointF(side, side), finalRect.bottomRight()).normalized();
            else if (m_activeResizeHandle == ResizeHandle::TopRight)
                finalRect = QRectF(QPointF(finalRect.left(), finalRect.bottom() - side), QPointF(finalRect.left() + side, finalRect.bottom())).normalized();
            else if (m_activeResizeHandle == ResizeHandle::BottomLeft)
                finalRect = QRectF(QPointF(finalRect.right() - side, finalRect.top()), QPointF(finalRect.right(), finalRect.top() + side)).normalized();
            else
                finalRect = QRectF(finalRect.topLeft(), finalRect.topLeft() + QPointF(side, side)).normalized();
        }

        QTransform localScale;
        localScale.translate(finalRect.left(), finalRect.top());
        localScale.scale(finalRect.width() / m_transformStartBounds.width(),
                         finalRect.height() / m_transformStartBounds.height());
        localScale.translate(-m_transformStartBounds.left(), -m_transformStartBounds.top());

        const QTransform transform = rotateToWorld * localScale * selectionInverseRotationTransform();

        for (int idx : selected) {
            if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
            updated[idx].path = transform.map(m_transformStartState[idx].path);
        }

        m_viewModel->shapeManager()->setShapes(updated);
        m_viewModel->shapeManager()->setSelectedShapes(selected);
        emitCanvasStatusThrottled();
        return;
    }

    if (!m_drawing) {
        emitCanvasStatusThrottled();
        return;
    }

    if (getDrawMode() == DrawMode::Deplacer || getDrawMode() == DrawMode::Supprimer || getDrawMode() == DrawMode::Pan)
        return;

    m_drawingState->currentPoint = freehandLogical;
    if (getDrawMode() == DrawMode::Gomme) {
        if (!m_drawingState->gommeErasing) return;
        m_drawingState->gommeCenter = freehandLogical;
        m_eraserTool->eraseAlong(m_drawingState->lastEraserPos, freehandLogical);
        m_drawingState->lastEraserPos = freehandLogical;
    } else if (getDrawMode() == DrawMode::Freehand) {
        const qreal minDistance = 2.0 / qMax<qreal>(0.35, m_transformer->scale());
        if (m_drawingState->strokePoints.isEmpty()) {
            m_drawingState->strokePoints.append(freehandLogical);
        } else {
            const QPointF lastPoint = m_drawingState->strokePoints.last();
            if (QLineF(lastPoint, freehandLogical).length() >= minDistance)
                m_drawingState->strokePoints.append(freehandLogical);
        }
    }
    emitCanvasStatusThrottled();
    update();
}

void CustomDrawArea::mouseReleaseEvent(QMouseEvent *event)
{
    const QPointF logical = applyDrawingAids(toLogical(event->position()));

    if (m_rotateInProgress) {
        const auto rotatedState = m_viewModel->getCurrentState();
        bool changed = rotatedState.size() != m_transformStartState.size();
        if (!changed) {
            for (size_t i = 0; i < rotatedState.size(); ++i) {
                if (rotatedState[i].path != m_transformStartState[i].path) {
                    changed = true;
                    break;
                }
            }
        }
        if (changed)
            m_viewModel->commitSnapshot(m_transformStartState, rotatedState, tr("Tourner forme"));

        m_rotateInProgress = false;
        emitHistoryState();
        emitSelectionState();
        emitCanvasStatus();
        update();
        return;
    }

    if (m_resizeInProgress) {
        const auto resizedState = m_viewModel->getCurrentState();
        bool changed = resizedState.size() != m_transformStartState.size();
        if (!changed) {
            for (size_t i = 0; i < resizedState.size(); ++i) {
                if (resizedState[i].path != m_transformStartState[i].path) {
                    changed = true;
                    break;
                }
            }
        }
        if (changed)
            m_viewModel->commitSnapshot(m_transformStartState, resizedState, tr("Redimensionner forme"));

        m_resizeInProgress = false;
        m_activeResizeHandle = ResizeHandle::None;
        emitHistoryState();
        emitSelectionState();
        emitCanvasStatus();
        update();
        return;
    }

    if (getDrawMode() == DrawMode::PointParPoint) {
        m_drawingState->currentPoint = logical;
        emitCanvasStatus();
        update();
        return;
    }

    m_mouseHandler->handleMouseRelease(event, logical);
    if (!m_drawing) return;
    m_drawing = false;

    if (m_moveInProgress || getDrawMode() == DrawMode::Deplacer || getDrawMode() == DrawMode::Supprimer || getDrawMode() == DrawMode::Pan) {
        if (m_moveInProgress) {
            const auto movedState = m_viewModel->getCurrentState();
            bool changed = movedState.size() != m_moveStartState.size();
            if (!changed) {
                for (size_t i = 0; i < movedState.size(); ++i) {
                    const auto &a = movedState[i];
                    const auto &b = m_moveStartState[i];
                    if (a.originalId != b.originalId || std::abs(a.rotationAngle - b.rotationAngle) > 1e-9 || a.path != b.path) {
                        changed = true;
                        break;
                    }
                }
            }
            if (changed)
                m_viewModel->commitSnapshot(m_moveStartState, movedState, tr("Deplacer forme"));
            m_moveInProgress = false;
            emitHistoryState();
        }
        m_activePlacementGuides.clear();
        emitCanvasStatus();
        update();
        return;
    }

    if (getDrawMode() == DrawMode::Gomme) {
        m_drawingState->gommeErasing = false;
        emitCanvasStatus();
        return;
    }

    QPainterPath path;
    if (getDrawMode() == DrawMode::Line) {
        path.moveTo(m_drawingState->startPoint);
        path.lineTo(logical);
    } else if (getDrawMode() == DrawMode::Rectangle) {
        path.addRect(QRectF(m_drawingState->startPoint, logical).normalized());
    } else if (getDrawMode() == DrawMode::Circle) {
        path.addEllipse(QRectF(m_drawingState->startPoint, logical).normalized());
    } else {
        if (getDrawMode() == DrawMode::Freehand && m_drawingState->strokePoints.size() <= 1) {
            const QPointF endPoint = logical;
            path.moveTo(m_drawingState->strokePoints.first());
            if (QLineF(m_drawingState->strokePoints.first(), endPoint).length() > 0.01)
                path.lineTo(endPoint);
        } else if (m_viewModel->smoothingLevel() <= 0) {
            path.moveTo(m_drawingState->strokePoints.first());
            for (int i = 1; i < m_drawingState->strokePoints.size(); ++i)
                path.lineTo(m_drawingState->strokePoints[i]);
        } else {
            path = PathGenerator::generateBezierPath(
                PathGenerator::applyChaikinAlgorithm(m_drawingState->strokePoints, m_viewModel->smoothingLevel()));
        }
    }

    if (!path.isEmpty()) {
        if (m_drawingState->extendingExistingPath
            && (getDrawMode() == DrawMode::Freehand || getDrawMode() == DrawMode::Line)) {
            const auto oldState = m_viewModel->getCurrentState();
            if (m_drawingState->extendingShapeIndex >= 0
                && m_drawingState->extendingShapeIndex < static_cast<int>(oldState.size())) {
                auto updated = oldState;
                updated[m_drawingState->extendingShapeIndex].path = buildResumedPath(path);
                m_viewModel->shapeManager()->setShapes(updated);
                m_viewModel->commitSnapshot(oldState, updated, tr("Prolonger trace"));
            }
            resetPointByPointResumeState();
            m_hoveredEndpoint = {};
        } else {
            const int id = m_viewModel->nextShapeId();
            m_viewModel->commitAddShape(path, id);
        }
        emitHistoryState();
    }
    m_activePlacementGuides.clear();
    emitCanvasStatus();
    update();
}

void CustomDrawArea::wheelEvent(QWheelEvent *event)
{
    const qreal factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    const QPointF cursor = event->position();
    const QPointF logicalAtCursor = toLogical(cursor);
    m_transformer->setScale(m_transformer->scale() * factor);
    m_transformer->setOffset(cursor - logicalAtCursor * m_transformer->scale());
    emitCanvasStatus();
    event->accept();
}
