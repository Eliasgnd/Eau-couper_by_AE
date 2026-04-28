#include "CanvasViewModel.h"
#include "domain/geometry/GeometryUtils.h"
#include <algorithm>
#include <cmath>

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

std::vector<bool> computeCollisionFlags(const std::vector<ShapeManager::Shape> &shapes, int *pairCount = nullptr)
{
    std::vector<bool> collision(shapes.size(), false);
    int count = 0;

    for (int i = 0; i < static_cast<int>(shapes.size()); ++i) {
        for (int j = i + 1; j < static_cast<int>(shapes.size()); ++j) {
            const QRectF a = shapes[i].bounds;
            const QRectF b = shapes[j].bounds;
            if (!a.adjusted(-1.0, -1.0, 1.0, 1.0).intersects(b.adjusted(-1.0, -1.0, 1.0, 1.0)))
                continue;
            if (!pathsOverlap(shapes[i].path, shapes[j].path, 0.75))
                continue;

            collision[i] = true;
            collision[j] = true;
            ++count;
        }
    }

    if (pairCount)
        *pairCount = count;
    return collision;
}
} // namespace

CanvasViewModel::CanvasViewModel(QObject *parent)
    : QObject(parent)
    , m_shapeManager(std::make_unique<ShapeManager>())
    , m_historyManager(std::make_unique<HistoryManager>(m_shapeManager.get()))
{
    connect(m_shapeManager.get(), &ShapeManager::shapesChanged, this, [this]() {
        m_collisionCacheDirty = true;
        emit shapesChanged();
        emit stateChanged();
    });
    connect(m_shapeManager.get(), &ShapeManager::selectionChanged, this, [this]() {
        emit selectionChanged();
        emit stateChanged();
    });
}

CanvasViewModel::~CanvasViewModel() = default;

ShapeManager* CanvasViewModel::shapeManager() const { return m_shapeManager.get(); }
const std::vector<ShapeManager::Shape>& CanvasViewModel::shapes() const { return m_shapeManager->shapes(); }
const std::vector<int>& CanvasViewModel::selectedShapes() const { return m_shapeManager->selectedShapes(); }
bool CanvasViewModel::hasSelection() const { return !m_shapeManager->selectedShapes().empty(); }
QRectF CanvasViewModel::selectedShapesBounds() const { return m_shapeManager->selectedShapesBounds(); }

bool CanvasViewModel::canUndo() const { return m_historyManager->canUndo(); }
bool CanvasViewModel::canRedo() const { return m_historyManager->canRedo(); }
QString CanvasViewModel::undoText() const { return m_historyManager->undoText(); }
QString CanvasViewModel::redoText() const { return m_historyManager->redoText(); }

int CanvasViewModel::smoothingLevel() const { return m_smoothingLevel; }
bool CanvasViewModel::isPrecisionConstraintEnabled() const { return m_precisionConstraintEnabled; }
bool CanvasViewModel::isSegmentStatusVisible() const { return m_segmentStatusVisible; }
PerformanceMode CanvasViewModel::performanceMode() const { return m_performanceMode; }
int CanvasViewModel::nextShapeId() { return m_nextShapeId++; }

void CanvasViewModel::setSmoothingLevel(int level)
{
    m_smoothingLevel = qMax(0, level);
    emit smoothingLevelChanged(m_smoothingLevel);
    emit stateChanged();
}

void CanvasViewModel::setPrecisionConstraintEnabled(bool enabled)
{
    m_precisionConstraintEnabled = enabled;
    emit stateChanged();
}

void CanvasViewModel::setSegmentStatusVisible(bool visible)
{
    m_segmentStatusVisible = visible;
    emit stateChanged();
}

void CanvasViewModel::setPerformanceMode(PerformanceMode mode)
{
    m_performanceMode = mode;
    emit performanceModeChanged(mode);
    emit stateChanged();
}

