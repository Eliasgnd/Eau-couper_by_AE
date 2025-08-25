#include "FormeVisualization.h"
#include "AspectRatioWrapper.h"

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
#include <QPainterPathStroker>
#include <QMessageBox>
#include <QAbstractGraphicsShapeItem>
#include <QRandomGenerator>
#include <QSizePolicy>        // <<< important pour QSizePolicy
#include <cmath>              // <<< pour std::round (si utilisé)
// #include <limits>          // (optionnel) retire-le si non utilisé
#include <QPolygonF>
#include <utility>

#include <QHash>
#include <QSet>
#include <QFuture>
#include <QtConcurrent>

#include "GeometryUtils.h"

namespace {
constexpr double kOverlapEpsilon   = 0.5;   // tolerance for true overlap
constexpr double kBroadPhaseMargin = 0.1;   // bbox expansion for spatial query
constexpr double kGridCellSize     = 50.0;  // uniform grid cell size in scene units
}

FormeVisualization::FormeVisualization(QWidget *parent)
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

    progressBar = new QProgressBar(this);
    progressBar->setValue(0);
    progressBar->setVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar { border: 2px solid #00BCD4; border-radius: 5px; "
        "background: #f3f3f3; text-align: center; font-size: 12px; color: black; }"
        "QProgressBar::chunk { background: #00BCD4; width: 10px; }"
        );

    layout->addWidget(graphicsView);
    layout->addWidget(progressBar);
    setLayout(layout);

    connect(scene, &QGraphicsScene::selectionChanged,
            this, &FormeVisualization::handleSelectionChanged);

    // Fit initial
    QTimer::singleShot(0, this, [this](){
        graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    });

    redraw();
}

QPainterPath FormeVisualization::bufferedPath(const QPainterPath &path, int spacing)
{
    // Si aucun espacement n'est demandé, renvoyer le chemin original
    if (spacing <= 0)
        return path;

    QPainterPathStroker stroker;
    // Ici, spacing est interprété comme la largeur totale désirée.
    // Le contour généré s'étend d'environ spacing/2 de chaque côté.
    stroker.setWidth(spacing);

    QPainterPath strokePath = stroker.createStroke(path);
    // Le buffer correspond à l'union du chemin original et de son contour élargi
    return path.united(strokePath);
}

void FormeVisualization::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (graphicsView && scene) {
        graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

bool FormeVisualization::eventFilter(QObject *watched, QEvent *event)
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

void FormeVisualization::setModel(ShapeModel::Type model)
{
    if (!editingEnabled)
        return;
    if (m_decoupeEnCours)
        return;
    cancelOptimization();

    currentModel = model;
    setPredefinedMode();
    redraw();
    update();
}

void FormeVisualization::updateDimensions(int largeur, int longueur)
{
    if (!editingEnabled)
        return;
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();        return;
    }
    cancelOptimization();
    currentLargeur = largeur;
    currentLongueur = longueur;
    if (m_isCustomMode && !m_customShapes.isEmpty())
        displayCustomShapes(m_customShapes);
    else
        redraw();
}

