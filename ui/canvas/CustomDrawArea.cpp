#include "CustomDrawArea.h"

#include "DrawModeManager.h"
#include "DrawingState.h"
#include "EraserTool.h"
#include "HistoryManager.h"
#include "MouseInteractionHandler.h"
#include "domain/shapes/PathGenerator.h"
#include "domain/shapes/ShapeManager.h"
#include "ShapeRenderer.h"
#include "TextTool.h"
#include "ViewTransformer.h"

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
    , m_shapeManager  (std::make_unique<ShapeManager>(this))
    , m_renderer      (std::make_unique<ShapeRenderer>(this))
    , m_modeManager   (std::make_unique<DrawModeManager>(this))
    , m_historyManager(std::make_unique<HistoryManager>(m_shapeManager.get(), this))
    , m_transformer   (std::make_unique<ViewTransformer>(this))
    , m_drawingState  (std::make_unique<DrawingState>())
    , m_mouseHandler  (std::make_unique<MouseInteractionHandler>(
          m_shapeManager.get(), m_modeManager.get(), m_transformer.get(),
          m_drawingState.get(), this))
    , m_eraserTool    (std::make_unique<EraserTool>(m_shapeManager.get(), this))
    , m_textTool      (std::make_unique<TextTool>(this))
{
    setMouseTracking(true);
    setAutoFillBackground(true);


    connect(m_shapeManager.get(), &ShapeManager::shapesChanged,
            this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_shapeManager.get(), &ShapeManager::selectionChanged,
            this, QOverload<>::of(&CustomDrawArea::update));
    connect(m_mouseHandler.get(), &MouseInteractionHandler::requestUpdate,
            this, QOverload<>::of(&CustomDrawArea::update));

    connect(m_modeManager.get(), &DrawModeManager::drawModeChanged,
            this, &CustomDrawArea::onDrawModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::closeModeChanged,
            this, &CustomDrawArea::closeModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::deplacerModeChanged,
            this, &CustomDrawArea::deplacerModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::supprimerModeChanged,
            this, &CustomDrawArea::supprimerModeChanged);
    connect(m_modeManager.get(), &DrawModeManager::gommeModeChanged,
            this, &CustomDrawArea::gommeModeChanged);
    // Selection overlays — relay from manager to widget's own signals
    connect(m_modeManager.get(), &DrawModeManager::shapeSelectionChanged,
            this, &CustomDrawArea::shapeSelection);
    connect(m_modeManager.get(), &DrawModeManager::multiSelectionChanged,
            this, &CustomDrawArea::multiSelectionModeChanged);

    connect(m_transformer.get(), &ViewTransformer::zoomChanged,
            this, &CustomDrawArea::zoomChanged);
    connect(m_transformer.get(), &ViewTransformer::viewTransformed,
            this, QOverload<>::of(&CustomDrawArea::update));
}

CustomDrawArea::~CustomDrawArea() = default;

// ---------------------------------------------------------------------------
// Mode de dessin
// ---------------------------------------------------------------------------

