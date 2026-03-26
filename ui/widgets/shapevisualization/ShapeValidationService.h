#pragma once

#include <QPainterPath>
#include <QList>

#include "GeometryUtils.h"

class ShapeValidationService
{
public:
    static ShapeValidationResult validate(const QList<QPainterPath> &paths,
                                          const QRectF &bounds,
                                          qreal minIntersectionSize = 1.0);
};
