#include "PlacementOptimizer.h"
#include <QTransform>
#include <QPair>
#include <limits>
#include <QtGlobal>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
#include <unordered_map>
#include <algorithm>

// --- INCLUSION DE CLIPPER2 ---
#include "clipper2/clipper.h"
#include "clipper2/clipper.minkowski.h"

using namespace Clipper2Lib;

namespace {

const double CLIPPER_SCALE = 1000.0;

// --- PARAMÈTRES DE PERFORMANCE ---
const double DECIMATION_TOLERANCE = 5.0;
const double DECIMATION_MARGIN    = 1.5;
// Epsilon faible : ne pas perdre de positions valides
const double ZONE_SIMPLIFY_EPSILON = 0.1 * CLIPPER_SCALE;
// Fréquence de purge des candidats devenus invalides
const int PURGE_EVERY_N_PIECES = 10;

// --- CLÉ DE DICTIONNAIRE COMPACTE ---
// Remplace QMap<QPair<int,int>> (O(log N)) par unordered_map (O(1) amorti)
inline uint64_t dictKey(int a, int b) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(a)) << 32) |
            static_cast<uint64_t>(static_cast<uint32_t>(b));
}

// 1. EXTRACTION ROBUSTE AVEC PRÉCISION VARIABLE
Paths64 extractAllPaths(const QPainterPath &path, double tolerance) {
    Paths64 res;
    QPointF lastPt;
    double toleranceSq = tolerance * tolerance;

    for (const QPolygonF &poly : path.simplified().toSubpathPolygons()) {
        if (poly.boundingRect().width() * poly.boundingRect().height() < 1.0) continue;

        Path64 p64;
        bool first = true;
        for (const QPointF &pt : poly) {
            if (first) {
                p64.push_back(Point64(pt.x() * CLIPPER_SCALE, pt.y() * CLIPPER_SCALE));
                lastPt = pt; first = false;
            } else {
                double distSq = (pt.x() - lastPt.x()) * (pt.x() - lastPt.x()) +
                                (pt.y() - lastPt.y()) * (pt.y() - lastPt.y());
                if (distSq > toleranceSq) {
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
    int64_t idx = static_cast<int64_t>(dx * CLIPPER_SCALE);
    int64_t idy = static_cast<int64_t>(dy * CLIPPER_SCALE);
    Paths64 res;
    res.reserve(paths.size());
    for (const auto &path : paths) {
        Path64 p; p.reserve(path.size());
        for (const auto &pt : path) p.push_back(Point64(pt.x + idx, pt.y + idy));
        res.push_back(p);
    }
    return res;
}

Path64 invertShape(const Path64& shape) {
    Path64 inverted; inverted.reserve(shape.size());
    for (int i = static_cast<int>(shape.size()) - 1; i >= 0; --i)
        inverted.push_back(Point64(-shape[i].x, -shape[i].y));
    return inverted;
}

// 2. NFP AVEC MARGE DYNAMIQUE
Paths64 calculateNFP(const Paths64& stationary, const Paths64& orbiting, double margin) {
    Paths64 totalNfp;
    for (const Path64& statPath : stationary) {
        for (const Path64& orbPath : orbiting) {
            Path64 invOrb = invertShape(orbPath);
            Paths64 nfp = MinkowskiSum(invOrb, statPath, true);
            totalNfp = Union(totalNfp, nfp, FillRule::NonZero);
        }
    }
    if (margin > 0.0)
        totalNfp = InflatePaths(totalNfp, margin * CLIPPER_SCALE, JoinType::Round, EndType::Polygon);
    return totalNfp;
}

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

struct CacheData {
    QByteArray shapeHash;
    QList<int> angles;
    int spacing;
    QList<OrientedShape> singleShapes;
    QList<OrientedShape> superShapes;
    QList<OrientedShape> allShapes;
    // O(1) amorti vs O(log N) pour QMap
    std::unordered_map<uint64_t, Paths64> dictionary;
};

CacheData globalCache;

QByteArray hashPath(const QPainterPath& path) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element& el = path.elementAt(i);
        stream << el.x << el.y << el.type;
    }
    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
}

// 3. SUPER-BLOC (calcul parallèle par angle)
OrientedShape createOptimalSuperBloc(const QPainterPath& base, const QList<int>& angles, int spacing) {
    Paths64 hdClipA = extractAllPaths(base, 0.5);
    QRectF boundsA = base.boundingRect();

    auto processAngle = [&](int angle) -> QPair<double, QPainterPath> {
        QTransform rot; rot.rotate(angle);
        QPainterPath pathB = rot.map(base);
        Paths64 hdClipB = extractAllPaths(pathB, 0.5);
        QRectF boundsB = pathB.boundingRect();
        Paths64 nfp = calculateNFP(hdClipA, hdClipB, static_cast<double>(spacing));

        double bestArea = std::numeric_limits<double>::max();
        QPainterPath bestPathB;
        for (const Path64& poly : nfp) {
            for (const Point64& pt : poly) {
                double dx = static_cast<double>(pt.x) / CLIPPER_SCALE;
                double dy = static_cast<double>(pt.y) / CLIPPER_SCALE;
                QRectF combinedBounds = boundsA.united(boundsB.translated(dx, dy));
                double area = combinedBounds.width() * combinedBounds.height();
                if (area < bestArea) {
                    bestArea = area;
                    QTransform t; t.translate(dx, dy);
                    bestPathB = t.map(pathB);
                }
            }
        }
        return {bestArea, bestPathB};
    };

    QList<QPair<double, QPainterPath>> results = QtConcurrent::blockingMapped(angles, processAngle);

    double globalBestArea = std::numeric_limits<double>::max();
    QPainterPath globalBestPathB;
    for (const auto& r : results) {
        if (r.first < globalBestArea) {
            globalBestArea = r.first;
            globalBestPathB = r.second;
        }
    }

    OrientedShape superBloc;
    superBloc.pieceCount = 2;
    QRectF totalBounds = boundsA.united(globalBestPathB.boundingRect());
    QTransform align; align.translate(-totalBounds.x(), -totalBounds.y());
    superBloc.originalPaths << align.map(base) << align.map(globalBestPathB);
    superBloc.bounds = QRectF(0, 0, totalBounds.width(), totalBounds.height());
    return superBloc;
}

// 4. FILTRE DE ZONE : bbox pré-calculée (rejet rapide) + règle NonZero (trous)
// La Rect64 évite ~70-80% des appels PointInPolygon inutiles.
bool isInsideZone(const Point64& pt, const Rect64& bbox, const Paths64& zone) {
    if (bbox.IsEmpty()) return false;
    if (pt.x < bbox.left || pt.x > bbox.right || pt.y < bbox.top || pt.y > bbox.bottom)
        return false;
    int winding = 0;
    for (const Path64& path : zone) {
        auto res = PointInPolygon(pt, path);
        if (res == PointInPolygonResult::IsOn) return true;
        if (res == PointInPolygonResult::IsInside)
            winding += IsPositive(path) ? 1 : -1;
    }
    return winding != 0;
}

} // namespace


PlacementResult PlacementOptimizer::run(const PlacementParams &params,
                                        const std::function<bool(int current, int total)> &onProgress)
{
    PlacementResult result;
    if (params.shapeCount <= 0 || params.prototypePath.isEmpty()) return result;

    const double W = params.containerRect.width();
    const double H = params.containerRect.height();
    QList<int> angles = params.angles.isEmpty() ? QList<int>{0} : params.angles;

    // --- PHASE 1 & 2 : VÉRIFICATION DU CACHE ET CALCUL MULTITHREAD ---
    QByteArray currentHash = hashPath(params.prototypePath);
    bool cacheValid = (globalCache.shapeHash == currentHash &&
                       globalCache.angles == angles &&
                       globalCache.spacing == params.spacing);

    if (!cacheValid) {
        if (onProgress) onProgress(0, params.shapeCount);
        QCoreApplication::processEvents();

        globalCache.shapeHash = currentHash;
        globalCache.angles    = angles;
        globalCache.spacing   = params.spacing;
        globalCache.singleShapes.clear();
        globalCache.superShapes.clear();
        globalCache.allShapes.clear();
        globalCache.dictionary.clear();

        int nextId = 0;

        // A. Formes simples (low-poly pour vitesse)
        for (int angle : angles) {
            QTransform rot; rot.rotate(angle);
            QPainterPath path = rot.map(params.prototypePath);
            OrientedShape shape = {nextId++, 1, {path}, extractAllPaths(path, DECIMATION_TOLERANCE), path.boundingRect()};
            globalCache.singleShapes << shape;
            globalCache.allShapes    << shape;
        }

        // B. Super-blocs de 2 pièces
        OrientedShape baseSuperBloc = createOptimalSuperBloc(params.prototypePath, angles, params.spacing);
        for (int angle : angles) {
            QTransform rot; rot.rotate(angle);
            OrientedShape shape;
            shape.id = nextId++;
            shape.pieceCount = 2;
            for (const QPainterPath& p : baseSuperBloc.originalPaths) shape.originalPaths << rot.map(p);
            QPainterPath combined;
            for (const auto& p : shape.originalPaths) combined.addPath(p);
            shape.clipperPaths = extractAllPaths(combined, DECIMATION_TOLERANCE);
            shape.bounds = combined.boundingRect();
            globalCache.superShapes << shape;
            globalCache.allShapes   << shape;
        }

        // C. Calcul multithread du dictionnaire NFP → unordered_map O(1)
        struct NfpJob    { int idA; int idB; Paths64 pathsA; Paths64 pathsB; double margin; };
        struct NfpResult { int idA; int idB; Paths64 nfp; };

        QList<NfpJob> jobs;
        for (const OrientedShape& shapeA : globalCache.allShapes)
            for (const OrientedShape& shapeB : globalCache.allShapes)
                jobs.append({shapeA.id, shapeB.id, shapeA.clipperPaths, shapeB.clipperPaths,
                              params.spacing + DECIMATION_MARGIN});

        auto computeNfpTask = [](const NfpJob& job) -> NfpResult {
            return {job.idA, job.idB, calculateNFP(job.pathsA, job.pathsB, job.margin)};
        };

        auto reduceNfpTask = [](std::unordered_map<uint64_t, Paths64>& dict, const NfpResult& res) {
            dict[dictKey(res.idA, res.idB)] = res.nfp;
        };

        globalCache.dictionary =
            QtConcurrent::blockingMappedReduced<std::unordered_map<uint64_t, Paths64>>(
                jobs, computeNfpTask, reduceNfpTask);
    }

    // --- PHASE 3 : PLACEMENT PAR NFP-VERTEX SEARCH INCRÉMENTAL ---
    //
    // Optimisations vs version précédente :
    //  - persistentCandidates : liste incrémentale, on n'ajoute que les nouveaux sommets NFP
    //    de la pièce qui vient d'être placée (O(N) total au lieu de O(N²))
    //  - zoneBboxes : bounding box pré-calculée → rejet rapide dans isInsideZone
    //  - purge périodique : toutes les PURGE_EVERY_N_PIECES pièces, supprime les
    //    candidats invalides pour éviter l'accumulation mémoire
    //  - une seule passe translateClipper par pièce placée (zone + candidats partagés)

    std::unordered_map<int, Paths64>             allowedZones;
    std::unordered_map<int, Rect64>              zoneBboxes;
    std::unordered_map<int, std::vector<Point64>> persistentCandidates;

    for (const OrientedShape& shape : globalCache.allShapes) {
        double minX = -shape.bounds.left()   + params.spacing;
        double minY = -shape.bounds.top()    + params.spacing;
        double maxX =  W - shape.bounds.right()  - params.spacing;
        double maxY =  H - shape.bounds.bottom() - params.spacing;

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

        zoneBboxes[shape.id] = GetBounds(allowedZones[shape.id]);

        // Candidats initiaux : coins de l'IFP
        auto& cands = persistentCandidates[shape.id];
        for (const Path64& p : allowedZones[shape.id])
            cands.insert(cands.end(), p.begin(), p.end());
    }

    QList<PlacedShape> placedShapes;
    int piecesPlaced = 0;

    while (piecesPlaced < params.shapeCount) {
        if (onProgress && !onProgress(piecesPlaced, params.shapeCount)) {
            result.cancelled = true; break;
        }

        bool piecePlacedThisTurn = false;
        double bestX = 0, bestY = 0;
        int bestId  = -1;
        double bestScore = std::numeric_limits<double>::max();
        OrientedShape winnerShape;
        int remaining = params.shapeCount - piecesPlaced;

        // Évalue les formes via les candidats persistants (NFP-vertex search)
        auto tryShapes = [&](const QList<OrientedShape>& shapes) {
            for (const OrientedShape& shape : shapes) {
                const Paths64& zone = allowedZones[shape.id];
                if (zone.empty()) continue;
                const Rect64& bbox                       = zoneBboxes[shape.id];
                const std::vector<Point64>& candidates   = persistentCandidates[shape.id];

                for (const Point64& pt : candidates) {
                    if (!isInsideZone(pt, bbox, zone)) continue;
                    double px    = static_cast<double>(pt.x) / CLIPPER_SCALE;
                    double py    = static_cast<double>(pt.y) / CLIPPER_SCALE;
                    double score = py * W + px;
                    if (score < bestScore) {
                        bestScore = score; bestX = px; bestY = py; bestId = shape.id;
                        piecePlacedThisTurn = true;
                        winnerShape = shape;
                    }
                }
            }
        };

        if (remaining >= 2) tryShapes(globalCache.superShapes);
        if (!piecePlacedThisTurn) tryShapes(globalCache.singleShapes);

        if (piecePlacedThisTurn) {
            placedShapes.push_back({bestId, bestX, bestY});
            for (const QPainterPath& p : winnerShape.originalPaths)
                result.placedPaths << p.translated(bestX, bestY);

            // Une seule passe : translateClipper est réutilisé pour zone ET candidats
            for (const OrientedShape& shape : globalCache.allShapes) {
                auto it = globalCache.dictionary.find(dictKey(bestId, shape.id));
                if (it == globalCache.dictionary.end()) continue;

                Paths64 translatedNfp = translateClipper(it->second, bestX, bestY);

                // Mise à jour zone + bbox
                Paths64& zone = allowedZones[shape.id];
                if (!zone.empty()) {
                    zone = Difference(zone, translatedNfp, FillRule::NonZero);
                    zone = SimplifyPaths(zone, ZONE_SIMPLIFY_EPSILON);
                    zoneBboxes[shape.id] = zone.empty() ? Rect64(0,0,0,0) : GetBounds(zone);
                }

                // Ajout incrémental des candidats NFP (positions tangentes à la pièce placée)
                auto& cands = persistentCandidates[shape.id];
                for (const Path64& p : translatedNfp)
                    cands.insert(cands.end(), p.begin(), p.end());
            }

            piecesPlaced += winnerShape.pieceCount;

            // Purge périodique : évite l'accumulation de candidats invalides
            if (piecesPlaced % PURGE_EVERY_N_PIECES == 0) {
                for (const OrientedShape& shape : globalCache.allShapes) {
                    auto& cands         = persistentCandidates[shape.id];
                    const Paths64& zone = allowedZones[shape.id];
                    const Rect64& bbox  = zoneBboxes[shape.id];
                    cands.erase(
                        std::remove_if(cands.begin(), cands.end(),
                            [&](const Point64& pt) { return !isInsideZone(pt, bbox, zone); }),
                        cands.end());
                }
            }
        } else {
            break;
        }
    }

    result.processedPositions = piecesPlaced;
    result.totalPositions     = params.shapeCount;
    return result;
}
