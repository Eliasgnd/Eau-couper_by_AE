#include "EraserTool.h"

#include "ShapeManager.h"
#include <QtGlobal>

#include <QLineF>
#include <QPainterPath>
#include <QPolygonF>
#include <QtGlobal>

#include <algorithm>
#include <cmath>


EraserTool::EraserTool(ShapeManager *shapeManager, QObject *parent)
    : QObject(parent)
    , m_shapeManager(shapeManager)
{
    Q_ASSERT(m_shapeManager != nullptr);
}

void EraserTool::setEraserRadius(qreal radius)
{
    if (radius > 0.0) m_eraseRadius = radius;
}

qreal EraserTool::eraserRadius() const
{
    return m_eraseRadius;
}

void EraserTool::applyEraserAt(const QPointF &center)
{
    if (!m_shapeManager) return;

    std::vector<ShapeManager::Shape> newShapes;
    newShapes.reserve(m_shapeManager->shapes().size());

    const qreal r = m_eraseRadius;
    constexpr double epsilon = 0.001;

    int maxOriginalId = 0;
    for (const auto &shape : m_shapeManager->shapes()) {
        maxOriginalId = qMax(maxOriginalId, shape.originalId);
    }
    int nextOriginalId = maxOriginalId + 1;

    for (const auto &shape : m_shapeManager->shapes()) {
        if (shape.path.isEmpty()) {
            newShapes.push_back(shape);
            continue;
        }

        const auto polygons = shape.path.toSubpathPolygons();
        if (polygons.isEmpty()) {
            newShapes.push_back(shape);
            continue;
        }

        bool shapeChanged = false;
        QList<QPainterPath> rebuiltPaths;

        for (const QPolygonF &poly : polygons) {
            if (poly.size() < 2) continue;

            const bool wasClosed = (poly.first() == poly.last());
            QList<QLineF> keptSegments;
            bool polyChanged = false;

            for (int i = 0; i + 1 < poly.size(); ++i) {
                const QPointF p1 = poly[i];
                const QPointF p2 = poly[i + 1];
                const bool p1Inside = QLineF(p1, center).length() <= r + epsilon;
                const bool p2Inside = QLineF(p2, center).length() <= r + epsilon;

                const QPointF d = p2 - p1;
                const double A = d.x() * d.x() + d.y() * d.y();
                const double B = 2.0 * ((p1.x() - center.x()) * d.x() + (p1.y() - center.y()) * d.y());
                const double C = (p1.x() - center.x()) * (p1.x() - center.x()) +
                                 (p1.y() - center.y()) * (p1.y() - center.y()) - r * r;
                const double disc = B * B - 4.0 * A * C;

                QVector<double> tValues;
                if (A > 0.0 && disc >= 0.0) {
                    const double sqrtDisc = std::sqrt(disc);
                    const double t1 = (-B - sqrtDisc) / (2.0 * A);
                    const double t2 = (-B + sqrtDisc) / (2.0 * A);
                    if (t1 >= 0.0 && t1 <= 1.0) tValues.push_back(t1);
                    if (t2 >= 0.0 && t2 <= 1.0) tValues.push_back(t2);
                    std::sort(tValues.begin(), tValues.end());
                }

                auto appendSegment = [&keptSegments](const QPointF &a, const QPointF &b) {
                    if (QLineF(a, b).length() > 0.001) keptSegments.append(QLineF(a, b));
                };

                if (!p1Inside && !p2Inside) {
                    if (tValues.size() >= 2) {
                        polyChanged = true;
                        const QPointF iA = p1 + d * tValues[0];
                        const QPointF iB = p1 + d * tValues[1];
                        appendSegment(p1, iA);
                        appendSegment(iB, p2);
                    } else if (tValues.size() == 1) {
                        polyChanged = true;
                    } else {
                        appendSegment(p1, p2);
                    }
                } else if (!p1Inside && p2Inside) {
                    if (!tValues.isEmpty()) {
                        polyChanged = true;
                        const QPointF i = p1 + d * tValues.first();
                        appendSegment(p1, i);
                    }
                } else if (p1Inside && !p2Inside) {
                    if (!tValues.isEmpty()) {
                        polyChanged = true;
                        const QPointF i = p1 + d * tValues.last();
                        appendSegment(i, p2);
                    }
                } else {
                    polyChanged = true;
                }
            }

            if (!polyChanged) {
                QPainterPath p;
                p.addPolygon(poly);
                if (wasClosed) p.closeSubpath();
                rebuiltPaths.append(p);
                continue;
            }

            shapeChanged = true;

            if (keptSegments.isEmpty()) continue;

            QList<QPainterPath> subPaths;
            QPainterPath currentPath;
            currentPath.moveTo(keptSegments.first().p1());
            currentPath.lineTo(keptSegments.first().p2());

            for (int j = 1; j < keptSegments.size(); ++j) {
                if (QLineF(currentPath.currentPosition(), keptSegments[j].p1()).length() <= 1e-3) {
                    currentPath.lineTo(keptSegments[j].p2());
                } else {
                    subPaths.append(currentPath);
                    currentPath = QPainterPath();
                    currentPath.moveTo(keptSegments[j].p1());
                    currentPath.lineTo(keptSegments[j].p2());
                }
            }
            subPaths.append(currentPath);

            if (wasClosed && subPaths.size() > 1) {
                QPointF firstStart = subPaths.first().elementAt(0);
                QPointF lastEnd = subPaths.last().currentPosition();
                if (QLineF(firstStart, lastEnd).length() <= 1e-3) {
                    QPainterPath lastPath = subPaths.takeLast();
                    lastPath.connectPath(subPaths.takeFirst());
                    subPaths.prepend(lastPath);
                }
            } else if (wasClosed && subPaths.size() == 1) {
                QPointF start = subPaths.first().elementAt(0);
                QPointF end = subPaths.first().currentPosition();
                if (QLineF(start, end).length() <= 1e-3) {
                    subPaths.first().closeSubpath();
                }
            }
            
            rebuiltPaths.append(subPaths);
        }

        if (!shapeChanged) {
            newShapes.push_back(shape);
            continue;
        }

        for (const QPainterPath &path : rebuiltPaths) {
            ShapeManager::Shape rebuilt = shape;
            rebuilt.path = path;
            rebuilt.originalId = nextOriginalId++;
            newShapes.push_back(rebuilt);
        }
    }

    m_shapeManager->setShapes(newShapes);
}

void EraserTool::eraseAlong(const QPointF &from, const QPointF &to)
{
    const qreal length = QLineF(from, to).length();
    const int steps = qMax(1, static_cast<int>(length / qMax<qreal>(1.0, m_eraseRadius * 0.4)));

    for (int i = 0; i <= steps; ++i) {
        const qreal t = static_cast<qreal>(i) / static_cast<qreal>(steps);
        applyEraserAt(from + (to - from) * t);
    }
}
