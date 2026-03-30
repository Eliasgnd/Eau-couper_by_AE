#include "PlacementOptimizer.h"
#include <QTransform>
#include <QMap>
#include <QPair>
#include <limits>
#include <QtGlobal>

// --- INCLUSION DE CLIPPER2 ---
#include "clipper2/clipper.h"
#include "clipper2/clipper.minkowski.h"

using namespace Clipper2Lib;

namespace {

const double CLIPPER_SCALE = 1000.0;

// 1. EXTRACTION ROBUSTE : Gère les formes avec des trous ou plusieurs morceaux
Paths64 extractAllPaths(const QPainterPath &path) {
    Paths64 res;
    QPointF lastPt;

    for (const QPolygonF &poly : path.simplified().toSubpathPolygons()) {
        if (poly.boundingRect().width() * poly.boundingRect().height() < 1.0) continue; // Ignore le bruit

        Path64 p64;
        bool first = true;
        for (const QPointF &pt : poly) {
            if (first) {
                p64.push_back(Point64(pt.x() * CLIPPER_SCALE, pt.y() * CLIPPER_SCALE));
                lastPt = pt; first = false;
            } else {
                double distSq = (pt.x() - lastPt.x()) * (pt.x() - lastPt.x()) +
                                (pt.y() - lastPt.y()) * (pt.y() - lastPt.y());
                if (distSq > 4.0) { // Filtre anti-freeze
                    p64.push_back(Point64(pt.x() * CLIPPER_SCALE, pt.y() * CLIPPER_SCALE));
                    lastPt = pt;
                }
            }
        }
        res.push_back(p64);
    }
    return res;
}

Paths64 translateClipper(const Paths64 &paths, double dx, double dy) {
    int64_t idx = dx * CLIPPER_SCALE;
    int64_t idy = dy * CLIPPER_SCALE;
    Paths64 res;
    res.reserve(paths.size());
    for (const auto &path : paths) {
        Path64 p;
        p.reserve(path.size());
        for (const auto &pt : path) {
            p.push_back(Point64(pt.x + idx, pt.y + idy));
        }
        res.push_back(p);
    }
    return res;
}

Path64 invertShape(const Path64& shape) {
    Path64 inverted;
    inverted.reserve(shape.size());
    for (const Point64& pt : shape) inverted.push_back(Point64(-pt.x, -pt.y));
    return inverted;
}

// 2. NFP MULTI-COMPOSANTS : Fait la somme de Minkowski de TOUTES les sous-parties
Paths64 calculateNFP(const Paths64& stationary, const Paths64& orbiting, int spacing) {
    Paths64 totalNfp;
    for (const Path64& statPath : stationary) {
        for (const Path64& orbPath : orbiting) {
            Path64 invOrb = invertShape(orbPath);
            Paths64 nfp = MinkowskiSum(invOrb, statPath, true);
            totalNfp = Union(totalNfp, nfp, FillRule::NonZero);
        }
    }
    if (spacing > 0) {
        totalNfp = InflatePaths(totalNfp, spacing * CLIPPER_SCALE, JoinType::Miter, EndType::Polygon);
    }
    return totalNfp;
}

// Structure unifiée pour une pièce simple OU un Super-Bloc
struct OrientedShape {
    int id;
    int pieceCount;
    QList<QPainterPath> originalPaths;
    Paths64 clipperPaths;
    QRectF bounds;
};

struct PlacedShape {
    int id;
    double x;
    double y;
};

// 3. LA FONCTION MAGIQUE : LE SUPER-BLOC
OrientedShape createOptimalSuperBloc(const QPainterPath& base, const QList<int>& angles, int spacing) {
    Paths64 clipA = extractAllPaths(base);
    QRectF boundsA = base.boundingRect();

    double bestArea = std::numeric_limits<double>::max();
    QPainterPath bestPathB;
    Paths64 bestClipB;
    double bestDx = 0, bestDy = 0;

    // On cherche quel angle s'emboîte le mieux avec la base (0°)
    for (int angle : angles) {
        QTransform rot; rot.rotate(angle);
        QPainterPath pathB = rot.map(base);
        Paths64 clipB = extractAllPaths(pathB);
        QRectF boundsB = pathB.boundingRect();

        Paths64 nfp = calculateNFP(clipA, clipB, spacing);

        // On teste TOUS les sommets du NFP (Emboîtements parfaits)
        for (const Path64& poly : nfp) {
            for (const Point64& pt : poly) {
                double dx = static_cast<double>(pt.x) / CLIPPER_SCALE;
                double dy = static_cast<double>(pt.y) / CLIPPER_SCALE;

                QRectF combinedBounds = boundsA.united(boundsB.translated(dx, dy));
                double area = combinedBounds.width() * combinedBounds.height();

                if (area < bestArea) {
                    bestArea = area;
                    bestDx = dx; bestDy = dy;
                    QTransform t; t.translate(dx, dy);
                    bestPathB = t.map(pathB);
                    bestClipB = translateClipper(clipB, dx, dy);
                }
            }
        }
    }

    OrientedShape superBloc;
    superBloc.pieceCount = 2;
    superBloc.clipperPaths = Union(clipA, bestClipB, FillRule::NonZero);

    QRectF totalBounds = boundsA.united(bestPathB.boundingRect());
    QTransform align; align.translate(-totalBounds.x(), -totalBounds.y());

    superBloc.originalPaths << align.map(base) << align.map(bestPathB);
    superBloc.clipperPaths = translateClipper(superBloc.clipperPaths, -totalBounds.x(), -totalBounds.y());
    superBloc.bounds = QRectF(0, 0, totalBounds.width(), totalBounds.height());

    return superBloc;
}

} // namespace anonyme


