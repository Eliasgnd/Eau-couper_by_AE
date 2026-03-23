#include "EraserTool.h"

#include <QLineF>
#include <QtGlobal>

EraserTool::EraserTool(QObject *parent)
    : QObject(parent)
{
}

qreal EraserTool::eraserRadius() const { return m_gommeRadius; }
bool EraserTool::isErasing() const { return m_gommeErasing; }

void EraserTool::setEraserRadius(qreal radius)
{
    if (radius <= 0.0) return;
    m_gommeRadius = radius;
    emit eraserRadiusChanged(m_gommeRadius);
}

void EraserTool::setErasing(bool erasing)
{
    m_gommeErasing = erasing;
}

void EraserTool::applyEraserAt(const QPointF &center)
{
    if (!m_gommeErasing) return;
    emit eraseRequested(center);
}

void EraserTool::eraseAlong(const QPointF &from, const QPointF &to)
{
    const qreal distance = QLineF(from, to).length();
    const int steps = qMax(1, static_cast<int>(distance / qMax<qreal>(1.0, m_gommeRadius * 0.5)));
    for (int i = 0; i <= steps; ++i) {
        const qreal t = static_cast<qreal>(i) / static_cast<qreal>(steps);
        applyEraserAt(from + (to - from) * t);
    }
}
