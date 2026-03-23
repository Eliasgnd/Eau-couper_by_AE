#ifndef SHAPEMANAGER_H
#define SHAPEMANAGER_H

#include <QObject>
#include <QPainterPath>
#include <QRectF>
#include <vector>

/**
 * @class ShapeManager
 * @brief Gère la liste des formes dessinées, la sélection et l'historique d'édition.
 *
 * @note Cette classe émet des signaux pour notifier les changements de formes
 * et de sélection.
 * @see CustomDrawArea
 */
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
    void setShapes(const std::vector<Shape> &shapes);
    const std::vector<Shape> &shapes() const;

    // Selection lifecycle.
    void selectShape(int index);
    void deselectShape(int index);
    void setSelectedShapes(const std::vector<int> &indices);
    void clearSelection();
    const std::vector<int> &selectedShapes() const;
    QRectF selectedShapesBounds() const;

    // Undo/redo snapshot stack (state-only for now).
    void pushState();
    void undoLastAction();

    // Import helpers.
    void addImportedLogo(const QPainterPath &logoPath, int originalId);
    void addImportedLogoSubpath(const QPainterPath &subpath, int originalId);

    // Selection copy/paste helpers.
    void copySelectedShapes();
    std::vector<Shape> pastedShapes(const QPointF &dest) const;

    // Connection helpers.
    void connectNearestEndpoints(int idx1, int idx2);
    void mergeShapesAndConnector(int idx1, int idx2);

signals:
    void shapesChanged();
    void selectionChanged();

private:
    std::vector<Shape> m_shapes;
    std::vector<int> m_selectedShapes;
    std::vector<std::vector<Shape>> m_undoStack;
    std::vector<Shape> m_copiedShapes;
    QPointF m_copyAnchor;
};

#endif // SHAPEMANAGER_H
