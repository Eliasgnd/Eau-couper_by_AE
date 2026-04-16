#include "ShapeVisualization.h"
#include "AspectRatioWrapper.h"
#include "GeometryTransformHelper.h"
#include "GridPlacementService.h"
#include "ImageExporter.h"
#include "LayoutManager.h"
#include "ShapeValidationService.h"

#include <QTimer>            // au lieu de "qtimer.h"
#include <QVBoxLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QResizeEvent>
#include <QDebug>
#include <QtMath>
#include <QTransform>
#include <QAbstractGraphicsShapeItem>
#include <QSizePolicy>        // <<< important pour QSizePolicy
#include <cmath>              // <<< pour std::round (si utilisé)
// #include <limits>          // (optionnel) retire-le si non utilisé

namespace {
const QString kInteractionLockedReason = QStringLiteral("Interaction verrouillée, modification impossible.");
}

ShapeVisualization::ShapeVisualization(QWidget *parent)
    : QWidget(parent),
    m_projectModel(nullptr)
{
    // >>> ICI (dans le corps), on peut écrire du code :
    // Le widget garde un ratio fixe (B : ratio au niveau du widget)
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
   // sp.setHeightForWidth(true);
    setSizePolicy(sp);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    graphicsView = new QGraphicsView(this);
    scene        = new QGraphicsScene(this);

    // Scène = taille du plateau (en mm)
    scene->setSceneRect(0, 0, m_sheetMm.width(), m_sheetMm.height());

    // Fond blanc, quoiqu’il arrive (le stylesheet global ne gagnera plus)
    graphicsView->setBackgroundBrush(Qt::white);
    scene->setBackgroundBrush(Qt::white);

    graphicsView->setRenderHint(QPainter::Antialiasing, true);
    graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);


    graphicsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    graphicsView->setFrameStyle(QFrame::NoFrame);
    graphicsView->setAlignment(Qt::AlignCenter);
    graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    graphicsView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    graphicsView->setScene(scene);
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->viewport()->installEventFilter(this);

    layout->addWidget(graphicsView);
    setLayout(layout);

    connect(scene, &QGraphicsScene::selectionChanged,
            this, &ShapeVisualization::handleSelectionChanged);

    // Fit initial
    QTimer::singleShot(0, this, [this](){
        graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    });
}

void ShapeVisualization::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (graphicsView && scene) {
        // Exécuter le fitInView en différé évite les distorsions si le layout est en train d'être calculé
        QTimer::singleShot(0, this, [this]() {
            graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
        });
    }
}

bool ShapeVisualization::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == graphicsView->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            QPointF scenePos = graphicsView->mapToScene(me->pos());
            QGraphicsItem *item = scene->itemAt(scenePos, graphicsView->transform());
            if (item && !m_cutMarkers.contains(item)) {
                item->setSelected(!item->isSelected());
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShapeVisualization::updateDimensions(int largeur, int longueur)
{
    if (!m_projectModel) return;
    if (!editingEnabled)
        return;
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }
    cancelOptimization();
    m_projectModel->setDimensions(largeur, longueur);
    if (m_projectModel->isCustomMode() && !m_projectModel->customShapes().isEmpty())
        displayCustomShapes(m_projectModel->customShapes());
    else
        redraw();
}

void ShapeVisualization::setSpacing(int newSpacing)
{
    if (!m_projectModel) return;
    if (!editingEnabled)
        return;
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    cancelOptimization();

    m_projectModel->setSpacing(newSpacing);
    emit spacingChanged(newSpacing);
    if (m_projectModel->isCustomMode() && !m_projectModel->customShapes().isEmpty())
        displayCustomShapes(m_projectModel->customShapes());
    else
        redraw();
}

void ShapeVisualization::setPredefinedMode()
{
    qDebug() << "[DEBUG] setPredefinedMode() appelé";

    if (!m_projectModel) return;
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    cancelOptimization();

    m_projectModel->setCustomMode(false);
    m_projectModel->setCustomShapeName(QString());
    m_projectModel->clearCustomShapes();
    emit optimizationStateChanged(false);
    redraw();
}