void CustomDrawArea::setDrawMode(DrawMode mode)
{
    Q_ASSERT(m_modeManager   != nullptr);
    Q_ASSERT(m_drawingState  != nullptr);
    if (m_modeManager->isSelectMode()) cancelSelection();
    if (m_modeManager->isCloseMode())  cancelCloseMode();
    m_drawingState->gommeErasing = false;
    m_drawingState->pointByPointPoints.clear();
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

// ---------------------------------------------------------------------------
// Formes
// ---------------------------------------------------------------------------

QList<QPolygonF> CustomDrawArea::getCustomShapes() const
{
    Q_ASSERT(m_shapeManager != nullptr);
    QList<QPolygonF> polygons;

    for (const auto &shape : m_shapeManager->shapes()) {
        // --- LA CORRECTION EST ICI ---
        // Au lieu de forcer un seul polygone (toFillPolygon), on extrait
        // chaque sous-tracé (chaque lettre et contour) séparément !
        const QList<QPolygonF> subPolys = shape.path.toSubpathPolygons();
        polygons.append(subPolys);
        // -----------------------------
    }

    return polygons;
}

void CustomDrawArea::clearDrawing()
{
    auto oldState = m_historyManager->getCurrentState();
    m_shapeManager->clearShapes();
    m_historyManager->commitSnapshot(oldState, {}, tr("Tout effacer"));
}

void CustomDrawArea::undoLastAction() {
    m_historyManager->undo();
}

void CustomDrawArea::setEraserRadius(qreal radius)
{
    Q_ASSERT(m_eraserTool != nullptr);
    m_eraserTool->setEraserRadius(radius);
}

void CustomDrawArea::addImportedLogo(const QPainterPath &logoPath)
{
    int id = m_nextShapeId++;
    m_shapeManager->addImportedLogo(logoPath, id);
    m_historyManager->commitAddShape(logoPath, id, tr("Import Logo"));
    update();
}

void CustomDrawArea::addImportedLogoSubpath(const QPainterPath &subpath)
{
    int id = m_nextShapeId++;
    m_shapeManager->addImportedLogoSubpath(subpath, id);
    m_historyManager->commitAddShape(subpath, id, tr("Import Sous-tracé"));
    update();
}

// ---------------------------------------------------------------------------
// Texte
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Sélection
// ---------------------------------------------------------------------------

void CustomDrawArea::startShapeSelection()
{
    cancelCloseMode();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_shapeManager->clearSelection();
    m_modeManager->startSelectConnect();
    update();
}

void CustomDrawArea::cancelSelection()
{
    if (!m_modeManager->isSelectMode()) return;
    m_shapeManager->clearSelection();
    m_modeManager->cancelAnySelection();
    update();
}

void CustomDrawArea::toggleMultiSelectMode()
{
    if (!m_modeManager->isSelectMode()) {
        cancelCloseMode();
        cancelSupprimerMode();
        cancelDeplacerMode();
        cancelGommeMode();
        m_shapeManager->clearSelection();
        m_modeManager->startSelectMulti();
    } else if (m_modeManager->isMultiSelectMode()) {
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
    auto selected = m_shapeManager->selectedShapes();
    if (selected.empty()) return;

    m_historyManager->commitDeleteShapes(selected);

    m_shapeManager->clearSelection();
    m_modeManager->cancelAnySelection();
}

void CustomDrawArea::copySelectedShapes()
{
    Q_ASSERT(m_shapeManager != nullptr);
    m_shapeManager->copySelectedShapes();
}

void CustomDrawArea::enablePasteMode() { m_modeManager->enablePasteMode(); }

void CustomDrawArea::pasteCopiedShapes(const QPointF &dest)
{
    const auto pasted = m_shapeManager->pastedShapes(dest);
    if (pasted.empty()) return;

    std::vector<ShapeManager::Shape> shapesWithIds;
    for (auto s : pasted) {
        s.originalId = m_nextShapeId++;
        shapesWithIds.push_back(s);
    }

    m_historyManager->commitPasteShapes(shapesWithIds);
}

// ---------------------------------------------------------------------------
// Grille
// ---------------------------------------------------------------------------

void CustomDrawArea::setSnapToGridEnabled(bool enabled) { Q_ASSERT(m_renderer); m_renderer->setSnapToGridEnabled(enabled); update(); }
bool CustomDrawArea::isSnapToGridEnabled()        const { Q_ASSERT(m_renderer); return m_renderer->isSnapToGridEnabled(); }
void CustomDrawArea::setGridVisible(bool visible)       { Q_ASSERT(m_renderer); m_renderer->setGridVisible(visible);      update(); }
bool CustomDrawArea::isGridVisible()              const { Q_ASSERT(m_renderer); return m_renderer->isGridVisible(); }
void CustomDrawArea::setGridSpacing(int px)             { Q_ASSERT(m_renderer); m_renderer->setGridSpacing(px);           update(); }
int  CustomDrawArea::gridSpacing()                const { Q_ASSERT(m_renderer); return m_renderer->gridSpacing(); }

// ---------------------------------------------------------------------------
// Requêtes d'état
// ---------------------------------------------------------------------------

bool CustomDrawArea::isDeplacerMode()  const { return getDrawMode() == DrawMode::Deplacer; }
bool CustomDrawArea::isSupprimerMode() const { return getDrawMode() == DrawMode::Supprimer; }
bool CustomDrawArea::isGommeMode()     const { return getDrawMode() == DrawMode::Gomme; }
bool CustomDrawArea::isConnectMode()   const { return m_modeManager->isConnectMode(); }

void CustomDrawArea::onDrawModeChanged(DrawModeManager::DrawMode mode)
{
    emit drawModeChanged(mode);
    update();
}

// ---------------------------------------------------------------------------
// Modes spéciaux
// ---------------------------------------------------------------------------

void CustomDrawArea::startCloseMode()
{
    cancelSelection();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_modeManager->startCloseMode();
}
void CustomDrawArea::cancelCloseMode()
{
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
void CustomDrawArea::cancelDeplacerMode()  { m_modeManager->cancelDeplacerMode(); }
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
void CustomDrawArea::cancelGommeMode()     { m_modeManager->cancelGommeMode(); }

void CustomDrawArea::setSmoothingLevel(int level)
{
    m_smoothingLevel = qMax(0, level);
    emit smoothingLevelChanged(m_smoothingLevel);
}

void CustomDrawArea::setTwoFingersOn(bool active)
{
    m_twoFingersOn = active;
    if (active) m_drawingState->gommeErasing = false;
}

void CustomDrawArea::handlePinchZoom(QPointF center, qreal factor)
{
    const qreal    oldScale      = m_transformer->scale();
    const QPointF  logicalCenter = toLogical(center);
    m_transformer->setScale(oldScale * factor);
    m_transformer->setOffset(center - logicalCenter * m_transformer->scale());
}

// ---------------------------------------------------------------------------
// Helpers privés
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Rendu
// ---------------------------------------------------------------------------

void CustomDrawArea::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);
    painter.translate(m_transformer->offset());
    painter.scale(m_transformer->scale(), m_transformer->scale());

    const QRectF visibleArea = QRectF(toLogical(QPointF(0, 0)),
                                      toLogical(QPointF(width(), height()))).normalized();
    m_renderer->render(painter, *m_shapeManager, visibleArea);

    if (!m_shapeManager->selectedShapes().empty()) {
        QPen halo(Qt::cyan, 6);   halo.setCosmetic(true);
        QPen normal(Qt::black, 2); normal.setCosmetic(true);
        for (int idx : m_shapeManager->selectedShapes()) {
            if (idx < 0 || idx >= static_cast<int>(m_shapeManager->shapes().size())) continue;
            const QPainterPath &path = m_shapeManager->shapes()[idx].path;
            painter.setPen(halo);   painter.setBrush(Qt::NoBrush); painter.drawPath(path);
            painter.setPen(normal);                                 painter.drawPath(path);
        }
    }

    if (getDrawMode() == DrawMode::Freehand && m_drawing &&
        m_drawingState->strokePoints.size() > 1) {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        painter.drawPath(PathGenerator::generateBezierPath(m_drawingState->strokePoints));
    }

    if (m_drawing && (getDrawMode() == DrawMode::Line      ||
                      getDrawMode() == DrawMode::Rectangle ||
                      getDrawMode() == DrawMode::Circle)) {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        const QRectF rect(m_drawingState->startPoint, m_drawingState->currentPoint);
        if      (getDrawMode() == DrawMode::Line)      painter.drawLine(m_drawingState->startPoint, m_drawingState->currentPoint);
        else if (getDrawMode() == DrawMode::Rectangle) painter.drawRect(rect.normalized());
        else if (getDrawMode() == DrawMode::Circle)    painter.drawEllipse(rect.normalized());
    }

    if (getDrawMode() == DrawMode::PointParPoint &&
        !m_drawingState->pointByPointPoints.isEmpty()) {
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        QPainterPath previewPath;
        previewPath.moveTo(m_drawingState->pointByPointPoints.first());
        for (int i = 1; i < m_drawingState->pointByPointPoints.size(); ++i)
            previewPath.lineTo(m_drawingState->pointByPointPoints[i]);
        previewPath.lineTo(m_drawingState->currentPoint);
        painter.drawPath(previewPath);
    }

    if (getDrawMode() == DrawMode::Gomme && m_drawingState->gommeErasing) {
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.drawEllipse(m_drawingState->gommeCenter,
                            m_eraserTool->eraserRadius(), m_eraserTool->eraserRadius());
    }
}

// ---------------------------------------------------------------------------
// Gestion souris
// ---------------------------------------------------------------------------

void CustomDrawArea::mousePressEvent(QMouseEvent *event)
{
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
    m_drawingState->currentPoint = logical;

    if (m_modeManager->isCloseMode()) {
        const int hit = hitTestShape(logical);
        if (hit >= 0) {
            auto oldState = m_historyManager->getCurrentState();
            auto shapes = m_shapeManager->shapes();
            QPainterPath &path = shapes[hit].path;
            if (path.elementCount() > 1) {
                const auto first = path.elementAt(0);
                path.lineTo(QPointF(first.x, first.y));
                m_shapeManager->setShapes(shapes);
                m_historyManager->commitSnapshot(oldState, shapes, tr("Fermer tracé"));
            }
        }
        m_modeManager->cancelCloseMode();
        update();
        return;
    }

    if (event->button() == Qt::RightButton && m_modeManager->isPasteMode()) {
        pasteCopiedShapes(logical);
        m_modeManager->cancelPasteMode();
        return;
    }

    if (event->button() == Qt::RightButton && getDrawMode() == DrawMode::PointParPoint) {
        m_drawingState->pointByPointPoints.clear();
        m_drawing = false;
        update();
        return;
    }

    if (m_modeManager->isSelectMode() && getDrawMode() != DrawMode::Deplacer) {
        const int hit = hitTestShape(logical);
        if (hit >= 0) {
            m_lastSelectClick = logical;
            std::vector<int> selected = m_shapeManager->selectedShapes();
            const bool alreadySelected = std::find(selected.begin(), selected.end(), hit) != selected.end();
            if (alreadySelected)
                selected.erase(std::remove(selected.begin(), selected.end(), hit), selected.end());
            else
                selected.push_back(hit);
            m_shapeManager->setSelectedShapes(selected);

            if (m_modeManager->isConnectMode() && selected.size() == 2) {
                auto old = m_historyManager->getCurrentState();
                m_shapeManager->connectNearestEndpoints(selected[0], selected[1]);
                m_shapeManager->mergeShapesAndConnector(selected[0], selected[1]);
                m_historyManager->commitSnapshot(old, m_shapeManager->shapes(), tr("Connecter formes"));
                m_shapeManager->clearSelection();
                m_modeManager->cancelSelectConnect();
            } else if (m_modeManager->isMultiSelectMode()) {
                // signal already emitted by DrawModeManager on startSelectMulti;
                // emit again to keep the button highlighted after each click
                emit multiSelectionModeChanged(true);
            }
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

            // 1. On génère le tracé texte de base
            QPainterPath rawTextPath;
            rawTextPath.addText(logical, m_textTool->getTextFont(), text);

            // 2. Conversion en filaire pur (Hollow Text)
            QPainterPath hollowPath;
            for (const QPolygonF& poly : rawTextPath.toSubpathPolygons()) {
                if (poly.isEmpty()) continue;
                hollowPath.moveTo(poly.first());
                for (int i = 1; i < poly.size(); ++i) {
                    hollowPath.lineTo(poly[i]);
                }
            }

            int id = m_nextShapeId++;
            // 3. On sauvegarde le tracé creux au lieu du texte brut
            m_historyManager->commitAddShape(hollowPath, id, tr("Ajouter texte"));
        }
        return;
    }

    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::PointParPoint) {
        m_drawingState->pointByPointPoints.append(logical);
        m_drawing = true;
        update();
        return;
    }

    m_mouseHandler->handleMousePress(event, logical);
    m_drawing = (event->button() == Qt::LeftButton);
    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::Deplacer) {
        m_moveStartState = m_historyManager->getCurrentState();
        m_moveInProgress = true;
    } else {
        m_moveInProgress = false;
    }
    m_drawingState->startPoint = logical;
    m_drawingState->strokePoints = {logical};

    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::Gomme) {
        // La gomme est complexe car elle peut modifier l'état à chaque mouvement.
        // On peut capturer l'état initial ici.
        m_drawingState->gommeErasing = true;
        m_drawingState->gommeCenter = logical;
        m_drawingState->lastEraserPos = logical;
        m_eraserTool->applyEraserAt(logical);
        update();
    }
}

