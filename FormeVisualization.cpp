#include "FormeVisualization.h"

#include <QTimer>            // au lieu de "qtimer.h"
#include <QVBoxLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QResizeEvent>
#include <QtMath>
#include <QTransform>
#include <QApplication>
#include <QPainterPathStroker>
#include <QMessageBox>
#include <QAbstractGraphicsShapeItem>
#include <QSizePolicy>        // <<< important pour QSizePolicy
#include <cmath>              // <<< pour std::round (si utilisé)
#include <QPolygonF>
#include <QSet>
#include <QElapsedTimer>
#include <QCache>

#include "GeometryUtils.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>
#include <QThreadPool>

#include <QtConcurrent>
#include <memory>
#include <atomic>

namespace {
constexpr double kBroadPhaseMargin = 0.1;   // bbox expansion for spatial query
constexpr double kGridCellSize     = 50.0;  // uniform grid cell size in scene units
enum InvalidReason { Valid = 0, OutOfBounds = 1, InteriorOverlap = 2 };
constexpr int    kCutPathUpdateFreq = 25;   // refresh cut path item every N points

QCache<quint64, QPainterPath> gPathCache(256);

quint64 hashPath(const QPainterPath &p, int spacing)
{
    quint64 h = qHash(spacing);
    for (int i = 0; i < p.elementCount(); ++i) {
        auto el = p.elementAt(i);
        h = qHash(el.x, h);
        h = qHash(el.y, h);
    }
    return h;
}

double polygonArea(const QPolygonF &poly)
{
    double area = 0.0;
    for (int i = 0, n = poly.size(); i < n; ++i) {
        const QPointF &p1 = poly[i];
        const QPointF &p2 = poly[(i + 1) % n];
        area += p1.x() * p2.y() - p2.x() * p1.y();
    }
    return std::abs(area) / 2.0;
}
}

// Create a three-tier LOD representation for complex paths. A raster fallback
// (P0) is displayed immediately, a simplified polygon proxy (P1) replaces it
// when ready, and the exact path (P2) is set afterwards. Computation happens on
// background threads and can be cancelled by destroying the temporary pixmap
// item before completion.
void FormeVisualization::addPathWithLOD(const QPainterPath &path, const QPointF &pos)
{
    if (!scene)
        return;

    // 0) Affichage instantané : raster à partir du "path" brut (pas de normalisation bloquante sur l’UI)
    const int rasterSize = qBound(512,
                                  int(qMax(path.boundingRect().width(), path.boundingRect().height())),
                                  1024);
    QPixmap pm = rasterFallback(path, rasterSize);
    auto *pix = new QGraphicsPixmapItem(pm);
    pix->setTransformationMode(Qt::FastTransformation);
    pix->setPos(pos);
    pix->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    scene->addItem(pix);

    // 1) Drapeau d’annulation partagé (annule si la vue/scene est détruite)
    auto cancel = std::make_shared<std::atomic_bool>(false);
    QObject::connect(this,  &QObject::destroyed, [cancel]{ cancel->store(true,  std::memory_order_release); });
    QObject::connect(scene, &QObject::destroyed, [cancel]{ cancel->store(true,  std::memory_order_release); });

    // 2) Étape proxy/clean en arrière-plan (PAS de travail lourd dans paint() / visibilité)
    QThreadPool::globalInstance()->start([this, path, pos, pix, cancel]{
        if (cancel->load(std::memory_order_acquire)) return;

        // Normalisation et proxy hors UI
        QPainterPath clean = normalizePath(path);
        QPainterPath proxy = buildProxyPath(clean);
        if (cancel->load(std::memory_order_acquire)) return;

        // 3) Remplacement du raster par l’item proxy sur l’UI thread
        QMetaObject::invokeMethod(this, [=]() {
            if (cancel->load(std::memory_order_acquire) || !scene) return;

            auto *item = new QGraphicsPathItem(proxy);
            item->setPen(QPen(Qt::black, 1));
            item->setBrush(Qt::NoBrush);
            item->setFlag(QGraphicsItem::ItemIsMovable,   true);
            item->setFlag(QGraphicsItem::ItemIsSelectable,true);
            item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
            item->setPos(pos);
            scene->addItem(item);

            // Remplacer le pixmap (et annuler tout travail qui s’y référait)
            scene->removeItem(pix);
            delete pix;

            // 4) Étape exacte : nouvelle annulation dédiée à l’item, avec garde de durée de vie
            auto cancel2 = std::make_shared<std::atomic_bool>(false);
            QObject::connect(this, &QObject::destroyed, [cancel2]{ cancel2->store(true, std::memory_order_release); });
            QGraphicsPathItem* guard = item;

            QThreadPool::globalInstance()->start([this, clean, guard, cancel2]{
                if (cancel2->load(std::memory_order_acquire)) return;

                QPainterPath exact = clean.simplified();
                if (cancel2->load(std::memory_order_acquire)) return;

                QMetaObject::invokeMethod(this, [=](){
                    if (cancel2->load(std::memory_order_acquire) || !scene) return;
                    // Vérifie que le pointeur correspond encore à un item présent dans la scène
                    if (!scene->items().contains(guard)) return;

                    guard->setPath(exact);
                    guard->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

                    QPainterPath proxy = buildProxyPath(exact);
                    const QTransform t = guard->sceneTransform();
                    const QPainterPath mapped = t.map(proxy);
                    const QRectF bbox = mapped.boundingRect();
                    m_cache[guard] = { proxy, mapped, {}, bbox, t };
                }, Qt::QueuedConnection);
            });
        }, Qt::QueuedConnection);
    });
}