void ShapeVisualization::redraw()
{
    if (!m_projectModel) return;

    // --- CORRECTION DOUBLONS ---
    // On rend le modèle muet un instant pour éviter le double dessin
    const bool wasBlocked = m_projectModel->blockSignals(true);
    m_projectModel->setCustomMode(false);
    m_projectModel->clearCustomShapes();
    m_projectModel->blockSignals(wasBlocked);
    // ---------------------------

    m_cutMarkers.clear();
    scene->clear();
    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    if (m_projectModel->shapeCount() <= 0)
        return;

    const QPainterPath prototypePath = m_projectModel->prototypeShapePath();
    if (prototypePath.isEmpty()) {
        emit shapesPlacedCount(0);
        return;
    }

    const QRectF bounds = prototypePath.boundingRect();
    const qreal effectiveWidth  = bounds.width();
    const qreal effectiveHeight = bounds.height();
    if (effectiveWidth <= 0 || effectiveHeight <= 0) {
        emit shapesPlacedCount(0);
        return;
    }

    GridPlacementRequest request;
    request.containerSize = QSizeF(drawingWidth, drawingHeight);
    request.shapeSize = QSizeF(effectiveWidth, effectiveHeight);
    request.shapeCount = m_projectModel->shapeCount();
    request.spacing = m_projectModel->spacing();
    const QList<QPointF> positions = GridPlacementService::computePositions(request);

    for (const QPointF &position : positions) {
        auto *shapeCopy = new QGraphicsPathItem(prototypePath);
        shapeCopy->setPen(QPen(Qt::black, 1));
        shapeCopy->setBrush(Qt::NoBrush);
        const QPointF offsetCorrection(-bounds.x(), -bounds.y());
        shapeCopy->setPos(position.x() + offsetCorrection.x(),
                          position.y() + offsetCorrection.y());
        shapeCopy->setFlag(QGraphicsItem::ItemIsMovable, true);
        shapeCopy->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem(shapeCopy);
    }
    emit shapesPlacedCount(positions.size());
}

void ShapeVisualization::displayCustomShapes(const QList<QPolygonF>& shapes)
{
    if (!m_projectModel) return;
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    cancelOptimization();

    // --- CORRECTION DOUBLONS ---
    const bool wasBlocked = m_projectModel->blockSignals(true);
    m_projectModel->setCustomMode(true);
    m_projectModel->setCustomShapes(shapes);
    m_projectModel->blockSignals(wasBlocked);
    // ---------------------------

    m_cutMarkers.clear();
    scene->clear();

    if (shapes.isEmpty()) return;

    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    QPainterPath combinedPath;
    for (const QPolygonF &poly : shapes) {
        combinedPath.addPolygon(poly);
    }
    QRectF polyBounds = combinedPath.boundingRect();
    if (polyBounds.width() <= 0 || polyBounds.height() <= 0) return;

    qreal desiredWidthInScene = m_projectModel->currentLargeur();
    qreal desiredHeightInScene = m_projectModel->currentLongueur();
    qreal scaleX = desiredWidthInScene / polyBounds.width();
    qreal scaleY = desiredHeightInScene / polyBounds.height();
    QTransform transform;
    transform.scale(scaleX, scaleY);
    QPainterPath scaledPath = transform.map(combinedPath);
    QRectF scaledBounds = scaledPath.boundingRect();

    GridPlacementRequest request;
    request.containerSize = QSizeF(drawingWidth, drawingHeight);
    request.shapeSize = scaledBounds.size();
    request.shapeCount = m_projectModel->shapeCount();
    request.spacing = m_projectModel->spacing();
    const QList<QPointF> positions = GridPlacementService::computePositions(request);

    for (const QPointF &position : positions) {
        QGraphicsPathItem *item = new QGraphicsPathItem(scaledPath);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);

        QRectF bounds = item->boundingRect();
        QPointF offset(-bounds.x(), -bounds.y());
        item->setPos(position.x() + offset.x(), position.y() + offset.y());
        scene->addItem(item);
        item->setSelected(false);
    }
    emit shapesPlacedCount(positions.size());
}

void ShapeVisualization::moveSelectedShapes(qreal dx, qreal dy)
{
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }
    // Parcours de tous les items sélectionnés dans la scène
    for (QGraphicsItem *item : scene->selectedItems()) {
        item->moveBy(dx, dy);
    }
    m_rotationPivotValid = false;
    emit shapeMoved(QPointF(dx, dy));
}


