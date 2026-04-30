#include "pathplanner.h"
#include "clipper2/clipper.h"

#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QPainterPath>
#include <QSet>
#include <QHash>
#include <QMap>
#include <QString>
#include <QLineF>
#include <QRectF>
#include <algorithm>
#include <limits>
#include <cmath>
#include <vector>

// --- INCLUSIONS LEMON GRAPH LIBRARY ---
#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/matching.h>
#include <lemon/euler.h>
#include <lemon/connectivity.h>

using namespace Clipper2Lib;

// ============================================================================
// STRUCTURES MATHÉMATIQUES
// ============================================================================
struct Edge {
    QPoint p1, p2;
    bool operator==(const Edge& o) const {
        return (p1 == o.p1 && p2 == o.p2) || (p1 == o.p2 && p2 == o.p1);
    }
};

inline size_t qHash(const Edge& e, size_t seed = 0) {
    return qHash(e.p1.x() ^ e.p2.x(), seed) ^ qHash(e.p1.y() ^ e.p2.y(), seed);
}

struct RawSeg { QPointF p1, p2; int depth; };

// ============================================================================
// FONCTION PRINCIPALE
// ============================================================================
QList<ContinuousCut> PathPlanner::getOptimizedPaths(QGraphicsScene *sc, QPointF startPos)
{
    return computeOptimizedPaths(extractRawPaths(sc), startPos);
}

