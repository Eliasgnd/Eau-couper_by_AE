#include "GeometryUtils.h"
#include <QImage>
#include <QPainter>
#include <QPainterPathStroker>
#include <QQueue>
#include <QtGlobal>
#include <QDebug>
#include <QPixmap>
#include <algorithm>

namespace {
struct CacheKey {
    const QPainterPath *ptr;
};
inline bool operator==(const CacheKey &a, const CacheKey &b) { return a.ptr == b.ptr; }
uint qHash(const CacheKey &key, uint seed = 0) {
    return ::qHash(reinterpret_cast<quintptr>(key.ptr), seed);
}

QHash<CacheKey,QPainterPath> gSimplifiedCache;
PipelineMetrics gMetrics;
bool gLowEndMode = false;
bool gSafeMode = false;

QPainterPath simplifyForProxyInternal(const QPainterPath &p, double tol) {
    CacheKey key{&p};
    auto it = gSimplifiedCache.constFind(key);
    if (it != gSimplifiedCache.constEnd())
        return it.value();
    QPainterPath result;
    for (const QPolygonF &poly : p.toFillPolygons()) {
        if (poly.size() < 3) continue;
        QVector<QPointF> pts = poly;
        if (!pts.isEmpty() && pts.first() == pts.last()) pts.removeLast();
        QVector<int> keep; keep << 0 << pts.size()-1;
        auto distToSegment = [](const QPointF &p1, const QPointF &p2, const QPointF &pt){
            QLineF l(p1,p2);
            double len2 = l.length()*l.length();
            if (len2 == 0) return QLineF(p1,pt).length();
            double t = ((pt.x()-p1.x())*(p2.x()-p1.x()) + (pt.y()-p1.y())*(p2.y()-p1.y()))/len2;
            t = qMax(0.0, qMin(1.0, t));
            QPointF proj(p1 + t*(p2-p1));
            return QLineF(proj, pt).length();
        };
        std::function<void(int,int)> dp = [&](int s,int e){
            double maxDist=0; int idx=-1;
            for(int i=s+1;i<e;i++){double d=distToSegment(pts[s], pts[e], pts[i]); if(d>maxDist){maxDist=d;idx=i;}}
            if(idx!=-1 && maxDist>tol){
                keep << idx; dp(s,idx); dp(idx,e);
            }
        };
        dp(0, pts.size()-1);
        std::sort(keep.begin(), keep.end());
        QPainterPath sub; sub.moveTo(pts[keep[0]]);
        for(int i=1;i<keep.size();++i) sub.lineTo(pts[keep[i]]);
        sub.closeSubpath(); result.addPath(sub);
    }
    gSimplifiedCache.insert(key,result);
    return result;
}

QPainterPath erodePath(const QPainterPath &p, double d){
    if (d <= 0) return p;
    QPainterPathStroker s; s.setWidth(d*2);
    QPainterPath stroke = s.createStroke(p);
    return p.subtracted(stroke);
}

bool rasterOverlap(const QPainterPath &a, const QPainterPath &b, int res){
    QRectF bounds = a.boundingRect().united(b.boundingRect());
    if (bounds.isEmpty()) return false;
    QImage img(res, res, QImage::Format_ARGB32);
    img.fill(Qt::black);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.translate(-bounds.topLeft());
    painter.scale(res / bounds.width(), res / bounds.height());
    painter.setPen(Qt::NoPen);
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.setBrush(QColor(255,0,0)); painter.drawPath(a);
    painter.setBrush(QColor(0,255,0)); painter.drawPath(b);
    painter.end();
    const uchar *bits = img.constBits();
    int pixels = img.width()*img.height();
    for (int i=0;i<pixels;++i){
        const uchar r = bits[4*i+2];
        const uchar g = bits[4*i+1];
        if (r && g) return true;
    }
    return false;
}
} // namespace

static double gEpsilon = 0.5;

double globalEpsilon(){ return gEpsilon; }
void setGlobalEpsilon(double eps){ gEpsilon = eps; }

QPainterPath simplifyForProxy(const QPainterPath &p, double tol)
{
    return simplifyForProxyInternal(p, tol);
}

QPainterPath buildProxyPath(const QPainterPath &path)
{
    QRectF bbox = path.boundingRect();
    double diag = QLineF(bbox.topLeft(), bbox.bottomRight()).length();
    double tau = qMax(0.5, diag * (gLowEndMode ? 0.005 : 0.003));
    return simplifyForProxyInternal(path, tau);
}

QPixmap rasterFallback(const QPainterPath &path, int res)
{
    QRectF bounds = path.boundingRect();
    if (bounds.isEmpty() || res <= 0)
        return QPixmap();
    QPixmap pm(res, res);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.translate(-bounds.topLeft());
    painter.scale(res / bounds.width(), res / bounds.height());
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawPath(path);
    painter.end();
    return pm;
}

PipelineMetrics lastPipelineMetrics(){ return gMetrics; }
void setLowEndMode(bool enabled){ gLowEndMode = enabled; }

