#include "pathplanner.h"

#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QPainterPath>
#include <QSet>
#include <QLineF>

// ---------------------------------------------------------------------------
//  Clé unique pour identifier un segment indépendamment de son sens
//  (Sert à éviter de découper deux fois la même ligne)
// ---------------------------------------------------------------------------
static inline quint64 keySeg(const QPoint &p1, const QPoint &p2)
{
    quint32 k1 = (quint32(p1.x()) << 16) | quint16(p1.y());
    quint32 k2 = (quint32(p2.x()) << 16) | quint16(p2.y());
    return (quint64)qMin(k1, k2) << 32 | qMax(k1, k2);
}

// ---------------------------------------------------------------------------
//  Extraction des segments individuels (gère les bords et les trous internes)
// ---------------------------------------------------------------------------
QList<Segment> PathPlanner::extractSegments(QGraphicsScene *sc)
{
    QList<Segment> out;
    QSet<quint64>  seen;

    for (QGraphicsItem *it : sc->items()) {
        // Ignorer les éléments invisibles ou les curseurs d'UI (zValue >= 50)
        if (!it->isVisible() || it->zValue() >= 50) continue;

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
            continue; // Forme non prise en charge

        path = it->mapToScene(path);

        // toSubpathPolygons isole strictement l'extérieur et les trous internes !
        QList<QPolygonF> subPolygons = path.toSubpathPolygons();

        for (const QPolygonF& poly : subPolygons) {
            const int n = poly.size();
            if (n < 2) continue;

            // On s'arrête à n-1 car toSubpathPolygons boucle déjà le dernier point sur le premier
            for (int i = 0; i < n - 1; ++i) {
                const QPoint p1 = poly[i].toPoint();
                const QPoint p2 = poly[i + 1].toPoint();

                if (p1 == p2) continue;

                const quint64 k = keySeg(p1, p2);
                if (seen.contains(k)) continue;  // Ce segment a déjà été ajouté

                seen.insert(k);
                out.append({ p1, p2, QLineF(p1, p2).length() });
            }
        }
    }
    return out;
}
