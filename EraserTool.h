#ifndef ERASERTOOL_H
#define ERASERTOOL_H

#include <QObject>
#include <QPointF>

class EraserTool : public QObject
{
    Q_OBJECT
public:
    explicit EraserTool(QObject *parent = nullptr);

    qreal eraserRadius() const;
    bool isErasing() const;

    void setEraserRadius(qreal radius);
    void setErasing(bool erasing);
    void applyEraserAt(const QPointF &center);
    void eraseAlong(const QPointF &from, const QPointF &to);

signals:
    void eraserRadiusChanged(qreal radius);
    void eraseRequested(const QPointF &center);

private:
    qreal m_gommeRadius = 20.0;
    bool m_gommeErasing = false;
};

#endif // ERASERTOOL_H
