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

#include "clipper2/clipper.h"
#include "clipper2/clipper.minkowski.h"

using namespace Clipper2Lib;

namespace {

const double CLIPPER_SCALE         = 1000.0;
const double DECIMATION_TOLERANCE  = 0.5;    // Précision max pour éviter les chevauchements
const double DECIMATION_MARGIN     = 1.5;
const double ZONE_SIMPLIFY_EPSILON = 0.5 * CLIPPER_SCALE;
const int    MAX_POLY_POINTS       = 250;    // Garde les courbes fluides

inline uint64_t dictKey(int a, int b) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(a)) << 32) |
           static_cast<uint64_t>(static_cast<uint32_t>(b));
}

Path64 decimatePath(const Path64 &src, int maxPoints) {
    if (static_cast<int>(src.size()) <= maxPoints) return src;
    Path64 out;
    out.reserve(static_cast<size_t>(maxPoints));
    double step = static_cast<double>(src.size()) / maxPoints;
    for (int i = 0; i < maxPoints; ++i)
        out.push_back(src[static_cast<size_t>(std::round(i * step)) % src.size()]);
    return out;
}

Paths64 extractAllPaths(const QPainterPath &path, double tolerance) {
    Paths64 res;
    QPointF lastPt;
    double toleranceSq = tolerance * tolerance;

    for (const QPolygonF &poly : path.simplified().toSubpathPolygons()) {
        if (poly.boundingRect().width() * poly.boundingRect().height() < 1.0)
            continue;
        Path64 p64;
        bool first = true;
        for (const QPointF &pt : poly) {
            if (first) {
                p64.push_back(Point64(pt.x()*CLIPPER_SCALE, pt.y()*CLIPPER_SCALE));
                lastPt = pt; first = false;
            } else {
                double d = (pt.x()-lastPt.x())*(pt.x()-lastPt.x()) +
                           (pt.y()-lastPt.y())*(pt.y()-lastPt.y());
                if (d > toleranceSq) {
                    p64.push_back(Point64(pt.x()*CLIPPER_SCALE, pt.y()*CLIPPER_SCALE));
                    lastPt = pt;
                }
            }
        }
        if (p64.size() >= 3) {
            p64 = decimatePath(p64, MAX_POLY_POINTS);
            res.push_back(p64);
        }
    }
    return res;
}

Paths64 translateClipper(const Paths64 &paths, double dx, double dy) {
    int64_t idx = static_cast<int64_t>(dx * CLIPPER_SCALE);
    int64_t idy = static_cast<int64_t>(dy * CLIPPER_SCALE);
    Paths64 res; res.reserve(paths.size());
    for (const auto &path : paths) {
        Path64 p; p.reserve(path.size());
        for (const auto &pt : path) p.push_back(Point64(pt.x+idx, pt.y+idy));
        res.push_back(p);
    }
    return res;
}

Path64 invertShape(const Path64 &shape) {
    Path64 inv; inv.reserve(shape.size());
    for (int i = static_cast<int>(shape.size())-1; i >= 0; --i)
        inv.push_back(Point64(-shape[i].x, -shape[i].y));
    return inv;
}

Paths64 calculateNFP(const Paths64 &stationary, const Paths64 &orbiting, double margin) {
    Paths64 statSimp = SimplifyPaths(stationary, ZONE_SIMPLIFY_EPSILON / 2.0);
    Paths64 orbSimp  = SimplifyPaths(orbiting,   ZONE_SIMPLIFY_EPSILON / 2.0);
    for (auto &p : statSimp) p = decimatePath(p, MAX_POLY_POINTS);
    for (auto &p : orbSimp)  p = decimatePath(p, MAX_POLY_POINTS);

    Paths64 totalNfp;
    for (const Path64 &statPath : statSimp) {
        for (const Path64 &orbPath : orbSimp) {
            Path64  invOrb = invertShape(orbPath);
            Paths64 nfp    = MinkowskiSum(invOrb, statPath, true);
            totalNfp = Union(totalNfp, nfp, FillRule::NonZero);
        }
    }
    if (margin > 0.0)
        totalNfp = InflatePaths(totalNfp, margin * CLIPPER_SCALE, JoinType::Round, EndType::Polygon);

    if (!totalNfp.empty())
        totalNfp = SimplifyPaths(totalNfp, ZONE_SIMPLIFY_EPSILON);

    return totalNfp;
}

Paths64 mirrorNFP(const Paths64 &nfpAB) {
    Paths64 res; res.reserve(nfpAB.size());
    for (const Path64 &path : nfpAB) {
        Path64 p; p.reserve(path.size());
        for (const auto &pt : path) p.push_back(Point64(-pt.x, -pt.y));
        std::reverse(p.begin(), p.end());
        res.push_back(p);
    }
    return res;
}

struct OrientedShape {
    int id;
    int pieceCount;
    QList<QPainterPath> originalPaths;
    Paths64 clipperPaths;
    QRectF bounds;
};

