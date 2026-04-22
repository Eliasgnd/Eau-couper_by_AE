#include "pathplanner.h"

#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QPainterPath>
#include <QSet>
#include <QString>
#include <QLineF>
#include <algorithm>
#include <limits>
#include <cmath>

// ============================================================================
// CLÉ DE HACHAGE : Permet d'éliminer instantanément les doubles découpes
// (Tolérance de 0.1 mm, peu importe le sens de tracé A->B ou B->A)
// ============================================================================
static inline QString getSegKey(const QPointF& p1, const QPointF& p2) {
    int x1 = qRound(p1.x() * 10.0);
    int y1 = qRound(p1.y() * 10.0);
    int x2 = qRound(p2.x() * 10.0);
    int y2 = qRound(p2.y() * 10.0);

    // Ordre canonique pour que A->B donne la même clé que B->A
    if (x1 > x2 || (x1 == x2 && y1 > y2)) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    return QString("%1_%2_%3_%4").arg(x1).arg(y1).arg(x2).arg(y2);
}

// Structure interne pour l'algorithme
struct RawSegment {
    QPointF p1;
    QPointF p2;
    int depth;
};

// ============================================================================
// FONCTION PRINCIPALE : Extraction et Optimisation complète
// ============================================================================
QList<ContinuousCut> PathPlanner::getOptimizedPaths(QGraphicsScene *sc, QPointF startPos)
{
    return computeOptimizedPaths(extractRawPaths(sc), startPos);
}

// ============================================================================
// ÉTAPE 2 : Calcul Pur (L'algorithme Graphe / Zéro Levée de buse de votre ami)
// ============================================================================
QList<ContinuousCut> PathPlanner::computeOptimizedPaths(QList<ContinuousCut> cuts, QPointF startPos)
{
    if (cuts.isEmpty()) return {};

    // 1. Déterminer qui est un trou et qui est un contour
    computeInclusionDepths(cuts);

    int maxDepth = 0;
    for (const auto& c : cuts) {
        maxDepth = qMax(maxDepth, c.depth);
    }

    // 2. EXPLOSION ET DÉDUPLICATION
    QList<RawSegment> uniqueSegments;
    QSet<QString> seenKeys;

    for (const auto& cut : cuts) {
        for (int i = 0; i < cut.points.size() - 1; ++i) {
            QPointF p1 = cut.points[i];
            QPointF p2 = cut.points[i+1];

            // Ignorer les micro-mouvements invisibles
            if (QLineF(p1, p2).length() < 0.1) continue;

            QString key = getSegKey(p1, p2);
            if (!seenKeys.contains(key)) {
                seenKeys.insert(key);
                uniqueSegments.append({p1, p2, cut.depth});
            }
        }
    }

    // 3. REASSEMBLAGE EULÉRIEN (Parcours continu le plus long possible)
    QList<ContinuousCut> optimized;
    QPointF currentPos = startPos;
    const double SNAP_TOLERANCE = 0.1; // S'accroche si les bouts sont à 0.1mm près

    // RÈGLE D'OR : On traite niveau par niveau (les Trous d'abord, puis l'extérieur)
    for (int d = maxDepth; d >= 0; --d) {

        // Isoler les segments du niveau de profondeur actuel
        QList<RawSegment> currentDepthSegs;
        for (const auto& seg : uniqueSegments) {
            if (seg.depth == d) currentDepthSegs.append(seg);
        }

        while (!currentDepthSegs.isEmpty()) {

            // --- A. TROUVER LE POINT D'ATTAQUE LE PLUS PROCHE ---
            int bestStartIdx = -1;
            double minDist = std::numeric_limits<double>::max();
            bool reverseStart = false;

            for (int i = 0; i < currentDepthSegs.size(); ++i) {
                double d1 = QLineF(currentPos, currentDepthSegs[i].p1).length();
                double d2 = QLineF(currentPos, currentDepthSegs[i].p2).length();

                if (d1 < minDist) { minDist = d1; bestStartIdx = i; reverseStart = false; }
                if (d2 < minDist) { minDist = d2; bestStartIdx = i; reverseStart = true; }
            }

            if (bestStartIdx == -1) break;

            // Créer le nouveau tracé ininterrompu
            ContinuousCut chain;
            chain.depth = d;

            RawSegment firstSeg = currentDepthSegs.takeAt(bestStartIdx);
            chain.points.append(reverseStart ? firstSeg.p2 : firstSeg.p1);

            QPointF chainTail = reverseStart ? firstSeg.p1 : firstSeg.p2;
            chain.points.append(chainTail);

            // --- B. AVALER GOULUMENT TOUT CE QUI TOUCHE SANS LEVER LA TÊTE ---
            bool progress = true;
            while (progress) {
                progress = false;
                int bestNextIdx = -1;
                bool nextReverse = false;

                // Chercher un segment qui prolonge exactement la position actuelle
                for (int i = 0; i < currentDepthSegs.size(); ++i) {
                    if (QLineF(chainTail, currentDepthSegs[i].p1).length() < SNAP_TOLERANCE) {
                        bestNextIdx = i; nextReverse = false; break;
                    }
                    if (QLineF(chainTail, currentDepthSegs[i].p2).length() < SNAP_TOLERANCE) {
                        bestNextIdx = i; nextReverse = true; break;
                    }
                }

                // S'il y a un trait connecté, on l'attache au trajet actuel !
                if (bestNextIdx != -1) {
                    RawSegment nextSeg = currentDepthSegs.takeAt(bestNextIdx);
                    chainTail = nextReverse ? nextSeg.p1 : nextSeg.p2;
                    chain.points.append(chainTail);
                    progress = true;
                }
            }

            // Vérifier si la forme s'est refermée sur elle-même à la fin de la chaîne
            chain.isClosed = (QLineF(chain.points.first(), chain.points.last()).length() < SNAP_TOLERANCE);
            if (chain.isClosed) {
                chain.points.last() = chain.points.first(); // Scelle mathématiquement
            }

            optimized.append(chain);
            currentPos = chain.points.last(); // La tête est ici, prête pour le prochain trajet
        }
    }

    // (Optionnel) Ajouter les entrées de coupe si vous l'utilisez
    // applyLeadIns(optimized, 3.0);

    return optimized;
}

