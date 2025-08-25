#pragma once
#include <QPainterPath>
#include <QPolygonF>
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
    QPainterPath inter = a.intersected(b);
    return pathArea(inter) > epsilon;
}
