#include "ImportedImageGeometryHelper.h"

#include <algorithm>
#include <QTransform>

QPainterPath ImportedImageGeometryHelper::fitCentered(const QPainterPath &path,
                                                      const QSize &targetSize,
                                                      double occupancy)
{
    if (path.isEmpty() || targetSize.width() <= 0 || targetSize.height() <= 0)
        return {};

    QRectF bounds = path.boundingRect();
    if (bounds.isEmpty())
        return {};

    const double maxDimension = std::max(bounds.width(), bounds.height());
    if (maxDimension <= 0.0)
        return {};

    const double areaWidth = std::max(1, targetSize.width());
    const double areaHeight = std::max(1, targetSize.height());
    const double scale = occupancy * std::min(areaWidth, areaHeight) / maxDimension;

    QTransform transform;
    transform.translate(-bounds.x(), -bounds.y());
    transform.scale(scale, scale);

    QPainterPath scaled = transform.map(path);
    QRectF scaledBounds = scaled.boundingRect();
    QPointF center(areaWidth / 2.0, areaHeight / 2.0);
    scaled.translate(center - scaledBounds.center());
    return scaled;
}
