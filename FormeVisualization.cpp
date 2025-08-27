#include "FormeVisualization.h"

#include <QTimer>            // au lieu de "qtimer.h"
#include <QVBoxLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QPixmap>
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
#include <QSignalBlocker>
#include <QDebug>

#include "GeometryUtils.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>
#include <QThreadPool>

#include <QtConcurrent>
#include <memory>
#include <atomic>

#ifdef QT_DEBUG
#include <cstdio>
static thread_local const char* gPhase = nullptr;
struct PhaseTag {
    const char* saved;
    explicit PhaseTag(const char* n) : saved(gPhase) { gPhase = n; }
    ~PhaseTag() { gPhase = saved; }
};
static void siMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(ctx);
    if (msg.contains("Self-intersection detected in polygon")) {
        qDebug() << "[SI]" << "phase=" << (gPhase ? gPhase : "(unknown)") << msg;
    }
    fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
}
static const auto siHandlerInstaller = [](){ qInstallMessageHandler(siMessageHandler); return 0; }();
#endif

namespace {
constexpr double kBroadPhaseMargin = 0.1;   // bbox expansion for spatial query
constexpr double kGridCellSize     = 50.0;  // uniform grid cell size in scene units
constexpr int    kSegmentSimplifyThreshold = 2000;
constexpr qreal  kLODBucketLow  = 0.8;      // LOD switch for simplified path
enum InvalidReason { Valid = 0, OutOfBounds = 1, InteriorOverlap = 2 };

struct PainterState {
    QPainter *p;
    explicit PainterState(QPainter *p) : p(p) { if (p) p->save(); }
    ~PainterState() { if (p) p->restore(); }
};

struct StyleKey {
    quint32 penColor = 0;
    quint32 brushColor = 0;
    qreal   penWidth = 0.0;
};

static QPair<QPen,QBrush> makeStyle(const StyleKey &k)
{
    return { QPen(QColor::fromRgba(k.penColor), k.penWidth),
             QBrush(QColor::fromRgba(k.brushColor)) };
}

static bool intersects(const QPainterPath &p, const QRectF &r)
{
    return p.intersects(r) || r.contains(p.boundingRect());
}

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

class LODPathItem : public QGraphicsPathItem
{
public:
    struct State {
        QPainterPath full;
        QPainterPath lite;
        QPainterPath stroke;
        QPixmap      pixmap;
        QRectF       bounds;
        StyleKey     key;
    };

    explicit LODPathItem(const QPainterPath &p,
                         const QPen &pen = QPen(),
                         const QBrush &brush = QBrush())
        : QGraphicsPathItem(p)
    {
        QGraphicsPathItem::setPen(pen);
        QGraphicsPathItem::setBrush(brush);
        setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        updateGeometryCaches(p);
        QGraphicsPathItem::setPath(m_state.full);
    }

    void setPath(const QPainterPath &p)
    {
        updateGeometryCaches(p);
        QGraphicsPathItem::setPath(m_state.full);
    }

    void setPen(const QPen &pen)
    {
        if (pen == this->pen())
            return;
        QGraphicsPathItem::setPen(pen);
        updateGeometryCaches(m_state.full);
    }

    void setBrush(const QBrush &brush)
    {
        if (brush == this->brush())
            return;
        QGraphicsPathItem::setBrush(brush);
        updateGeometryCaches(m_state.full);
    }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override
    {
        const QRectF exposed = option->exposedRect;
        if (!intersects(m_state.full, exposed))
            return;

        PainterState guard(painter);
        painter->setClipRect(exposed);

        painter->setBrush(Qt::NoBrush);
        QPen pen(Qt::black, 1);
        pen.setCosmetic(true);
        painter->setPen(pen);

        const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
        painter->setRenderHint(QPainter::Antialiasing, lod >= kLODBucketLow);
        painter->drawPath(selectByLOD(m_state, lod));
    }

    QPainterPath shape() const override
    {
        if (m_state.stroke.isEmpty())
            return m_state.full;
        return m_state.full.united(m_state.stroke);
    }

private:
    static const QPainterPath &selectByLOD(const State &s, qreal lod)
    {
        return (lod < kLODBucketLow) ? s.lite : s.full;
    }

