#include "ShapeVisualization.h"
#include "AspectRatioWrapper.h"
#include "GeometryUtils.h"
#include "ImageExporter.h"
#include "PlacementOptimizer.h"

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
#include <QApplication>
#include <QAbstractGraphicsShapeItem>
#include <QSizePolicy>        // <<< important pour QSizePolicy
#include <cmath>              // <<< pour std::round (si utilisé)
// #include <limits>          // (optionnel) retire-le si non utilisé

namespace {
const QString kInteractionLockedReason = QStringLiteral("Interaction verrouillée, modification impossible.");

QPainterPath buildPrototypePath(ShapeModel::Type model,
                                int largeur,
                                int longueur,
                                bool isCustomMode,
                                const QList<QPolygonF> &customShapes)
{
    QPainterPath prototypePath;

    if (isCustomMode && !customShapes.isEmpty()) {
        for (const QPolygonF &poly : customShapes)
            prototypePath.addPolygon(poly);
        prototypePath = prototypePath.simplified();

        const QRectF customBounds = prototypePath.boundingRect();
        const double scaleX = (customBounds.width() > 0) ? (largeur / customBounds.width()) : 1.0;
        const double scaleY = (customBounds.height() > 0) ? (longueur / customBounds.height()) : 1.0;
        QTransform scaleTransform;
        scaleTransform.scale(scaleX, scaleY);
        return scaleTransform.map(prototypePath);
    }

    QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(model, largeur, longueur);
    if (shapesList.isEmpty())
        return {};

    QGraphicsItem *prototype = shapesList.first();
    if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(prototype))
        prototypePath = pathItem->path();
    else if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(prototype))
        prototypePath.addRect(rectItem->rect());
    else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(prototype))
        prototypePath.addEllipse(ellipseItem->rect());
    else if (auto polyItem = dynamic_cast<QGraphicsPolygonItem*>(prototype))
        prototypePath.addPolygon(polyItem->polygon());

    return prototypePath;
}
}

ShapeVisualization::ShapeVisualization(QWidget *parent)
    : QWidget(parent),
    currentModel(ShapeModel::Type::Circle),
    currentLargeur(0),
    currentLongueur(0),
    shapeCount(0),
    spacing(0),
    m_isCustomMode(false)
{
    // >>> ICI (dans le corps), on peut écrire du code :
    // Le widget garde un ratio fixe (B : ratio au niveau du widget)
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sp.setHeightForWidth(true);
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

    // Dessine la bordure du plateau pour visualiser la limite
    // Utilise une bordure blanche plus visible et place-la au-dessus des formes
    m_sheetBorder = scene->addRect(scene->sceneRect(),
                                   QPen(Qt::white, 2), QBrush(Qt::NoBrush));
    m_sheetBorder->setZValue(1000);
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

    redraw();
}

void ShapeVisualization::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (graphicsView && scene) {
        graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
        qDebug() << "[FV] w,h =" << width() << height();
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

void ShapeVisualization::setModel(ShapeModel::Type model)
{
    if (!editingEnabled)
        return;
    if (!m_interactionEnabled)
        return;
    cancelOptimization();

    currentModel = model;
    setPredefinedMode();
    redraw();
    update();
}

void ShapeVisualization::updateDimensions(int largeur, int longueur)
{
    if (!editingEnabled)
        return;
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }
    cancelOptimization();
    currentLargeur = largeur;
    currentLongueur = longueur;
    if (m_isCustomMode && !m_customShapes.isEmpty())
        displayCustomShapes(m_customShapes);
    else
        redraw();
}

void ShapeVisualization::setShapeCount(int count, ShapeModel::Type type, int width, int height)
{
    if (!editingEnabled)
        return;
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    cancelOptimization();

    shapeCount = count;
    currentModel = type;
    currentLargeur = width;
    currentLongueur = height;
    if (m_isCustomMode && !m_customShapes.isEmpty())
        displayCustomShapes(m_customShapes);
    else
        redraw();
}

