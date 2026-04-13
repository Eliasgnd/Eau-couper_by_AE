#pragma once

#include <QPointF>
#include <QSizeF>
#include <QList>

struct GridPlacementRequest {
    QSizeF containerSize;
    QSizeF shapeSize;
    int shapeCount {0};
    int spacing {0};
    QPointF origin {0.0, 0.0};
};

class GridPlacementService
{
public:
    static QList<QPointF> computePositions(const GridPlacementRequest &request);
};
