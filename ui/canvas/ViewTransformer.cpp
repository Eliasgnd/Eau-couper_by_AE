#include "ViewTransformer.h"

#include <QtGlobal>

ViewTransformer::ViewTransformer(QObject *parent)
    : QObject(parent)
{
}

void ViewTransformer::setScale(double scale)
{
    const double clamped = qBound(0.1, scale, 10.0);
    if (qFuzzyCompare(m_scale, clamped)) return;

    m_scale = clamped;
    emit zoomChanged(m_scale);
    emit viewTransformed();
}

void ViewTransformer::applyPanDelta(const QPointF &delta)
{
    m_offset += delta;
    emit viewTransformed();
}

void ViewTransformer::setOffset(const QPointF &offset)
{
    if (offset == m_offset) return;
    m_offset = offset;
    emit viewTransformed();
}

QPointF ViewTransformer::clampToCanvas(const QPointF &p, const QSizeF &canvasSize) const
{
    return QPointF(qBound(0.0, p.x(), canvasSize.width()),
                   qBound(0.0, p.y(), canvasSize.height()));
}
