#include "PlacementOptimizer.h"

#include <QPainterPathStroker>
#include <QTransform>

namespace {

QPainterPath bufferedPath(const QPainterPath &path, int spacing)
{
    if (spacing <= 0)
        return path;

    QPainterPathStroker stroker;
    stroker.setWidth(spacing);
    return path.united(stroker.createStroke(path));
}

QPainterPath normalizeToOrigin(const QPainterPath &path)
{
    const QRectF bounds = path.boundingRect();
    QTransform norm;
    norm.translate(-bounds.x(), -bounds.y());
    QPainterPath normalized = norm.map(path);
    normalized.closeSubpath();
    return normalized;
}

QList<QPainterPath> buildOrientedPrototypes(const QPainterPath &prototypePath, const QList<int> &angles)
{
    QList<QPainterPath> oriented;
    const QPainterPath normalized = normalizeToOrigin(prototypePath);
    const QPointF center = normalized.boundingRect().center();

    for (int angle : angles) {
        QTransform rotation;
        if (angle != 0) {
            rotation.translate(center.x(), center.y());
            rotation.rotate(angle);
            rotation.translate(-center.x(), -center.y());
        }
        oriented << normalizeToOrigin(rotation.map(normalized));
    }

    return oriented;
}

} // namespace

PlacementResult PlacementOptimizer::run(const PlacementParams &params,
                                        const std::function<bool(int current, int total)> &onProgress)
{
    PlacementResult result;
    if (params.shapeCount <= 0 || params.step <= 0 || params.prototypePath.isEmpty())
        return result;

    const int drawingWidth = static_cast<int>(params.containerRect.width());
    const int drawingHeight = static_cast<int>(params.containerRect.height());
    result.totalPositions = ((drawingWidth / params.step) + 1) * ((drawingHeight / params.step) + 1);

    const QList<int> angles = params.angles.isEmpty() ? QList<int>{0} : params.angles;
    const QList<QPainterPath> prototypes = buildOrientedPrototypes(params.prototypePath, angles);

    struct PathInfo { QPainterPath collisionPath; QRectF bbox; };
    QList<PathInfo> placedInfos;

    for (int y = 0; y <= drawingHeight && result.placedPaths.size() < params.shapeCount; y += params.step) {
        for (int x = 0; x <= drawingWidth && result.placedPaths.size() < params.shapeCount; x += params.step) {
            ++result.processedPositions;
            if (onProgress && !onProgress(result.processedPositions, result.totalPositions)) {
                result.cancelled = true;
                return result;
            }

            for (const QPainterPath &proto : prototypes) {
                QPainterPath candidate = proto.translated(x, y);
                candidate.closeSubpath();

                const QPainterPath collisionCandidate = bufferedPath(candidate, params.spacing);
                if (!params.containerRect.contains(collisionCandidate.boundingRect()))
                    continue;

                const QRectF candBBox = collisionCandidate.boundingRect();
                bool collision = false;
                for (const PathInfo &existing : placedInfos) {
                    if (!existing.bbox.intersects(candBBox))
                        continue;

                    const QPainterPath inter = collisionCandidate.intersected(existing.collisionPath);
                    const QRectF br = inter.boundingRect();
                    if (!br.isNull() && br.width() > 1.0 && br.height() > 1.0) {
                        collision = true;
                        break;
                    }
                }

                if (!collision) {
                    result.placedPaths << candidate;
                    placedInfos << PathInfo{collisionCandidate, candBBox};
                    break;
                }
            }
        }
    }

    return result;
}
