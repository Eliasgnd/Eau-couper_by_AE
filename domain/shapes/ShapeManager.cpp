#include "ShapeManager.h"

#include <QLineF>
#include <algorithm>
#include <limits>
#include <QLoggingCategory>
#include <QtMath>

Q_LOGGING_CATEGORY(lcShapeManager, "ae.draw.shape_manager")

namespace {
QPointF pathStart(const QPainterPath &path)
{
    if (path.elementCount() == 0) return {};
    const auto e = path.elementAt(0);
    return {e.x, e.y};
}

QPointF pathEnd(const QPainterPath &path)
{
    if (path.elementCount() == 0) return {};
    const auto e = path.elementAt(path.elementCount() - 1);
    return {e.x, e.y};
}
}

ShapeManager::ShapeManager(QObject *parent)
    : QObject(parent)
{
}

void ShapeManager::addShape(const QPainterPath &path, int originalId)
{
    addShape({path, originalId, 0.0});
}

void ShapeManager::addShape(const Shape &shape)
{
    m_shapes.push_back(shape);
    emit shapesChanged();
}

bool ShapeManager::removeShape(int index)
{
    if (index < 0 || index >= m_shapes.size()) return false;
    m_shapes.erase(m_shapes.begin() + index);
    m_selectedShapes.erase(std::remove_if(m_selectedShapes.begin(), m_selectedShapes.end(),
                                          [index](int i) { return i == index; }),
                           m_selectedShapes.end());
    emit shapesChanged();
    emit selectionChanged();
    return true;
}

void ShapeManager::clearShapes()
{
    m_shapes.clear();
    m_selectedShapes.clear();
    emit shapesChanged();
    emit selectionChanged();
}

void ShapeManager::setShapes(const std::vector<Shape> &shapes)
{
    m_shapes = shapes;
    m_selectedShapes.clear();
    emit shapesChanged();
    emit selectionChanged();
}

const std::vector<ShapeManager::Shape> &ShapeManager::shapes() const { return m_shapes; }

void ShapeManager::selectShape(int index)
{
    if (index < 0 || index >= m_shapes.size()) return;
    if (std::find(m_selectedShapes.begin(), m_selectedShapes.end(), index) != m_selectedShapes.end()) return;
    m_selectedShapes.push_back(index);
    emit selectionChanged();
}

void ShapeManager::deselectShape(int index)
{
    const int oldSize = m_selectedShapes.size();
    m_selectedShapes.erase(std::remove(m_selectedShapes.begin(), m_selectedShapes.end(), index), m_selectedShapes.end());
    if (oldSize != m_selectedShapes.size()) emit selectionChanged();
}

void ShapeManager::setSelectedShapes(const std::vector<int> &indices)
{
    m_selectedShapes.clear();
    m_selectedShapes.reserve(indices.size());
    for (const int idx : indices) {
        if (idx >= 0 && idx < m_shapes.size()
            && std::find(m_selectedShapes.begin(), m_selectedShapes.end(), idx) == m_selectedShapes.end()) {
            m_selectedShapes.push_back(idx);
        }
    }
    emit selectionChanged();
}

void ShapeManager::clearSelection()
{
    if (m_selectedShapes.empty()) return;
    m_selectedShapes.clear();
    emit selectionChanged();
}

const std::vector<int> &ShapeManager::selectedShapes() const { return m_selectedShapes; }

QRectF ShapeManager::selectedShapesBounds() const
{
    QRectF bounds;
    bool initialized = false;
    for (int idx : m_selectedShapes) {
        if (idx < 0 || idx >= m_shapes.size()) continue;
        bounds = initialized ? bounds.united(m_shapes[idx].path.boundingRect()) : m_shapes[idx].path.boundingRect();
        initialized = true;
    }
    return bounds;
}

void ShapeManager::pushState()
{
    m_undoStack.push_back(m_shapes);
}

void ShapeManager::undoLastAction()
{
    if (m_undoStack.empty()) return;
    m_shapes = m_undoStack.back();
    m_undoStack.pop_back();
    m_selectedShapes.clear();
    emit shapesChanged();
    emit selectionChanged();
}

void ShapeManager::addImportedLogo(const QPainterPath &logoPath, int originalId)
{
    addShape(logoPath, originalId);
}

void ShapeManager::addImportedLogoSubpath(const QPainterPath &subpath, int originalId)
{
    addShape(subpath, originalId);
}

void ShapeManager::copySelectedShapes()
{
    m_copiedShapes.clear();
    if (m_selectedShapes.empty()) return;

    m_copyAnchor = selectedShapesBounds().topLeft();
    m_copiedShapes.reserve(m_selectedShapes.size());
    for (int idx : m_selectedShapes) {
        if (idx >= 0 && idx < m_shapes.size()) m_copiedShapes.push_back(m_shapes[idx]);
    }
}

std::vector<ShapeManager::Shape> ShapeManager::pastedShapes(const QPointF &dest) const
{
    std::vector<Shape> pasted;
    pasted.reserve(m_copiedShapes.size());
    const QPointF delta = dest - m_copyAnchor;
    for (const Shape &shape : m_copiedShapes) {
        Shape clone = shape;
        clone.path.translate(delta);
        pasted.push_back(clone);
    }
    return pasted;
}

void ShapeManager::connectNearestEndpoints(int idx1, int idx2)
{
    if (idx1 < 0 || idx1 >= m_shapes.size() || idx2 < 0 || idx2 >= m_shapes.size() || idx1 == idx2) return;

    const QPainterPath &a = m_shapes[idx1].path;
    const QPainterPath &b = m_shapes[idx2].path;
    if (a.isEmpty() || b.isEmpty()) return;

    const QPointF aStart = pathStart(a);
    const QPointF aEnd = pathEnd(a);
    const QPointF bStart = pathStart(b);
    const QPointF bEnd = pathEnd(b);

    const std::vector<QPair<QPointF, QPointF>> candidatePairs = {
        {aStart, bStart}, {aStart, bEnd}, {aEnd, bStart}, {aEnd, bEnd}
    };

    qreal minDistance = std::numeric_limits<qreal>::max();
    QPair<QPointF, QPointF> bestPair;
    for (const auto &pair : candidatePairs) {
        const qreal d = QLineF(pair.first, pair.second).length();
        if (d < minDistance) {
            minDistance = d;
            bestPair = pair;
        }
    }

    QPainterPath connector;
    connector.moveTo(bestPair.first);
    connector.lineTo(bestPair.second);
    addShape(connector, -1);

    qCDebug(lcShapeManager) << "Connector created between" << idx1 << "and" << idx2;
}

void ShapeManager::mergeShapesAndConnector(int idx1, int idx2)
{
    if (idx1 < 0 || idx1 >= m_shapes.size() || idx2 < 0 || idx2 >= m_shapes.size() || idx1 == idx2) return;

    const int first = qMin(idx1, idx2);
    const int second = qMax(idx1, idx2);

    QPainterPath merged = m_shapes[first].path;
    merged.connectPath(m_shapes[second].path);

    m_shapes.erase(m_shapes.begin() + second);
    m_shapes.erase(m_shapes.begin() + first);
    m_shapes.push_back({merged, -1, 0.0});

    m_selectedShapes.clear();
    emit shapesChanged();
    emit selectionChanged();
}
