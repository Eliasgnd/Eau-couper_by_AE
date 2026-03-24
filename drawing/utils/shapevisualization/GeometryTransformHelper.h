#pragma once

#include <QGraphicsItem>
#include <QList>
#include <QPointF>

class GeometryTransformHelper
{
public:
    static QPointF computeUnifiedPivot(const QList<QGraphicsItem*> &items);
};
