#ifndef ERASERTOOL_H
#define ERASERTOOL_H

#include <QObject>
#include <QPointF>

class ShapeManager;

class EraserTool : public QObject
{
    Q_OBJECT
public:
    explicit EraserTool(ShapeManager *shapeManager, QObject *parent = nullptr);

    void setEraserRadius(qreal radius);
    qreal eraserRadius() const;

    void applyEraserAt(const QPointF &center);
    void eraseAlong(const QPointF &from, const QPointF &to);

private:
    ShapeManager *m_shapeManager = nullptr;
    qreal m_eraseRadius = 20.0;
};

#endif // ERASERTOOL_H
