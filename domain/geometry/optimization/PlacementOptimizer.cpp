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
#include <QMutex>
#include <QPainterPath>
#include <QDataStream>
#include <QDebug> // 👇 NOUVEAU: Inclusion pour le débogage

#include "clipper2/clipper.h"
#include "clipper2/clipper.minkowski.h"
#include "PlacementOptimizer.h"
#include "clipper2/clipper.offset.h"


using namespace Clipper2Lib;

namespace {

const double CLIPPER_SCALE         = 1000.0;
const double DECIMATION_TOLERANCE  = 0.5;
const double DECIMATION_MARGIN     = -0.05;
const double ZONE_SIMPLIFY_EPSILON = 0.005 * CLIPPER_SCALE;
const int    MAX_POLY_POINTS       = 100;

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

// Nouvelle signature avec flag
Paths64 extractAllPaths(const QPainterPath &path, double tolerance, bool isSuperBloc = false) {
    Paths64 res;
    QPointF lastPt;
    double toleranceSq = tolerance * tolerance;

    // Compter les vrais sous-chemins séparés
    int realSubPathCount = 0;
    for (int i = 0; i < path.elementCount(); ++i)
        if (path.elementAt(i).type == QPainterPath::MoveToElement)
            ++realSubPathCount;

    // BOUCLIER : seulement si c'est un super-bloc (plusieurs pièces intentionnelles)
    // PAS pour une forme simple concave (C, U...) qui a plusieurs sous-chemins Qt
    if (isSuperBloc && realSubPathCount > 1) {
        QTransform scale;
        scale.scale(CLIPPER_SCALE, CLIPPER_SCALE);
        QRectF bbox = scale.map(path).boundingRect();
        Path64 p64;
        p64.push_back(Point64(std::round(bbox.left()),   std::round(bbox.top())));
        p64.push_back(Point64(std::round(bbox.right()),  std::round(bbox.top())));
        p64.push_back(Point64(std::round(bbox.right()),  std::round(bbox.bottom())));
        p64.push_back(Point64(std::round(bbox.left()),   std::round(bbox.bottom())));
        return {p64};
    }

    // Toutes les formes → contour réel
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

Paths64 fanTriangulate(const Path64 &path) {
    if (path.size() < 3) return {path};
    double cx = 0, cy = 0;
    for (const auto &pt : path) { cx += pt.x; cy += pt.y; }
    cx /= path.size(); cy /= path.size();
    Point64 center(static_cast<int64_t>(cx), static_cast<int64_t>(cy));
    Paths64 tris;
    size_t n = path.size();
    for (size_t i = 0; i < n; ++i) {
        Path64 tri = {center, path[i], path[(i+1)%n]};
        if (std::abs(Area(tri)) > 100)
            tris.push_back(tri);
    }
    return tris.empty() ? Paths64{path} : tris;
}

Paths64 calculateNFP(const Paths64 &stationary, const Paths64 &orbiting, double margin) {
    // Simplification ultra précise pour préserver la concavité
    Paths64 statSimp = SimplifyPaths(stationary, 1.0);
    Paths64 orbSimp  = SimplifyPaths(orbiting,   1.0);
    for (auto &p : statSimp) p = decimatePath(p, MAX_POLY_POINTS);
    for (auto &p : orbSimp)  p = decimatePath(p, MAX_POLY_POINTS);

    // Forcer orientation positive
    for (auto &p : statSimp) if (Area(p) < 0) std::reverse(p.begin(), p.end());
    for (auto &p : orbSimp)  if (Area(p) < 0) std::reverse(p.begin(), p.end());

    Paths64 totalNfp;
    for (const Path64 &statPath : statSimp) {
        for (const Path64 &orbPath : orbSimp) {
            Path64 invOrb = invertShape(orbPath);
            Paths64 nfp = MinkowskiSum(invOrb, statPath, true);

            // Garder uniquement le plus grand path (les autres sont des artefacts)
            if (nfp.size() > 1) {
                double maxArea = 0;
                Path64 best;
                for (const auto &p : nfp) {
                    double a = std::abs(Area(p));
                    if (a > maxArea) { maxArea = a; best = p; }
                }
                nfp = {best};
            }

            totalNfp = Union(totalNfp, nfp, FillRule::NonZero);
        }
    }

    if (margin != 0.0 && std::abs(margin) > 0.001)
        totalNfp = InflatePaths(totalNfp, margin * CLIPPER_SCALE,
                                JoinType::Miter, EndType::Polygon);

    // PAS de SimplifyPaths ici — ça détruirait les creux du NFP concave

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
    int angle;
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
    qDebug() << "[SUPERBLOC] Début de la création du super bloc avec espacement:" << spacing;

    Paths64 hdClipA = extractAllPaths(base, DECIMATION_TOLERANCE);
    QRectF  boundsA = base.boundingRect();

    auto processAngle = [&](int angle) -> QPair<double, QPainterPath> {
        QTransform rot; rot.rotate(angle);
        QPainterPath pathB   = rot.map(base);
        Paths64      hdClipB = extractAllPaths(pathB, DECIMATION_TOLERANCE);
        QRectF       boundsB = pathB.boundingRect();

        // Utiliser le VRAI NFP pour trouver où B s'emboîte dans A
        Paths64 nfp = calculateNFP(hdClipA, hdClipB, static_cast<double>(spacing));

        double       bestArea  = std::numeric_limits<double>::max();
        QPainterPath bestPathB;

        double bestDx = 0.0;
        double bestDy = 0.0;
        bool found = false;

        for (const Path64 &poly : nfp) {
            for (const Point64 &pt : poly) {
                double dx = static_cast<double>(pt.x) / CLIPPER_SCALE;
                double dy = static_cast<double>(pt.y) / CLIPPER_SCALE;

                // Calcul manuel pur (ultra-rapide)
                double minX = std::min(boundsA.left(), boundsB.left() + dx);
                double minY = std::min(boundsA.top(), boundsB.top() + dy);
                double maxX = std::max(boundsA.right(), boundsB.right() + dx);
                double maxY = std::max(boundsA.bottom(), boundsB.bottom() + dy);

                double area = (maxX - minX) * (maxY - minY);

                if (area < bestArea) {
                    bestArea = area;
                    bestDx = dx;
                    bestDy = dy;
                    found = true;
                }
            }
        }

        // On génère la forme Qt complexe uniquement pour le GAGNANT
        if (found) {
            QTransform t; t.translate(bestDx, bestDy);
            bestPathB = t.map(pathB);
        }

        // Fallback si NFP vide
        if (bestPathB.isEmpty()) {
            QTransform t; t.translate(boundsA.right() - boundsB.left(), 0);
            bestPathB = t.map(pathB);
            bestArea  = boundsA.united(bestPathB.boundingRect()).width() *
                       boundsA.united(bestPathB.boundingRect()).height();
        }

        return {bestArea, bestPathB};
    };

    QList<QPair<double, QPainterPath>> results;
    if (angles.size() > 1) results = QtConcurrent::blockingMapped(angles, processAngle);
    else results << processAngle(angles.first());

    double globalBestArea = std::numeric_limits<double>::max();
    QPainterPath globalBestPathB;
    for (const auto &r : results) {
        if (r.first < globalBestArea) {
            globalBestArea = r.first;
            globalBestPathB = r.second;
        }
    }

    qDebug() << "[SUPERBLOC] Meilleur score obtenu :" << globalBestArea;

    OrientedShape superBloc;
    superBloc.pieceCount = 2;
    QRectF totalBounds = boundsA.united(globalBestPathB.boundingRect());
    QTransform align; align.translate(-totalBounds.x(), -totalBounds.y());
    superBloc.originalPaths << align.map(base) << align.map(globalBestPathB);
    superBloc.bounds = QRectF(0, 0, totalBounds.width(), totalBounds.height());

    return superBloc;
}

// ── CONFIGURATION D'UN SCÉNARIO ──
struct ScenarioConfig {
    bool useSuperBlocs;
    bool gravityLeftTop;
    QList<int> allowedAngles;

    // Fonction utilitaire pour le debug
    QString toString() const {
        return QString("Scen[GLT:%1, SuperB:%2, Angles:%3]")
            .arg(gravityLeftTop ? "OUI" : "NON")
            .arg(useSuperBlocs ? "OUI" : "NON")
            .arg(allowedAngles.size());
    }
};

// ── RÉSULTAT D'UN SCÉNARIO ──
struct ScenarioResult {
    bool cancelled = false;
    int piecesPlaced = 0;
    double boundingArea = std::numeric_limits<double>::max();
    QList<QPainterPath> placedPaths;
};

static QMutex progressMutex;

} // namespace


// ============================================================================
// RUN FUNCTION
// ============================================================================
PlacementResult PlacementOptimizer::run(
    const PlacementParams &params,
    const std::function<bool(int, int)> &onProgress)
{
    qDebug() << "\n=======================================================";
    qDebug() << "[OPTIMIZER] Lancement d'une nouvelle optimisation !";
    qDebug() << "[OPTIMIZER] Quantité de pièces à placer:" << params.shapeCount;
    qDebug() << "[OPTIMIZER] Zone conteneur :" << params.containerRect;

    PlacementResult finalResult;
    if (params.shapeCount <= 0 || params.prototypePath.isEmpty()) {
        qDebug() << "[OPTIMIZER] ERREUR : 0 forme ou prototype vide. Annulation.";
        return finalResult;
    }

    const double W = params.containerRect.width();
    const double H = params.containerRect.height();

    QList<int> angles = params.angles;
    if (!angles.contains(180)) {
        angles.append(180);
    }
    if (angles.isEmpty()) {
        angles = QList<int>{0, 180};
    }
    qDebug() << "[OPTIMIZER] Angles utilisés :" << angles;

    // ── PHASE 1 : CACHE & NFP ──
    QByteArray currentHash = hashPath(params.prototypePath);
    bool cacheValid = (globalCache.shapeHash == currentHash &&
                       globalCache.angles    == angles      &&
                       globalCache.spacing   == params.spacing);

    qDebug() << "[CACHE] Vérification du cache... Valide ?" << (cacheValid ? "OUI" : "NON");

    if (!cacheValid) {
        qDebug() << "[CACHE] Cache invalide ou absent. Recalcul des formes et NFP...";
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
        // Création des formes simples (1 pièce)
        qDebug() << "[INIT] Génération des formes simples pour" << angles.size() << "angles...";
        for (int angle : angles) {
            QTransform rot; rot.rotate(angle);
            QPainterPath path = rot.map(params.prototypePath);
            OrientedShape shape = {
                nextId++, 1, {path}, extractAllPaths(path, DECIMATION_TOLERANCE), path.boundingRect(), angle
            };
            globalCache.singleShapes << shape; globalCache.allShapes << shape;
        }

        // Création des super-blocs (2 pièces imbriquées)
        qDebug() << "[INIT] Génération des super-blocs...";
        OrientedShape baseSuperBloc = createOptimalSuperBloc(params.prototypePath, angles, params.spacing);
        for (int angle : angles) {
            QTransform rot; rot.rotate(angle);
            OrientedShape shape;
            shape.id = nextId++; shape.pieceCount = 2; shape.angle = angle;
            for (const QPainterPath &p : baseSuperBloc.originalPaths) shape.originalPaths << rot.map(p);

            QPainterPath combined;
            for (const auto &p : shape.originalPaths) combined.addPath(p);
            shape.clipperPaths = extractAllPaths(combined, DECIMATION_TOLERANCE, true);
            shape.bounds       = combined.boundingRect();

            globalCache.superShapes << shape; globalCache.allShapes << shape;
        }

        // Calcul des NFP
        struct NfpJob { int idA, idB; Paths64 pathsA, pathsB; double margin; };
        struct NfpResult { int idA, idB; Paths64 nfp; };
        QList<NfpJob> jobs;

        for (int i = 0; i < globalCache.allShapes.size(); ++i) {
            for (int j = i; j < globalCache.allShapes.size(); ++j) {
                jobs.append({globalCache.allShapes[i].id, globalCache.allShapes[j].id,
                             globalCache.allShapes[i].clipperPaths, globalCache.allShapes[j].clipperPaths,
                             params.spacing + DECIMATION_MARGIN});
            }
        }

        qDebug() << "[NFP] Lancement de" << jobs.size() << "tâches Minkowski (NFP) en parallèle...";
        auto computeNfp = [](const NfpJob &job) -> NfpResult {
            return {job.idA, job.idB, calculateNFP(job.pathsA, job.pathsB, job.margin)};
        };

        int threadCount = QThreadPool::globalInstance()->maxThreadCount();
        QThreadPool::globalInstance()->setMaxThreadCount(threadCount);
        QList<NfpResult> nfpResults = QtConcurrent::blockingMapped(jobs, std::function<NfpResult(const NfpJob&)>(computeNfp));
        QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

        qDebug() << "[NFP] Tâches terminées. Remplissage du dictionnaire...";
        for (const NfpResult &res : nfpResults) {
            globalCache.dictionary[dictKey(res.idA, res.idB)] = res.nfp;
            if (res.idA != res.idB) globalCache.dictionary[dictKey(res.idB, res.idA)] = mirrorNFP(res.nfp);
        }
    } else {
        qDebug() << "[CACHE] Utilisation des formes et NFP existants en mémoire.";
    }

    // ── ZONES INITIALES DE PLACEMENT ──
    qDebug() << "[ZONES] Création des zones initiales possibles (Inner Fit Polygon rectangulaires)...";
    std::unordered_map<int, Paths64> initialZones;
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
            initialZones[shape.id] = {ifpRect};
        } else {
            qDebug() << "[ZONES] ATTENTION: La forme d'ID" << shape.id << "ne rentre pas dans le conteneur !";
            initialZones[shape.id] = Paths64();
        }
    }

    // ── PHASE 2 : CRÉATION DES SCÉNARIOS ──
    QList<ScenarioConfig> scenarios;
    QList<int> horizAngles, vertAngles;

    for (int a : angles) {
        if (a % 180 == 0) horizAngles << a;
        else if (a % 90 == 0) vertAngles << a;
    }

    scenarios << ScenarioConfig{true, false, angles} << ScenarioConfig{false, false, angles}
              << ScenarioConfig{true, true, angles}  << ScenarioConfig{false, true, angles};

    if (!horizAngles.isEmpty() && horizAngles.size() < angles.size()) {
        scenarios << ScenarioConfig{true, false, horizAngles} << ScenarioConfig{true, true, horizAngles};
    }

    if (!vertAngles.isEmpty() && vertAngles.size() < angles.size()) {
        scenarios << ScenarioConfig{true, false, vertAngles} << ScenarioConfig{true, true, vertAngles};
    }

    qDebug() << "[SCENARIOS] Prêt à lancer" << scenarios.size() << "scénarios en parallèle.";

    // ── PHASE 3 : LE MOTEUR D'EXÉCUTION ──
    auto executeScenario = [&](const ScenarioConfig &config) -> ScenarioResult {
        QString sName = config.toString();

        ScenarioResult res;
        auto localZones = initialZones;

        // OPTI 2 : On fait des copies modifiables pour pouvoir supprimer les formes pleines
        QList<OrientedShape> activeSingles, activeSupers;
        for (const auto &s : globalCache.singleShapes) if (config.allowedAngles.contains(s.angle)) activeSingles << s;
        if (config.useSuperBlocs) {
            for (const auto &s : globalCache.superShapes) if (config.allowedAngles.contains(s.angle)) activeSupers << s;
        }

        struct PlacementCandidate {
            bool found = false;
            double x = 0, y = 0, score = std::numeric_limits<double>::max();
            const OrientedShape* shape = nullptr;
        };

        // OPTI 1 : Stockage temporaire abstrait avec POINTEUR (Zéro copie !)
        struct PlacedItem { const OrientedShape* shape; double x, y; };
        QList<PlacedItem> placedHistory;

        QRectF globalBoundingBox;

        while (res.piecesPlaced < params.shapeCount) {
            if (onProgress) {
                QMutexLocker locker(&progressMutex);
                if (!onProgress(res.piecesPlaced, params.shapeCount)) {
                    res.cancelled = true;
                    break;
                }
            }

            auto findBest = [&](QList<OrientedShape> &shapes) -> PlacementCandidate {
                PlacementCandidate best;
                for (int i = 0; i < shapes.size(); ) {
                    const OrientedShape &shape = shapes[i];
                    const Paths64 &zone = localZones[shape.id];

                    if (zone.empty()) {
                        shapes.removeAt(i);
                        continue;
                    }

                    for (const Path64 &path : zone) {
                        for (const Point64 &pt : path) {
                            double px = static_cast<double>(pt.x) / CLIPPER_SCALE;
                            double py = static_cast<double>(pt.y) / CLIPPER_SCALE;

                            double score = config.gravityLeftTop ? ((px * 1000.0) + py) : ((py * 1000.0) + px);

                            if (score < best.score) {
                                best.score = score;
                                best.x = px;
                                best.y = py;
                                best.shape = &shape; // On sauvegarde l'adresse
                                best.found = true;
                            }
                        }
                    }
                    ++i;
                }
                return best;
            };

            PlacementCandidate bestSuper = findBest(activeSupers);
            PlacementCandidate bestSingle = findBest(activeSingles);
            PlacementCandidate winner;

            if (bestSuper.found && bestSingle.found) {
                // 👇 On utilise -> car shape est un pointeur
                double tolerance = config.gravityLeftTop ? (bestSingle.shape->bounds.width() * 0.6)
                                                         : (bestSingle.shape->bounds.height() * 0.6);
                double valSuper  = config.gravityLeftTop ? bestSuper.x : bestSuper.y;
                double valSingle = config.gravityLeftTop ? bestSingle.x : bestSingle.y;

                if (valSuper <= valSingle + tolerance) winner = bestSuper;
                else winner = bestSingle;
            }
            else if (bestSuper.found) winner = bestSuper;
            else if (bestSingle.found) winner = bestSingle;
            else {
                qDebug() << "[EXEC]" << sName << "-> PLUS DE PLACE. Pièces placées :" << res.piecesPlaced << "/" << params.shapeCount;
                break;
            }

            // OPTI 1 : On enregistre juste le pointeur et les datas
            placedHistory.append({winner.shape, winner.x, winner.y});

            // 👇 On utilise -> pour accéder à bounds
            QRectF translatedBounds = winner.shape->bounds.translated(winner.x, winner.y);
            globalBoundingBox = globalBoundingBox.united(translatedBounds);

            // OPTI 3 : Mise à jour des zones libres
            auto updateZonesForShapes = [&](const QList<OrientedShape> &shapesList) {
                for (const OrientedShape &shape : shapesList) {
                    Paths64 &zone = localZones[shape.id];
                    if (zone.empty()) continue;

                    // 👇 On utilise -> pour accéder à l'id du gagnant
                    auto it = globalCache.dictionary.find(dictKey(winner.shape->id, shape.id));
                    if (it == globalCache.dictionary.end()) continue;

                    Paths64 tNfp = translateClipper(it->second, winner.x, winner.y);
                    zone = Difference(zone, tNfp, FillRule::NonZero);

                    // OPTIONNEL : Nettoyage Clipper si ça ralentit sur la fin
                    // if (!zone.empty()) zone = CleanPaths(zone, 1.5);
                }
            };

            updateZonesForShapes(activeSingles);
            updateZonesForShapes(activeSupers);

            // 👇 On utilise -> pour accéder au pieceCount
            res.piecesPlaced += winner.shape->pieceCount;
        }

        // OPTI 1 (Suite) : Construction des QPainterPath à la toute fin
        for (const PlacedItem &item : placedHistory) {
            // 👇 On utilise -> car item.shape est un pointeur
            for (const QPainterPath &p : item.shape->originalPaths) {
                res.placedPaths << p.translated(item.x, item.y);
            }
        }

        res.boundingArea = globalBoundingBox.width() * globalBoundingBox.height();
        qDebug() << "[EXEC]" << sName << "-> TERMINÉ avec" << res.piecesPlaced << "pièces. Surface occupée:" << res.boundingArea;
        return res;
    };

    // ── PHASE 4 : DÉMARRAGE EN PARALLÈLE ──
    qDebug() << "[OPTIMIZER] Lancement des scénarios...";
    QList<ScenarioResult> results = QtConcurrent::blockingMapped(scenarios, std::function<ScenarioResult(const ScenarioConfig&)>(executeScenario));

    // ── PHASE 5 : ÉLECTION DU GAGNANT ──
    qDebug() << "[OPTIMIZER] Scénarios terminés. Élection du meilleur résultat...";
    ScenarioResult bestScenario;
    bestScenario.piecesPlaced = -1;

    int winnerIndex = -1;
    for (int i = 0; i < results.size(); ++i) {
        const ScenarioResult &res = results[i];
        if (res.cancelled) {
            qDebug() << "[OPTIMIZER] Processus annulé.";
            return finalResult;
        }

        if (res.piecesPlaced > bestScenario.piecesPlaced) {
            bestScenario = res;
            winnerIndex = i;
        }
        else if (res.piecesPlaced == bestScenario.piecesPlaced) {
            // Si égalité de pièces, on prend celui qui emballe le tout dans la plus petite surface
            if (res.boundingArea < bestScenario.boundingArea) {
                bestScenario = res;
                winnerIndex = i;
            }
        }
    }

    qDebug() << "[OPTIMIZER] >>> GAGNANT ÉLU: Scénario #" << winnerIndex
             << "avec" << bestScenario.piecesPlaced << "pièces (Surface:" << bestScenario.boundingArea << ") <<<";
    qDebug() << "=======================================================\n";

    finalResult.placedPaths        = bestScenario.placedPaths;
    finalResult.processedPositions = bestScenario.piecesPlaced;
    finalResult.totalPositions     = params.shapeCount;
    return finalResult;
}