void FormeVisualization::setShapeCount(int count, ShapeModel::Type type, int width, int height)
{
    if (!editingEnabled)
        return;
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
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

void FormeVisualization::setSpacing(int newSpacing)
{
    if (!editingEnabled)
        return;
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
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

void FormeVisualization::setPredefinedMode()
{
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();        return;
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
void FormeVisualization::optimizePlacement() {
    if (m_decoupeEnCours || m_optimizationRunning) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();        return;
    }

    // Remise à zéro de l'espacement
    setSpacing(0);

    m_optimizationRunning = true;
    m_cancelOptimization = false;
    emit optimizationStateChanged(true);
    scene->clear();
    scene->clearSelection();
    progressBar->setVisible(true);
    progressBar->setValue(0);

    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    QRectF containerRect(0, 0, drawingWidth, drawingHeight);

    int adaptedLargeur = currentLargeur;
    int adaptedLongueur = currentLongueur;

    QPainterPath prototypePath;
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        for (const QPolygonF &poly : m_customShapes)
            prototypePath.addPolygon(poly);
        prototypePath = prototypePath.simplified();
        QRectF customBounds = prototypePath.boundingRect();
        double scaleX = (customBounds.width() > 0) ? (adaptedLargeur / customBounds.width()) : 1.0;
        double scaleY = (customBounds.height() > 0) ? (adaptedLongueur / customBounds.height()) : 1.0;
        QTransform scaleTransform;
        scaleTransform.scale(scaleX, scaleY);
        prototypePath = scaleTransform.map(prototypePath);
    } else {
        QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(currentModel, adaptedLargeur, adaptedLongueur);
        if (shapesList.isEmpty()) {
            //qDebug() << "Erreur : Aucun prototype de forme disponible.";

            m_optimizationRunning = false;
            progressBar->setVisible(false);
            return;
        }
        QGraphicsItem *prototype = shapesList.first();
        if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(prototype))
            prototypePath = pathItem->path();
        else if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(prototype))
            prototypePath.addRect(rectItem->rect());
        else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(prototype))
            prototypePath.addEllipse(ellipseItem->rect());
        else if (auto polyItem = dynamic_cast<QGraphicsPolygonItem*>(prototype))
            prototypePath.addPolygon(polyItem->polygon());
        else {
            //qDebug() << "Erreur : Type de forme inconnu pour l'optimisation.";
            progressBar->setVisible(false);
            return;
        }
    }

    QRectF protoRect = prototypePath.boundingRect();
    QPointF center = protoRect.center();

    const int step = 5;
    int count = shapeCount;
    QList<int> angles = {0, 180, 90};

    struct PathInfo { QGraphicsPathItem *item; QPainterPath path; QRectF bbox; };
    QList<PathInfo> placedPaths;
    int shapesPlaced = 0;
    int totalPositions = ((drawingWidth / step) + 1) * ((drawingHeight / step) + 1);
    progressBar->setMinimum(0);
    progressBar->setMaximum(totalPositions);
    int progressCounter = 0;

    for (int y = 0; y <= drawingHeight && shapesPlaced < count; y += step) {
        for (int x = 0; x <= drawingWidth && shapesPlaced < count; x += step) {
            progressCounter++;
            progressBar->setValue(progressCounter);
            qApp->processEvents();
            if (m_cancelOptimization) {
                m_cancelOptimization = false;
                m_optimizationRunning = false;
                emit optimizationStateChanged(false);
                progressBar->setVisible(false);
                return;
            }

            for (int angle : angles) {
                QTransform trans;
                trans.translate(x, y);
                trans.translate(center.x(), center.y());
                trans.rotate(angle);
                trans.translate(-center.x(), -center.y());
                QPainterPath candidate = trans.map(prototypePath);

                QRectF candRect = candidate.boundingRect();
                QTransform adjust;
                adjust.translate(x - candRect.x(), y - candRect.y());
                candidate = adjust.map(candidate);
                candidate.closeSubpath();

                QPainterPath candidatePath = candidate.simplified();
                if (!containerRect.contains(candidatePath.boundingRect()))
                    continue;

                QRectF candBBox = candidatePath.boundingRect();
                QRectF searchRect = candBBox.adjusted(-kBroadPhaseMargin, -kBroadPhaseMargin,
                                                     kBroadPhaseMargin, kBroadPhaseMargin);
                QList<QGraphicsItem*> nearby = scene->items(searchRect, Qt::IntersectsItemBoundingRect);
                bool collision = false;
                for (QGraphicsItem *other : nearby) {
                    for (const PathInfo &existing : placedPaths) {
                        if (existing.item == other && pathsOverlap(candidatePath, existing.path)) {
                            collision = true;
                            break;
                        }
                    }
                    if (collision)
                        break;
                }

                if (!collision) {
                    QGraphicsPathItem *item = new QGraphicsPathItem(candidatePath);
                    item->setPen(QPen(Qt::black, 1));
                    item->setBrush(Qt::NoBrush);
                    item->setFlag(QGraphicsItem::ItemIsMovable, true);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, true);

                    // Ajuste la position en fonction du boundingRect réel de l'élément
                    QRectF bounds = item->boundingRect();
                    QPointF offset(x - bounds.x(), y - bounds.y());
                    item->moveBy(offset.x(), offset.y());
                    scene->addItem(item);
                    item->setSelected(false);

                    // Enregistre la position corrigée pour les futurs tests de collision
                    candidatePath.translate(offset);
                    placedPaths.append({item, candidatePath, candidatePath.boundingRect()});
                    shapesPlaced++;
                    break;
                }
            }
            if (shapesPlaced >= count)
                break;
        }
    }

    //qDebug() << "Formes placées:" << shapesPlaced;

    m_optimizationRunning = false;
    emit shapesPlacedCount(shapesPlaced);
    emit optimizationStateChanged(true);
    progressBar->setVisible(false);
}