void ShapeVisualization::setSpacing(int newSpacing)
{
    if (!editingEnabled)
        return;
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    cancelOptimization();

    spacing = newSpacing;
    emit spacingChanged(newSpacing);
    if (m_isCustomMode && !m_customShapes.isEmpty())
        displayCustomShapes(m_customShapes);
    else
        redraw();
}

void ShapeVisualization::setPredefinedMode()
{
    qDebug() << "[DEBUG] setPredefinedMode() appelé";

    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    cancelOptimization();

    m_isCustomMode = false;
    m_currentCustomShapeName.clear();
    m_customShapes.clear();
    emit optimizationStateChanged(false);
    redraw();
}

/*
   ****** OPTIMIZE PLACEMENT 1 (avec caching et test rapide) ******
*/
void ShapeVisualization::optimizePlacement() {
    if (!m_interactionEnabled || m_optimizationRunning) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    // Remise à zéro de l'espacement
    setSpacing(0);

    m_optimizationRunning = true;
    m_cancelOptimization = false;
    emit optimizationStateChanged(true);
    scene->clear();
    scene->clearSelection();
    const QPainterPath prototypePath = buildPrototypePath(currentModel,
                                                          currentLargeur,
                                                          currentLongueur,
                                                          m_isCustomMode,
                                                          m_customShapes);
    if (prototypePath.isEmpty()) {
        m_optimizationRunning = false;
        emit progressUpdated(0, 0);
        return;
    }

    PlacementParams params;
    params.prototypePath = prototypePath;
    params.containerRect = scene->sceneRect();
    params.shapeCount = shapeCount;
    params.spacing = spacing;
    params.step = 5;
    params.angles = {0, 180, 90};

    const PlacementResult result = PlacementOptimizer::run(
        params,
        [this](int current, int total) {
            emit progressUpdated(current, total);
            qApp->processEvents();
            if (m_cancelOptimization) {
                m_cancelOptimization = false;
                return false;
            }
            return true;
        });

    if (result.cancelled) {
        m_optimizationRunning = false;
        emit optimizationStateChanged(false);
        emit progressUpdated(0, 0);
        return;
    }

    for (const QPainterPath &path : result.placedPaths) {
        QGraphicsPathItem *item = new QGraphicsPathItem(path);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem(item);
        item->setSelected(false);
    }

    m_optimizationRunning = false;
    emit shapesPlacedCount(result.placedPaths.size());
    emit optimizationStateChanged(true);
    emit progressUpdated(result.totalPositions, result.totalPositions);
}

void ShapeVisualization::optimizePlacement2() {
    if (!m_interactionEnabled || m_optimizationRunning) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    setSpacing(0);
    m_optimizationRunning = true;
    m_cancelOptimization = false;
    scene->clear();
    scene->clearSelection();
    graphicsView->update();

    const QPainterPath prototypePath = buildPrototypePath(currentModel,
                                                          currentLargeur,
                                                          currentLongueur,
                                                          m_isCustomMode,
                                                          m_customShapes);
    if (prototypePath.isEmpty()) {
        m_optimizationRunning = false;
        emit progressUpdated(0, 0);
        return;
    }

    PlacementParams params;
    params.prototypePath = prototypePath;
    params.containerRect = scene->sceneRect();
    params.shapeCount = shapeCount;
    params.spacing = spacing;
    params.step = 5;
    params.angles = {0, 180};

    const PlacementResult result = PlacementOptimizer::run(
        params,
        [this](int current, int total) {
            emit progressUpdated(current, total);
            qApp->processEvents();
            if (m_cancelOptimization) {
                m_cancelOptimization = false;
                return false;
            }
            return true;
        });

    if (result.cancelled) {
        m_optimizationRunning = false;
        emit optimizationStateChanged(false);
        emit progressUpdated(0, 0);
        return;
    }

    for (const QPainterPath &path : result.placedPaths) {
        QGraphicsPathItem *item = new QGraphicsPathItem(path);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem(item);
    }

    m_optimizationRunning = false;
    emit shapesPlacedCount(result.placedPaths.size());
    emit progressUpdated(result.totalPositions, result.totalPositions);
}




