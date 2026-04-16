#include "pathplanner.h"

#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QPainterPath>
#include <QLineF>
#include <algorithm>
#include <cmath>

// ============================================================================
// FONCTION PRINCIPALE : Extraction et Optimisation complète
// ============================================================================
QList<ContinuousCut> PathPlanner::getOptimizedPaths(QGraphicsScene *sc, QPointF startPos)
{
    // 1. Extraction des formes de la scène
    QList<ContinuousCut> cuts = extractRawPaths(sc);
    if (cuts.isEmpty()) return {};

    // 2. Calcul des inclusions (Règle Inside-Out : Trous avant Contours)
    computeInclusionDepths(cuts);

    // 3. Tri optimisé (Plus Proche Voisin + Priorité aux trous)
    QList<ContinuousCut> optimized;
    QPointF currentPos = startPos;

    while (!cuts.isEmpty()) {
        // Trouver la profondeur maximale restante
        int maxDepth = -1;
        for (const auto& c : cuts) {
            if (c.depth > maxDepth) {
                maxDepth = c.depth;
            }
        }

        int bestIdx = -1;
        double minDist = 1e10; // Valeur très grande par défaut
        bool mustReverse = false;

        // Chercher la forme la plus proche parmi celles qui ont la profondeur max
        for (int i = 0; i < cuts.size(); ++i) {
            if (cuts[i].depth != maxDepth) continue;

            double d1 = QLineF(currentPos, cuts[i].points.first()).length();

            // Si la forme est fermée, on ne peut pas la prendre à l'envers
            double d2 = cuts[i].isClosed ? 1e10 : QLineF(currentPos, cuts[i].points.last()).length();

            if (d1 < minDist) {
                minDist = d1;
                bestIdx = i;
                mustReverse = false;
            }
            if (d2 < minDist) {
                minDist = d2;
                bestIdx = i;
                mustReverse = true;
            }
        }

        // Sécurité : si on ne trouve rien (ne devrait pas arriver), on force la sortie
        if (bestIdx == -1) break;

        // Extraire la meilleure forme et l'inverser si nécessaire
        ContinuousCut next = cuts.takeAt(bestIdx);
        if (mustReverse && !next.isClosed) {
            std::reverse(next.points.begin(), next.points.end());
        }

        optimized.append(next);
        currentPos = optimized.last().points.last();
    }

    // 4. Ajouter les entrées de coupe (Lead-in) pour percer à côté des pièces finies
    applyLeadIns(optimized, 3.0); // Décalage de 3 mm dans le vide

    return optimized;
}

// ============================================================================
// FONCTIONS UTILITAIRES INTERNES
// ============================================================================

QList<ContinuousCut> PathPlanner::extractRawPaths(QGraphicsScene *sc)
{
    QList<ContinuousCut> out;

    for (QGraphicsItem *it : sc->items()) {
        if (!it->isVisible() || it->zValue() >= 50) continue;

        QPainterPath path;

        if (auto *r = qgraphicsitem_cast<QGraphicsRectItem *>(it))
            path.addRect(r->rect());
        else if (auto *p = qgraphicsitem_cast<QGraphicsPathItem *>(it))
            path = p->path();
        else if (auto *e = qgraphicsitem_cast<QGraphicsEllipseItem *>(it))
            path.addEllipse(e->rect());
        else if (auto *poly = qgraphicsitem_cast<QGraphicsPolygonItem *>(it))
            path.addPolygon(poly->polygon());
        else
            continue;

        path = it->mapToScene(path);

        for (const QPolygonF& poly : path.toSubpathPolygons()) {
            if (poly.size() < 2) continue;

            ContinuousCut cut;
            cut.points = poly;
            // Une forme est fermée si son premier point touche exactement son dernier point
            cut.isClosed = (poly.first() == poly.last());
            cut.depth = 0;

            out.append(cut);
        }
    }
    return out;
}

void PathPlanner::computeInclusionDepths(QList<ContinuousCut>& cuts)
{
    // On compare chaque forme avec toutes les autres
    for (int i = 0; i < cuts.size(); ++i) {
        cuts[i].depth = 0;
        for (int j = 0; j < cuts.size(); ++j) {
            if (i == j || !cuts[j].isClosed) continue;

            // Si la forme 'i' commence à l'intérieur de la forme fermée 'j'
            if (cuts[j].points.containsPoint(cuts[i].points.first(), Qt::OddEvenFill)) {
                cuts[i].depth++;
            }
        }
    }
}

void PathPlanner::applyLeadIns(QList<ContinuousCut>& cuts, double distance)
{
    for (auto& cut : cuts) {
        // Le perçage décalé n'a de sens que pour des formes fermées (cercles, carrés...)
        if (!cut.isClosed || cut.points.size() < 2) continue;

        QPointF p0 = cut.points[0];
        QPointF p1 = cut.points[1];

        // Calcul du vecteur directionnel (de p0 vers p1)
        double dx = p1.x() - p0.x();
        double dy = p1.y() - p0.y();
        double len = std::hypot(dx, dy);

        if (len < 0.1) continue; // Sécurité division par zéro

        // Créer un point décalé perpendiculairement au tracé pour commencer la coupe
        QPointF leadIn(p0.x() - dy / len * distance, p0.y() + dx / len * distance);

        // Insérer le point de perçage au tout début de la forme
        cut.points.prepend(leadIn);
    }
}