// ============================================================================
// L'ALGORITHME INDUSTRIEL (LEMON Blossom Matching + Eulerian Path)
// ============================================================================
QList<ContinuousCut> PathPlanner::computeOptimizedPaths(QList<ContinuousCut> cuts, QPointF startPos)
{
    if (cuts.isEmpty()) return {};

    computeInclusionDepths(cuts);

    // 1. EXTRACTION & NODING (Saucissonnage des T-Junctions)
    QList<RawSeg> rawSegments;
    QList<QPointF> allVertices;

    for (const auto& cut : cuts) {
        for (int i = 0; i < cut.points.size(); ++i) {
            allVertices.append(cut.points[i]);
            if (!cut.isClosed && i == cut.points.size() - 1) break;

            QPointF p1 = cut.points[i];
            QPointF p2 = cut.points[(i + 1) % cut.points.size()];
            if (QLineF(p1, p2).length() > 0.01) {
                rawSegments.append({p1, p2, cut.depth});
            }
        }
    }

    // TRES IMPORTANT : Tolérance augmentée à 0.5mm pour attraper les micro-écarts de l'optimiseur
    const double SPLIT_TOLERANCE = 0.5;
    QList<RawSeg> splitSegments;

    for (const RawSeg& seg : rawSegments) {
        QPointF a = seg.p1;
        QPointF b = seg.p2;
        double l2 = std::pow(a.x() - b.x(), 2) + std::pow(a.y() - b.y(), 2);

        struct SplitPt { QPointF pt; double t; };
        QList<SplitPt> splitPoints;

        for (const QPointF& pt : allVertices) {
            double t = ((pt.x() - a.x()) * (b.x() - a.x()) + (pt.y() - a.y()) * (b.y() - a.y())) / l2;
            if (t > 1e-3 && t < 1.0 - 1e-3) {
                QPointF proj(a.x() + t * (b.x() - a.x()), a.y() + t * (b.y() - a.y()));
                if (std::hypot(pt.x() - proj.x(), pt.y() - proj.y()) <= SPLIT_TOLERANCE) {
                    splitPoints.append({proj, t}); // On utilise la projection pour être parfaitement aligné !
                }
            }
        }

        if (splitPoints.isEmpty()) {
            splitSegments.append(seg);
        } else {
            std::sort(splitPoints.begin(), splitPoints.end(), [](const SplitPt& s1, const SplitPt& s2) { return s1.t < s2.t; });
            QPointF currentStart = a;
            for (const SplitPt& sp : splitPoints) {
                if (std::hypot(sp.pt.x() - currentStart.x(), sp.pt.y() - currentStart.y()) > 1e-3) {
                    splitSegments.append({currentStart, sp.pt, seg.depth});
                    currentStart = sp.pt;
                }
            }
            if (std::hypot(b.x() - currentStart.x(), b.y() - currentStart.y()) > 1e-3) {
                splitSegments.append({currentStart, b, seg.depth});
            }
        }
    }

    // 2. AIMANTATION DES POINTS ET DÉDOUBLONNAGE
    QList<QPointF> snappedPoints;
    auto snapPoint = [&](QPointF pt) -> QPointF {
        for (const QPointF& sp : snappedPoints) {
            // Si le point est à moins de 0.2mm d'un autre, on les fusionne de force !
            if (std::hypot(pt.x() - sp.x(), pt.y() - sp.y()) < 0.2) {
                return sp;
            }
        }
        snappedPoints.append(pt);
        return pt;
    };

    QHash<Edge, int> uniqueEdges;
    const double SCALE = 1000.0;
    const double INV_SCALE = 1.0 / 1000.0;

    for (const auto& seg : splitSegments) {
        // L'aimantation répare les micro-espacements de l'optimiseur
        QPointF p1 = snapPoint(seg.p1);
        QPointF p2 = snapPoint(seg.p2);

        // On ignore les lignes microscopiques créées par erreur
        if (std::hypot(p1.x() - p2.x(), p1.y() - p2.y()) < 0.05) continue;

        QPoint pt1(qRound(p1.x() * SCALE), qRound(p1.y() * SCALE));
        QPoint pt2(qRound(p2.x() * SCALE), qRound(p2.y() * SCALE));

        if (pt1 != pt2) {
            Edge e = {pt1, pt2};
            if (uniqueEdges.contains(e)) {
                // Ligne commune parfaitement superposée -> on ne la coupera qu'une seule fois !
                uniqueEdges[e] = std::max(uniqueEdges[e], seg.depth);
            } else {
                uniqueEdges.insert(e, seg.depth);
            }
        }
    }

    QMap<int, QList<Edge>> edgesByDepth;
    int maxDepth = 0;
    for (auto it = uniqueEdges.begin(); it != uniqueEdges.end(); ++it) {
        edgesByDepth[it.value()].append(it.key());
        if (it.value() > maxDepth) maxDepth = it.value();
    }

    QList<ContinuousCut> allSubPaths;
    QPointF currentMachinePos = startPos;

    // 3. THÉORIE DES GRAPHES PAR PROFONDEUR (LEMON)
    for (int d = maxDepth; d >= 0; --d) {
        if (!edgesByDepth.contains(d)) continue;

        QList<Edge> currentEdges = edgesByDepth[d];

        lemon::ListGraph g;
        lemon::ListGraph::EdgeMap<bool> isVirtual(g, false);

        QHash<QPoint, int> qptToNodeId;
        QHash<int, QPoint> nodeIdToQpt;

        auto getNode = [&](const QPoint& pt) {
            if (!qptToNodeId.contains(pt)) {
                auto n = g.addNode();
                int id = g.id(n);
                qptToNodeId[pt] = id;
                nodeIdToQpt[id] = pt;
            }
            return g.nodeFromId(qptToNodeId[pt]);
        };

        for (const Edge& e : currentEdges) {
            auto u = getNode(e.p1);
            auto v = getNode(e.p2);
            auto edge = g.addEdge(u, v);
            isVirtual[edge] = false;
        }

        lemon::ListGraph::NodeMap<int> compMap(g);
        int numIslands = lemon::connectedComponents(g, compMap);

        for (int islandId = 0; islandId < numIslands; ++islandId) {
            QList<lemon::ListGraph::Node> islandNodes;
            QList<lemon::ListGraph::Node> oddNodes;

            for (lemon::ListGraph::NodeIt n(g); n != lemon::INVALID; ++n) {
                if (compMap[n] == islandId) {
                    islandNodes.append(n);

                    int deg = 0;
                    for (lemon::ListGraph::IncEdgeIt e(g, n); e != lemon::INVALID; ++e) deg++;
                    if (deg % 2 != 0) oddNodes.append(n);
                }
            }

            // --- LE MATCHING DE BLOSSOM ---
            if (!oddNodes.isEmpty()) {
                lemon::SmartGraph mGraph;
                lemon::SmartGraph::EdgeMap<double> weights(mGraph);

                QHash<int, int> origToM;
                QHash<int, int> mToOrig;

                for (auto n : oddNodes) {
                    auto mn = mGraph.addNode();
                    origToM[g.id(n)] = mGraph.id(mn);
                    mToOrig[mGraph.id(mn)] = g.id(n);
                }

                const double MAX_W = 10000000.0;
                for (int i = 0; i < oddNodes.size(); ++i) {
                    for (int j = i + 1; j < oddNodes.size(); ++j) {
                        QPoint p1 = nodeIdToQpt[g.id(oddNodes[i])];
                        QPoint p2 = nodeIdToQpt[g.id(oddNodes[j])];
                        double dist = std::hypot(p1.x() - p2.x(), p1.y() - p2.y());

                        auto n1 = mGraph.nodeFromId(origToM[g.id(oddNodes[i])]);
                        auto n2 = mGraph.nodeFromId(origToM[g.id(oddNodes[j])]);
                        auto edge = mGraph.addEdge(n1, n2);
                        weights[edge] = MAX_W - dist;
                    }
                }

                lemon::MaxWeightedPerfectMatching<lemon::SmartGraph, lemon::SmartGraph::EdgeMap<double>> mwpm(mGraph, weights);
                mwpm.run();

                QSet<int> matched;
                for (auto it = mToOrig.begin(); it != mToOrig.end(); ++it) {
                    int idMn = it.key();
                    if (matched.contains(idMn)) continue;

                    lemon::SmartGraph::Node mn = mGraph.nodeFromId(idMn);
                    auto mate = mwpm.mate(mn);

                    if (mate != lemon::INVALID) {
                        int idMate = mGraph.id(mate);
                        matched.insert(idMn);
                        matched.insert(idMate);

                        auto orig1 = g.nodeFromId(mToOrig[idMn]);
                        auto orig2 = g.nodeFromId(mToOrig[idMate]);
                        auto newVirtualEdge = g.addEdge(orig1, orig2);
                        isVirtual[newVirtualEdge] = true;
                    }
                }
            }

            // --- GÉNÉRATION DU CIRCUIT EULÉRIEN ---
            lemon::ListGraph::Node startNode = islandNodes.first();
            double minDist = std::numeric_limits<double>::max();

            for (auto n : islandNodes) {
                QPoint p = nodeIdToQpt[g.id(n)];
                double dist = std::hypot(p.x() * INV_SCALE - currentMachinePos.x(), p.y() * INV_SCALE - currentMachinePos.y());
                if (dist < minDist) {
                    minDist = dist;
                    startNode = n;
                }
            }

            ContinuousCut chain;
            chain.depth = d;
            lemon::ListGraph::Node currNode = startNode;
            QPoint startP = nodeIdToQpt[g.id(currNode)];
            chain.points.append(QPointF(startP.x() * INV_SCALE, startP.y() * INV_SCALE));

            for (lemon::EulerIt<lemon::ListGraph> e(g, startNode); e != lemon::INVALID; ++e) {
                lemon::ListGraph::Node u = g.u(e);
                lemon::ListGraph::Node v = g.v(e);
                lemon::ListGraph::Node nextNode = (u == currNode) ? v : u;
                QPoint nextP = nodeIdToQpt[g.id(nextNode)];

                if (isVirtual[e]) {
                    if (chain.points.size() > 1) {
                        chain.isClosed = (QLineF(chain.points.first(), chain.points.last()).length() < 0.1);
                        allSubPaths.append(chain);
                    }
                    chain.points.clear();
                    chain.points.append(QPointF(nextP.x() * INV_SCALE, nextP.y() * INV_SCALE));
                } else {
                    chain.points.append(QPointF(nextP.x() * INV_SCALE, nextP.y() * INV_SCALE));
                }
                currNode = nextNode;
            }

            if (chain.points.size() > 1) {
                chain.isClosed = (QLineF(chain.points.first(), chain.points.last()).length() < 0.1);
                allSubPaths.append(chain);
            }
            currentMachinePos = QPointF(nodeIdToQpt[g.id(currNode)].x() * INV_SCALE, nodeIdToQpt[g.id(currNode)].y() * INV_SCALE);
        }
    }

    // 4. ASSEMBLAGE GLOBAL (Nearest Neighbor)
    QList<ContinuousCut> finalOptimized;
    QPointF machinePos = startPos;

    while (!allSubPaths.isEmpty()) {
        int bestIdx = -1;
        bool reverseDir = false;
        double minDist = std::numeric_limits<double>::max();

        for (int i = 0; i < allSubPaths.size(); ++i) {
            double dStart = QLineF(machinePos, allSubPaths[i].points.first()).length();
            double dEnd = QLineF(machinePos, allSubPaths[i].points.last()).length();

            if (dStart < minDist) {
                minDist = dStart; bestIdx = i; reverseDir = false;
            }
            if (!allSubPaths[i].isClosed && dEnd < minDist) {
                minDist = dEnd; bestIdx = i; reverseDir = true;
            }
        }

        ContinuousCut nextCut = allSubPaths.takeAt(bestIdx);
        if (reverseDir) std::reverse(nextCut.points.begin(), nextCut.points.end());

        // Nettoyage colinéaire final pour adoucir le tracé de la buse
        if (nextCut.points.size() > 2) {
            QPolygonF simplified;
            simplified.append(nextCut.points.first());
            for (int i = 1; i < nextCut.points.size() - 1; ++i) {
                QPointF pPrev = simplified.last();
                QPointF pCurr = nextCut.points[i];
                QPointF pNext = nextCut.points[i+1];
                double cross = (pCurr.x() - pPrev.x()) * (pNext.y() - pCurr.y()) -
                               (pCurr.y() - pPrev.y()) * (pNext.x() - pCurr.x());

                // Fusion des points alignés pour éviter à la buse de freiner pour rien
                if (std::abs(cross) > 1e-2) simplified.append(pCurr);
            }
            simplified.append(nextCut.points.last());
            nextCut.points = simplified;
        }

        finalOptimized.append(nextCut);
        machinePos = nextCut.points.last();
    }

    return finalOptimized;
}