void ShapeVisualization::redraw()
{
    scene->clear();
    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    m_isCustomMode = false;
    m_customShapes.clear();

    if (shapeCount <= 0)
        return;


    int adaptedLargeur = currentLargeur;
    int adaptedLongueur = currentLongueur;

    QList<QGraphicsItem*> shapes = ShapeModel::generateShapes(currentModel, adaptedLargeur, adaptedLongueur);
    if (shapes.isEmpty()) {
        //qDebug() << "Erreur : generateShapes a retourné une liste vide.";
        emit shapesPlacedCount(0);
        return;
    }
    QGraphicsItem *prototype = shapes.first();

    QRectF bounds = prototype->boundingRect();
    qreal effectiveWidth = bounds.width();
    qreal effectiveHeight = bounds.height();
    if (effectiveWidth <= 0 || effectiveHeight <= 0) {
        //qDebug() << "Erreur : dimensions invalides pour la forme.";
        emit shapesPlacedCount(0);
        return;
    }
    qreal cellWidth = effectiveWidth + spacing;
    qreal cellHeight = effectiveHeight + spacing;

    int maxCols = qFloor(drawingWidth / cellWidth);
    int maxRows = qFloor(drawingHeight / cellHeight);
    int totalCells = maxCols * maxRows;
    int shapesToPlace = qMin(shapeCount, totalCells);

    //qDebug() << "Grille :" << maxCols << "colonnes x" << maxRows << "lignes ="
    //         << totalCells << "cellules. Placement de" << shapesToPlace << "formes.";

    for (int i = 0; i < shapesToPlace; ++i) {
        int col = i % maxCols;
        int row = i / maxCols;
        qreal xPos = col * cellWidth;
        qreal yPos = row * cellHeight;

        QGraphicsItem *shapeCopy = nullptr;
        if (auto rect = dynamic_cast<QGraphicsRectItem*>(prototype))
            shapeCopy = new QGraphicsRectItem(rect->rect());
        else if (auto ellipse = dynamic_cast<QGraphicsEllipseItem*>(prototype))
            shapeCopy = new QGraphicsEllipseItem(ellipse->rect());
        else if (auto polygon = dynamic_cast<QGraphicsPolygonItem*>(prototype))
            shapeCopy = new QGraphicsPolygonItem(polygon->polygon());
        else if (auto path = dynamic_cast<QGraphicsPathItem*>(prototype))
            shapeCopy = new QGraphicsPathItem(path->path());

        if (!shapeCopy) {
            //qDebug() << "Erreur : Impossible de copier la forme !";
            continue;
        }
        QPointF offsetCorrection(-bounds.x(), -bounds.y());
        shapeCopy->setPos(xPos + offsetCorrection.x(), yPos + offsetCorrection.y());
        shapeCopy->setFlag(QGraphicsItem::ItemIsMovable, true);
        shapeCopy->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem(shapeCopy);
    }
    //qDebug() << "Formes prédéfinies placées:" << shapesToPlace;
    emit shapesPlacedCount(shapesToPlace);
}

