#include "ShapeManager.h"

#include "domain/geometry/GeometryUtils.h"

#include <QLineF>
#include <array>
#include <algorithm>
#include <limits>
#include <utility>
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

QPainterPath orientedPath(const QPainterPath &path, bool reverse)
{
    return reverse ? path.toReversed() : path;
}
}

ShapeManager::ShapeManager(QObject *parent)
    : QObject(parent)
{
}

void ShapeManager::Shape::refreshGeometryCache()
{
    bounds = path.boundingRect();
    elementCount = path.elementCount();
    proxyPath = QPainterPath();
    proxyDirty = true;
}

const QPainterPath &ShapeManager::Shape::renderPath(bool preferProxy) const
{
    if (!preferProxy || elementCount < 600)
        return path;

    if (proxyDirty) {
        proxyPath = buildProxyPath(path);
        proxyDirty = false;
    }
    return proxyPath.isEmpty() ? path : proxyPath;
}

void ShapeManager::refreshShape(Shape &shape) const
{
    shape.refreshGeometryCache();
}

void ShapeManager::notifyShapesChanged()
{
    if (m_bulkDepth > 0) {
        m_pendingShapesChanged = true;
        return;
    }
    emit shapesChanged();
}

void ShapeManager::notifySelectionChanged()
{
    if (m_bulkDepth > 0) {
        m_pendingSelectionChanged = true;
        return;
    }
    emit selectionChanged();
}

void ShapeManager::beginBulkUpdate()
{
    ++m_bulkDepth;
}

void ShapeManager::endBulkUpdate()
{
    if (m_bulkDepth <= 0)
        return;
    --m_bulkDepth;
    if (m_bulkDepth > 0)
        return;

    const bool shouldEmitShapesChanged = m_pendingShapesChanged;
    const bool shouldEmitSelectionChanged = m_pendingSelectionChanged;
    m_pendingShapesChanged = false;
    m_pendingSelectionChanged = false;
    if (shouldEmitShapesChanged)
        emit shapesChanged();
    if (shouldEmitSelectionChanged)
        emit selectionChanged();
}

void ShapeManager::restoreValidSelection()
{
    std::vector<int> preservedSelection;
    preservedSelection.reserve(m_selectedShapes.size());
    for (int idx : m_selectedShapes) {
        if (idx >= 0 && idx < static_cast<int>(m_shapes.size())
            && std::find(preservedSelection.begin(), preservedSelection.end(), idx) == preservedSelection.end()) {
            preservedSelection.push_back(idx);
        }
    }

    if (preservedSelection != m_selectedShapes) {
        m_selectedShapes = std::move(preservedSelection);
        notifySelectionChanged();
    } else {
        m_selectedShapes = std::move(preservedSelection);
    }
}

void ShapeManager::addShape(const QPainterPath &path, int originalId)
{
    addShape({path, originalId, 0.0});
}

void ShapeManager::addShape(const Shape &shape)
{
    Shape cached = shape;
    refreshShape(cached);
    m_shapes.push_back(cached);
    notifyShapesChanged();
}

bool ShapeManager::removeShape(int index)
{
    if (index < 0 || index >= static_cast<int>(m_shapes.size())) return false;
    m_shapes.erase(m_shapes.begin() + index);
    std::vector<int> updatedSelection;
    updatedSelection.reserve(m_selectedShapes.size());
    for (int selectedIndex : m_selectedShapes) {
        if (selectedIndex == index)
            continue;
        if (selectedIndex > index)
            --selectedIndex;
        updatedSelection.push_back(selectedIndex);
    }
    m_selectedShapes = std::move(updatedSelection);
    notifyShapesChanged();
    notifySelectionChanged();
    return true;
}

void ShapeManager::clearShapes()
{
    m_shapes.clear();
    m_selectedShapes.clear();
    notifyShapesChanged();
    notifySelectionChanged();
}

void ShapeManager::setShapes(const std::vector<Shape> &shapes)
{
    m_shapes = shapes;
    for (Shape &shape : m_shapes)
        refreshShape(shape);
    restoreValidSelection();
    notifyShapesChanged();
}

void ShapeManager::replaceShapesPreservingSelection(const std::vector<Shape> &shapes)
{
    setShapes(shapes);
}

void ShapeManager::appendShapes(const std::vector<Shape> &shapes)
{
    if (shapes.empty())
        return;

    m_shapes.reserve(m_shapes.size() + shapes.size());
    for (Shape shape : shapes) {
        refreshShape(shape);
        m_shapes.push_back(std::move(shape));
    }
    notifyShapesChanged();
}

const std::vector<ShapeManager::Shape> &ShapeManager::shapes() const { return m_shapes; }

