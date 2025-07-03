#include "pathplanner.h"

#include <QSet>
#include <QHash>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QtMath>
#include <limits>

// ---------------------------------------------------------------------------
//  1.  Fonctions de hachage Qt-compatibles
// ---------------------------------------------------------------------------
#include <QtGlobal>                    // QT_VERSION, …
#include <QtCore/qhashfunctions.h>     // qHashMulti (Qt ≥ 5.15)

/*----------------------------------------------*
 * a) QPoint : Qt ≥ 5 possède déjà qHash(QPoint) *
 *----------------------------------------------*/
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
inline size_t qHash(const QPoint &pt, size_t seed = 0) noexcept
{
    return seed
           ^ (static_cast<size_t>(pt.x()) << 16)
           ^  static_cast<size_t>(pt.y());
}
#endif

/*-----------------------------------------------------------*
 * b) QPair<QPoint,QPoint> : Qt n’en fournit pas par défaut. *
 *-----------------------------------------------------------*/
inline size_t qHash(const QPair<QPoint, QPoint> &key,
                    size_t seed = 0) noexcept
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    // Utilise l’implémentation générique moderne
    return qHashMulti(seed, key.first, key.second);
#else
    // Fallback compatible Qt < 5.15
    seed ^= qHash(key.first, 0) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
    seed ^= qHash(key.second, 0);
    return seed;
#endif
}

// ---------------------------------------------------------------------------
//  2.  Aide pour fusionner des segments non orientés
// ---------------------------------------------------------------------------
static inline quint64 keySeg(const QPoint &p1, const QPoint &p2)
{
    quint32 k1 = (quint32(p1.x()) << 16) | quint16(p1.y());
    quint32 k2 = (quint32(p2.x()) << 16) | quint16(p2.y());
    return (quint64)qMin(k1, k2) << 32 | qMax(k1, k2);
}

// ---------------------------------------------------------------------------
//  3.  Extraction des segments
// ---------------------------------------------------------------------------
QList<Segment> PathPlanner::extractSegments(QGraphicsScene *sc)
{
    QList<Segment> out;
    QSet<quint64>  seen;

    for (QGraphicsItem *it : sc->items()) {
        QPainterPath path;

        if (auto *r = qgraphicsitem_cast<QGraphicsRectItem *>(it))
            path.addRect(r->rect());
        else if (auto *p = qgraphicsitem_cast<QGraphicsPathItem *>(it))
            path = p->path();
        else if (auto *e = qgraphicsitem_cast<QGraphicsEllipseItem *>(it))
            path.addEllipse(e->rect());
        else if (auto *polyItem = qgraphicsitem_cast<QGraphicsPolygonItem *>(it))
            path.addPolygon(polyItem->polygon());
        else
            continue;                                   // Non pris en charge

        path = it->mapToScene(path);
        QPolygonF poly = path.toFillPolygon();
        const int n    = poly.size();

        for (int i = 0; i < n; ++i) {
            const QPoint p1 = poly[i].toPoint();
            const QPoint p2 = poly[(i + 1) % n].toPoint();
            if (p1 == p2)
                continue;

            const quint64 k = keySeg(p1, p2);
            if (seen.contains(k))
                continue;                               // Segment déjà ajouté

            seen.insert(k);
            out.append({ p1, p2, QLineF(p1, p2).length() });
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
//  4.  Construction d’un chemin eulérien (algorithme d’Hierholzer + appairage
//      glouton des sommets de degré impair).
// ---------------------------------------------------------------------------
QVector<QPoint> PathPlanner::buildEulerPath(const QList<Segment> &segs)
{
    if (segs.isEmpty())
        return {};

    // --- Adjacences ---------------------------------------------------------
    QHash<QPoint, QVector<QPoint>> adj;
    for (const Segment &s : segs) {
        adj[s.a].append(s.b);
        adj[s.b].append(s.a);
    }
    if (adj.isEmpty())
        return {};

    // --- Sommets de degré impair -------------------------------------------
    QVector<QPoint> odd;
    for (auto it = adj.cbegin(); it != adj.cend(); ++it)
        if (it.value().size() & 1)
            odd.append(it.key());

    // --- Appairage glouton (ajoute des arêtes fictives) ---------------------
    while (odd.size() > 1) {
        const QPoint p = odd.takeLast();
        int   bestIdx  = 0;
        qreal bestDist = std::numeric_limits<qreal>::infinity();

        for (int i = 0; i < odd.size(); ++i) {
            const qreal d = QLineF(p, odd[i]).length();
            if (d < bestDist) {
                bestDist = d;
                bestIdx  = i;
            }
        }
        const QPoint q = odd.takeAt(bestIdx);
        adj[p].append(q);
        adj[q].append(p);
    }

    // --- Parcours d’Hierholzer ---------------------------------------------
    QVector<QPoint> stack, path;
    stack.append(adj.begin().key());

    QSet<QPair<QPoint, QPoint>> used;
    while (!stack.isEmpty()) {
        QPoint v = stack.last();
        if (!adj[v].isEmpty()) {
            QPoint u = adj[v].takeLast();
            QPair<QPoint, QPoint> edge(v, u);

            if (used.contains(edge))
                continue;

            used.insert(edge);
            used.insert({ u, v });
            stack.append(u);
        } else {
            path.append(v);
            stack.removeLast();
        }
    }

    std::reverse(path.begin(), path.end());
    return path;
}
