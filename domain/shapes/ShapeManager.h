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
        QRectF bounds;
        int elementCount = 0;
        mutable QPainterPath proxyPath;
        mutable bool proxyDirty = true;

        void refreshGeometryCache();
        const QPainterPath &renderPath(bool preferProxy) const;
    };

    explicit ShapeManager(QObject *parent = nullptr);

    // Shape collection lifecycle.
    void addShape(const QPainterPath &path, int originalId);
    void addShape(const Shape &shape);
    bool removeShape(int index);
    void clearShapes();
    void setShapes(const std::vector<Shape> &shapes);
    void replaceShapesPreservingSelection(const std::vector<Shape> &shapes);
    void appendShapes(const std::vector<Shape> &shapes);
    const std::vector<Shape> &shapes() const;
    std::vector<int> visibleShapeIndices(const QRectF &visibleArea, qreal margin = 0.0) const;
    void beginBulkUpdate();
    void endBulkUpdate();

    // Selection lifecycle.
    void selectShape(int index);
    void deselectShape(int index);
    void setSelectedShapes(const std::vector<int> &indices);
    void clearSelection();
    const std::vector<int> &selectedShapes() const;
    QRectF selectedShapesBounds() const;
    void translateSelectedShapes(const QPointF &delta);

    // Undo/redo snapshot stack (state-only for now).
    void pushState();
    void undoLastAction();

    // Import helpers.
    void addImportedLogo(const QPainterPath &logoPath, int originalId);
    void addImportedLogoSubpath(const QPainterPath &subpath, int originalId);

    // Connection helpers.
    void connectNearestEndpoints(int idx1, int idx2);
    void mergeShapesAndConnector(int idx1, int idx2);
    void mergeShapesByNearestEndpoints(int idx1, int idx2);

signals:
    void shapesChanged();
    void selectionChanged();

private:
    void refreshShape(Shape &shape) const;
    void notifyShapesChanged();
    void notifySelectionChanged();
    void restoreValidSelection();

    std::vector<Shape> m_shapes;
    std::vector<int> m_selectedShapes;
    std::vector<std::vector<Shape>> m_undoStack;
    int m_bulkDepth = 0;
    bool m_pendingShapesChanged = false;
    bool m_pendingSelectionChanged = false;
};

#endif // SHAPEMANAGER_H
