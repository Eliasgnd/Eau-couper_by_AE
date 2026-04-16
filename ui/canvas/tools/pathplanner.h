#pragma once

#include <QGraphicsScene>
#include <QList>
#include <QPolygonF>

struct ContinuousCut {
    QPolygonF points;
    bool isClosed;
    int depth = 0; // 0 = contour, 1 = trou, etc.
};

class PathPlanner
{
public:
    // La fonction unique qui fait tout : extraction + tri + optimisation
    static QList<ContinuousCut> getOptimizedPaths(QGraphicsScene *sc, QPointF startPos);

private:
    // Fonctions internes de traitement
    static QList<ContinuousCut> extractRawPaths(QGraphicsScene *sc);
    static void computeInclusionDepths(QList<ContinuousCut>& cuts);
    static void applyLeadIns(QList<ContinuousCut>& cuts, double distance);
};
