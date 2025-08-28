#pragma once
#include <QPainterPath>
#include <QPolygonF>
#include <QLineF>
#include <QList>
#include <QElapsedTimer>
#include <QHash>
#include <QQueue>
#include <functional>
#include <cmath>

inline double pathArea(const QPainterPath &path)
{
    double area = 0.0;
    const auto polys = path.toFillPolygons();
    for (const QPolygonF &poly : polys) {
        double polyArea = 0.0;
        for (int i = 0, n = poly.size(); i < n; ++i) {
            const QPointF &p1 = poly[i];
            const QPointF &p2 = poly[(i + 1) % n];
            polyArea += p1.x() * p2.y() - p2.x() * p1.y();
        }
        area += polyArea / 2.0;
    }
    return std::fabs(area);
}

struct PipelineMetrics {
    int pairsProcessed = 0;
    qint64 tier0Ms = 0;
    qint64 tier1Ms = 0;
    qint64 tier2Ms = 0;
    qint64 tier3Ms = 0;
    int rasterResolution = 0;
    double simplificationTolerance = 0.0;
};

// Adaptive multi-tier overlap test. Uses original geometry for final decision.
// Optional precomputed proxies may be supplied to bypass repeated
// simplification. epsilon is a distance tolerance; interiors are considered
// overlapping only if the intersected area exceeds epsilon*epsilon.
bool pathsOverlap(const QPainterPath &a, const QPainterPath &b,
                  const QPainterPath &pa = QPainterPath(),
                  const QPainterPath &pb = QPainterPath(),
                  double epsilon = 0.5);

// Overload accepting precomputed proxies directly.
bool pathsOverlap(const QPainterPath &paProxy,
                  const QPainterPath &pbProxy,
                  double epsilon);

// Enable or disable low-end mode which relaxes tolerances,
// lowers raster resolution and skips exact overlap checks.
void setLowEndMode(bool enabled);

// Retrieve metrics collected during the most recent pathsOverlap invocation.
PipelineMetrics lastPipelineMetrics();

// Work queue allowing overlap checks to be spread over multiple frames.
class CutQueue {
public:
    void enqueue(const QPainterPath &a, const QPainterPath &b, std::function<void(bool)> cb);
    void process(int budgetMs = 12);
    void cancel();
private:
    struct Item { QPainterPath a; QPainterPath b; std::function<void(bool)> cb; };
    QQueue<Item> m_items;
    bool m_cancelled = false;
};

inline QPainterPath normalizePath(const QPainterPath &path, double eps = 1e-6)
{
    QPainterPath result;
    QPointF lastPoint;
    bool hasLast = false;
    for (int i = 0; i < path.elementCount(); ++i) {
        auto el = path.elementAt(i);
        QPointF pt(el.x, el.y);
        if (std::isnan(pt.x()) || std::isnan(pt.y()))
            continue;
        if (hasLast && QLineF(lastPoint, pt).length() <= eps)
            continue;
        if (!hasLast)
            result.moveTo(pt);
        else
            result.lineTo(pt);
        lastPoint = pt;
        hasLast = true;
    }
    if (result.elementCount() > 2)
        result.closeSubpath();
    return result.simplified();
}

inline bool isPathTooComplex(const QPainterPath &path, int maxElements)
{
    return path.elementCount() > maxElements;
}

constexpr int kMaxPathElements = 10000;

// Remove duplicates, enforce orientation and detect self-intersections.
// Returns false if the polygon was invalid and cleared.
bool sanitizePolygon(QPolygonF &poly, double eps = 1e-6);

// Sanitize a list of polygons, discarding invalid ones. Returns true only if
// all polygons were valid.
bool sanitizePolygons(QList<QPolygonF> &polys, double eps = 1e-6);

// Validate polygons, removing invalid ones. If none remain and safeMode is
// enabled, a bounding box proxy is inserted and a warning message is filled.
// Returns true only when all polygons were valid.
bool validateAndProxyPolygons(QList<QPolygonF> &polys,
                              bool safeMode,
                              QString *warning = nullptr,
                              double eps = 1e-6);

// Enable a global safe mode which forces the use of proxies for invalid shapes.
void setSafeMode(bool enabled);
bool safeModeEnabled();

// Global epsilon used by geometry utilities. Allows consistent tolerance
// tuning across the application on low-end devices.
double globalEpsilon();
void setGlobalEpsilon(double eps);

inline double epsilonArea() { double e = globalEpsilon(); return e * e; }

// Simplify a path using a Douglas–Peucker style algorithm with tolerance tol.
// The original path is untouched and still available for the final cut.
QPainterPath simplifyForProxy(const QPainterPath &p, double tol);

// Build a lightweight proxy for quick display/tests. The tolerance is derived
// from the path's bounding box (roughly 0.3–0.5% of the diagonal).
QPainterPath buildProxyPath(const QPainterPath &path);

// Render a raster fallback of the path for instant preview.
// The result is a square pixmap of size res x res in device coordinates.
QPixmap rasterFallback(const QPainterPath &path, int res = 512);
