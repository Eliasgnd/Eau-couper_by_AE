#include "PathGenerator.h"

#include <QLineF>
#include <QtGlobal>

QPainterPath PathGenerator::generateBezierPath(const QList<QPointF> &pts)
{
    QPainterPath path;
    if (pts.isEmpty()) return path;

    path.moveTo(pts.first());
    for (int i = 1; i < pts.size(); ++i) {
        const QPointF midpoint = (pts[i - 1] + pts[i]) * 0.5;
        path.quadTo(pts[i - 1], midpoint);
    }
    path.lineTo(pts.last());
    return path;
}

QList<QPointF> PathGenerator::applyChaikinAlgorithm(const QList<QPointF> &inputPoints, int iterations)
{
    QList<QPointF> points = inputPoints;
    for (int iter = 0; iter < iterations; ++iter) {
        if (points.size() < 2) break;

        QList<QPointF> smooth;
        smooth.reserve(points.size() * 2);
        smooth.append(points.first());

        for (int i = 0; i < points.size() - 1; ++i) {
            const QPointF &a = points[i];
            const QPointF &b = points[i + 1];
            smooth.append(a * 0.75 + b * 0.25);
            smooth.append(a * 0.25 + b * 0.75);
        }

        smooth.append(points.last());
        points.swap(smooth);
    }
    return points;
}

QList<QPainterPath> PathGenerator::separateIntoSubpaths(const QPainterPath &path)
{
    QList<QPainterPath> subpaths;
    const QList<QPolygonF> polygons = path.toSubpathPolygons();
    subpaths.reserve(polygons.size());
    for (const QPolygonF &poly : polygons) {
        if (poly.isEmpty()) continue;
        QPainterPath subpath;
        subpath.addPolygon(poly);
        // On simplifie pour nettoyer les points redondants
        subpaths.append(subpath.simplified());
    }
    return subpaths;
}

double PathGenerator::distance(const QPointF &p1, const QPointF &p2)
{
    return QLineF(p1, p2).length();
}

double PathGenerator::smoothingAlpha(int level)
{
    return qBound(0.05, 0.12 + static_cast<double>(level) * 0.08, 0.95);
}

int PathGenerator::computeSmoothingIterations(const QList<QPointF> &pts,
                                              double minAlpha,
                                              double maxAlpha,
                                              int maxLevel,
                                              double lowSpeedThreshold)
{
    if (pts.size() < 3) return 1;

    double cumulative = 0.0;
    for (int i = 1; i < pts.size(); ++i) {
        cumulative += distance(pts[i - 1], pts[i]);
    }
    const double avgDistance = cumulative / static_cast<double>(pts.size() - 1);

    if (avgDistance <= lowSpeedThreshold) return maxLevel;
    const double normalized = qBound(0.0, (avgDistance - minAlpha) / qMax(0.001, maxAlpha - minAlpha), 1.0);
    return qMax(1, static_cast<int>((1.0 - normalized) * static_cast<double>(maxLevel)));
}
