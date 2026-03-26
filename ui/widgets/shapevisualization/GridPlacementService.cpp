#include "GridPlacementService.h"

#include <QtMath>

QList<QPointF> GridPlacementService::computePositions(const GridPlacementRequest &request)
{
    QList<QPointF> positions;

    if (request.shapeCount <= 0)
        return positions;
    if (request.shapeSize.width() <= 0 || request.shapeSize.height() <= 0)
        return positions;
    if (request.containerSize.width() <= 0 || request.containerSize.height() <= 0)
        return positions;

    const qreal cellWidth = request.shapeSize.width() + request.spacing;
    const qreal cellHeight = request.shapeSize.height() + request.spacing;
    if (cellWidth <= 0 || cellHeight <= 0)
        return positions;

    const int maxCols = qFloor(request.containerSize.width() / cellWidth);
    const int maxRows = qFloor(request.containerSize.height() / cellHeight);
    const int totalCells = maxCols * maxRows;
    const int toPlace = qMin(request.shapeCount, totalCells);

    positions.reserve(toPlace);
    for (int i = 0; i < toPlace; ++i) {
        const int col = i % maxCols;
        const int row = i / maxCols;
        positions.append(QPointF(request.origin.x() + (col * cellWidth),
                                 request.origin.y() + (row * cellHeight)));
    }

    return positions;
}
