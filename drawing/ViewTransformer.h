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

    void setScale(double scale);
    double scale() const { return m_scale; }

    void applyPanDelta(const QPointF &delta);
    void setOffset(const QPointF &offset);
    const QPointF &offset() const { return m_offset; }

    QPointF clampToCanvas(const QPointF &p, const QSizeF &canvasSize) const;

signals:
    void zoomChanged(double newScale);
    void viewTransformed();

private:
    double m_scale = 1.0;
    QPointF m_offset = QPointF(0, 0);
};

#endif // VIEWTRANSFORMER_H