void ShapeVisualization::rotateSelectedShapes(qreal angleDelta)
{
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }
    const auto selected = scene->selectedItems();
    if (selected.isEmpty())
        return;

    if (!m_rotationPivotValid) {
        m_rotationPivot = GeometryTransformHelper::computeUnifiedPivot(selected);
        for (QGraphicsItem *item : selected)
            item->setTransformOriginPoint(item->mapFromScene(m_rotationPivot));
        m_rotationPivotValid = true;
    }

    for (QGraphicsItem *item : selected)
        item->setRotation(item->rotation() + angleDelta);
}

void ShapeVisualization::deleteSelectedShapes()
{
    if (!m_interactionEnabled)
    {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    bool removed = false;
    int deletedCount = 0;
    const auto selected = scene->selectedItems();
    for (QGraphicsItem *item : selected) {
        if (m_cutMarkers.contains(item))
            continue;
        scene->removeItem(item);
        delete item;
        removed = true;
        ++deletedCount;
    }
    if (removed) {
        emit shapesPlacedCount(countPlacedShapes());
        emit shapesDeleted(deletedCount);
    }
}

void ShapeVisualization::addShapeBottomRight()
{
    if (!m_projectModel) return;
    if (!m_interactionEnabled)
    {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    QGraphicsItem *newItem = nullptr;

    if (m_projectModel->isCustomMode() && !m_projectModel->customShapes().isEmpty()) {
        QPainterPath combinedPath;
        for (const QPolygonF &poly : m_projectModel->customShapes())
            combinedPath.addPolygon(poly);
        QRectF polyBounds = combinedPath.boundingRect();
        if (polyBounds.width() <= 0 || polyBounds.height() <= 0)
            return;

        qreal desiredWidth = m_projectModel->currentLargeur();
        qreal desiredHeight = m_projectModel->currentLongueur();

        qreal scaleX = desiredWidth / polyBounds.width();
        qreal scaleY = desiredHeight / polyBounds.height();

        QTransform transform;
        transform.scale(scaleX, scaleY);
        QPainterPath scaledPath = transform.map(combinedPath);
        QRectF bounds = scaledPath.boundingRect();

        auto *item = new QGraphicsPathItem(scaledPath);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        newItem = item;

        QPointF offset(-bounds.x(), -bounds.y());
        newItem->setPos(drawingWidth - bounds.width() + offset.x(),
                        drawingHeight - bounds.height() + offset.y());
    } else {
        const QPainterPath shapePath = m_projectModel->prototypeShapePath();
        if (shapePath.isEmpty())
            return;

        const QRectF bounds = shapePath.boundingRect();
        auto *pathItem = new QGraphicsPathItem(shapePath);
        pathItem->setPen(QPen(Qt::black, 1));
        pathItem->setBrush(Qt::NoBrush);
        newItem = pathItem;
        const QPointF offset(-bounds.x(), -bounds.y());
        newItem->setPos(drawingWidth - bounds.width() + offset.x(),
                        drawingHeight - bounds.height() + offset.y());
    }

    if (newItem) {
        newItem->setFlag(QGraphicsItem::ItemIsMovable, true);
        newItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem(newItem);
        newItem->setSelected(false);
        emit shapesPlacedCount(countPlacedShapes());
    }
}

void ShapeVisualization::setCustomMode() {
    if (!m_projectModel) return;
    cancelOptimization();

    // --- CORRECTION DOUBLONS ---
    const bool wasBlocked = m_projectModel->blockSignals(true);
    m_projectModel->setCustomMode(true);
    m_projectModel->blockSignals(wasBlocked);
    // ---------------------------

    emit optimizationStateChanged(false);
}

// -----------------------------------------------------------------------------
// Ajout d’un point rouge
// -----------------------------------------------------------------------------
void ShapeVisualization::colorPositionRed(const QPoint& p)
{
    auto *dot = scene->addEllipse(p.x()-1, p.y()-1, 2, 2,
                                  QPen(Qt::NoPen), QBrush(Qt::red));
    m_cutMarkers << dot;
    graphicsView->viewport()->update();
}

// -----------------------------------------------------------------------------
// Ajout d’un point bleu
// -----------------------------------------------------------------------------
void ShapeVisualization::colorPositionBlue(const QPoint& p)
{
    auto *dot = scene->addEllipse(p.x(), p.y(), 2, 2,
                                  QPen(Qt::NoPen), QBrush(Qt::blue));
    m_cutMarkers << dot;
    graphicsView->viewport()->update();
}

// -----------------------------------------------------------------------------
// Remise à zéro des marqueurs de découpe
// -----------------------------------------------------------------------------
void ShapeVisualization::resetCutMarkers()
{
    for (auto *item : std::as_const(m_cutMarkers)) {
        scene->removeItem(item);
        delete item;
    }
    m_cutMarkers.clear();
}

void ShapeVisualization::setDecoupeEnCours(bool running)
{
    setInteractionEnabled(!running);
    setEditingEnabled(!running);
}

void ShapeVisualization::addCutMarker(QGraphicsItem* item)
{
    if (item && !m_cutMarkers.contains(item))
        m_cutMarkers << item;
}

QGraphicsScene* ShapeVisualization::getScene() const {
    return scene;
}

// ======================================================================================================================
//Balaye la scène pour trouver les pixels noirs des formes et ainsi avoir les coordonnées de chaque pixel dans une liste
// ======================================================================================================================



QList<QPoint> ShapeVisualization::getBlackPixels() {
    return ImageExporter::extractBlackPixels(scene);
}

void ShapeVisualization::setEditingEnabled(bool enabled)
{
    editingEnabled = enabled;
}

bool ShapeVisualization::isEditingEnabled() const
{
    return editingEnabled;
}

void ShapeVisualization::setInteractionEnabled(bool enabled)
{
    m_interactionEnabled = enabled;
    for (QGraphicsItem *item : scene->items()) {
        item->setFlag(QGraphicsItem::ItemIsMovable, enabled);
        item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
    }
}

bool ShapeVisualization::isInteractionEnabled() const
{
    return m_interactionEnabled;
}

void ShapeVisualization::cancelOptimization()
{
    if (!m_projectModel) return;
    m_projectModel->setOptimizationRunning(false);
}

void ShapeVisualization::setOptimizationRunning(bool running)
{
    if (!m_projectModel) return;
    m_projectModel->setOptimizationRunning(running);
}


void ShapeVisualization::setOptimizationResult(const QList<QPainterPath> &placedPaths, bool optimized)
{
    // scene->clear() détruit tous les items Qt (m_sheetBorder, m_cutMarkers…)
    // → on invalide les pointeurs AVANT la suppression pour éviter les dangling pointers
    m_sheetBorder = nullptr;
    m_cutMarkers.clear();

    scene->clear();
    scene->clearSelection();

    const QRectF placementBounds = scene->sceneRect().adjusted(-1.0, -1.0, 1.0, 1.0);
    int acceptedCount = 0;
    for (const QPainterPath &path : placedPaths) {
        if (!placementBounds.contains(path.boundingRect()))
            continue;

        auto *item = new QGraphicsPathItem(path);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem(item);
        item->setSelected(false);
        ++acceptedCount;
    }

    emit shapesPlacedCount(acceptedCount);
    emit optimizationStateChanged(optimized);
}

QGraphicsView* ShapeVisualization::getGraphicsView() const
{
    return graphicsView;
}

int ShapeVisualization::countPlacedShapes() const
{
    int count = 0;
    for (QGraphicsItem *item : scene->items()) {
        if (m_cutMarkers.contains(item))
            continue;
        if (dynamic_cast<QGraphicsPathItem*>(item) ||
            dynamic_cast<QGraphicsRectItem*>(item) ||
            dynamic_cast<QGraphicsEllipseItem*>(item) ||
            dynamic_cast<QGraphicsPolygonItem*>(item)) {
            ++count;
        }
    }
    return count;
}

bool ShapeVisualization::validateShapes()
{
    resetAllShapeColors();

    QList<QAbstractGraphicsShapeItem*> shapes;
    QList<QPainterPath> paths;
    for (QGraphicsItem *item : scene->items()) {
        if (m_cutMarkers.contains(item))
            continue;
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            shapes << shape;
            paths << shape->mapToScene(shape->shape());
        }
    }

    const QRectF bounds = scene->sceneRect().adjusted(-1, -1, 1, 1);
    const ShapeValidationResult validation = ShapeValidationService::validate(paths, bounds, 1.0);

    // --- CORRECTION DU SAUT D'INTERFACE ---
    // Création d'un stylo rouge "Cosmétique" pour ne pas altérer la taille mathématique
    QPen errorPen(Qt::red, 1);
    errorPen.setCosmetic(true); // <---- LA LIGNE MAGIQUE

    for (int idx : validation.outOfBoundsIndices) {
        if (idx >= 0 && idx < shapes.size())
            shapes[idx]->setPen(errorPen);
    }
    for (int idx : validation.collisionIndices) {
        if (idx >= 0 && idx < shapes.size())
            shapes[idx]->setPen(errorPen);
    }
    // --------------------------------------

    emit shapesPlacedCount(shapes.size());
    return validation.allValid;
}
void ShapeVisualization::handleSelectionChanged()
{
    m_rotationPivotValid = false;
    emit shapeSelectionChanged(scene->selectedItems().size());
    for (QGraphicsItem *item : scene->selectedItems()) {
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            shape->setPen(QPen(Qt::black, 1));
        }
    }
}