void CustomDrawArea::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && getDrawMode() == DrawMode::PointParPoint) {
        const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
        if (m_drawingState->pointByPointPoints.isEmpty()) return;

        if (m_drawingState->pointByPointPoints.size() >= 2) {
            QPainterPath path;
            path.moveTo(m_drawingState->pointByPointPoints.first());
            for (int i = 1; i < m_drawingState->pointByPointPoints.size(); ++i)
                path.lineTo(m_drawingState->pointByPointPoints[i]);
            int id = m_nextShapeId++;
            m_historyManager->commitAddShape(path, id, tr("Tracé Point-par-point"));
        }
        m_drawingState->pointByPointPoints.clear();
        m_drawing = false;
        update();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CustomDrawArea::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
    if (getDrawMode() == DrawMode::PointParPoint && !m_drawingState->pointByPointPoints.isEmpty()) {
        m_drawingState->currentPoint = logical;
        update();
    }

    m_mouseHandler->handleMouseMove(event, logical);
    if (!m_drawing) return;

    if (getDrawMode() == DrawMode::Deplacer || getDrawMode() == DrawMode::Supprimer || getDrawMode() == DrawMode::Pan) return;

    m_drawingState->currentPoint = logical;
    if (getDrawMode() == DrawMode::Gomme) {
        if (!m_drawingState->gommeErasing) return;
        m_drawingState->gommeCenter = logical;
        m_eraserTool->eraseAlong(m_drawingState->lastEraserPos, logical);
        m_drawingState->lastEraserPos = logical;
    } else if (getDrawMode() == DrawMode::Freehand) {
        // AJOUT : Filtrage de distance (debounce spatial)
        const qreal MIN_DISTANCE = 3.0; // Ignore les mouvements de moins de 3 pixels

        if (m_drawingState->strokePoints.isEmpty()) {
            m_drawingState->strokePoints.append(logical);
        } else {
            QPointF lastPoint = m_drawingState->strokePoints.last();
            if (QLineF(lastPoint, logical).length() >= MIN_DISTANCE) {
                m_drawingState->strokePoints.append(logical);
            }
        }
    }
    update();
}

