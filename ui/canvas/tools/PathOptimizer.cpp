#include "PathOptimizer.h"
#include <QLineF>
#include <algorithm>
#include <limits>
#include <cmath>

QList<ContinuousCut> PathOptimizer::optimize(QList<ContinuousCut> rawCuts, QPointF startPos)
{
    if (rawCuts.isEmpty()) return {};

    // 1. Analyser les inclusions (Intérieur avant Extérieur)
    calculateDepths(rawCuts);

    // 2. Générer le trajet optimisé (Nearest Neighbor + Profondeur)
    QList<ContinuousCut> optimizedPath;
    QPointF currentPos = startPos;

    while (!rawCuts.isEmpty()) {
        ContinuousCut nextCut = extractNextBest(currentPos, rawCuts);
        optimizedPath.append(nextCut);
        currentPos = nextCut.points.last(); // Mise à jour de la position de la tête
    }

    // 3. Ajouter les entrées/sorties de coupe (Lead-in) pour les formes fermées
    applyLeadIns(optimizedPath, 3.0); // Amorce de 3 mm dans la chute

    return optimizedPath;
}

void PathOptimizer::calculateDepths(QList<ContinuousCut>& cuts)
{
    // On compare chaque forme avec toutes les autres
    for (int i = 0; i < cuts.size(); ++i) {
        cuts[i].depth = 0;
        for (int j = 0; j < cuts.size(); ++j) {
            if (i == j) continue;

            // Si la forme J est fermée, on regarde si elle englobe la forme I
            if (cuts[j].isClosed) {
                QRectF boxI = cuts[i].points.boundingRect();
                QRectF boxJ = cuts[j].points.boundingRect();

                // Un test rapide avec la BoundingBox, puis un test précis avec containsPoint
                if (boxJ.contains(boxI)) {
                    if (cuts[j].points.containsPoint(cuts[i].points.first(), Qt::OddEvenFill)) {
                        cuts[i].depth++; // La forme I est à l'intérieur de J !
                    }
                }
            }
        }
    }
}

ContinuousCut PathOptimizer::extractNextBest(const QPointF& currentPos, QList<ContinuousCut>& remainingCuts)
{
    // RÈGLE 1 : Trouver la profondeur maximale restante (on doit couper les trous profonds en premier)
    int maxDepth = -1;
    for (const auto& cut : remainingCuts) {
        if (cut.depth > maxDepth) maxDepth = cut.depth;
    }

    // RÈGLE 2 : Trouver le point le plus proche parmi les formes de cette profondeur max
    int bestIdx = 0;
    double bestDi = std::numeric_limits<double>::max();
    bool reverseBest = false;
    int bestStartIndex = 0;

    for (int i = 0; i < remainingCuts.size(); ++i) {
        const auto& cut = remainingCuts[i];

        // On IGNORE les contours externes s'il reste des trous intérieurs à faire !
        if (cut.depth != maxDepth) continue;

        if (cut.isClosed) {
            for (int j = 0; j < cut.points.size() - 1; ++j) {
                double d = QLineF(currentPos, cut.points[j]).length();
                if (d < bestDi) {
                    bestDi = d; bestIdx = i; reverseBest = false; bestStartIndex = j;
                }
            }
        } else {
            double d1 = QLineF(currentPos, cut.points.first()).length();
            if (d1 < bestDi) {
                bestDi = d1; bestIdx = i; reverseBest = false; bestStartIndex = 0;
            }
            double d2 = QLineF(currentPos, cut.points.last()).length();
            if (d2 < bestDi) {
                bestDi = d2; bestIdx = i; reverseBest = true; bestStartIndex = cut.points.size() - 1;
            }
        }
    }

    // Extraction et application de la transformation (décalage ou inversion)
    ContinuousCut bestCut = remainingCuts.takeAt(bestIdx);

    if (bestCut.isClosed && bestStartIndex > 0) {
        QPolygonF newPoly;
        for (int j = bestStartIndex; j < bestCut.points.size() - 1; ++j) newPoly.append(bestCut.points[j]);
        for (int j = 0; j <= bestStartIndex; ++j) newPoly.append(bestCut.points[j]);
        bestCut.points = newPoly;
    } else if (!bestCut.isClosed && reverseBest) {
        std::reverse(bestCut.points.begin(), bestCut.points.end());
    }

    return bestCut;
}

void PathOptimizer::applyLeadIns(QList<ContinuousCut>& cuts, double leadDistance)
{
    for (auto& cut : cuts) {
        // L'amorce de coupe n'a de sens que pour des formes fermées (trous/contours)
        if (!cut.isClosed || cut.points.size() < 2) continue;

        QPointF p0 = cut.points[0];
        QPointF p1 = cut.points[1];

        // Calcul du vecteur directionnel du premier segment
        double dx = p1.x() - p0.x();
        double dy = p1.y() - p0.y();
        double len = std::hypot(dx, dy);

        if (len < 0.001) continue;

        // Calcul d'un vecteur normal (perpendiculaire)
        double nx = -dy / len;
        double ny = dx / len;

        // Création du point d'amorce, décalé dans le vide
        QPointF leadInPoint(p0.x() + nx * leadDistance, p0.y() + ny * leadDistance);

        // On insère l'amorce au tout début
        cut.points.prepend(leadInPoint);

        // Optionnel : On peut ajouter un "Overcut" (couper un peu plus loin à la fin pour être sûr que la pièce se détache)
        cut.points.append(p1);
    }
}
