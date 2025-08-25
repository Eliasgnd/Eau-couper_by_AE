#pragma once
#include <QPainterPath>
#include <QPolygonF>
#include <QLineF>
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

inline bool pathsOverlap(const QPainterPath &a, const QPainterPath &b, double epsilon = 0.5)
{
    if (!a.boundingRect().intersects(b.boundingRect()))
        return false;
    QPainterPath inter = a.intersected(b);
    return pathArea(inter) > epsilon;
}

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
