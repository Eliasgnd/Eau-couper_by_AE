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
// ---------------------------------------------------------------------------
static inline quint64 keySeg(const QPoint &p1, const QPoint &p2)
{
    quint32 k1 = (quint32(p1.x()) << 16) | quint16(p1.y());
    quint32 k2 = (quint32(p2.x()) << 16) | quint16(p2.y());
    return (quint64)qMin(k1, k2) << 32 | qMax(k1, k2);
}

// ---------------------------------------------------------------------------
//  Extraction des segments individuels avec filtrage des micro-segments
// ---------------------------------------------------------------------------
QList<Segment> PathPlanner::extractSegments(QGraphicsScene *sc)
{
    QList<Segment> out;
    QSet<quint64>  seen;

    // SEUIL DE FILTRAGE (en pixels/mm).
    // Si la distance entre deux points est inférieure à ce seuil, on ignore le point.
    // Réglez entre 0.5 (plus précis) et 2.0 (plus fluide/rapide).
    const double MIN_DIST = 1.0;

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

        QList<QPolygonF> subPolygons = path.toSubpathPolygons();

        for (const QPolygonF& poly : subPolygons) {
            const int n = poly.size();
            if (n < 2) continue;

            // On garde le premier point en mémoire
            QPointF lastSavedPoint = poly[0];

            for (int i = 1; i < n; ++i) {
                QPointF currentPoint = poly[i];
                bool isLastPoint = (i == n - 1); // Toujours envoyer le dernier point pour fermer la forme

                // FILTRAGE : Si le point actuel est trop proche du dernier point sauvegardé,
                // on l'ignore (sauf si c'est le point final de la boucle).
                if (!isLastPoint && QLineF(lastSavedPoint, currentPoint).length() < MIN_DIST) {
                    continue;
                }

                const QPoint p1 = lastSavedPoint.toPoint();
                const QPoint p2 = currentPoint.toPoint();

                if (p1 != p2) {
                    const quint64 k = keySeg(p1, p2);
                    if (!seen.contains(k)) {
                        seen.insert(k);
                        out.append({ p1, p2, QLineF(p1, p2).length() });
                    }
                }

                // On met à jour le dernier point sauvegardé
                lastSavedPoint = currentPoint;
            }
        }
    }
    return out;
}
