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

    // Shape collection lifecycle.
    void addShape(const QPainterPath &path, int originalId);
    void addShape(const Shape &shape);
    bool removeShape(int index);
    void clearShapes();
    void setShapes(const QList<Shape> &shapes);
    const QList<Shape> &shapes() const;

    // Selection lifecycle.
    void selectShape(int index);
    void deselectShape(int index);
    void setSelectedShapes(const QVector<int> &indices);
    void clearSelection();
    const QVector<int> &selectedShapes() const;
    QRectF selectedShapesBounds() const;

    // Undo/redo snapshot stack (state-only for now).
    void pushState();
    void undoLastAction();

    // Import helpers.
    void addImportedLogo(const QPainterPath &logoPath, int originalId);
    void addImportedLogoSubpath(const QPainterPath &subpath, int originalId);

    // Selection copy/paste helpers.
    void copySelectedShapes();
    QList<Shape> pastedShapes(const QPointF &dest) const;

    // Connection helpers.
    void connectNearestEndpoints(int idx1, int idx2);
    void mergeShapesAndConnector(int idx1, int idx2);

signals:
    void shapesChanged();
    void selectionChanged();

private:
    QList<Shape> m_shapes;
    QVector<int> m_selectedShapes;
    QList<QList<Shape>> m_undoStack;
    QList<Shape> m_copiedShapes;
    QPointF m_copyAnchor;
};

#endif // SHAPEMANAGER_H
