#include "ShapeManager.h"

#include <QLineF>

ShapeManager::ShapeManager(QObject *parent)
    : QObject(parent)
{
}

const QList<ShapeManager::Shape> &ShapeManager::shapes() const { return m_shapes; }

void ShapeManager::setShapes(const QList<Shape> &shapes)
{
    m_shapes = shapes;
    emit shapesChanged();
}

void ShapeManager::clearShapes()
{
    m_shapes.clear();
    m_selectedShapes.clear();
    emit shapesChanged();
    emit selectionChanged();
}

void ShapeManager::addShape(const Shape &shape)
{
    m_shapes.append(shape);
    emit shapesChanged();
}

bool ShapeManager::removeShape(int index)
{
    if (index < 0 || index >= m_shapes.size()) return false;
    m_shapes.removeAt(index);
    emit shapesChanged();
    return true;
}

const QVector<int> &ShapeManager::selectedShapes() const { return m_selectedShapes; }

void ShapeManager::setSelectedShapes(const QVector<int> &indices)
{
    m_selectedShapes = indices;
    emit selectionChanged();
}

void ShapeManager::clearSelection()
{
    m_selectedShapes.clear();
    emit selectionChanged();
}

QRectF ShapeManager::selectedShapesBounds() const
{
    QRectF bounds;
    bool first = true;
    for (int idx : m_selectedShapes) {
        if (idx < 0 || idx >= m_shapes.size()) continue;
        if (first) {
            bounds = m_shapes[idx].path.boundingRect();
            first = false;
        } else {
            bounds = bounds.united(m_shapes[idx].path.boundingRect());
        }
    }
    return bounds;
}

void ShapeManager::addImportedLogo(const QPainterPath &logoPath, int originalId)
{
    addShape({logoPath, originalId, 0.0});
}

void ShapeManager::addImportedLogoSubpath(const QPainterPath &subpath, int originalId)
{
    addShape({subpath, originalId, 0.0});
}

void ShapeManager::copySelectedShapes()
{
    m_copiedShapes.clear();
    if (m_selectedShapes.isEmpty()) return;

    m_copyAnchor = selectedShapesBounds().topLeft();
    for (int idx : m_selectedShapes) {
        if (idx >= 0 && idx < m_shapes.size()) m_copiedShapes.append(m_shapes[idx]);
    }
}

QList<ShapeManager::Shape> ShapeManager::pastedShapes(const QPointF &dest) const
{
    QList<Shape> pasted;
    const QPointF delta = dest - m_copyAnchor;
    for (const Shape &shape : m_copiedShapes) {
        Shape c = shape;
        c.path.translate(delta);
        pasted.append(c);
    }
    return pasted;
}

void ShapeManager::connectNearestEndpoints(int, int)
{
    emit shapesChanged();
}

void ShapeManager::mergeShapesAndConnector(int, int)
{
    emit shapesChanged();
}