bool pathsOverlap(const QPainterPath &a, const QPainterPath &b, double epsilon){
    gMetrics = PipelineMetrics();
    gMetrics.pairsProcessed = 1;
    QElapsedTimer timer; timer.start();
    int budgetMs = 12;

    // Tier0: broad phase
    if (!a.boundingRect().intersects(b.boundingRect())) return false;
    gMetrics.tier0Ms = timer.nsecsElapsed()/1000000; // near-zero

    // Tier1: simplification
    double tau = gLowEndMode ? epsilon/2.0 : epsilon/10.0;
    QPainterPath sa = simplifyForProxyInternal(a, tau);
    QPainterPath sb = simplifyForProxyInternal(b, tau);
    gMetrics.simplificationTolerance = tau;
    gMetrics.tier1Ms = timer.nsecsElapsed()/1000000 - gMetrics.tier0Ms;
    if (!sa.boundingRect().intersects(sb.boundingRect())) return false;

    // Tier2: raster proxy
    int raster = gLowEndMode ? 512 : 1024;
    gMetrics.rasterResolution = raster;
    if (!rasterOverlap(sa, sb, raster)) return false;
    gMetrics.tier2Ms = timer.nsecsElapsed()/1000000 - gMetrics.tier0Ms - gMetrics.tier1Ms;
    if (timer.elapsed() > budgetMs){
        QPainterPath ea = erodePath(sa, (sa.boundingRect().width()/raster)*2);
        QPainterPath eb = erodePath(sb, (sb.boundingRect().width()/raster)*2);
        return rasterOverlap(ea, eb, raster);
    }

    // Tier3: exact interior area test
    QPainterPath pa = a; pa.setFillRule(Qt::OddEvenFill);
    QPainterPath pb = b; pb.setFillRule(Qt::OddEvenFill);
    QPainterPath inter = pa.intersected(pb);
    inter.setFillRule(Qt::OddEvenFill);
    gMetrics.tier3Ms = timer.nsecsElapsed()/1000000 - gMetrics.tier0Ms - gMetrics.tier1Ms - gMetrics.tier2Ms;
    QRectF ib = inter.boundingRect();
    double bboxArea = ib.width()*ib.height();
    double epsArea = qMax(1e-6 * bboxArea, 0.25);
    return pathArea(inter) > epsArea;
}

void CutQueue::enqueue(const QPainterPath &a, const QPainterPath &b, std::function<void(bool)> cb){
    m_items.enqueue({a,b,cb});
}

void CutQueue::process(int budgetMs){
    QElapsedTimer timer; timer.start();
    while(!m_items.isEmpty() && !m_cancelled){
        auto it = m_items.dequeue();
        bool overlap = pathsOverlap(it.a, it.b);
        if (it.cb) it.cb(overlap);
        if (timer.elapsed() > budgetMs) break;
    }
}

void CutQueue::cancel(){ m_cancelled = true; m_items.clear(); }

bool sanitizePolygon(QPolygonF &poly, double eps)
{
    QPolygonF cleaned;
    QPointF prev;
    bool hasPrev = false;
    for (const QPointF &pt : poly) {
        if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
            continue;
        if (hasPrev && QLineF(prev, pt).length() <= eps)
            continue;
        cleaned << pt;
        prev = pt;
        hasPrev = true;
    }
    if (cleaned.size() < 3) {
        poly = QPolygonF();
        return true;
    }
    if (cleaned.first() != cleaned.last())
        cleaned << cleaned.first();

    double area = 0.0;
    for (int i = 0; i < cleaned.size() - 1; ++i)
        area += cleaned[i].x()*cleaned[i+1].y() - cleaned[i+1].x()*cleaned[i].y();
    if (area < 0)
        std::reverse(cleaned.begin(), cleaned.end());

    bool selfIntersect = false;
    for (int i = 0; i < cleaned.size() - 1 && !selfIntersect; ++i) {
        QPointF a1 = cleaned[i], a2 = cleaned[i + 1];
        for (int j = i + 1; j < cleaned.size() - 1; ++j) {
            if (std::abs(j - i) <= 1 || (i == 0 && j == cleaned.size() - 2))
                continue;
            QPointF b1 = cleaned[j], b2 = cleaned[j + 1];
            if (QLineF(a1, a2).intersects(QLineF(b1, b2), nullptr) == QLineF::BoundedIntersection) {
                selfIntersect = true;
                break;
            }
        }
    }
    if (selfIntersect)
        qInfo() << "Self-intersection detected in polygon";
    poly = cleaned;
    return true;
}

bool sanitizePolygons(QList<QPolygonF> &polys, double eps)
{
    QList<QPolygonF> result;
    for (QPolygonF p : polys) {
        sanitizePolygon(p, eps);
        if (!p.isEmpty())
            result << p;
    }
    polys = result;
    return true;
}

bool validateAndProxyPolygons(QList<QPolygonF> &polys,
                              bool safeMode,
                              QString *warning,
                              double eps)
{
    QList<QPolygonF> copy = polys; // deep copy
    bool ok = true;
    try {
        ok = sanitizePolygons(copy, eps);
    } catch (...) {
        ok = false;
    }
    if (!ok || copy.isEmpty()) {
        if (!safeMode)
            return false;
        QRectF bounds;
        for (const QPolygonF &p : polys)
            bounds = bounds.united(p.boundingRect());
        if (bounds.isNull())
            return false;
        QPolygonF proxy;
        proxy << bounds.topLeft() << QPointF(bounds.right(), bounds.top())
              << bounds.bottomRight() << QPointF(bounds.left(), bounds.bottom())
              << bounds.topLeft();
        polys = {proxy};
        if (warning)
            *warning = QStringLiteral("Invalid geometry replaced with proxy");
        return true;
    }
    polys = copy;
    if (warning)
        warning->clear();
    return true;
}

void setSafeMode(bool enabled){ gSafeMode = enabled; }
bool safeModeEnabled(){ return gSafeMode; }
