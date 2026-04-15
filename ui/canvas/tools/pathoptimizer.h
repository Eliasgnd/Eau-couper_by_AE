#pragma once
#include <QList>
#include <QPointF>
#include "pathplanner.h"

class PathOptimizer
{
public:
    // La fonction principale qui fait toute la magie
    static QList<ContinuousCut> optimize(QList<ContinuousCut> rawCuts, QPointF startPos);

private:
    // Calcule qui est à l'intérieur de qui (Règle d'or Inside-Out)
    static void calculateDepths(QList<ContinuousCut>& cuts);

    // Ajoute une amorce de coupe pour ne pas percer sur la pièce finale
    static void applyLeadIns(QList<ContinuousCut>& cuts, double leadDistance);

    // Trouve la meilleure prochaine forme (Nearest Neighbor + Priorité Profondeur)
    static ContinuousCut extractNextBest(const QPointF& currentPos, QList<ContinuousCut>& remainingCuts);
};