// ============================================================================
// LA FUSION EST GÉRÉE DIRECTEMENT EN HAUT
// ============================================================================
void PathPlanner::mergeCommonLines(QList<ContinuousCut>& /*cuts*/)
{
    // Obsolète : L'algorithme eulérien ci-dessus le fait naturellement
}

// ============================================================================
// EXTRACTION DES TRACÉS (VOTRE VERSION avec nettoyage anti-bugs)
// ============================================================================
QList<ContinuousCut> PathPlanner::extractRawPaths(QGraphicsScene *sc)
{
    QList<ContinuousCut> out;

    for (QGraphicsItem *it : sc->items()) {
        if (!it->isVisible() || it->zValue() >= 50) continue;

        // VOTRE FILTRE 1 : Élimine les objets globalement minuscules
        if (it->boundingRect().width() < 10.0 && it->boundingRect().height() < 10.0) {
            continue;
        }

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

            // --- VOTRE FILTRE 2 : Filtrer les sous-tracés fantômes/points isolés ---
            double pathLength = 0.0;
            for (int i = 1; i < poly.size(); ++i) {
                pathLength += QLineF(poly[i-1], poly[i]).length();
            }
            // Si la forme entière fait moins d'un millimètre, on ignore.
            if (pathLength < 1.0) continue;
            // -----------------------------------------------------------------------

            ContinuousCut cut;
            cut.points = poly;

            // VOTRE SÉCURITÉ 3 : L'égalité flottante pour la fermeture
            cut.isClosed = (QLineF(poly.first(), poly.last()).length() < 0.1);
            cut.depth = 0;

            out.append(cut);
        }
    }
    return out;
}

// ============================================================================
// GESTION DES INCLUSIONS (Inside-Out)
// ============================================================================
void PathPlanner::computeInclusionDepths(QList<ContinuousCut>& cuts)
{
    for (int i = 0; i < cuts.size(); ++i) {
        cuts[i].depth = 0;
        for (int j = 0; j < cuts.size(); ++j) {
            if (i == j || !cuts[j].isClosed) continue;

            if (cuts[j].points.containsPoint(cuts[i].points.first(), Qt::OddEvenFill)) {
                cuts[i].depth++;
            }
        }
    }
}

// ============================================================================
// ENTRÉES DE COUPE (Lead-in) - Restauré depuis votre code d'origine
// ============================================================================
void PathPlanner::applyLeadIns(QList<ContinuousCut>& cuts, double distance)
{
    for (auto& cut : cuts) {
        if (!cut.isClosed || cut.points.size() < 2) continue;

        QPointF p0 = cut.points[0];
        QPointF p1 = cut.points[1];

        double dx = p1.x() - p0.x();
        double dy = p1.y() - p0.y();
        double len = std::hypot(dx, dy);

        if (len < 0.1) continue;

        QPointF leadIn(p0.x() - dy / len * distance, p0.y() + dx / len * distance);
        cut.points.prepend(leadIn);
    }
}