void ShapeVisualization::resetAllShapeColors()
{
    // Création d'un stylo noir Cosmétique
    QPen normalPen(Qt::black, 1);
    normalPen.setCosmetic(true); // <---- LA LIGNE MAGIQUE

    for (QGraphicsItem *item : scene->items()) {
        if (m_cutMarkers.contains(item))
            continue;
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            shape->setPen(normalPen);
        }
    }
    graphicsView->viewport()->update();
}


void ShapeVisualization::applyLayout(const LayoutData &layout)
{
    if (!m_projectModel) return;
    if (!m_projectModel->isCustomMode())
        return;

    // --- AJOUTEZ LES DEUX LIGNES ICI ---
    m_cutMarkers.clear();
    scene->clear();
    // -----------------------------------

    m_projectModel->setDimensions(layout.largeur, layout.longueur);
    m_projectModel->setSpacing(layout.spacing);
    m_projectModel->setShapeCount(layout.items.size());

    // Le LayoutManager va maintenant dessiner sur une scène parfaitement propre
    LayoutManager::applyLayout(scene,
                               layout,
                               m_projectModel->customShapes(),
                               m_interactionEnabled);

    emit shapesPlacedCount(layout.items.size());
    graphicsView->viewport()->update();
}

LayoutData ShapeVisualization::captureCurrentLayout(const QString &name) const
{
    if (!m_projectModel) return {};
    return LayoutManager::captureLayout(scene,
                                        name,
                                        m_projectModel->currentLargeur(),
                                        m_projectModel->currentLongueur(),
                                        m_projectModel->spacing(),
                                        m_cutMarkers);
}

