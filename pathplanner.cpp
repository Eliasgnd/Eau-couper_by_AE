#include "pathplanner.h"
#include <QSet>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QtMath>
#include <limits>

#include <QPoint>
#include <QHash>

inline uint qHash(const QPoint &pt, uint seed = 0) noexcept {
    return seed ^ (static_cast<uint>(pt.x()) << 16) ^ static_cast<uint>(pt.y());
}

// Helper pour fusionner segments non orientés
static inline quint64 keySeg(const QPoint& p1, const QPoint& p2) {
    quint32 k1 = (quint32(p1.x()) << 16) | quint16(p1.y());
    quint32 k2 = (quint32(p2.x()) << 16) | quint16(p2.y());
    return (quint64)qMin(k1, k2) << 32 | qMax(k1, k2);
}

// Hash pour QPair<QPoint,QPoint>
static inline uint qHash(const QPair<QPoint,QPoint>& key, uint seed = 0) {
    return qHash(key.first, seed) ^ (qHash(key.second, seed) << 1);
}

QList<Segment> PathPlanner::extractSegments(QGraphicsScene* sc) {
    QList<Segment> out;
    QSet<quint64> seen;
    for (QGraphicsItem* it : sc->items()) {
        QPainterPath path;
        if (auto* r = qgraphicsitem_cast<QGraphicsRectItem*>(it)) {
            path.addRect(r->rect());
        } else if (auto* p = qgraphicsitem_cast<QGraphicsPathItem*>(it)) {
            path = p->path();
        } else if (auto* e = qgraphicsitem_cast<QGraphicsEllipseItem*>(it)) {
            path.addEllipse(e->rect());
        } else if (auto* polyItem = qgraphicsitem_cast<QGraphicsPolygonItem*>(it)) {
            path.addPolygon(polyItem->polygon());
        } else {
            continue;
        }
        path = it->mapToScene(path);
        QPolygonF poly = path.toFillPolygon();
        int n = poly.size();
        for (int i = 0; i < n; ++i) {
            QPoint p1 = poly[i].toPoint();
            QPoint p2 = poly[(i + 1) % n].toPoint();
            if (p1 == p2) continue;
            quint64 k = keySeg(p1, p2);
            if (seen.contains(k)) continue; // déjà ajouté (coupe commune)
            seen.insert(k);
            Segment s{p1, p2, QLineF(p1, p2).length()};
            out.append(s);
        }
    }
    return out;
}

QVector<QPoint> PathPlanner::buildEulerPath(const QList<Segment>& segs) {
    // --- Garde-fous si vide
    if (segs.isEmpty()) return {};

    // Construction de la liste d'adjacence
    QHash<QPoint, QVector<QPoint>> adj;
    for (const Segment& s : segs) {
        adj[s.a].append(s.b);
        adj[s.b].append(s.a);
    }
    if (adj.isEmpty()) return {};

    // Identifier nœuds de degré impair
    QVector<QPoint> odd;
    for (auto it = adj.begin(); it != adj.end(); ++it) {
        if (it.value().size() % 2 != 0) odd.append(it.key());
    }

    // Appariement glouton des nœuds impairs
    while (odd.size() > 1) {
        QPoint p = odd.takeLast();
        int bestIndex = 0;
        double bestDist = std::numeric_limits<double>::infinity();
        for (int i = 0; i < odd.size(); ++i) {
            double d = QLineF(p, odd[i]).length();
            if (d < bestDist) { bestDist = d; bestIndex = i; }
        }
        QPoint q = odd.takeAt(bestIndex);
        adj[p].append(q);
        adj[q].append(p);
    }

    // Algorithme de Hierholzer (chemin eulérien)
    QVector<QPoint> stack, path;
    stack.append(adj.begin().key());
    QSet<QPair<QPoint,QPoint>> used;

    while (!stack.isEmpty()) {
        QPoint v = stack.last();
        if (!adj[v].isEmpty()) {
            QPoint u = adj[v].takeLast();
            QPair<QPoint,QPoint> edge(v, u);
            if (used.contains(edge)) continue;
            used.insert(edge);
            used.insert(QPair<QPoint,QPoint>(u, v));
            stack.append(u);
        } else {
            path.append(v);
            stack.removeLast();
        }
    }
    std::reverse(path.begin(), path.end());
    return path;
}