void CustomDrawArea::mouseReleaseEvent(QMouseEvent *event)
{
    const QPointF logical = snapToGridIfNeeded(toLogical(event->position()));
    if (getDrawMode() == DrawMode::PointParPoint) {
        m_drawingState->currentPoint = logical;
        update();
        return;
    }

    m_mouseHandler->handleMouseRelease(event, logical);
    if (!m_drawing) return;
    m_drawing = false;

    if (getDrawMode() == DrawMode::Deplacer || getDrawMode() == DrawMode::Supprimer || getDrawMode() == DrawMode::Pan) {
        if (getDrawMode() == DrawMode::Deplacer && m_moveInProgress) {
            const auto movedState = m_historyManager->getCurrentState();
            const bool changed = [&]() {
                if (movedState.size() != m_moveStartState.size()) return true;
                for (size_t i = 0; i < movedState.size(); ++i) {
                    const auto &a = movedState[i];
                    const auto &b = m_moveStartState[i];
                    if (a.originalId != b.originalId) return true;
                    if (std::abs(a.rotationAngle - b.rotationAngle) > 1e-9) return true;
                    if (a.path != b.path) return true;
                }
                return false;
            }();
            if (changed)
                m_historyManager->commitSnapshot(m_moveStartState, movedState, tr("Déplacer forme"));
            m_moveInProgress = false;
        }
        return;
    }

    if (getDrawMode() == DrawMode::Gomme) {
        m_drawingState->gommeErasing = false;
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
        path = PathGenerator::generateBezierPath(
            PathGenerator::applyChaikinAlgorithm(m_drawingState->strokePoints, m_smoothingLevel));
    }

    if (!path.isEmpty()) {
        int id = m_nextShapeId++;
        m_historyManager->commitAddShape(path, id);
    }
}

void CustomDrawArea::wheelEvent(QWheelEvent *event)
{
    const qreal   factor         = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    const QPointF cursor         = event->position();
    const QPointF logicalAtCursor = toLogical(cursor);
    m_transformer->setScale(m_transformer->scale() * factor);
    m_transformer->setOffset(cursor - logicalAtCursor * m_transformer->scale());
    event->accept();
}