void ShapeVisualization::displayCustomShapes(const QList<QPolygonF>& shapes)
{
    if (!m_interactionEnabled) {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    cancelOptimization();

    scene->clear();

    if (shapes.isEmpty()) {
        //qDebug() << "Aucune forme personnalisée à afficher.";
        return;
    }

    m_isCustomMode = true;
    m_customShapes = shapes;

    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    QPainterPath combinedPath;
    for (const QPolygonF &poly : shapes) {
        combinedPath.addPolygon(poly);
    }
    QRectF polyBounds = combinedPath.boundingRect();
    if (polyBounds.width() <= 0 || polyBounds.height() <= 0) {
        //qDebug() << "Erreur : dimensions invalides pour le dessin custom combiné.";
        return;
    }

    qreal desiredWidthInScene = currentLargeur;
    qreal desiredHeightInScene = currentLongueur;
    qreal scaleX = desiredWidthInScene / polyBounds.width();
    qreal scaleY = desiredHeightInScene / polyBounds.height();
    QTransform transform;
    transform.scale(scaleX, scaleY);
    QPainterPath scaledPath = transform.map(combinedPath);
    QRectF scaledBounds = scaledPath.boundingRect();

    qreal cellWidth = scaledBounds.width() + spacing;
    qreal cellHeight = scaledBounds.height() + spacing;
    int maxCols = qFloor(drawingWidth / cellWidth);
    int maxRows = qFloor(drawingHeight / cellHeight);
    int totalCells = maxCols * maxRows;
    int shapesToPlace = qMin(shapeCount, totalCells);

    for (int i = 0; i < shapesToPlace; ++i) {
        int col = i % maxCols;
        int row = i / maxCols;
        qreal xPos = col * cellWidth;
        qreal yPos = row * cellHeight;

        QGraphicsPathItem *item = new QGraphicsPathItem(scaledPath);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        // Calcul de l'offset à partir du boundingRect réel de l'item afin
        // d'intégrer la largeur du trait. Cela évite un décalage d'un pixel en
        // vertical observé avec les formes personnalisées.
        QRectF bounds = item->boundingRect();
        QPointF offset(-bounds.x(), -bounds.y());
        item->setPos(xPos + offset.x(), yPos + offset.y());
        scene->addItem(item);
        item->setSelected(false);

    }

    //qDebug() << "Affichage de" << shapesToPlace << "copies du dessin custom dans ShapeVisualization.";
    emit shapesPlacedCount(shapesToPlace);
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
        if (selected.size() == 1) {
            m_rotationPivot = selected.first()->sceneBoundingRect().center();
        } else {
            QRectF unitedRect;
            bool first = true;
            for (QGraphicsItem *item : selected) {
                if (first) {
                    unitedRect = item->sceneBoundingRect();
                    first = false;
                } else {
                    unitedRect = unitedRect.united(item->sceneBoundingRect());
                }
            }
            m_rotationPivot = unitedRect.center();
        }
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
    const auto selected = scene->selectedItems();
    for (QGraphicsItem *item : selected) {
        if (m_cutMarkers.contains(item))
            continue;
        scene->removeItem(item);
        delete item;
        removed = true;
    }
    if (removed)
        emit shapesPlacedCount(countPlacedShapes());
}

void ShapeVisualization::addShapeBottomRight()
{
    if (!m_interactionEnabled)
    {
        emit actionRefused(kInteractionLockedReason);
        return;
    }

    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    QGraphicsItem *newItem = nullptr;

    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        QPainterPath combinedPath;
        for (const QPolygonF &poly : m_customShapes)
            combinedPath.addPolygon(poly);
        QRectF polyBounds = combinedPath.boundingRect();
        if (polyBounds.width() <= 0 || polyBounds.height() <= 0)
            return;

        qreal desiredWidth = currentLargeur;
        qreal desiredHeight = currentLongueur;

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
        QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(currentModel, currentLargeur, currentLongueur);
        if (shapesList.isEmpty())
            return;

        newItem = shapesList.takeFirst();
        QRectF bounds = newItem->boundingRect();
        QPointF offset(-bounds.x(), -bounds.y());
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
    cancelOptimization();
    m_isCustomMode = true;
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

QGraphicsScene* ShapeVisualization::getScene() const {
    return scene;
}

// ======================================================================================================================
//Balaye la scène pour trouver les pixels noirs des formes et ainsi avoir les coordonnées de chaque pixel dans une liste
// ======================================================================================================================



QList<QPoint> ShapeVisualization::getBlackPixels() {
    return ImageExporter::extractBlackPixels(scene);
}

// Démarre la barre de progression avec un maximum de étapes
void ShapeVisualization::startDecoupeProgress(int maxSteps) {
    m_decoupeProgressMax = maxSteps;
    emit progressUpdated(0, maxSteps);
}

// Met à jour la valeur courante de la barre de progression
void ShapeVisualization::updateDecoupeProgress(int currentStep) {
    emit progressUpdated(currentStep, m_decoupeProgressMax);
}

// Cache la barre de progression en fin de découpe
void ShapeVisualization::endDecoupeProgress() {
    m_decoupeProgressMax = 0;
    emit progressUpdated(0, 0);
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
    if (m_optimizationRunning)
        m_cancelOptimization = true;
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
    const ShapeValidationResult validation = validatePlacedPaths(paths, bounds, 1.0);

    for (int idx : validation.outOfBoundsIndices) {
        if (idx >= 0 && idx < shapes.size())
            shapes[idx]->setPen(QPen(Qt::red, 1));
    }
    for (int idx : validation.collisionIndices) {
        if (idx >= 0 && idx < shapes.size())
            shapes[idx]->setPen(QPen(Qt::red, 1));
    }

    emit shapesPlacedCount(shapes.size());
    return validation.allValid;
}

void ShapeVisualization::handleSelectionChanged()
{
    m_rotationPivotValid = false;
    for (QGraphicsItem *item : scene->selectedItems()) {
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            shape->setPen(QPen(Qt::black, 1));
        }
    }
}

void ShapeVisualization::resetAllShapeColors()
{
    for (QGraphicsItem *item : scene->items()) {
        if (m_cutMarkers.contains(item))
            continue;
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            shape->setPen(QPen(Qt::black, 1));
        }
    }
    graphicsView->viewport()->update();
}