PlacementResult PlacementOptimizer::run(const PlacementParams &params,
                                        const std::function<bool(int current, int total)> &onProgress)
{
    PlacementResult result;
    if (params.shapeCount <= 0 || params.prototypePath.isEmpty()) return result;

    const double W = params.containerRect.width();
    const double H = params.containerRect.height();
    QList<int> angles = params.angles.isEmpty() ? QList<int>{0} : params.angles;

    int nextId = 0;
    QList<OrientedShape> singleShapes;
    QList<OrientedShape> superShapes;
    QList<OrientedShape> allShapes;

    // --- PHASE 1 : GÉNÉRATION DES SIMPLES ET DU SUPER-BLOC ---
    // 1.A Simples
    for (int angle : angles) {
        QTransform rot; rot.rotate(angle);
        QPainterPath path = rot.map(params.prototypePath);
        OrientedShape shape = {nextId++, 1, {path}, extractAllPaths(path), path.boundingRect()};
        singleShapes << shape;
        allShapes << shape;
    }

    // 1.B Super-Blocs
    OrientedShape baseSuperBloc = createOptimalSuperBloc(params.prototypePath, angles, params.spacing);
    for (int angle : angles) {
        QTransform rot; rot.rotate(angle);
        OrientedShape shape;
        shape.id = nextId++;
        shape.pieceCount = 2;
        for(const QPainterPath& p : baseSuperBloc.originalPaths) shape.originalPaths << rot.map(p);
        shape.clipperPaths = extractAllPaths(shape.originalPaths[0]); // Hack rapide, on extrait le tout
        QPainterPath combined; for(const auto& p: shape.originalPaths) combined.addPath(p);
        shape.clipperPaths = extractAllPaths(combined);
        shape.bounds = combined.boundingRect();
        superShapes << shape;
        allShapes << shape;
    }

    // --- PHASE 2 : DICTIONNAIRE NFP ABSOLU ---
    if (onProgress && !onProgress(0, params.shapeCount)) return result;

    QMap<QPair<int, int>, Paths64> nfpDictionary;
    for (const OrientedShape& shapeA : allShapes) {
        for (const OrientedShape& shapeB : allShapes) {
            nfpDictionary[qMakePair(shapeA.id, shapeB.id)] =
                calculateNFP(shapeA.clipperPaths, shapeB.clipperPaths, params.spacing);
        }
    }

    // --- PHASE 3 : PLACEMENT OPTIMISÉ ---
    QMap<int, Paths64> allowedZones;
    for (const OrientedShape& shape : allShapes) {
        double minX = -shape.bounds.left() + params.spacing;
        double minY = -shape.bounds.top() + params.spacing;
        double maxX = W - shape.bounds.right() - params.spacing;
        double maxY = H - shape.bounds.bottom() - params.spacing;

        if (maxX >= minX && maxY >= minY) {
            Path64 ifpRect;
            ifpRect.push_back(Point64(minX * CLIPPER_SCALE, minY * CLIPPER_SCALE));
            ifpRect.push_back(Point64(maxX * CLIPPER_SCALE, minY * CLIPPER_SCALE));
            ifpRect.push_back(Point64(maxX * CLIPPER_SCALE, maxY * CLIPPER_SCALE));
            ifpRect.push_back(Point64(minX * CLIPPER_SCALE, maxY * CLIPPER_SCALE));
            allowedZones[shape.id] = {ifpRect};
        } else {
            allowedZones[shape.id] = Paths64();
        }
    }

    QList<PlacedShape> placedShapes;
    int piecesPlaced = 0;

    while (piecesPlaced < params.shapeCount) {
        if (onProgress && !onProgress(piecesPlaced, params.shapeCount)) {
            result.cancelled = true; break;
        }

        bool piecePlacedThisTurn = false;
        double bestX = 0, bestY = 0;
        int bestId = -1;
        double bestScore = std::numeric_limits<double>::max();

        int remaining = params.shapeCount - piecesPlaced;
        const QList<OrientedShape>& currentList = (remaining >= 2) ? superShapes : singleShapes;

        for (const OrientedShape& shape : currentList) {
            for (const Path64& path : allowedZones[shape.id]) {
                for (const Point64& pt : path) {
                    double px = static_cast<double>(pt.x) / CLIPPER_SCALE;
                    double py = static_cast<double>(pt.y) / CLIPPER_SCALE;

                    double score = (py * 10000.0) + px;

                    if (score < bestScore) {
                        bestScore = score; bestX = px; bestY = py; bestId = shape.id;
                        piecePlacedThisTurn = true;
                    }
                }
            }
        }

        if (piecePlacedThisTurn) {
            // Retrouver la forme gagnante
            OrientedShape winner;
            for(const auto& s : currentList) if(s.id == bestId) winner = s;

            placedShapes.push_back({bestId, bestX, bestY});
            for (const QPainterPath& p : winner.originalPaths) {
                result.placedPaths << p.translated(bestX, bestY);
            }

            // SOUSTRACTION INCRÉMENTALE : Espace dispo = Espace dispo - NFP
            for (const OrientedShape& shape : allShapes) {
                if (allowedZones[shape.id].empty()) continue;
                Paths64 nfp = nfpDictionary[qMakePair(bestId, shape.id)];
                Paths64 translatedNfp = translateClipper(nfp, bestX, bestY);
                allowedZones[shape.id] = Difference(allowedZones[shape.id], translatedNfp, FillRule::NonZero);
            }

            piecesPlaced += winner.pieceCount;
        } else {
            break; // Conteneur plein
        }
    }

    result.processedPositions = piecesPlaced;
    result.totalPositions = params.shapeCount;
    return result;
}
