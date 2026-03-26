#ifndef PATHGENERATOR_H
#define PATHGENERATOR_H

#include <QList>
#include <QPainterPath>
#include <QPointF>

class PathGenerator
{
public:
    static QPainterPath generateBezierPath(const QList<QPointF> &pts);
    static QList<QPointF> applyChaikinAlgorithm(const QList<QPointF> &inputPoints, int iterations);
    static double distance(const QPointF &p1, const QPointF &p2);
    static double smoothingAlpha(int level);
    static int computeSmoothingIterations(const QList<QPointF> &pts,
                                          double minAlpha,
                                          double maxAlpha,
                                          int maxLevel,
                                          double lowSpeedThreshold);
    static QList<QPainterPath> separateIntoSubpaths(const QPainterPath &path);
};

#endif // PATHGENERATOR_H