void ShapeVisualization::applyLayout(const LayoutData &layout)
{
    if (!m_isCustomMode)
        return;

    scene->clear();
    currentLargeur = layout.largeur;
    currentLongueur = layout.longueur;
    spacing = layout.spacing;
    shapeCount = layout.items.size();

    QPainterPath combinedPath;
    for (const QPolygonF &poly : m_customShapes)
        combinedPath.addPolygon(poly);
    QRectF bounds = combinedPath.boundingRect();
    qreal scaleX = (bounds.width() > 0) ? static_cast<qreal>(layout.largeur) / bounds.width() : 1.0;
    qreal scaleY = (bounds.height() > 0) ? static_cast<qreal>(layout.longueur) / bounds.height() : 1.0;
    QTransform scale;
    scale.scale(scaleX, scaleY);
    QPainterPath scaledPath = scale.map(combinedPath);
    QRectF scaledBounds = scaledPath.boundingRect();

    for (const LayoutItem &li : layout.items) {
        QGraphicsPathItem *item = new QGraphicsPathItem(scaledPath);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setTransformOriginPoint(item->boundingRect().center());

        QRectF bounds = item->boundingRect();
        QPointF offset(-bounds.x(), -bounds.y());

        item->setRotation(li.rotation);
        item->setPos(li.x + offset.x(), li.y + offset.y());
        scene->addItem(item);
        item->setSelected(false);
    }

    scene->clearSelection();
    emit shapesPlacedCount(layout.items.size());
    graphicsView->viewport()->update();
}

LayoutData ShapeVisualization::captureCurrentLayout(const QString &name) const
{
    LayoutData layout;
    layout.name = name;
    layout.largeur = currentLargeur;
    layout.longueur = currentLongueur;
    layout.spacing = spacing;

    for (QGraphicsItem *item : scene->items()) {
        if (m_cutMarkers.contains(item))
            continue;
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            QRectF bounds = shape->boundingRect();
            QPointF offset(-bounds.x(), -bounds.y());

            LayoutItem li;
            li.x = shape->pos().x() - offset.x();
            li.y = shape->pos().y() - offset.y();
            li.rotation = shape->rotation();
            layout.items.append(li);
        }
    }
    return layout;
}

int ShapeVisualization::heightForWidth(int w) const
{
    if (m_aspect <= 0.0) return QWidget::heightForWidth(w);
    return static_cast<int>(std::round(w / m_aspect));
}

QSize ShapeVisualization::sizeHint() const
{
    // Taille "agréable" par défaut tout en respectant le ratio
    int w = 900;
    return { w, heightForWidth(w) };
}

QSize ShapeVisualization::minimumSizeHint() const
{
    int w = 300;
    return { w, heightForWidth(w) };
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

    // La scène est exprimée en mm : on la recale sur le nouveau plateau
    scene->setSceneRect(0, 0, m_sheetMm.width(), m_sheetMm.height());
    if (m_sheetBorder)
        m_sheetBorder->setRect(scene->sceneRect());

    // Prévenir le layout que nos contraintes ont changé
    updateGeometry();

    // Recentrer / rescaler le contenu
    if (graphicsView && scene)
        graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    emit sheetSizeMmChanged(m_sheetMm);
}