struct CacheData {
    QByteArray shapeHash;
    QList<int> angles;
    int spacing;
    QList<OrientedShape> singleShapes;
    QList<OrientedShape> superShapes;
    QList<OrientedShape> allShapes;
    std::unordered_map<uint64_t, Paths64> dictionary;
};

CacheData globalCache;

QByteArray hashPath(const QPainterPath &path) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &el = path.elementAt(i);
        stream << el.x << el.y << el.type;
    }
    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
}

OrientedShape createOptimalSuperBloc(const QPainterPath &base, const QList<int> &angles, int spacing) {
    Paths64 hdClipA = extractAllPaths(base, 0.5);
    QRectF  boundsA = base.boundingRect();

    auto processAngle = [&](int angle) -> QPair<double, QPainterPath> {
        QTransform rot; rot.rotate(angle);
        QPainterPath pathB   = rot.map(base);
        Paths64      hdClipB = extractAllPaths(pathB, 0.5);
        QRectF       boundsB = pathB.boundingRect();
        Paths64      nfp     = calculateNFP(hdClipA, hdClipB, static_cast<double>(spacing));

        double       bestArea  = std::numeric_limits<double>::max();
        QPainterPath bestPathB;

        for (const Path64 &poly : nfp) {
            for (const Point64 &pt : poly) {
                double dx = static_cast<double>(pt.x) / CLIPPER_SCALE;
                double dy = static_cast<double>(pt.y) / CLIPPER_SCALE;
                QRectF cb   = boundsA.united(boundsB.translated(dx, dy));
                double area = cb.width() * cb.height();
                if (area < bestArea) {
                    bestArea = area;
                    QTransform t; t.translate(dx, dy);
                    bestPathB = t.map(pathB);
                }
            }
        }
        return {bestArea, bestPathB};
    };

    QList<QPair<double, QPainterPath>> results;
    if (angles.size() > 1)
        results = QtConcurrent::blockingMapped(angles, processAngle);
    else
        results << processAngle(angles.first());

    double globalBestArea = std::numeric_limits<double>::max();
    QPainterPath globalBestPathB;
    for (const auto &r : results) {
        if (r.first < globalBestArea) {
            globalBestArea = r.first; globalBestPathB = r.second;
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

} // namespace

// ============================================================================
// RUN FUNCTION
// ============================================================================
PlacementResult PlacementOptimizer::run(
    const PlacementParams &params,
    const std::function<bool(int, int)> &onProgress)
{
    PlacementResult result;
    if (params.shapeCount <= 0 || params.prototypePath.isEmpty()) return result;

    const double W = params.containerRect.width();
    const double H = params.containerRect.height();
    QList<int> angles = params.angles.isEmpty() ? QList<int>{0} : params.angles;

    // ── PHASE 1 : CACHE ──
    QByteArray currentHash = hashPath(params.prototypePath);
    bool cacheValid = (globalCache.shapeHash == currentHash &&
                       globalCache.angles    == angles      &&
                       globalCache.spacing   == params.spacing);

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

        for (int angle : angles) {
            QTransform rot; rot.rotate(angle);
            QPainterPath path = rot.map(params.prototypePath);
            OrientedShape shape = {
                nextId++, 1, {path},
                extractAllPaths(path, DECIMATION_TOLERANCE),
                path.boundingRect()
            };
            globalCache.singleShapes << shape;
            globalCache.allShapes    << shape;
        }

        OrientedShape baseSuperBloc = createOptimalSuperBloc(params.prototypePath, angles, params.spacing);
        for (int angle : angles) {
            QTransform rot; rot.rotate(angle);
            OrientedShape shape;
            shape.id         = nextId++;
            shape.pieceCount = 2;
            for (const QPainterPath &p : baseSuperBloc.originalPaths)
                shape.originalPaths << rot.map(p);

            QPainterPath combined;
            for (const auto &p : shape.originalPaths) combined.addPath(p);

            shape.clipperPaths = extractAllPaths(combined, DECIMATION_TOLERANCE);
            shape.bounds       = combined.boundingRect();

            globalCache.superShapes << shape;
            globalCache.allShapes   << shape;
        }

        struct NfpJob    { int idA, idB; Paths64 pathsA, pathsB; double margin; };
        struct NfpResult { int idA, idB; Paths64 nfp; };

        QList<NfpJob> jobs;
        const auto &shapes = globalCache.allShapes;
        for (int i = 0; i < shapes.size(); ++i) {
            for (int j = i; j < shapes.size(); ++j) {
                jobs.append({shapes[i].id, shapes[j].id,
                             shapes[i].clipperPaths, shapes[j].clipperPaths,
                             params.spacing + DECIMATION_MARGIN});
            }
        }

        auto computeNfp = [](const NfpJob &job) -> NfpResult {
            return {job.idA, job.idB, calculateNFP(job.pathsA, job.pathsB, job.margin)};
        };

        int threadCount = qMin(QThreadPool::globalInstance()->maxThreadCount(), 4);
        QThreadPool::globalInstance()->setMaxThreadCount(threadCount);

        QList<NfpResult> nfpResults = QtConcurrent::blockingMapped(jobs, std::function<NfpResult(const NfpJob&)>(computeNfp));

        QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

        for (const NfpResult &res : nfpResults) {
            globalCache.dictionary[dictKey(res.idA, res.idB)] = res.nfp;
            if (res.idA != res.idB)
                globalCache.dictionary[dictKey(res.idB, res.idA)] = mirrorNFP(res.nfp);
        }
    }

    // ── PHASE 2 : ZONES IFP ──
    std::unordered_map<int, Paths64> allowedZones;

    for (const OrientedShape &shape : globalCache.allShapes) {
        double minX = -shape.bounds.left()      + params.spacing;
        double minY = -shape.bounds.top()       + params.spacing;
        double maxX =  W - shape.bounds.right() - params.spacing;
        double maxY =  H - shape.bounds.bottom()- params.spacing;

        if (maxX >= minX && maxY >= minY) {
            Path64 ifpRect;
            ifpRect.push_back(Point64(minX*CLIPPER_SCALE, minY*CLIPPER_SCALE));
            ifpRect.push_back(Point64(maxX*CLIPPER_SCALE, minY*CLIPPER_SCALE));
            ifpRect.push_back(Point64(maxX*CLIPPER_SCALE, maxY*CLIPPER_SCALE));
            ifpRect.push_back(Point64(minX*CLIPPER_SCALE, maxY*CLIPPER_SCALE));
            allowedZones[shape.id] = {ifpRect};
        } else {
            allowedZones[shape.id] = Paths64();
        }
    }

    // ── PHASE 3 : PLACEMENT GRAVITY FILL (Minimisation de l'encombrement) ──
    int piecesPlaced = 0;

    // NOUVEAU : On garde en mémoire l'encombrement global de toutes les pièces posées
    QRectF currentPlacedBounds;

    while (piecesPlaced < params.shapeCount) {
        if (onProgress && !onProgress(piecesPlaced, params.shapeCount)) {
            result.cancelled = true;
            break;
        }

        bool          found     = false;
        double        bestX = 0, bestY = 0;
        double        bestScore = std::numeric_limits<double>::max();
        int           bestId    = -1;
        OrientedShape winner;

        auto tryShapes = [&](const QList<OrientedShape> &shapes) {
            for (const OrientedShape &shape : shapes) {
                const Paths64 &zone = allowedZones[shape.id];
                if (zone.empty()) continue;

                for (const Path64 &path : zone) {
                    for (const Point64 &pt : path) {
                        double px = static_cast<double>(pt.x) / CLIPPER_SCALE;
                        double py = static_cast<double>(pt.y) / CLIPPER_SCALE;

                        // ── LA MAGIE DU GRAVITY FILL EST ICI ──

                        // 1. On simule la boîte englobante de la pièce à cette position
                        QRectF candidateRect = shape.bounds.translated(px, py);

                        // 2. On fusionne avec la boîte englobante globale actuelle
                        QRectF simulatedBounds = currentPlacedBounds.isValid() ?
                                                     currentPlacedBounds.united(candidateRect) :
                                                     candidateRect;

                        // 3. Le score principal est la surface totale de l'encombrement (on veut la minimiser)
                        double areaScore = simulatedBounds.width() * simulatedBounds.height();

                        // 4. Critère secondaire (Tie-Breaker) : si la surface est identique (ex: on bouche un trou),
                        // on pousse vers le bas à gauche.
                        double score = areaScore + (py * 0.1) + (px * 0.01);

                        if (score < bestScore) {
                            bestScore = score;
                            bestX = px;
                            bestY = py;
                            bestId = shape.id;
                            found = true;
                            winner = shape;
                        }
                    }
                }
            }
        };

        if (params.shapeCount - piecesPlaced >= 2)
            tryShapes(globalCache.superShapes);

        if (!found)
            tryShapes(globalCache.singleShapes);

        if (!found) break;

        for (const QPainterPath &p : winner.originalPaths)
            result.placedPaths << p.translated(bestX, bestY);

        // NOUVEAU : On met à jour l'encombrement global avec la pièce gagnante
        QRectF placedRect = winner.bounds.translated(bestX, bestY);
        currentPlacedBounds = currentPlacedBounds.isValid() ?
                                  currentPlacedBounds.united(placedRect) :
                                  placedRect;

        for (const OrientedShape &shape : globalCache.allShapes) {
            auto it = globalCache.dictionary.find(dictKey(bestId, shape.id));
            if (it == globalCache.dictionary.end()) continue;

            Paths64  tNfp = translateClipper(it->second, bestX, bestY);
            Paths64 &zone = allowedZones[shape.id];

            if (!zone.empty()) {
                zone = Difference(zone, tNfp, FillRule::NonZero);
                if (zone.size() > 4 || (!zone.empty() && zone[0].size() > 20)) {
                    zone = SimplifyPaths(zone, ZONE_SIMPLIFY_EPSILON);
                }
            }
        }

        piecesPlaced += winner.pieceCount;
    }

    result.processedPositions = piecesPlaced;
    result.totalPositions     = params.shapeCount;
    return result;
}
