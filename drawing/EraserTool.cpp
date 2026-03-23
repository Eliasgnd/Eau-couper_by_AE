#include "EraserTool.h"

#include "ShapeManager.h"
#include <QtGlobal>

#include <QLineF>
#include <QPainterPath>
#include <QtGlobal>

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

    const QRectF eraseRect(center.x() - m_eraseRadius,
                           center.y() - m_eraseRadius,
                           m_eraseRadius * 2.0,
                           m_eraseRadius * 2.0);

    QList<ShapeManager::Shape> filtered;
    filtered.reserve(m_shapeManager->shapes().size());
    for (const auto &shape : m_shapeManager->shapes()) {
        if (!shape.path.boundingRect().intersects(eraseRect)) {
            filtered.append(shape);
        }
    }

    m_shapeManager->setShapes(filtered);
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
