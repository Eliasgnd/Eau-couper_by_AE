#ifndef SHAPEMANAGER_H
#define SHAPEMANAGER_H

#include <QObject>
#include <QList>
#include <QPainterPath>
#include <QRectF>
#include <QVector>

class ShapeManager : public QObject
{
    Q_OBJECT
public:
    struct Shape {
        QPainterPath path;
        int originalId = -1;
        qreal rotationAngle = 0.0;
    };

    explicit ShapeManager(QObject *parent = nullptr);

    const QList<Shape> &shapes() const;
    void setShapes(const QList<Shape> &shapes);
    void clearShapes();
    void addShape(const Shape &shape);
    bool removeShape(int index);

    const QVector<int> &selectedShapes() const;
    void setSelectedShapes(const QVector<int> &indices);
    void clearSelection();
    QRectF selectedShapesBounds() const;

    void addImportedLogo(const QPainterPath &logoPath, int originalId);
    void addImportedLogoSubpath(const QPainterPath &subpath, int originalId);

    void copySelectedShapes();
    QList<Shape> pastedShapes(const QPointF &dest) const;

    void connectNearestEndpoints(int idx1, int idx2);
    void mergeShapesAndConnector(int idx1, int idx2);

signals:
    void shapesChanged();
    void selectionChanged();

private:
    QList<Shape> m_shapes;
    QVector<int> m_selectedShapes;
    QList<Shape> m_copiedShapes;
    QPointF m_copyAnchor;
};

#endif // SHAPEMANAGER_H
