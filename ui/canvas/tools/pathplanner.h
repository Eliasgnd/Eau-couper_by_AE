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
    // Wrapper complet (extraction + calcul) — thread principal uniquement
    static QList<ContinuousCut> getOptimizedPaths(QGraphicsScene *sc, QPointF startPos);

    // Étape 1 : lecture de la scène (doit s'exécuter sur le thread principal)
    static QList<ContinuousCut> extractRawPaths(QGraphicsScene *sc);

    // Étape 2 : calcul pur (peut s'exécuter sur un worker thread)
    static QList<ContinuousCut> computeOptimizedPaths(QList<ContinuousCut> cuts, QPointF startPos);

private:
    static void computeInclusionDepths(QList<ContinuousCut>& cuts);
    static void applyLeadIns(QList<ContinuousCut>& cuts, double distance);

    // Ajout de la déclaration manquante ici :
    static void mergeCommonLines(QList<ContinuousCut>& cuts);
};
