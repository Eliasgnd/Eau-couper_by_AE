#include "EraserTool.h"

#include "ShapeManager.h"
#include <QtGlobal>

#include <QLineF>
#include <QPainterPath>
#include <QPolygonF>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {
QVector<QLineF> pathToLines(const QPainterPath &path)
{
    QVector<QLineF> lines;
    const auto polygons = path.toSubpathPolygons();
    for (const QPolygonF &poly : polygons) {
        for (int i = 0; i + 1 < poly.size(); ++i) {
            lines.push_back(QLineF(poly[i], poly[i + 1]));
        }
    }
    return lines;
}

QList<QPainterPath> mergeSegments(QList<QLineF> segments)
{
    QList<QPainterPath> merged;
    constexpr qreal tol = 0.5;

    while (!segments.isEmpty()) {
        QLineF current = segments.takeFirst();
        QList<QPointF> chain{current.p1(), current.p2()};

        bool progress = true;
        while (progress) {
            progress = false;
            for (int i = 0; i < segments.size(); ++i) {
                const QLineF seg = segments[i];
                if (QLineF(chain.last(), seg.p1()).length() <= tol) {
                    chain.append(seg.p2());
                } else if (QLineF(chain.last(), seg.p2()).length() <= tol) {
                    chain.append(seg.p1());
                } else if (QLineF(chain.first(), seg.p2()).length() <= tol) {
                    chain.prepend(seg.p1());
                } else if (QLineF(chain.first(), seg.p1()).length() <= tol) {
                    chain.prepend(seg.p2());
                } else {
                    continue;
                }
                segments.removeAt(i);
                progress = true;
                break;
            }
        }

        if (chain.size() < 2) continue;
        QPainterPath p;
        p.moveTo(chain.first());
        for (int i = 1; i < chain.size(); ++i) p.lineTo(chain[i]);
        merged.append(p);
    }

    return merged;
}
}

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

    QList<ShapeManager::Shape> newShapes;
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
            newShapes.append(shape);
            continue;
        }

        const QVector<QLineF> lines = pathToLines(shape.path);
        if (lines.isEmpty()) {
            newShapes.append(shape);
            continue;
        }

        QList<QLineF> keptSegments;
        bool changed = false;

        for (const QLineF &segment : lines) {
            const QPointF p1 = segment.p1();
            const QPointF p2 = segment.p2();
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
                if (QLineF(a, b).length() > 0.0) keptSegments.append(QLineF(a, b));
            };

            if (!p1Inside && !p2Inside) {
                if (tValues.size() >= 2) {
                    changed = true;
                    const QPointF iA = p1 + d * tValues[0];
                    const QPointF iB = p1 + d * tValues[1];
                    if ((iA - p1).manhattanLength() > 0.1) appendSegment(p1, iA);
                    if ((p2 - iB).manhattanLength() > 0.1) appendSegment(iB, p2);
                } else if (tValues.size() == 1) {
                    changed = true;
                } else {
                    appendSegment(p1, p2);
                }
            } else if (!p1Inside && p2Inside) {
                if (!tValues.isEmpty()) {
                    changed = true;
                    const QPointF i = p1 + d * tValues.first();
                    if ((i - p1).manhattanLength() > 0.1) appendSegment(p1, i);
                }
            } else if (p1Inside && !p2Inside) {
                if (!tValues.isEmpty()) {
                    changed = true;
                    const QPointF i = p1 + d * tValues.last();
                    if ((p2 - i).manhattanLength() > 0.1) appendSegment(i, p2);
                }
            } else {
                changed = true;
            }
        }

        if (!changed) {
            newShapes.append(shape);
            continue;
        }

        const QList<QPainterPath> rebuiltPaths = mergeSegments(keptSegments);
        for (const QPainterPath &path : rebuiltPaths) {
            ShapeManager::Shape rebuilt = shape;
            rebuilt.path = path;
            rebuilt.originalId = nextOriginalId++;
            newShapes.append(rebuilt);
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
