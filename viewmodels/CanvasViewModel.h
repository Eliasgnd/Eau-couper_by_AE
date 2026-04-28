#ifndef CANVASVIEWMODEL_H
#define CANVASVIEWMODEL_H

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QPainterPath>
#include <QList>
#include <memory>
#include <vector>

#include "domain/shapes/ShapeManager.h"
#include "ui/canvas/HistoryManager.h"
#include "shared/PerformanceMode.h"

class CanvasViewModel : public QObject
{
    Q_OBJECT
public:
    explicit CanvasViewModel(QObject *parent = nullptr);
    ~CanvasViewModel() override;

    // === Access to ShapeManager (read-only for view) ===
    ShapeManager* shapeManager() const;
    const std::vector<ShapeManager::Shape>& shapes() const;
    const std::vector<int>& selectedShapes() const;
    bool hasSelection() const;
    QRectF selectedShapesBounds() const;

    // === Access to HistoryManager ===
    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;

    // === Canvas State ===
    int smoothingLevel() const;
    bool isPrecisionConstraintEnabled() const;
    bool isSegmentStatusVisible() const;
    PerformanceMode performanceMode() const;
    int nextShapeId();

    // === Commands (Mutations) ===
    void clearDrawing();
    void undo();
    void redo();
    void addShape(const QPainterPath& path, int id);
    void addImportedLogo(const QPainterPath& logoPath);
    void addImportedLogoSubpath(const QPainterPath& subpath);
    void addImportedLogoSubpaths(const QList<QPainterPath>& subpaths);
    void deleteSelectedShapes();
    void duplicateSelectedShapes();
    void duplicateSelectedShapesLinear(int copies, const QPointF& step);
    void resizeSelectedShapes(qreal targetWidth, qreal targetHeight);
    void rotateSelectedShapes(qreal angleDegrees);
    void moveSelectedShapes(qreal dx, qreal dy, const QString& label = QString());
    void setSelectedShapesPosition(qreal x, qreal y);
    void alignSelectedLeft();
    void alignSelectedHCenter();
    void alignSelectedTop();
    void alignSelectedVCenter();
    void distributeSelectedHorizontally();
    void distributeSelectedVertically();
    void commitTransform(const std::vector<ShapeManager::Shape>& updated, const QString& label);
    void commitSelectedTransform(const std::vector<ShapeManager::Shape>& updated, const QString& label);
    void extendShape(int shapeIndex, const QPainterPath& newPath, bool fromStart);
    std::vector<ShapeManager::Shape> getCurrentState() const;
    void commitSnapshot(const std::vector<ShapeManager::Shape>& oldState, const std::vector<ShapeManager::Shape>& newState, const QString& label);
    void commitAddShape(const QPainterPath &path, int id, const QString &label = QString());

    // === State Setters ===
    void setSmoothingLevel(int level);
    void setPrecisionConstraintEnabled(bool enabled);
    void setSegmentStatusVisible(bool visible);
    void setPerformanceMode(PerformanceMode mode);

    // === Validation ===
    QString validationSummary() const;
    bool hasValidationIssues() const;
    std::vector<bool> getCollisionFlags(int* pairCount = nullptr) const;
    bool isPathClosed(const QPainterPath &path) const;

signals:
    void shapesChanged();
    void selectionChanged();
    void historyStateChanged(bool canUndo, const QString& undoText,
                             bool canRedo, const QString& redoText);
    void smoothingLevelChanged(int level);
    void performanceModeChanged(PerformanceMode mode);
    void stateChanged();

private:
    std::unique_ptr<ShapeManager> m_shapeManager;
    std::unique_ptr<HistoryManager> m_historyManager;

    int m_nextShapeId = 1;
    int m_smoothingLevel = 1;
    bool m_precisionConstraintEnabled = false;
    bool m_segmentStatusVisible = false;
    PerformanceMode m_performanceMode = PerformanceMode::Balanced;

    mutable bool m_collisionCacheDirty = true;
    mutable std::vector<bool> m_collisionCache;
    mutable int m_collisionPairCount = 0;
};

#endif // CANVASVIEWMODEL_H