    static void drawState(QPainter *p, const State &s, qreal lod)
    {
        if (!s.pixmap.isNull()) {
            p->drawPixmap(s.bounds.topLeft(), s.pixmap);
            return;
        }
        auto style = makeStyle(s.key);
        p->setRenderHint(QPainter::Antialiasing, lod >= kLODBucketLow);
        p->setPen(style.first);
        p->setBrush(style.second);
        p->drawPath(selectByLOD(s, lod));
    }

    enum class StrokeMode { None, Cosmetic, Normal };

    using StrokeFn = void(*)(LODPathItem*, const QPainterPath&);
    static void buildNone(LODPathItem* self, const QPainterPath&) { self->m_state.stroke = {}; }
    static void buildStroke(LODPathItem* self, const QPainterPath& path) {
        QPainterPathStroker stroker(self->pen());
        {
#ifdef QT_DEBUG
            PhaseTag tag("stroker.createStroke");
#endif
            self->m_state.stroke = stroker.createStroke(path);
        }
    }
    static constexpr StrokeFn kStrokeDispatch[] = { buildNone, buildNone, buildStroke };

    void updateGeometryCaches(const QPainterPath &p)
    {
        m_state.full = p;
        m_state.full.setFillRule(Qt::OddEvenFill);
        m_geomHash = hashPath(m_state.full, 0);

        m_state.lite = simplify(m_state.full);
        qDebug() << "[LODItem] updateGeometryCaches"
                 << "fullElems=" << m_state.full.elementCount()
                 << "bounds=" << m_state.full.boundingRect();

        const int ec = m_state.full.elementCount();
        if (ec > 20000)
            m_state.stroke = QPainterPath();
        else {
            StrokeMode mode = (pen().style()==Qt::NoPen ? StrokeMode::None :
                               (pen().isCosmetic()||pen().widthF()<=0.0 ? StrokeMode::Cosmetic : StrokeMode::Normal));
            kStrokeDispatch[static_cast<int>(mode)](this, m_state.full);
        }

        m_state.bounds = m_state.full.boundingRect();
        m_state.key.penColor   = pen().color().rgba();
        m_state.key.brushColor = brush().color().rgba();
        m_state.key.penWidth   = pen().widthF();

        const QSizeF bs = m_state.bounds.size();
        const int MAX_DIM = 4096;
        if (bs.width()>MAX_DIM || bs.height()>MAX_DIM)
            m_state.pixmap = QPixmap();
        else if (m_state.full.elementCount()>kSegmentSimplifyThreshold)
            m_state.pixmap = renderPixmap(m_state.full, pen(), brush(), 1.0);
        else
            m_state.pixmap = QPixmap();
        qDebug() << "[LODItem] cache"
                 << "pix=" << !m_state.pixmap.isNull()
                 << "penW=" << m_state.key.penWidth;

        const qreal area = m_state.bounds.width()*m_state.bounds.height();
        setCacheMode(area>250000.0 ? QGraphicsItem::ItemCoordinateCache : QGraphicsItem::DeviceCoordinateCache);
    }

    static QPainterPath simplify(const QPainterPath &p)
    {
        QRectF br = p.boundingRect();
        if (p.elementCount() > kSegmentSimplifyThreshold ||
            br.width() * br.height() > 1e6)
            return p;

        QPainterPath tmp = p;
        tmp.setFillRule(Qt::OddEvenFill);

        QPainterPath simp;
        {
#ifdef QT_DEBUG
            PhaseTag tag("simplify.toFillPolygons");
#endif
            for (const QPolygonF &poly : p.toFillPolygons())
                if (poly.size()>=3)
                    simp.addPolygon(poly);
        }
        simp.setFillRule(Qt::OddEvenFill);
#ifdef QT_DEBUG
        PhaseTag tag2("simplify.return");
#endif
        return simp;
    }


