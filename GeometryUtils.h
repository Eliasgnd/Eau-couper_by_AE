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
// epsilon defines the minimum interior intersection area considered a collision.
bool pathsOverlap(const QPainterPath &a, const QPainterPath &b, double epsilon = 0.5);

// Enable or disable low-end mode which relaxes tolerances and lowers raster resolution.
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

bool sanitizePolygon(QPolygonF &poly, double eps = 1e-6);
bool sanitizePolygons(QList<QPolygonF> &polys, double eps = 1e-6);
