#include "ShapeValidationService.h"

ShapeValidationResult ShapeValidationService::validate(const QList<QPainterPath> &paths,
                                                       const QRectF &bounds,
                                                       qreal minIntersectionSize)
{
    return validatePlacedPaths(paths, bounds, minIntersectionSize);
}