void FormeVisualization::optimizePlacement2() {
    if (m_decoupeEnCours || m_optimizationRunning) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
        return;
    }

    setSpacing(0);
    m_optimizationRunning = true;
    m_cancelOptimization = false;
    scene->clear();
    scene->clearSelection();
    graphicsView->update();

    progressBar->setVisible(true);
    progressBar->setValue(0);

    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    int adaptedLargeur = currentLargeur;
    int adaptedLongueur = currentLongueur;

    QPainterPath prototypePath;
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        for (const QPolygonF &poly : m_customShapes)
            prototypePath.addPolygon(poly);
        QRectF customBounds = prototypePath.boundingRect();
        double scaleX = (customBounds.width() > 0) ? (adaptedLargeur / customBounds.width()) : 1.0;
        double scaleY = (customBounds.height() > 0) ? (adaptedLongueur / customBounds.height()) : 1.0;
        QTransform scaleTransform;
        scaleTransform.scale(scaleX, scaleY);
        prototypePath = scaleTransform.map(prototypePath);
    } else {
        QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(currentModel, adaptedLargeur, adaptedLongueur);
        if (shapesList.isEmpty()) {
            m_optimizationRunning = false;
            progressBar->setVisible(false);
            return;
        }
        QGraphicsItem *prototype = shapesList.first();
        if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(prototype))
            prototypePath = pathItem->path();
        else if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(prototype))
            prototypePath.addRect(rectItem->rect());
        else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(prototype))
            prototypePath.addEllipse(ellipseItem->rect());
        else if (auto polyItem = dynamic_cast<QGraphicsPolygonItem*>(prototype))
            prototypePath.addPolygon(polyItem->polygon());
        else {
            return;
        }
    }

    QRectF bounds = prototypePath.boundingRect();
    QTransform normTransform;
    normTransform.translate(-bounds.x(), -bounds.y());
    prototypePath = normTransform.map(prototypePath);
    prototypePath.closeSubpath();

    struct PathInfo { QGraphicsPathItem *item; QPainterPath path; QRectF bbox; };
    QList<PathInfo> placedPaths;
    int shapesPlaced = 0;
    int count = shapeCount;
    const int step = 5;
    bool finished = false;
    QRectF containerRect(0, 0, drawingWidth, drawingHeight);

    int totalPositions = ((drawingWidth / step) + 1) * ((drawingHeight / step) + 1);
    progressBar->setMinimum(0);
    progressBar->setMaximum(totalPositions);
    int progressCounter = 0;

    for (int y = 0; y <= drawingHeight && !finished; y += step) {
        for (int x = 0; x <= drawingWidth && !finished; x += step) {
            progressCounter++;
            progressBar->setValue(progressCounter);
            qApp->processEvents();
            if (m_cancelOptimization) {
                m_cancelOptimization = false;
                m_optimizationRunning = false;
                emit optimizationStateChanged(false);
                progressBar->setVisible(false);
                return;
            }

            // Boucle sur les deux rotations : 0° et 180°
            for (int angle : {0, 180}) {
                QTransform rotationTransform;
                if (angle != 0) {
                    QPointF center = prototypePath.boundingRect().center();
                    rotationTransform.translate(center.x(), center.y());
                    rotationTransform.rotate(angle);
                    rotationTransform.translate(-center.x(), -center.y());
                }

                QTransform candidateTransform = QTransform().translate(x, y) * rotationTransform;
                QPainterPath candidate = candidateTransform.map(prototypePath);
                candidate.closeSubpath();

                QPainterPath candidatePath = candidate.simplified();
                if (!containerRect.contains(candidatePath.boundingRect()))
                    continue;

                QRectF candBBox = candidatePath.boundingRect();
                QRectF searchRect = candBBox.adjusted(-kBroadPhaseMargin, -kBroadPhaseMargin,
                                                     kBroadPhaseMargin, kBroadPhaseMargin);
                QList<QGraphicsItem*> nearby = scene->items(searchRect, Qt::IntersectsItemBoundingRect);
                bool collision = false;
                for (QGraphicsItem *other : nearby) {
                    for (const PathInfo &existing : placedPaths) {
                        if (existing.item == other && pathsOverlap(candidatePath, existing.path)) {
                            collision = true;
                            break;
                        }
                    }
                    if (collision)
                        break;
                }

                if (!collision) {
                    QGraphicsPathItem *item = new QGraphicsPathItem(candidatePath);
                    item->setPen(QPen(Qt::black, 1));
                    item->setBrush(Qt::NoBrush);
                    item->setFlag(QGraphicsItem::ItemIsMovable, true);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, true);

                    QRectF bounds = item->boundingRect();
                    QPointF offset(x - bounds.x(), y - bounds.y());
                    item->moveBy(offset.x(), offset.y());
                    scene->addItem(item);

                    candidatePath.translate(offset);
                    placedPaths.append({item, candidatePath, candidatePath.boundingRect()});
                    shapesPlaced++;
                    if (shapesPlaced >= count) {
                        finished = true;
                        break;
                    }
                    break;
                }
            }
        }
    }

    m_optimizationRunning = false;
    emit shapesPlacedCount(shapesPlaced);
    progressBar->setVisible(false);
}