std::vector<int> ShapeManager::visibleShapeIndices(const QRectF &visibleArea, qreal margin) const
{
    const QRectF expanded = visibleArea.adjusted(-margin, -margin, margin, margin);
    std::vector<int> indices;
    indices.reserve(m_shapes.size());
    for (int i = 0; i < static_cast<int>(m_shapes.size()); ++i) {
        if (m_shapes[i].bounds.isNull() || expanded.intersects(m_shapes[i].bounds))
            indices.push_back(i);
    }
    return indices;
}

void ShapeManager::selectShape(int index)
{
    if (index < 0 || index >= m_shapes.size()) return;
    if (std::find(m_selectedShapes.begin(), m_selectedShapes.end(), index) != m_selectedShapes.end()) return;
    m_selectedShapes.push_back(index);
    notifySelectionChanged();
}

void ShapeManager::deselectShape(int index)
{
    const auto oldSize = m_selectedShapes.size();
    m_selectedShapes.erase(std::remove(m_selectedShapes.begin(), m_selectedShapes.end(), index), m_selectedShapes.end());
    if (oldSize != m_selectedShapes.size()) notifySelectionChanged();
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
    notifySelectionChanged();
}

void ShapeManager::clearSelection()
{
    if (m_selectedShapes.empty()) return;
    m_selectedShapes.clear();
    notifySelectionChanged();
}

const std::vector<int> &ShapeManager::selectedShapes() const { return m_selectedShapes; }

void ShapeManager::translateSelectedShapes(const QPointF &delta)
{
    if (m_selectedShapes.empty() || (qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y())))
        return;

    for (int idx : m_selectedShapes) {
        if (idx < 0 || idx >= static_cast<int>(m_shapes.size()))
            continue;
        m_shapes[idx].path.translate(delta);
        refreshShape(m_shapes[idx]);
    }
    notifyShapesChanged();
}

QRectF ShapeManager::selectedShapesBounds() const
{
    QRectF bounds;
    bool initialized = false;
    for (int idx : m_selectedShapes) {
        if (idx < 0 || idx >= m_shapes.size()) continue;
        bounds = initialized ? bounds.united(m_shapes[idx].bounds) : m_shapes[idx].bounds;
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
    for (Shape &shape : m_shapes)
        refreshShape(shape);
    m_undoStack.pop_back();
    m_selectedShapes.clear();
    notifyShapesChanged();
    notifySelectionChanged();
}

void ShapeManager::addImportedLogo(const QPainterPath &logoPath, int originalId)
{
    addShape(logoPath, originalId);
}

void ShapeManager::addImportedLogoSubpath(const QPainterPath &subpath, int originalId)
{
    addShape(subpath, originalId);
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
    Shape shape{merged, -1, 0.0};
    refreshShape(shape);
    m_shapes.push_back(shape);

    m_selectedShapes.clear();
    notifyShapesChanged();
    notifySelectionChanged();
}

void ShapeManager::mergeShapesByNearestEndpoints(int idx1, int idx2)
{
    if (idx1 < 0 || idx1 >= m_shapes.size() || idx2 < 0 || idx2 >= m_shapes.size() || idx1 == idx2) return;

    const int first = qMin(idx1, idx2);
    const int second = qMax(idx1, idx2);

    const QPainterPath &a = m_shapes[first].path;
    const QPainterPath &b = m_shapes[second].path;
    if (a.isEmpty() || b.isEmpty()) return;

    struct Candidate {
        qreal distance;
        bool reverseA;
        bool reverseB;
    };

    const std::array<Candidate, 4> candidates = {{
        {QLineF(pathStart(a), pathStart(b)).length(), true,  false},
        {QLineF(pathStart(a), pathEnd(b)).length(),   true,  true},
        {QLineF(pathEnd(a),   pathStart(b)).length(), false, false},
        {QLineF(pathEnd(a),   pathEnd(b)).length(),   false, true}
    }};

    const auto best = std::min_element(candidates.begin(), candidates.end(),
                                       [](const Candidate &lhs, const Candidate &rhs) {
                                           return lhs.distance < rhs.distance;
                                       });

    QPainterPath merged = orientedPath(a, best->reverseA);
    merged.connectPath(orientedPath(b, best->reverseB));

    const int mergedId = m_shapes[first].originalId >= 0
        ? m_shapes[first].originalId
        : m_shapes[second].originalId;
    const qreal mergedRotation = m_shapes[first].rotationAngle;

    m_shapes.erase(m_shapes.begin() + second);
    m_shapes.erase(m_shapes.begin() + first);
    Shape shape{merged, mergedId, mergedRotation};
    refreshShape(shape);
    m_shapes.push_back(shape);

    m_selectedShapes.clear();
    notifyShapesChanged();
    notifySelectionChanged();
}