double polygonArea(const QPolygonF &poly)
{
    double area = 0.0;
    for (int i = 0, n = poly.size(); i < n; ++i) {
        const QPointF &p1 = poly[i];
        const QPointF &p2 = poly[(i + 1) % n];
        area += p1.x() * p2.y() - p2.x() * p1.y();
    }
    return std::abs(area) / 2.0;
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

    // Item pour afficher les marqueurs de découpe cumulés
    m_cutPathItem = scene->addPath(QPainterPath(), QPen(Qt::NoPen));
    m_cutPathItem->setZValue(1001);
    // Disable antialiasing during bulk updates to keep the UI responsive
    graphicsView->setRenderHint(QPainter::Antialiasing, false);
    graphicsView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);


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
    if (spacing <= 0) {
        QPainterPath p = path; p.setFillRule(Qt::OddEvenFill); return p;
    }

    QPainterPathStroker stroker;
    // Ici, spacing est interprété comme la largeur totale désirée.
    // Le contour généré s'étend d'environ spacing/2 de chaque côté.
    stroker.setWidth(spacing);

    QPainterPath p = path; p.setFillRule(Qt::OddEvenFill);
    QPainterPath strokePath = stroker.createStroke(p);
    QPainterPath result = p.united(strokePath);
    result.setFillRule(Qt::OddEvenFill);
    return result;
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
            if (item && item != m_cutPathItem) {
                item->setSelected(!item->isSelected());
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool FormeVisualization::warnIfCutting()
{
    if (!m_decoupeEnCours && !m_optimizationRunning)
        return false;

    auto *msg = new QMessageBox(QMessageBox::Warning,
                                 "Découpe en cours",
                                 "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                 QMessageBox::Ok,
                                 this);
    msg->setModal(false);
    msg->show();
    return true;
}

void FormeVisualization::setModel(ShapeModel::Type model)
{
    if (!editingEnabled)
        return;
    if (warnIfCutting())
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
    if (warnIfCutting())
        return;
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
    if (warnIfCutting())
        return;

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
    if (warnIfCutting())
        return;

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
    if (warnIfCutting())
        return;

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
    if (warnIfCutting())
        return;

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

    QPainterPath prototypePath;
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        for (const QPolygonF &poly : m_customShapes)
            prototypePath.addPolygon(poly);
        prototypePath = prototypePath.simplified();
        QRectF customBounds = prototypePath.boundingRect();
        double scaleX = (customBounds.width() > 0) ? (currentLargeur / customBounds.width()) : 1.0;
        double scaleY = (customBounds.height() > 0) ? (currentLongueur / customBounds.height()) : 1.0;
        QTransform scaleTransform;
        scaleTransform.scale(scaleX, scaleY);
        prototypePath = scaleTransform.map(prototypePath);
    } else {
        QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(currentModel, currentLargeur, currentLongueur);
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

    struct PathInfo { QPainterPath path; QRectF bbox; };
    QHash<QGraphicsItem*, PathInfo> placedPaths;
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

                QPainterPath candidatePath = normalizePath(candidate);
                QPainterPath candidateProxy = buildProxyPath(candidatePath);
                if (isPathTooComplex(candidateProxy, kMaxPathElements))
                    continue;

                QRectF candBBox = candidateProxy.boundingRect();
                if (!containerRect.contains(candBBox))
                    continue;
                QRectF searchRect = candBBox.adjusted(-kBroadPhaseMargin, -kBroadPhaseMargin,
                                                     kBroadPhaseMargin, kBroadPhaseMargin);
                QList<QGraphicsItem*> nearby = scene->items(searchRect, Qt::IntersectsItemBoundingRect);
                bool collision = false;
                for (QGraphicsItem *other : nearby) {
                    auto it = placedPaths.constFind(other);
                    if (it == placedPaths.constEnd())
                        continue;
                    const PathInfo &existing = it.value();
                    if (!candBBox.intersects(existing.bbox))
                        continue;
                    if (pathsOverlap(candidateProxy, existing.path)) {
                        collision = true;
                        break;
                    }
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
                    candBBox.translate(offset);
                    candidateProxy.translate(offset);
                    placedPaths.insert(item, {candidateProxy, candBBox});
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
    if (warnIfCutting())
        return;

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

    QPainterPath prototypePath;
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        for (const QPolygonF &poly : m_customShapes)
            prototypePath.addPolygon(poly);
        QRectF customBounds = prototypePath.boundingRect();
        double scaleX = (customBounds.width() > 0) ? (currentLargeur / customBounds.width()) : 1.0;
        double scaleY = (customBounds.height() > 0) ? (currentLongueur / customBounds.height()) : 1.0;
        QTransform scaleTransform;
        scaleTransform.scale(scaleX, scaleY);
        prototypePath = scaleTransform.map(prototypePath);
    } else {
        QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(currentModel, currentLargeur, currentLongueur);
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

    struct PathInfo { QPainterPath path; QRectF bbox; };
    QHash<QGraphicsItem*, PathInfo> placedPaths;
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

                QPainterPath candidatePath = normalizePath(candidate);
                QPainterPath candidateProxy = buildProxyPath(candidatePath);
                if (isPathTooComplex(candidateProxy, kMaxPathElements))
                    continue;

                QRectF candBBox = candidateProxy.boundingRect();
                if (!containerRect.contains(candBBox))
                    continue;
                QRectF searchRect = candBBox.adjusted(-kBroadPhaseMargin, -kBroadPhaseMargin,
                                                     kBroadPhaseMargin, kBroadPhaseMargin);
                QList<QGraphicsItem*> nearby = scene->items(searchRect, Qt::IntersectsItemBoundingRect);
                bool collision = false;
                for (QGraphicsItem *other : nearby) {
                    auto it = placedPaths.constFind(other);
                    if (it == placedPaths.constEnd())
                        continue;
                    const PathInfo &existing = it.value();
                    if (!candBBox.intersects(existing.bbox))
                        continue;
                    if (pathsOverlap(candidateProxy, existing.path)) {
                        collision = true;
                        break;
                    }
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

                    candBBox.translate(offset);
                    candidateProxy.translate(offset);
                    placedPaths.insert(item, {candidateProxy, candBBox});
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
    auto *vp = graphicsView->viewport();
    scene->blockSignals(true);
    vp->setUpdatesEnabled(false);

    scene->clear();
    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    m_isCustomMode = false;
    m_customShapes.clear();

    if (shapeCount <= 0) {
        emit shapesPlacedCount(0);
        scene->blockSignals(false);
        vp->setUpdatesEnabled(true);
        vp->update();
        return;
    }

    QList<QGraphicsItem*> shapes = ShapeModel::generateShapes(currentModel, currentLargeur, currentLongueur);
    if (shapes.isEmpty()) {
        emit shapesPlacedCount(0);
        scene->blockSignals(false);
        vp->setUpdatesEnabled(true);
        vp->update();
        return;
    }

    QGraphicsItem *prototype = shapes.first();
    QRectF bounds = prototype->boundingRect();
    qreal effectiveWidth  = bounds.width();
    qreal effectiveHeight = bounds.height();
    if (effectiveWidth <= 0 || effectiveHeight <= 0) {
        emit shapesPlacedCount(0);
        scene->blockSignals(false);
        vp->setUpdatesEnabled(true);
        vp->update();
        return;
    }

    qreal cellWidth  = effectiveWidth  + spacing;
    qreal cellHeight = effectiveHeight + spacing;
    int maxCols = qFloor(drawingWidth / cellWidth);
    int maxRows = qFloor(drawingHeight / cellHeight);
    int totalCells    = maxCols * maxRows;
    int shapesToPlace = qMin(shapeCount, totalCells);

    for (int i = 0; i < shapesToPlace; ++i) {
        int col = i % maxCols;
        int row = i / maxCols;
        qreal xPos = col * cellWidth;
        qreal yPos = row * cellHeight;

        if (auto pathItem = qgraphicsitem_cast<QGraphicsPathItem*>(prototype)) {
            QPointF offset(-bounds.x(), -bounds.y());
            addPathWithLOD(pathItem->path(), QPointF(xPos + offset.x(), yPos + offset.y()));
            continue;
        }

        QGraphicsItem *shapeCopy = nullptr;
        if (auto rect = qgraphicsitem_cast<QGraphicsRectItem*>(prototype))
            shapeCopy = new QGraphicsRectItem(rect->rect());
        else if (auto ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(prototype))
            shapeCopy = new QGraphicsEllipseItem(ellipse->rect());
        else if (auto polygon = qgraphicsitem_cast<QGraphicsPolygonItem*>(prototype))
            shapeCopy = new QGraphicsPolygonItem(polygon->polygon());
        else if (auto path2 = qgraphicsitem_cast<QGraphicsPathItem*>(prototype))
            shapeCopy = new QGraphicsPathItem(path2->path());

        if (!shapeCopy) continue;

        QPointF offset(-bounds.x(), -bounds.y());
        shapeCopy->setPos(xPos + offset.x(), yPos + offset.y());
        shapeCopy->setFlag(QGraphicsItem::ItemIsMovable, true);
        shapeCopy->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem(shapeCopy);
    }

    emit shapesPlacedCount(shapesToPlace);
    scene->blockSignals(false);
    vp->setUpdatesEnabled(true);
    vp->update();
}

void FormeVisualization::displayCustomShapes(const QList<QPolygonF>& shapes)
{
    if (warnIfCutting())
        return;

    cancelOptimization();
    auto *vp = graphicsView->viewport();
    scene->blockSignals(true);
    vp->setUpdatesEnabled(false);

    scene->clear();

    if (shapes.isEmpty()) {
        //qDebug() << "Aucune forme personnalisée à afficher.";
        scene->blockSignals(false);
        vp->setUpdatesEnabled(true);
        vp->update();
        return;
    }

    m_isCustomMode = true;
    m_customShapes = shapes;

    const QRectF sr = scene->sceneRect();
    const int drawingWidth  = int(sr.width());
    const int drawingHeight = int(sr.height());

    QPainterPath combinedPath;
    for (const QPolygonF &poly : shapes)
        combinedPath.addPolygon(poly);
    QRectF polyBounds = combinedPath.boundingRect();
    if (polyBounds.width() <= 0 || polyBounds.height() <= 0) {
        scene->blockSignals(false);
        vp->setUpdatesEnabled(true);
        vp->update();
        return;
    }

    qreal desiredWidthInScene = currentLargeur;
    qreal desiredHeightInScene = currentLongueur;
    qreal scaleX = desiredWidthInScene / polyBounds.width();
    qreal scaleY = desiredHeightInScene / polyBounds.height();
    QTransform transform; transform.scale(scaleX, scaleY);
    QPainterPath scaledPath = normalizePath(transform.map(combinedPath));
    scaledPath.setFillRule(Qt::OddEvenFill);
    if (isPathTooComplex(scaledPath, kMaxPathElements)) {
        scene->blockSignals(false);
        vp->setUpdatesEnabled(true);
        vp->update();
        return;
    }
    QRectF scaledBounds = scaledPath.boundingRect();

    qreal cellWidth = scaledBounds.width() + spacing;
    qreal cellHeight = scaledBounds.height() + spacing;
    int maxCols = qFloor(drawingWidth / cellWidth);
    int maxRows = qFloor(drawingHeight / cellHeight);
    int totalCells = maxCols * maxRows;
    int shapesToPlace = qMin(shapeCount, totalCells);

    QVector<QGraphicsPathItem*> items;
    items.reserve(shapesToPlace);
    QPainterPath proxy = buildProxyPath(scaledPath);
    for (int i = 0; i < shapesToPlace; ++i) {
        int col = i % maxCols;
        int row = i / maxCols;
        qreal xPos = col * cellWidth;
        qreal yPos = row * cellHeight;

        QGraphicsPathItem *item = new QGraphicsPathItem(proxy);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        QRectF bounds = item->boundingRect();
        QPointF offset(-bounds.x(), -bounds.y());
        item->setPos(xPos + offset.x(), yPos + offset.y());
        scene->addItem(item);
        item->setSelected(false);
        items << item;
    }

    emit shapesPlacedCount(shapesToPlace);

    const quint64 key = hashPath(scaledPath, spacing);
    if (QPainterPath *cached = gPathCache.object(key)) {
        QPainterPath exact = *cached; exact.setFillRule(Qt::OddEvenFill);
        QPainterPath proxyLoc = buildProxyPath(exact);
        for (QGraphicsPathItem *item : items) {
            if (!item) continue;
            item->setPath(exact);
            item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
            const QTransform t = item->sceneTransform();
            QPainterPath mapped = t.map(proxyLoc);
            QRectF bbox = mapped.boundingRect();
            m_cache[item] = {proxyLoc, mapped, {}, bbox, t};
        }
    } else {
        QtConcurrent::run([this, items, scaledPath, key]() {
            QPainterPath exact = scaledPath.simplified();
            exact.setFillRule(Qt::OddEvenFill);
            QPainterPath proxyLoc = buildProxyPath(exact);
            QMetaObject::invokeMethod(this, [this, items, exact, proxyLoc, key]() {
                gPathCache.insert(key, new QPainterPath(exact));
                for (QGraphicsPathItem *item : items) {
                    if (!item) continue;
                    item->setPath(exact);
                    item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
                    const QTransform t = item->sceneTransform();
                    QPainterPath mapped = t.map(proxyLoc);
                    QRectF bbox = mapped.boundingRect();
                    m_cache[item] = {proxyLoc, mapped, {}, bbox, t};
                }
            }, Qt::QueuedConnection);
        });
    }

    scene->blockSignals(false);
    vp->setUpdatesEnabled(true);
    vp->update();
}

void FormeVisualization::moveSelectedShapes(qreal dx, qreal dy)
{
    if (warnIfCutting())
        return;
    // Parcours de tous les items sélectionnés dans la scène
    for (QGraphicsItem *item : scene->selectedItems()) {
        item->moveBy(dx, dy);
    }
    m_rotationPivotValid = false;
}


void FormeVisualization::rotateSelectedShapes(qreal angleDelta)
{
    if (warnIfCutting())
        return;
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
    if (warnIfCutting())
        return;

    bool removed = false;
    const auto selected = scene->selectedItems();
    for (QGraphicsItem *item : selected) {
        if (item == m_cutPathItem)
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
    if (warnIfCutting())
        return;

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
        scaledPath.setFillRule(Qt::OddEvenFill);
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

void FormeVisualization::colorPositionRed(const QPoint& p)
{
    if (!m_cutPathItem)
        return;
    m_cutPath.addEllipse(p.x() - 1, p.y() - 1, 2, 2);
    ++m_cutPathPoints;
    if (m_cutPathPoints % kCutPathUpdateFreq == 0)
        m_cutPathItem->setPath(m_cutPath);
    if (m_lastCutColor != Qt::red) {
        m_cutPathItem->setBrush(Qt::red);
        m_lastCutColor = Qt::red;
    }
}

// -----------------------------------------------------------------------------
// Ajout d’un point bleu
// -----------------------------------------------------------------------------
void FormeVisualization::colorPositionBlue(const QPoint& p)
{
    if (!m_cutPathItem)
        return;
    m_cutPath.addEllipse(p.x() - 1, p.y() - 1, 2, 2);
    ++m_cutPathPoints;
    if (m_cutPathPoints % kCutPathUpdateFreq == 0)
        m_cutPathItem->setPath(m_cutPath);
    if (m_lastCutColor != Qt::blue) {
        m_cutPathItem->setBrush(Qt::blue);
        m_lastCutColor = Qt::blue;
    }
}

// -----------------------------------------------------------------------------
// Remise à zéro des marqueurs de découpe
// -----------------------------------------------------------------------------
void FormeVisualization::resetCutMarkers()
{
    m_cutPath = QPainterPath();
    m_cutPathPoints = 0;
    if (m_cutPathItem)
        m_cutPathItem->setPath(m_cutPath);
    m_lastCutColor = Qt::transparent;
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
void FormeVisualization::setDecoupeEnCours(bool etat)
{
    m_decoupeEnCours = etat;
    // Interdire ou autoriser le déplacement manuel des items
    for (QGraphicsItem *item : scene->items()) {
        if (item == m_cutPathItem)
            continue;
        item->setFlag(QGraphicsItem::ItemIsMovable, !etat);
    }
}

int FormeVisualization::countPlacedShapes() const
{
    int count = 0;
    for (QGraphicsItem *item : scene->items()) {
        if (item == m_cutPathItem)
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
    qApp->setProperty("invalidReason", 0);
    auto *vp = graphicsView->viewport();
    vp->setUpdatesEnabled(false);
    const bool oldAA = graphicsView->renderHints() & QPainter::Antialiasing;
    graphicsView->setRenderHint(QPainter::Antialiasing, false);

    resetAllShapeColors();
    bool allValid = true;
    QList<QAbstractGraphicsShapeItem*> shapes;
    for (QGraphicsItem *item : scene->items()) {
        if (item == m_cutPathItem)
            continue;
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item))
            shapes << shape;
    }

    double eps = globalEpsilon();
    QRectF bounds = scene->sceneRect().adjusted(-eps, -eps, eps, eps);
    QVector<QPainterPath> paths;
    QVector<QRectF> bboxes;
    paths.reserve(shapes.size());
    bboxes.reserve(shapes.size());

    for (auto *shape : shapes) {
        auto &cache = m_cache[shape];
        if (cache.base.isEmpty()) {
            cache.base = buildProxyPath(shape->shape());
            cache.base.setFillRule(Qt::OddEvenFill);
        }
        QTransform t = shape->sceneTransform();
        if (cache.transform != t) {
            cache.path      = t.map(cache.base);
            cache.path.setFillRule(Qt::OddEvenFill);
            cache.bbox      = cache.path.boundingRect();
            cache.transform = t;
        }
        paths << cache.path;
        bboxes << cache.bbox;
        if (!bounds.contains(cache.bbox)) {
            shape->setPen(QPen(Qt::red, 1));
            allValid = false;
            if (qApp->property("invalidReason").toInt() == 0)
                qApp->setProperty("invalidReason", OutOfBounds);
        }
    }

    struct ShapeGeom { QPainterPath path; QRectF bbox; };
    QVector<ShapeGeom> geoms;
    geoms.reserve(paths.size());
    for (int i = 0; i < paths.size(); ++i)
        geoms.push_back({paths[i], bboxes[i]});

    QElapsedTimer timer;
    timer.start();
    int pairsTested = 0;

    QHash<QPair<int,int>, QVector<int>> grid;
    grid.reserve(geoms.size() * 2);
    QSet<quint64> tested;
    QVector<bool> invalid(shapes.size(), false);

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
                if (invalid[i] && invalid[j])
                    continue;
                const quint64 key = (static_cast<quint64>(qMin(i, j)) << 32) | qMax(i, j);
                if (tested.contains(key))
                    continue;
                tested.insert(key);
                ++pairsTested;
                QRectF b1 = geoms[i].bbox.adjusted(-kBroadPhaseMargin, -kBroadPhaseMargin,
                                                   kBroadPhaseMargin, kBroadPhaseMargin);
                if (!b1.intersects(geoms[j].bbox))
                    continue;
                if (pathsOverlap(geoms[i].path, geoms[j].path)) {
                    shapes[i]->setPen(QPen(Qt::red, 1));
                    shapes[j]->setPen(QPen(Qt::red, 1));
                    invalid[i] = true;
                    invalid[j] = true;
                    allValid = false;
                    if (qApp->property("invalidReason").toInt() == 0)
                        qApp->setProperty("invalidReason", InteriorOverlap);
                }
            }
        }
    }

    qApp->setProperty("validationPairs", pairsTested);
    qApp->setProperty("validationMs", timer.elapsed());

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
        if (item == m_cutPathItem)
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
    // Recrée les éléments spéciaux de la scène
    m_cutPath = QPainterPath();
    m_cutPathPoints = 0;
    m_lastCutColor = Qt::transparent;
    m_cutPathItem = scene->addPath(QPainterPath(), QPen(Qt::NoPen));
    m_cutPathItem->setZValue(1001);
    m_sheetBorder = scene->addRect(scene->sceneRect(),
                                   QPen(Qt::white, 2), QBrush(Qt::NoBrush));
    m_sheetBorder->setZValue(1000);

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
    scaledPath.setFillRule(Qt::OddEvenFill);
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
        if (item == m_cutPathItem)
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