void FormeVisualization::redraw()
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

void FormeVisualization::displayCustomShapes(const QList<QPolygonF>& shapes)
{
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
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

    //qDebug() << "Affichage de" << shapesToPlace << "copies du dessin custom dans FormeVisualization.";
    emit shapesPlacedCount(shapesToPlace);
}

void FormeVisualization::moveSelectedShapes(qreal dx, qreal dy)
{
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
        return;
    }
    // Parcours de tous les items sélectionnés dans la scène
    for (QGraphicsItem *item : scene->selectedItems()) {
        item->moveBy(dx, dy);
    }
    m_rotationPivotValid = false;
}


void FormeVisualization::rotateSelectedShapes(qreal angleDelta)
{
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
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

void FormeVisualization::deleteSelectedShapes()
{
    if (m_decoupeEnCours)
    {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
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

void FormeVisualization::addShapeBottomRight()
{
    if (m_decoupeEnCours)
    {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
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

void FormeVisualization::setCustomMode() {
    cancelOptimization();
    m_isCustomMode = true;
    emit optimizationStateChanged(false);
}

// -----------------------------------------------------------------------------
// Ajout d’un point rouge
// -----------------------------------------------------------------------------
void FormeVisualization::colorPositionRed(const QPoint& p)
{
    auto *dot = scene->addEllipse(p.x()-1, p.y()-1, 2, 2,
                                  QPen(Qt::NoPen), QBrush(Qt::red));
    m_cutMarkers << dot;
    graphicsView->viewport()->update();
}

// -----------------------------------------------------------------------------
// Ajout d’un point bleu
// -----------------------------------------------------------------------------
void FormeVisualization::colorPositionBlue(const QPoint& p)
{
    auto *dot = scene->addEllipse(p.x(), p.y(), 2, 2,
                                  QPen(Qt::NoPen), QBrush(Qt::blue));
    m_cutMarkers << dot;
    graphicsView->viewport()->update();
}

// -----------------------------------------------------------------------------
// Remise à zéro des marqueurs de découpe
// -----------------------------------------------------------------------------
void FormeVisualization::resetCutMarkers()
{
    for (auto *item : std::as_const(m_cutMarkers)) {
        scene->removeItem(item);
        delete item;
    }
    m_cutMarkers.clear();
}

QGraphicsScene* FormeVisualization::getScene() const {
    return scene;
}

// ======================================================================================================================
//Balaye la scène pour trouver les pixels noirs des formes et ainsi avoir les coordonnées de chaque pixel dans une liste
// ======================================================================================================================



QList<QPoint> FormeVisualization::getBlackPixels() {

    QList<QPoint> pixelsNoirs;

    // Convertir la scène en image
    QImage image(scene->sceneRect().size().toSize(), QImage::Format_RGB32);
    image.fill(Qt::white);  // Fond blanc pour éviter les erreurs
    QPainter painter(&image);
    scene->render(&painter);

    // Parcourir chaque pixel de l'image

    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            if (image.pixelColor(x, y) == Qt::black) {
                pixelsNoirs.append(QPoint(x, y));
                //qDebug() << "Pixel noir à :" << QPoint(x, y);  //simplement pour visualiser les coordonnées des pixels noirs.
            }
        }

    }

    return pixelsNoirs;
}

// Démarre la barre de progression avec un maximum de étapes
void FormeVisualization::startDecoupeProgress(int maxSteps) {
    progressBar->setMaximum(maxSteps);
    progressBar->setValue(0);
    progressBar->setVisible(true);
}

// Met à jour la valeur courante de la barre de progression
void FormeVisualization::updateDecoupeProgress(int currentStep) {
    progressBar->setValue(currentStep);
}

// Cache la barre de progression en fin de découpe
void FormeVisualization::endDecoupeProgress() {
    progressBar->setVisible(false);
}

void FormeVisualization::setEditingEnabled(bool enabled)
{
    editingEnabled = enabled;
}

bool FormeVisualization::isEditingEnabled() const
{
    return editingEnabled;
}

void FormeVisualization::setDecoupeEnCours(bool etat)
{
    m_decoupeEnCours = etat;
    // Interdire ou autoriser le déplacement manuel des items
    for (QGraphicsItem *item : scene->items()) {
        item->setFlag(QGraphicsItem::ItemIsMovable, !etat);
    }
}

bool FormeVisualization::isDecoupeEnCours() const
{
    return m_decoupeEnCours;
}

void FormeVisualization::cancelOptimization()
{
    if (m_optimizationRunning)
        m_cancelOptimization = true;
}

QGraphicsView* FormeVisualization::getGraphicsView() const
{
    return graphicsView;
}

int FormeVisualization::countPlacedShapes() const
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

bool FormeVisualization::validateShapes()
{
    auto *vp = graphicsView->viewport();
    vp->setUpdatesEnabled(false);
    const bool oldAA = graphicsView->renderHints() & QPainter::Antialiasing;
    graphicsView->setRenderHint(QPainter::Antialiasing, false);

    resetAllShapeColors();
    bool allValid = true;
    QList<QAbstractGraphicsShapeItem*> shapes;
    for (QGraphicsItem *item : scene->items()) {
        if (m_cutMarkers.contains(item))
            continue;
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item))
            shapes << shape;
    }

    QRectF bounds = scene->sceneRect().adjusted(-1, -1, 1, 1);
    QVector<QPainterPath> paths;
    QVector<QRectF> bboxes;
    paths.reserve(shapes.size());
    bboxes.reserve(shapes.size());

    for (auto *shape : shapes) {
        auto &cache = m_cache[shape];
        if (cache.base.isEmpty())
            cache.base = shape->shape().simplified();
        QTransform t = shape->sceneTransform();
        if (cache.transform != t) {
            cache.path      = t.map(cache.base);
            cache.bbox      = cache.path.boundingRect();
            cache.transform = t;
        }
        paths << cache.path;
        bboxes << cache.bbox;
        if (!bounds.contains(cache.bbox)) {
            shape->setPen(QPen(Qt::red, 1));
            allValid = false;
        }
    }

    struct ShapeGeom { QPainterPath path; QRectF bbox; };
    QVector<ShapeGeom> geoms;
    geoms.reserve(paths.size());
    for (int i = 0; i < paths.size(); ++i)
        geoms.push_back({paths[i], bboxes[i]});

    auto future = QtConcurrent::run([geoms]() {
        QSet<int> collided;
        QHash<QPair<int,int>, QVector<int>> grid;
        grid.reserve(geoms.size() * 2);
        QSet<quint64> tested;

        for (int i = 0; i < geoms.size(); ++i) {
            const QRectF &b = geoms[i].bbox;
            const int x0 = static_cast<int>(std::floor(b.left()  / kGridCellSize));
            const int y0 = static_cast<int>(std::floor(b.top()   / kGridCellSize));
            const int x1 = static_cast<int>(std::floor(b.right() / kGridCellSize));
            const int y1 = static_cast<int>(std::floor(b.bottom()/ kGridCellSize));
            for (int x = x0; x <= x1; ++x)
                for (int y = y0; y <= y1; ++y)
                    grid[{x, y}].append(i);
        }

        for (auto it = grid.constBegin(); it != grid.constEnd(); ++it) {
            const QVector<int> &idxs = it.value();
            for (int a = 0; a < idxs.size(); ++a) {
                for (int b = a + 1; b < idxs.size(); ++b) {
                    int i = idxs[a];
                    int j = idxs[b];
                    const quint64 key = (static_cast<quint64>(qMin(i,j)) << 32) | qMax(i,j);
                    if (tested.contains(key))
                        continue;
                    tested.insert(key);
                    QRectF b1 = geoms[i].bbox.adjusted(-kBroadPhaseMargin, -kBroadPhaseMargin,
                                                       kBroadPhaseMargin, kBroadPhaseMargin);
                    if (!b1.intersects(geoms[j].bbox))
                        continue;
                    if (pathsOverlap(geoms[i].path, geoms[j].path, kOverlapEpsilon)) {
                        collided << i << j;
                    }
                }
            }
        }
        return collided;
    });

    while (!future.isFinished())
        QCoreApplication::processEvents();

    QSet<int> collided = future.result();
    for (int idx : collided)
        shapes[idx]->setPen(QPen(Qt::red, 1));
    if (!collided.isEmpty())
        allValid = false;

    emit shapesPlacedCount(shapes.size());

    graphicsView->setRenderHint(QPainter::Antialiasing, oldAA);
    vp->setUpdatesEnabled(true);
    return allValid;
}

void FormeVisualization::handleSelectionChanged()
{
    m_rotationPivotValid = false;
    for (QGraphicsItem *item : scene->selectedItems()) {
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            shape->setPen(QPen(Qt::black, 1));
        }
    }
}

void FormeVisualization::resetAllShapeColors()
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


void FormeVisualization::applyLayout(const LayoutData &layout)
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

LayoutData FormeVisualization::captureCurrentLayout(const QString &name) const
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

double FormeVisualization::evaluateWasteArea(const QList<QPainterPath>& placedPaths,
                                             int drawingWidth, int drawingHeight)
{
    QPainterPath united;
    for (const QPainterPath &p : placedPaths)
        united = united.united(p);

    QRectF br = united.boundingRect();
    double used = br.width() * br.height();
    double total = static_cast<double>(drawingWidth) * drawingHeight;
    return total - used;
}

int FormeVisualization::heightForWidth(int w) const
{
    if (m_aspect <= 0.0) return QWidget::heightForWidth(w);
    return static_cast<int>(std::round(w / m_aspect));
}

QSize FormeVisualization::sizeHint() const
{
    // Taille "agréable" par défaut tout en respectant le ratio
    int w = 900;
    return { w, heightForWidth(w) };
}

QSize FormeVisualization::minimumSizeHint() const
{
    int w = 300;
    return { w, heightForWidth(w) };
}

void FormeVisualization::setSheetSizeMm(const QSizeF& mm)
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

