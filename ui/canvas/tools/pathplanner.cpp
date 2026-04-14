#include "pathplanner.h"

#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QPainterPath>
#include <QLineF>

QList<ContinuousCut> PathPlanner::extractContinuousPaths(QGraphicsScene *sc)
{
    QList<ContinuousCut> outPaths;

    // SEUIL DE FILTRAGE : 1.0 = on ignore les points espacés de moins de 1 pixel/mm
    // C'est ce qui règle le bug de lenteur/saccade du STM32 !
    const double MIN_DIST = 1.0;

    for (QGraphicsItem *it : sc->items()) {
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
            continue;

        path = it->mapToScene(path);

        for (const QPolygonF& poly : path.toSubpathPolygons()) {
            if (poly.size() < 2) continue;

            ContinuousCut cut;
            cut.isClosed = (poly.first() == poly.last());
            cut.points.append(poly[0]);

            for (int i = 1; i < poly.size(); ++i) {
                bool isLastPoint = (i == poly.size() - 1);

                // On ignore les points trop proches (sauf si c'est la fin du tracé)
                if (!isLastPoint && QLineF(cut.points.last(), poly[i]).length() < MIN_DIST) {
                    continue;
                }

                // On évite les doublons parfaits
                if (poly[i] != cut.points.last()) {
                    cut.points.append(poly[i]);
                }
            }

            if (cut.points.size() > 1) {
                outPaths.append(cut);
            }
        }
    }
    return outPaths;
}