int ShapeVisualization::heightForWidth(int w) const
{
    return QWidget::heightForWidth(w);
}

QSize ShapeVisualization::sizeHint() const
{
    return QSize(800, 600); // Ne plus calculer de ratio ici
}

QSize ShapeVisualization::minimumSizeHint() const
{
    return QSize(100, 100); // Empêche l'agrandissement forcé de la fenêtre globale
}


QList<QPolygonF> ShapeVisualization::currentCustomShapes() const
{
    return m_projectModel ? m_projectModel->customShapes() : QList<QPolygonF>{};
}

void ShapeVisualization::setProjectModel(ShapeVisualizationViewModel *model)
{
    if (!model || model == m_projectModel)
        return;

    if (m_projectModel)
        disconnect(m_projectModel, nullptr, this, nullptr);

    m_projectModel = model;
    if (!m_projectModel->parent())
        m_projectModel->setParent(this);

    connect(m_projectModel, &ShapeVisualizationViewModel::dataChanged,
            this, [this]() {
                if (m_projectModel->isCustomMode() && !m_projectModel->customShapes().isEmpty())
                    displayCustomShapes(m_projectModel->customShapes());
                else
                    redraw();
            });

    redraw();
}

void ShapeVisualization::setSheetSizeMm(const QSizeF& mm)
{
    if (mm.width() <= 0 || mm.height() <= 0)
        return;

    if (qFuzzyCompare(m_sheetMm.width(), mm.width()) &&
        qFuzzyCompare(m_sheetMm.height(), mm.height()))
        return;

    m_sheetMm = mm;
    m_aspect  = m_sheetMm.width() / m_sheetMm.height();

    scene->setSceneRect(0, 0, m_sheetMm.width(), m_sheetMm.height());
    if (m_sheetBorder)
        m_sheetBorder->setRect(scene->sceneRect());

    updateGeometry();

    if (graphicsView && scene) {
        // Exécuter le fitInView en différé ici aussi
        QTimer::singleShot(0, this, [this]() {
            graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
        });
    }

    emit sheetSizeMmChanged(m_sheetMm);
}
