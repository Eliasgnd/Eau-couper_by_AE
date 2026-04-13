#include "GeometryTransformHelper.h"

QPointF GeometryTransformHelper::computeUnifiedPivot(const QList<QGraphicsItem*> &items)
{
    if (items.isEmpty())
        return {};

    if (items.size() == 1)
        return items.first()->sceneBoundingRect().center();

    QRectF unitedRect;
    bool first = true;
    for (QGraphicsItem *item : items) {
        if (first) {
            unitedRect = item->sceneBoundingRect();
            first = false;
        } else {
            unitedRect = unitedRect.united(item->sceneBoundingRect());
        }
    }
    return unitedRect.center();
}