    static QPixmap renderPixmap(const QPainterPath &path, const QPen &pen, const QBrush &brush, qreal dpr)
    {
        QRectF br = path.boundingRect();
        QPixmap empty;
        if (!br.isValid() || br.isEmpty())
            return empty;
        QSize sz = (br.size()*dpr).toSize().expandedTo(QSize(1,1));
        if (sz.width()>4096 || sz.height()>4096)
            return empty;
        QPixmap pm(sz);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        QPainter painter(&pm);
        PainterState guard(&painter);
        painter.translate(-br.topLeft());
        painter.setPen(pen);
        painter.setBrush(brush);
        {
#ifdef QT_DEBUG
            PhaseTag tag("renderPixmap.drawPath");
#endif
            painter.drawPath(path);
        }
        return pm;
    }

    State m_state;
    quint64 m_geomHash {0};
};


// Create a three-tier LOD representation for complex paths. A raster fallback
// (P0) is displayed immediately, a simplified polygon proxy (P1) replaces it
// when ready, and the exact path (P2) is set afterwards. Computation happens on
// background threads and can be cancelled by destroying the temporary pixmap
// item before completion.
void FormeVisualization::addPathWithLOD(const QPainterPath &path, const QPointF &pos)
{
    if (!scene) return;

    // Snapshots (évite races)
    const qreal   W = currentLargeur;
    const qreal   H = currentLongueur;
    const QPointF P = pos;
    const bool    hasSize = (W > 0 && H > 0);

    QElapsedTimer lodT; lodT.start();
    qDebug() << "[LOD] enter"
             << "elems=" << path.elementCount()
             << "bbox="  << path.boundingRect()
             << "W=" << W << "H=" << H << "pos=" << P;

    // ── A) Taille connue → pas de pixmap
    if (hasSize) {
        auto *item = new LODPathItem(path);
        item->setFlag(QGraphicsItem::ItemIsMovable,    true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

        applySize(item, W, H);
        const QRectF br = item->path().boundingRect();
        item->setPos(P - br.topLeft());
        scene->addItem(item);

        auto cancel = std::make_shared<std::atomic_bool>(false);
        QObject::connect(this,  &QObject::destroyed, [cancel]{ cancel->store(true, std::memory_order_release); });
        QObject::connect(scene, &QObject::destroyed, [cancel]{ cancel->store(true, std::memory_order_release); });

        QGraphicsPathItem *guard = item;
        QThreadPool::globalInstance()->start([this, path, guard, W, H, P, cancel, lodT]{
            if (cancel->load(std::memory_order_acquire)) return;

            // Worker #1 : clean + proxy
            const QPainterPath clean = [&](){
#ifdef QT_DEBUG
                PhaseTag tag("normalizePath");
#endif
                return normalizePath(path);
            }();
            const QPainterPath proxy = [&](){
#ifdef QT_DEBUG
                PhaseTag tag("buildProxyPath");
#endif
                return buildProxyPath(clean);
            }();
            qDebug() << "[LOD] worker1 done"
                     << "ms=" << lodT.elapsed()
                     << "cleanElems=" << clean.elementCount()
                     << "proxyBbox="  << proxy.boundingRect();
            if (cancel->load(std::memory_order_acquire)) return;

            // UI : appliquer proxy
            QMetaObject::invokeMethod(this, [this, guard, cancel, proxy, W, H, P, clean, lodT]{
                if (cancel->load(std::memory_order_acquire) || !scene) return;
                if (!scene->items().contains(guard)) return;

                guard->setPath(proxy);
                qDebug() << "[LOD] UI proxy" << "W=" << W << "H=" << H
                         << "proxyBbox=" << proxy.boundingRect();
                applySize(guard, W, H);
                const QRectF br2 = guard->path().boundingRect();
                guard->setPos(P - br2.topLeft());

                auto cancel2 = std::make_shared<std::atomic_bool>(false);
                QObject::connect(this, &QObject::destroyed, [cancel2]{ cancel2->store(true, std::memory_order_release); });

                // Worker #2 : exact
                QThreadPool::globalInstance()->start([this, clean, guard, W, H, P, cancel2, lodT]{
                    if (cancel2->load(std::memory_order_acquire)) return;
                    const QPainterPath exact = clean;
                    qDebug() << "[LOD] worker2 exact ready"
                             << "exactElems=" << exact.elementCount()
                             << "ms=" << lodT.elapsed();
                    if (cancel2->load(std::memory_order_acquire)) return;

                    // UI : appliquer exact
                    QMetaObject::invokeMethod(this, [this, guard, cancel2, exact, W, H, P, lodT]{
                        if (cancel2->load(std::memory_order_acquire) || !scene) return;
                        if (!scene->items().contains(guard)) return;

                        guard->setPath(exact);
                        applySize(guard, W, H);
                        const QRectF br3 = guard->path().boundingRect();
                        guard->setPos(P - br3.topLeft());
                        qDebug() << "[LOD] UI exact applied"
                                 << "finalBbox=" << exact.boundingRect()
                                 << "totalMs="  << lodT.elapsed();

                        const QTransform t = guard->sceneTransform();
                        const QPainterPath mapped = t.map(exact);
                        m_cache[guard] = { exact, mapped, {}, mapped.boundingRect(), t };
                    }, Qt::QueuedConnection);
                });
            }, Qt::QueuedConnection);
        });
        return;
    }

    // ── B) Pas de taille → pixmap fallback immédiat
    const int rasterSize = qBound(512,
                                  int(qMax(path.boundingRect().width(), path.boundingRect().height())),
                                  1024);
    QPixmap pm = rasterFallback(path, rasterSize);
    auto *pix  = new QGraphicsPixmapItem(pm);
    pix->setTransformationMode(Qt::FastTransformation);
    pix->setPos(P);
    pix->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    scene->addItem(pix);

    auto cancel = std::make_shared<std::atomic_bool>(false);
    QObject::connect(this,  &QObject::destroyed, [cancel]{ cancel->store(true, std::memory_order_release); });
    QObject::connect(scene, &QObject::destroyed, [cancel]{ cancel->store(true, std::memory_order_release); });

    QThreadPool::globalInstance()->start([this, path, P, pix, cancel, W, H, lodT]{
        if (cancel->load(std::memory_order_acquire)) return;

        // Worker #1 : clean + proxy
        const QPainterPath clean = [&](){
#ifdef QT_DEBUG
            PhaseTag tag("normalizePath");
#endif
            return normalizePath(path);
        }();
        const QPainterPath proxy = [&](){
#ifdef QT_DEBUG
            PhaseTag tag("buildProxyPath");
#endif
            return buildProxyPath(clean);
        }();
        if (cancel->load(std::memory_order_acquire)) return;

        // UI : remplacer le pixmap par l’item(proxy)
        QMetaObject::invokeMethod(this, [this, P, pix, cancel, proxy, W, H, clean, lodT]{
            if (cancel->load(std::memory_order_acquire) || !scene) return;

            auto *item = new LODPathItem(proxy);
            item->setFlag(QGraphicsItem::ItemIsMovable,    true);
            item->setFlag(QGraphicsItem::ItemIsSelectable, true);
            item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
            // contours noirs uniquement
            QPen pen(Qt::black, 1.0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            pen.setCosmetic(true);
            item->setPen(pen);
            item->setBrush(Qt::NoBrush);

            scene->addItem(item);
            scene->removeItem(pix);
            delete pix;

            if (W > 0 && H > 0) {
                applySize(item, W, H);
                const QRectF br2 = item->path().boundingRect();
                item->setPos(P - br2.topLeft());
            } else {
                item->setPos(P);
            }

            auto cancel2 = std::make_shared<std::atomic_bool>(false);
            QObject::connect(this, &QObject::destroyed, [cancel2]{ cancel2->store(true, std::memory_order_release); });
            QGraphicsPathItem *guard = item;

            // Worker #2 : exact
            QThreadPool::globalInstance()->start([this, clean, guard, W, H, P, cancel2, lodT]{
                if (cancel2->load(std::memory_order_acquire)) return;
                const QPainterPath exact = clean;
                if (cancel2->load(std::memory_order_acquire)) return;

                // UI : appliquer exact
                QMetaObject::invokeMethod(this, [this, guard, cancel2, exact, W, H, P, lodT]{
                    if (cancel2->load(std::memory_order_acquire) || !scene) return;
                    if (!scene->items().contains(guard)) return;

                    guard->setPath(exact);
                    guard->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

                    if (W > 0 && H > 0) {
                        applySize(guard, W, H);
                        const QRectF br3 = guard->path().boundingRect();
                        guard->setPos(P - br3.topLeft());
                    }

                    const QTransform t = guard->sceneTransform();
                    const QPainterPath mapped = t.map(exact);
                    m_cache[guard] = { exact, mapped, {}, mapped.boundingRect(), t };
                }, Qt::QueuedConnection);
            });
        }, Qt::QueuedConnection);
    });
}






void FormeVisualization::applySize(QGraphicsPathItem *item, qreal W, qreal H)
{
    if (!item) return;

    QElapsedTimer t; t.start();

    QSignalBlocker blocker(scene);

    // Ne réapplique pas le style si déjà correct
    if (!(item->brush().style()==Qt::NoBrush &&
          item->pen().color()==Qt::black &&
          item->pen().isCosmetic() &&
          qFuzzyCompare(item->pen().widthF(), 1.0))) {
        item->setBrush(Qt::NoBrush);
        QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        pen.setCosmetic(true);
        item->setPen(pen);
    }

    if (W <= 0 || H <= 0) return;

    QPainterPath path = item->path();
    path.setFillRule(Qt::OddEvenFill);
    const QRectF brBefore = path.boundingRect();
    if (brBefore.isEmpty()) return;

    qDebug() << "[SIZE] in"
             << "W=" << W << "H=" << H
             << "bboxBefore=" << brBefore
             << "currT=" << item->transform();

    // Déjà à la bonne taille ?
    if (std::fabs(brBefore.width()-W) <= 0.01 &&
        std::fabs(brBefore.height()-H) <= 0.01) {
        qDebug() << "[SIZE] skip (already sized)" << "ms=" << t.elapsed();
        return;
    }

    // Échelle “baked” dans la géométrie (pas dans la transform)
    const QPointF c = brBefore.center();
    QTransform T;
    T.translate(c.x(), c.y());
    T.scale(W / brBefore.width(), H / brBefore.height());
    T.translate(-c.x(), -c.y());

    item->setPath(T.map(path));        // bake size
    item->setTransform(QTransform());  // transform identité

    const QRectF brAfter = item->path().boundingRect();
    qDebug() << "[SIZE] out"
             << "bboxAfter=" << brAfter
             << "ms=" << t.elapsed();

    item->setData(kSizedOnInsert, 1);
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
            if (item && !m_cutMarkers.contains(item)) {
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
        {
#ifdef QT_DEBUG
            PhaseTag tag("path.simplified");
#endif
            prototypePath = prototypePath.simplified();
        }
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
                if (isPathTooComplex(candidatePath, kMaxPathElements))
                    continue;

                QRectF candBBox = candidatePath.boundingRect();
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
                    if (pathsOverlap(candidatePath, existing.path)) {
                        collision = true;
                        break;
                    }
                }

                if (!collision) {
                    QGraphicsPathItem *item = new LODPathItem(candidatePath);
                    QPen pen(Qt::black, 1);
                    pen.setCosmetic(true);
                    item->setPen(pen);
                    item->setBrush(Qt::NoBrush);
                    item->setFlag(QGraphicsItem::ItemIsMovable, true);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, true);


                    applySize(item, currentLargeur, currentLongueur);


                    // Ajuste la position en fonction du boundingRect réel de l'élément
                    QRectF bounds = item->boundingRect();
                    QPointF offset(x - bounds.x(), y - bounds.y());
                    item->moveBy(offset.x(), offset.y());
                    scene->addItem(item);
                    item->setSelected(false);

                    // Enregistre la position corrigée pour les futurs tests de collision
                    candBBox.translate(offset);
                    candidatePath.translate(offset);
                    placedPaths.insert(item, {candidatePath, candBBox});
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
                if (isPathTooComplex(candidatePath, kMaxPathElements))
                    continue;

                QRectF candBBox = candidatePath.boundingRect();
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
                    if (pathsOverlap(candidatePath, existing.path)) {
                        collision = true;
                        break;
                    }
                }

                if (!collision) {
                    QGraphicsPathItem *item = new LODPathItem(candidatePath);
                    QPen pen(Qt::black, 1);
                    pen.setCosmetic(true);
                    item->setPen(pen);
                    item->setBrush(Qt::NoBrush);
                    item->setFlag(QGraphicsItem::ItemIsMovable, true);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, true);


                    applySize(item, currentLargeur, currentLongueur);


                    QRectF bounds = item->boundingRect();
                    QPointF offset(x - bounds.x(), y - bounds.y());
                    item->moveBy(offset.x(), offset.y());
                    scene->addItem(item);

                    candBBox.translate(offset);
                    candidatePath.translate(offset);
                    placedPaths.insert(item, {candidatePath, candBBox});
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
            qDebug() << "[DISP] addPathWithLOD"
                     << "i=" << i << "xPos=" << xPos << "yPos=" << yPos;
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
            shapeCopy = new LODPathItem(path2->path());

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
    qDebug() << "[DISP] custom"
             << "targetW/H=" << currentLargeur << currentLongueur
             << "polyBbox=" << polyBounds << "count=" << shapes.size();

    if (polyBounds.width() <= 0 || polyBounds.height() <= 0) {
        scene->blockSignals(false);
        vp->setUpdatesEnabled(true);
        vp->update();
        return;
    }

    qreal targetW = currentLargeur;
    qreal targetH = currentLongueur;

    qreal cellWidth = targetW + spacing;
    qreal cellHeight = targetH + spacing;
    int maxCols = qFloor(drawingWidth / cellWidth);
    int maxRows = qFloor(drawingHeight / cellHeight);
    int totalCells = maxCols * maxRows;
    int shapesToPlace = qMin(shapeCount, totalCells);
    qDebug() << "[DISP] grid"
             << "cell=" << cellWidth << "x" << cellHeight
             << "max=" << maxCols << "x" << maxRows
             << "place=" << shapesToPlace;


    for (int i = 0; i < shapesToPlace; ++i) {
        int col = i % maxCols;
        int row = i / maxCols;
        qreal xPos = col * cellWidth;
        qreal yPos = row * cellHeight;

        QGraphicsPathItem *item = new LODPathItem(combinedPath);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        applySize(item, targetW, targetH);
        QRectF br2 = item->transform().mapRect(polyBounds);
        item->setPos(xPos - br2.x(), yPos - br2.y());
        scene->addItem(item);
        item->setSelected(false);
    }

    emit shapesPlacedCount(shapesToPlace);

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

        auto *item = new LODPathItem(scaledPath);
        QPen pen(Qt::black, 1);
        pen.setCosmetic(true);
        item->setPen(pen);
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
void FormeVisualization::addCutMarker(const QPoint& p, const QColor& color, bool center)
{
    int x = center ? p.x() - 1 : p.x();
    int y = center ? p.y() - 1 : p.y();
    auto *dot = scene->addEllipse(x, y, 2, 2,
                                  QPen(Qt::NoPen), QBrush(color));
    m_cutMarkers << dot;
    graphicsView->viewport()->update();
}

void FormeVisualization::colorPositionRed(const QPoint& p)
{
    addCutMarker(p, Qt::red, true);
}

// -----------------------------------------------------------------------------
// Ajout d’un point bleu
// -----------------------------------------------------------------------------
void FormeVisualization::colorPositionBlue(const QPoint& p)
{
    addCutMarker(p, Qt::blue);
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
        item->setFlag(QGraphicsItem::ItemIsMovable, !etat);
    }
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
    qApp->setProperty("invalidReason", 0);
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

    double eps = globalEpsilon();
    QRectF bounds = scene->sceneRect().adjusted(-eps, -eps, eps, eps);
    QVector<QPainterPath> paths;
    QVector<QRectF> bboxes;
    paths.reserve(shapes.size());
    bboxes.reserve(shapes.size());

    for (auto *shape : shapes) {
        auto &cache = m_cache[shape];
        if (cache.base.isEmpty()) {
            {
#ifdef QT_DEBUG
                PhaseTag tag("path.simplified");
#endif
                cache.base = shape->shape().simplified();
            }
            cache.base.setFillRule(Qt::OddEvenFill);
        }
        QTransform t = shape->sceneTransform();
        if (cache.transform != t) {
            cache.path      = t.map(cache.base);
            cache.path.setFillRule(Qt::OddEvenFill);
            cache.bbox      = cache.path.boundingRect();
            cache.polys     = cache.path.toFillPolygons();
            sanitizePolygons(cache.polys);
            cache.transform = t;
        }
        if (cache.polys.isEmpty()) {
            cache.polys = cache.path.toFillPolygons();
            sanitizePolygons(cache.polys);
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

    for (auto it = grid.constBegin(); it != grid.constEnd() && allValid; ++it) {
        const QVector<int> &idxs = it.value();
        for (int a = 0; a < idxs.size() && allValid; ++a) {
            for (int b = a + 1; b < idxs.size(); ++b) {
                int i = idxs[a];
                int j = idxs[b];
                const quint64 key = (static_cast<quint64>(qMin(i, j)) << 32) | qMax(i, j);
                if (tested.contains(key))
                    continue;
                tested.insert(key);
                ++pairsTested;
                QRectF b1 = geoms[i].bbox.adjusted(-kBroadPhaseMargin, -kBroadPhaseMargin,
                                                   kBroadPhaseMargin, kBroadPhaseMargin);
                if (!b1.intersects(geoms[j].bbox))
                    continue;
                QPainterPath pa = geoms[i].path;
                QPainterPath pb = geoms[j].path;
                Qt::FillRule rule = (pa.fillRule() == Qt::WindingFill || pb.fillRule() == Qt::WindingFill)
                                    ? Qt::WindingFill : Qt::OddEvenFill;
                pa.setFillRule(rule);
                pb.setFillRule(rule);
                QPainterPath inter = pa.intersected(pb);
                inter.setFillRule(rule);
                if (!inter.isEmpty()) {
                    double area = 0.0;
                    const auto polys = inter.toFillPolygons(QTransform());
                    for (const QPolygonF &poly : polys)
                        area += ::polygonArea(poly);
                    QRectF ib = inter.boundingRect();
                    double epsArea = qMax(1e-6 * ib.width() * ib.height(), 0.25);
                    if (area > epsArea) {
                        shapes[i]->setPen(QPen(Qt::red, 1));
                        shapes[j]->setPen(QPen(Qt::red, 1));
                        allValid = false;
                        if (qApp->property("invalidReason").toInt() == 0)
                            qApp->setProperty("invalidReason", InteriorOverlap);
                    }
                }
                if (!allValid)
                    break;
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
            QPen pen(Qt::black, 1);
            pen.setCosmetic(true);
            shape->setPen(pen);
        }
    }
}

void FormeVisualization::resetAllShapeColors()
{
    for (QGraphicsItem *item : scene->items()) {
        if (m_cutMarkers.contains(item))
            continue;
        if (auto shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
            QPen pen(Qt::black, 1);
            pen.setCosmetic(true);
            shape->setPen(pen);
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
    scaledPath.setFillRule(Qt::OddEvenFill);
    QRectF scaledBounds = scaledPath.boundingRect();

    for (const LayoutItem &li : layout.items) {
        QGraphicsPathItem *item = new LODPathItem(scaledPath);
        QPen pen(Qt::black, 1);
        pen.setCosmetic(true);
        item->setPen(pen);
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setTransformOriginPoint(item->boundingRect().center());


        applySize(item, currentLargeur, currentLongueur);


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