// ============================================================================
// EXTRACTION DES TRACÉS DEPUIS L'UI
// ============================================================================
QList<ContinuousCut> PathPlanner::extractRawPaths(QGraphicsScene *sc)
{
    QList<ContinuousCut> out;
    for (QGraphicsItem *it : sc->items()) {
        if (!it->isVisible() || it->zValue() >= 50) continue;
        // Les calques negatifs sont des aides visuelles (fond, cadre machine,
        // zone de decoupe) et ne doivent jamais devenir des trajectoires.
        if (it->zValue() < 0.0) continue;
        if (it->boundingRect().width() < 1.0 && it->boundingRect().height() < 1.0) continue;

        QPainterPath path;
        if (auto *r = qgraphicsitem_cast<QGraphicsRectItem *>(it)) path.addRect(r->rect());
        else if (auto *p = qgraphicsitem_cast<QGraphicsPathItem *>(it)) path = p->path();
        else if (auto *e = qgraphicsitem_cast<QGraphicsEllipseItem *>(it)) path.addEllipse(e->rect());
        else if (auto *poly = qgraphicsitem_cast<QGraphicsPolygonItem *>(it)) path.addPolygon(poly->polygon());
        else continue;

        path = it->mapToScene(path);
        for (const QPolygonF& poly : path.toSubpathPolygons()) {
            if (poly.size() < 2) continue;
            ContinuousCut cut;
            cut.points = poly;
            cut.isClosed = (QLineF(poly.first(), poly.last()).length() < 0.1);
            cut.depth = 0;
            out.append(cut);
        }
    }
    return out;
}

// ============================================================================
// CALCUL DES PROFONDEURS (INSIDE-OUT)
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
// ENTRÉES DE COUPE (LEAD-INS)
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
