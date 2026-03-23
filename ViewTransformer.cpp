#include "ViewTransformer.h"

#include <QtGlobal>

ViewTransformer::ViewTransformer(QObject *parent)
    : QObject(parent)
{
}

qreal ViewTransformer::scale() const { return m_scale; }
QPointF ViewTransformer::offset() const { return m_offset; }

void ViewTransformer::setScale(qreal newScale)
{
    const qreal clamped = qBound<qreal>(1.0, newScale, 10.0);
    if (qFuzzyCompare(m_scale, clamped)) return;
    m_scale = clamped;
    emit zoomChanged(m_scale);
    emit viewTransformed();
}

void ViewTransformer::applyPanDelta(const QPointF &delta)
{
    m_offset -= delta;
    emit viewTransformed();
}

QPointF ViewTransformer::clampToCanvas(const QPointF &offset, const QSizeF &widgetSize, const QSizeF &canvasSize) const
{
    const qreal minOffsetX = widgetSize.width() - canvasSize.width() * m_scale;
    const qreal minOffsetY = widgetSize.height() - canvasSize.height() * m_scale;
    return QPointF(qBound(minOffsetX, offset.x(), 0.0), qBound(minOffsetY, offset.y(), 0.0));
}
