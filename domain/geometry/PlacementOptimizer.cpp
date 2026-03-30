#include "PlacementOptimizer.h"
#include <QTransform>
#include <QMap>
#include <QPair>
#include <limits>
#include <QtGlobal>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>

// --- INCLUSION DE CLIPPER2 ---
#include "clipper2/clipper.h"
#include "clipper2/clipper.minkowski.h"

using namespace Clipper2Lib;

namespace {

const double CLIPPER_SCALE = 1000.0;

// --- LES PARAMÈTRES DE PERFORMANCE GLOBAUX ---
const double DECIMATION_TOLERANCE = 5.0;
const double DECIMATION_MARGIN = 1.5;

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
    int64_t idx = dx * CLIPPER_SCALE;
    int64_t idy = dy * CLIPPER_SCALE;
    Paths64 res;
    res.reserve(paths.size());
    for (const auto &path : paths) {
        Path64 p; p.reserve(path.size());
        for (const auto &pt : path) p.push_back(Point64(pt.x + idx, pt.y + idy));
        res.push_back(p);
    }
    return res;
}

// Inversion mathématique rigoureuse
Path64 invertShape(const Path64& shape) {
    Path64 inverted; inverted.reserve(shape.size());
    for (int i = shape.size() - 1; i >= 0; --i) {
        inverted.push_back(Point64(-shape[i].x, -shape[i].y));
    }
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
    if (margin > 0.0) {
        totalNfp = InflatePaths(totalNfp, margin * CLIPPER_SCALE, JoinType::Round, EndType::Polygon);
    }
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
    QMap<QPair<int, int>, Paths64> dictionary;
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

// 3. LA FONCTION MAGIQUE : LE SUPER-BLOC (En Haute Définition !)
OrientedShape createOptimalSuperBloc(const QPainterPath& base, const QList<int>& angles, int spacing) {
    // PRÉCISION EXTRÊME : Tolérance de 0.5 pixels pour que les deux pièces s'emboîtent au millimètre
    Paths64 hdClipA = extractAllPaths(base, 0.5);
    QRectF boundsA = base.boundingRect();
    double bestArea = std::numeric_limits<double>::max();
    QPainterPath bestPathB;

    for (int angle : angles) {
        QTransform rot; rot.rotate(angle);
        QPainterPath pathB = rot.map(base);
        Paths64 hdClipB = extractAllPaths(pathB, 0.5); // Haute définition
        QRectF boundsB = pathB.boundingRect();

        // MARGE EXACTE : Pas de marge de sécurité interne, juste l'espacement demandé par l'utilisateur
        Paths64 nfp = calculateNFP(hdClipA, hdClipB, static_cast<double>(spacing));

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
    }

    OrientedShape superBloc;
    superBloc.pieceCount = 2;
    QRectF totalBounds = boundsA.united(bestPathB.boundingRect());
    QTransform align; align.translate(-totalBounds.x(), -totalBounds.y());

    superBloc.originalPaths << align.map(base) << align.map(bestPathB);
    superBloc.bounds = QRectF(0, 0, totalBounds.width(), totalBounds.height());

    // On ne génère pas de clipperPaths HD ici, ce sera géré en "Low-Poly" dans le run principal.
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

    // --- PHASE 1 & 2 : VÉRIFICATION DU CACHE ET CALCUL MULTITHREAD ---
    QByteArray currentHash = hashPath(params.prototypePath);
    bool cacheValid = (globalCache.shapeHash == currentHash &&
                       globalCache.angles == angles &&
                       globalCache.spacing == params.spacing);

    if (!cacheValid) {
        if (onProgress) onProgress(0, params.shapeCount);
        QCoreApplication::processEvents();

        globalCache.shapeHash = currentHash;
        globalCache.angles = angles;
        globalCache.spacing = params.spacing;
        globalCache.singleShapes.clear();
        globalCache.superShapes.clear();
        globalCache.allShapes.clear();
        globalCache.dictionary.clear();

        int nextId = 0;

        // A. Génération des simples (En version Low-Poly pour la vitesse globale)
        for (int angle : angles) {
            QTransform rot; rot.rotate(angle);
            QPainterPath path = rot.map(params.prototypePath);
            OrientedShape shape = {nextId++, 1, {path}, extractAllPaths(path, DECIMATION_TOLERANCE), path.boundingRect()};
            globalCache.singleShapes << shape;
            globalCache.allShapes << shape;
        }

        // B. Génération des Super-Blocs
        OrientedShape baseSuperBloc = createOptimalSuperBloc(params.prototypePath, angles, params.spacing);
        for (int angle : angles) {
            QTransform rot; rot.rotate(angle);
            OrientedShape shape;
            shape.id = nextId++;
            shape.pieceCount = 2;
            for(const QPainterPath& p : baseSuperBloc.originalPaths) shape.originalPaths << rot.map(p);

            QPainterPath combined; for(const auto& p: shape.originalPaths) combined.addPath(p);

            // On extrait la doublette complète en Low-Poly !
            shape.clipperPaths = extractAllPaths(combined, DECIMATION_TOLERANCE);
            shape.bounds = combined.boundingRect();
            globalCache.superShapes << shape;
            globalCache.allShapes << shape;
        }

        // C. Calcul Multithread du Dictionnaire NFP
        // La marge secrète (DECIMATION_MARGIN) est ajoutée ici pour compenser la géométrie Low-Poly
        struct NfpJob { int idA; int idB; Paths64 pathsA; Paths64 pathsB; double margin; };
        struct NfpResult { int idA; int idB; Paths64 nfp; };

        QList<NfpJob> jobs;
        for (const OrientedShape& shapeA : globalCache.allShapes) {
            for (const OrientedShape& shapeB : globalCache.allShapes) {
                jobs.append({shapeA.id, shapeB.id, shapeA.clipperPaths, shapeB.clipperPaths, params.spacing + DECIMATION_MARGIN});
            }
        }

        auto computeNfpTask = [](const NfpJob& job) -> NfpResult {
            return {job.idA, job.idB, calculateNFP(job.pathsA, job.pathsB, job.margin)};
        };

        auto reduceNfpTask = [](QMap<QPair<int, int>, Paths64>& dictionary, const NfpResult& res) {
            dictionary[qMakePair(res.idA, res.idB)] = res.nfp;
        };

        globalCache.dictionary = QtConcurrent::blockingMappedReduced<QMap<QPair<int, int>, Paths64>>(
            jobs, computeNfpTask, reduceNfpTask
            );
    }

    // --- PHASE 3 : PLACEMENT OPTIMISÉ (Instantané + Bouche-Trou) ---
    QMap<int, Paths64> allowedZones;
    for (const OrientedShape& shape : globalCache.allShapes) {
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
        OrientedShape winnerShape;

        int remaining = params.shapeCount - piecesPlaced;

        // TENTATIVE 1 : Placer un Super-Bloc
        if (remaining >= 2) {
            for (const OrientedShape& shape : globalCache.superShapes) {
                for (const Path64& path : allowedZones[shape.id]) {
                    for (const Point64& pt : path) {
                        double px = static_cast<double>(pt.x) / CLIPPER_SCALE;
                        double py = static_cast<double>(pt.y) / CLIPPER_SCALE;
                        double score = (py * 10000.0) + px;

                        if (score < bestScore) {
                            bestScore = score; bestX = px; bestY = py; bestId = shape.id;
                            piecePlacedThisTurn = true;
                            winnerShape = shape;
                        }
                    }
                }
            }
        }

        // TENTATIVE 2 : Le "Bouche-Trou" (Fallback)
        if (!piecePlacedThisTurn) {
            for (const OrientedShape& shape : globalCache.singleShapes) {
                for (const Path64& path : allowedZones[shape.id]) {
                    for (const Point64& pt : path) {
                        double px = static_cast<double>(pt.x) / CLIPPER_SCALE;
                        double py = static_cast<double>(pt.y) / CLIPPER_SCALE;
                        double score = (py * 10000.0) + px;

                        if (score < bestScore) {
                            bestScore = score; bestX = px; bestY = py; bestId = shape.id;
                            piecePlacedThisTurn = true;
                            winnerShape = shape;
                        }
                    }
                }
            }
        }

        if (piecePlacedThisTurn) {
            placedShapes.push_back({bestId, bestX, bestY});
            for (const QPainterPath& p : winnerShape.originalPaths) {
                result.placedPaths << p.translated(bestX, bestY);
            }

            for (const OrientedShape& shape : globalCache.allShapes) {
                if (allowedZones[shape.id].empty()) continue;
                Paths64 nfp = globalCache.dictionary[qMakePair(bestId, shape.id)];
                Paths64 translatedNfp = translateClipper(nfp, bestX, bestY);
                allowedZones[shape.id] = Difference(allowedZones[shape.id], translatedNfp, FillRule::NonZero);
            }

            piecesPlaced += winnerShape.pieceCount;
        } else {
            break;
        }
    }

    result.processedPositions = piecesPlaced;
    result.totalPositions = params.shapeCount;
    return result;
}
