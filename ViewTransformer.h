#ifndef VIEWTRANSFORMER_H
#define VIEWTRANSFORMER_H

#include <QObject>
#include <QPointF>
#include <QSizeF>

class ViewTransformer : public QObject
{
    Q_OBJECT
public:
    explicit ViewTransformer(QObject *parent = nullptr);

    qreal scale() const;
    QPointF offset() const;

    void setScale(qreal newScale);
    void applyPanDelta(const QPointF &delta);
    QPointF clampToCanvas(const QPointF &offset, const QSizeF &widgetSize, const QSizeF &canvasSize) const;

signals:
    void zoomChanged(qreal newScale);
    void viewTransformed();

private:
    qreal m_scale = 1.0;
    QPointF m_offset;
};

#endif // VIEWTRANSFORMER_H
