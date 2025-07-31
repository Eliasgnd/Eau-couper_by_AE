#ifndef GEOMETRYUTILS_H
#define GEOMETRYUTILS_H

#include <QPainterPath>
#include <QPolygonF>
#include <QList>
#include <cmath>

namespace GeometryUtils {

inline bool segmentsCross(const QPointF& a, const QPointF& b,
                          const QPointF& c, const QPointF& d)
{
    auto orient = [](const QPointF& p1, const QPointF& p2, const QPointF& p3) {
        return (p2.x() - p1.x()) * (p3.y() - p1.y()) -
               (p2.y() - p1.y()) * (p3.x() - p1.x());
    };

    const double o1 = orient(a, b, c);
    const double o2 = orient(a, b, d);
    const double o3 = orient(c, d, a);
    const double o4 = orient(c, d, b);

    const double eps = 1e-6;

    if (std::abs(o1) < eps || std::abs(o2) < eps ||
        std::abs(o3) < eps || std::abs(o4) < eps)
        return false;

    return ((o1 > 0) != (o2 > 0)) && ((o3 > 0) != (o4 > 0));
}

inline bool pathsCross(const QPainterPath& p1, const QPainterPath& p2)
{
    const QPolygonF poly1 = p1.toFillPolygon();
    const QPolygonF poly2 = p2.toFillPolygon();

    for (int i = 0; i < poly1.size(); ++i) {
        const QPointF a1 = poly1[i];
        const QPointF a2 = poly1[(i + 1) % poly1.size()];
        for (int j = 0; j < poly2.size(); ++j) {
            const QPointF b1 = poly2[j];
            const QPointF b2 = poly2[(j + 1) % poly2.size()];
            if (segmentsCross(a1, a2, b1, b2))
                return true;
        }
    }
    return false;
}

inline bool anyPathsCross(const QList<QPainterPath>& paths)
{
    for (int i = 0; i < paths.size(); ++i)
        for (int j = i + 1; j < paths.size(); ++j)
            if (pathsCross(paths[i], paths[j]))
                return true;
    return false;
}

} // namespace GeometryUtils

#endif // GEOMETRYUTILS_H