void CanvasViewModel::clearDrawing()
{
    auto oldState = m_historyManager->getCurrentState();
    m_shapeManager->clearShapes();
    m_historyManager->commitSnapshot(oldState, {}, tr("Tout effacer"));
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::undo()
{
    m_historyManager->undo();
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::redo()
{
    m_historyManager->redo();
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::addShape(const QPainterPath& path, int id)
{
    m_historyManager->commitAddShape(path, id);
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::addImportedLogo(const QPainterPath& logoPath)
{
    const int id = m_nextShapeId++;
    m_shapeManager->addImportedLogo(logoPath, id);
    m_historyManager->commitAddShape(logoPath, id, tr("Import Logo"));
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::addImportedLogoSubpath(const QPainterPath& subpath)
{
    const int id = m_nextShapeId++;
    m_shapeManager->addImportedLogoSubpath(subpath, id);
    m_historyManager->commitAddShape(subpath, id, tr("Import Sous-trace"));
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::addImportedLogoSubpaths(const QList<QPainterPath>& subpaths)
{
    if (subpaths.isEmpty()) return;

    const auto oldState = m_historyManager->getCurrentState();
    auto updated = oldState;
    updated.reserve(updated.size() + subpaths.size());
    for (const QPainterPath &subpath : subpaths) {
        if (subpath.isEmpty()) continue;
        updated.push_back({subpath, m_nextShapeId++, 0.0});
    }

    if (updated.size() == oldState.size()) return;

    m_shapeManager->replaceShapesPreservingSelection(updated);
    m_historyManager->commitSnapshot(oldState, updated, tr("Import traces"));
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::deleteSelectedShapes()
{
    const auto selected = m_shapeManager->selectedShapes();
    if (selected.empty()) return;

    m_historyManager->commitDeleteShapes(selected);
    m_shapeManager->clearSelection();
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::duplicateSelectedShapes()
{
    if (!hasSelection()) return;
    duplicateSelectedShapesLinear(1, QPointF(qMax<qreal>(40.0, selectedShapesBounds().width() * 0.15),
                                             qMax<qreal>(40.0, selectedShapesBounds().height() * 0.15)));
}

void CanvasViewModel::duplicateSelectedShapesLinear(int copies, const QPointF& step)
{
    if (!hasSelection() || copies <= 0) return;

    const auto oldState = m_historyManager->getCurrentState();
    const auto baseShapes = m_shapeManager->shapes();
    auto updated = baseShapes;
    const auto selected = m_shapeManager->selectedShapes();

    for (int copyIndex = 1; copyIndex <= copies; ++copyIndex) {
        const QPointF delta(step.x() * copyIndex, step.y() * copyIndex);
        for (int idx : selected) {
            if (idx < 0 || idx >= static_cast<int>(baseShapes.size())) continue;
            auto clone = baseShapes[idx];
            clone.path.translate(delta);
            clone.originalId = m_nextShapeId++;
            updated.push_back(clone);
        }
    }

    m_shapeManager->setShapes(updated);
    m_historyManager->commitSnapshot(oldState, updated, tr("Dupliquer en serie"));
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::resizeSelectedShapes(qreal targetWidth, qreal targetHeight)
{
    if (!hasSelection()) return;

    const QRectF bounds = selectedShapesBounds();
    if (bounds.width() <= 0.0 || bounds.height() <= 0.0) return;

    const qreal safeWidth = qMax<qreal>(1.0, targetWidth);
    const qreal safeHeight = qMax<qreal>(1.0, targetHeight);
    const qreal scaleX = safeWidth / bounds.width();
    const qreal scaleY = safeHeight / bounds.height();

    auto updated = m_shapeManager->shapes();
    const auto selected = m_shapeManager->selectedShapes();
    for (int idx : selected) {
        if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
        QTransform transform;
        transform.translate(bounds.left(), bounds.top());
        transform.scale(scaleX, scaleY);
        transform.translate(-bounds.left(), -bounds.top());
        updated[idx].path = transform.map(updated[idx].path);
    }

    commitSelectedTransform(updated, tr("Redimensionner forme"));
}

void CanvasViewModel::rotateSelectedShapes(qreal angleDegrees)
{
    if (!hasSelection()) return;

    const QRectF bounds = selectedShapesBounds();
    if (!bounds.isValid()) return;

    auto updated = m_shapeManager->shapes();
    const auto selected = m_shapeManager->selectedShapes();
    const QPointF center = bounds.center();

    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(angleDegrees);
    transform.translate(-center.x(), -center.y());

    for (int idx : selected) {
        if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
        updated[idx].path = transform.map(updated[idx].path);
        updated[idx].rotationAngle += angleDegrees;
    }

    commitSelectedTransform(updated, tr("Tourner forme"));
}

void CanvasViewModel::moveSelectedShapes(qreal dx, qreal dy, const QString& label)
{
    if (!hasSelection()) return;

    auto updated = m_shapeManager->shapes();
    const auto selected = m_shapeManager->selectedShapes();
    for (int idx : selected) {
        if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
        updated[idx].path.translate(dx, dy);
    }

    commitSelectedTransform(updated, label.isEmpty() ? tr("Deplacer forme") : label);
}

void CanvasViewModel::setSelectedShapesPosition(qreal x, qreal y)
{
    if (!hasSelection()) return;
    const QRectF bounds = selectedShapesBounds();
    moveSelectedShapes(x - bounds.left(), y - bounds.top(), tr("Positionner forme"));
}

void CanvasViewModel::alignSelectedLeft()
{
    if (!hasSelection()) return;
    const QRectF selectionBounds = selectedShapesBounds();
    auto updated = m_shapeManager->shapes();
    for (int idx : m_shapeManager->selectedShapes()) {
        if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
        const QRectF bounds = updated[idx].path.boundingRect();
        updated[idx].path.translate(selectionBounds.left() - bounds.left(), 0.0);
    }
    commitSelectedTransform(updated, tr("Aligner a gauche"));
}

void CanvasViewModel::alignSelectedHCenter()
{
    if (!hasSelection()) return;
    const qreal centerX = selectedShapesBounds().center().x();
    auto updated = m_shapeManager->shapes();
    for (int idx : m_shapeManager->selectedShapes()) {
        if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
        const QRectF bounds = updated[idx].path.boundingRect();
        updated[idx].path.translate(centerX - bounds.center().x(), 0.0);
    }
    commitSelectedTransform(updated, tr("Aligner au centre horizontal"));
}

void CanvasViewModel::alignSelectedTop()
{
    if (!hasSelection()) return;
    const QRectF selectionBounds = selectedShapesBounds();
    auto updated = m_shapeManager->shapes();
    for (int idx : m_shapeManager->selectedShapes()) {
        if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
        const QRectF bounds = updated[idx].path.boundingRect();
        updated[idx].path.translate(0.0, selectionBounds.top() - bounds.top());
    }
    commitSelectedTransform(updated, tr("Aligner en haut"));
}

void CanvasViewModel::alignSelectedVCenter()
{
    if (!hasSelection()) return;
    const qreal centerY = selectedShapesBounds().center().y();
    auto updated = m_shapeManager->shapes();
    for (int idx : m_shapeManager->selectedShapes()) {
        if (idx < 0 || idx >= static_cast<int>(updated.size())) continue;
        const QRectF bounds = updated[idx].path.boundingRect();
        updated[idx].path.translate(0.0, centerY - bounds.center().y());
    }
    commitSelectedTransform(updated, tr("Aligner au centre vertical"));
}

void CanvasViewModel::distributeSelectedHorizontally()
{
    const auto selected = m_shapeManager->selectedShapes();
    if (selected.size() < 3) return;

    std::vector<int> ordered = selected;
    const auto& currentShapes = m_shapeManager->shapes();
    std::sort(ordered.begin(), ordered.end(), [&](int a, int b) {
        return currentShapes[a].path.boundingRect().center().x() < currentShapes[b].path.boundingRect().center().x();
    });

    const qreal start = currentShapes[ordered.front()].path.boundingRect().center().x();
    const qreal end = currentShapes[ordered.back()].path.boundingRect().center().x();
    const qreal step = (end - start) / (ordered.size() - 1);

    auto updated = currentShapes;
    for (int i = 1; i < static_cast<int>(ordered.size()) - 1; ++i) {
        const QRectF bounds = updated[ordered[i]].path.boundingRect();
        updated[ordered[i]].path.translate(start + step * i - bounds.center().x(), 0.0);
    }
    commitSelectedTransform(updated, tr("Distribuer horizontalement"));
}

void CanvasViewModel::distributeSelectedVertically()
{
    const auto selected = m_shapeManager->selectedShapes();
    if (selected.size() < 3) return;

    std::vector<int> ordered = selected;
    const auto& currentShapes = m_shapeManager->shapes();
    std::sort(ordered.begin(), ordered.end(), [&](int a, int b) {
        return currentShapes[a].path.boundingRect().center().y() < currentShapes[b].path.boundingRect().center().y();
    });

    const qreal start = currentShapes[ordered.front()].path.boundingRect().center().y();
    const qreal end = currentShapes[ordered.back()].path.boundingRect().center().y();
    const qreal step = (end - start) / (ordered.size() - 1);

    auto updated = currentShapes;
    for (int i = 1; i < static_cast<int>(ordered.size()) - 1; ++i) {
        const QRectF bounds = updated[ordered[i]].path.boundingRect();
        updated[ordered[i]].path.translate(0.0, start + step * i - bounds.center().y());
    }
    commitSelectedTransform(updated, tr("Distribuer verticalement"));
}

void CanvasViewModel::commitTransform(const std::vector<ShapeManager::Shape>& updated, const QString& label)
{
    const auto oldState = m_historyManager->getCurrentState();
    m_shapeManager->setShapes(updated);
    m_historyManager->commitSnapshot(oldState, updated, label);
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::commitSelectedTransform(const std::vector<ShapeManager::Shape>& updated, const QString& label)
{
    const auto oldState = m_historyManager->getCurrentState();
    const auto selected = m_shapeManager->selectedShapes();
    m_shapeManager->setShapes(updated);
    m_shapeManager->setSelectedShapes(selected);
    m_historyManager->commitSnapshot(oldState, updated, label);
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}

void CanvasViewModel::extendShape(int shapeIndex, const QPainterPath& newPath, bool fromStart)
{
    const auto oldState = m_historyManager->getCurrentState();
    if (shapeIndex >= 0 && shapeIndex < static_cast<int>(oldState.size())) {
        auto updated = oldState;
        QPainterPath path = oldState[shapeIndex].path;
        if (fromStart) path = path.toReversed();
        path.connectPath(newPath);
        updated[shapeIndex].path = path;
        
        m_shapeManager->setShapes(updated);
        m_historyManager->commitSnapshot(oldState, updated, tr("Prolonger trace"));
        emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
        emit stateChanged();
    }
}

bool CanvasViewModel::isPathClosed(const QPainterPath &path) const
{
    if (path.elementCount() < 3) return false;
    return QLineF(pathStart(path), pathEnd(path)).length() <= 1.5;
}

std::vector<bool> CanvasViewModel::getCollisionFlags(int *pairCount) const
{
    if (m_collisionCacheDirty) {
        m_collisionCache = computeCollisionFlags(m_shapeManager->shapes(), &m_collisionPairCount);
        m_collisionCacheDirty = false;
    }
    if (pairCount)
        *pairCount = m_collisionPairCount;
    return m_collisionCache;
}

bool CanvasViewModel::hasValidationIssues() const
{
    const auto &currentShapes = m_shapeManager->shapes();
    for (const auto &shape : currentShapes) {
        const QRectF bounds = shape.bounds;
        if (!isPathClosed(shape.path) || bounds.width() < 12.0 || bounds.height() < 12.0)
            return true;
    }
    const auto collision = getCollisionFlags();
    return std::any_of(collision.begin(), collision.end(), [](bool flagged) { return flagged; });
}

QString CanvasViewModel::validationSummary() const
{
    const auto &currentShapes = m_shapeManager->shapes();
    int openCount = 0;
    int smallCount = 0;
    QRectF globalBounds;
    bool initialized = false;

    for (const auto &shape : currentShapes) {
        const QRectF bounds = shape.bounds;
        globalBounds = initialized ? globalBounds.united(bounds) : bounds;
        initialized = true;
        if (!isPathClosed(shape.path))
            ++openCount;
        if (bounds.width() < 12.0 || bounds.height() < 12.0)
            ++smallCount;
    }

    int closeCount = 0;
    getCollisionFlags(&closeCount);

    QStringList lines;
    lines << tr("Formes : %1").arg(currentShapes.size());
    lines << tr("Dimensions globales : %1 x %2 px")
                 .arg(qRound(globalBounds.width()))
                 .arg(qRound(globalBounds.height()));
    lines << tr("Formes fermees : %1").arg(currentShapes.size() - openCount);
    lines << tr("Formes ouvertes : %1").arg(openCount);
    if (smallCount > 0)
        lines << tr("Formes tres petites : %1").arg(smallCount);
    if (closeCount > 0)
        lines << tr("Elements proches/superposes : %1").arg(closeCount);
    if (hasValidationIssues())
        lines << tr("Verification conseillee avant application.");
    else
        lines << tr("Aucune alerte detectee.");
    return lines.join('\n');
}


std::vector<ShapeManager::Shape> CanvasViewModel::getCurrentState() const { return m_historyManager->getCurrentState(); }
void CanvasViewModel::commitSnapshot(const std::vector<ShapeManager::Shape>& oldState, const std::vector<ShapeManager::Shape>& newState, const QString& label) {
    m_historyManager->commitSnapshot(oldState, newState, label);
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}
void CanvasViewModel::commitAddShape(const QPainterPath &path, int id, const QString &label) {
    m_historyManager->commitAddShape(path, id, label);
    emit historyStateChanged(canUndo(), undoText(), canRedo(), redoText());
    emit stateChanged();
}
