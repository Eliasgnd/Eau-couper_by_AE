#ifndef CUSTOMDRAWAREA_H
#define CUSTOMDRAWAREA_H

#include <QFont>
#include <QElapsedTimer>
#include <QPainterPath>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QWidget>
#include <memory>
#include <vector>

#include "DrawModeManager.h"
#include "DrawingState.h"
#include "PlacementAssist.h"
#include "viewmodels/CanvasViewModel.h"
#include "domain/shapes/ShapeManager.h"
#include "shared/PerformanceMode.h"

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;
class EraserTool;
class MouseInteractionHandler;
class ShapeRenderer;
class TextTool;
class ViewTransformer;

class CustomDrawArea : public QWidget
{
    Q_OBJECT
public:
    using DrawMode = DrawModeManager::DrawMode;

    explicit CustomDrawArea(CanvasViewModel *viewModel, QWidget *parent = nullptr);
    ~CustomDrawArea() override;

    void     setDrawMode(DrawMode mode);
    DrawMode getDrawMode() const;
    void     restorePreviousMode();

    QList<QPolygonF> getCustomShapes() const;

    void clearDrawing();
    void undoLastAction();
    void redoLastAction();
    void setEraserRadius(qreal radius);

    void addImportedLogo(const QPainterPath &logoPath);
    void addImportedLogoSubpath(const QPainterPath &subpath);
    void addImportedLogoSubpaths(const QList<QPainterPath> &subpaths);
    void setPerformanceMode(PerformanceMode mode);
    PerformanceMode performanceMode() const;

    void  setTextFont(const QFont &font);
    QFont getTextFont() const;

    void startShapeSelection();
    void cancelSelection();
    void toggleMultiSelectMode();
    bool hasSelection() const;
    void deleteSelectedShapes();
    void duplicateSelectedShapes();
    QRectF selectedShapesBounds() const;
    int selectedShapesCount() const;
    qreal selectedRotationAngle() const;
    void resizeSelectedShapes(qreal targetWidth, qreal targetHeight);
    void rotateSelectedShapes(qreal angleDegrees);
    void setSelectedRotation(qreal angleDegrees);
    void moveSelectedShapes(qreal dx, qreal dy, const QString &label = QString());
    void setSelectedShapesPosition(qreal x, qreal y);
    void alignSelectedLeft();
    void alignSelectedHCenter();
    void alignSelectedTop();
    void alignSelectedVCenter();
    void distributeSelectedHorizontally();
    void distributeSelectedVertically();
    void centerSelectionInViewport();
    void duplicateSelectedShapesLinear(int copies, const QPointF &step);
    void zoomToSelection();
    void fitAllShapesInView();
    void finishPointByPointShape();
    void undoPointByPointPoint();
    void undoPointByPointSegment();

    void setSnapToGridEnabled(bool enabled);
    bool isSnapToGridEnabled() const;
    void setPlacementAssistEnabled(bool enabled);
    bool isPlacementAssistEnabled() const;
    void setPlacementMagnetEnabled(bool enabled);
    bool isPlacementMagnetEnabled() const;
    void setGridVisible(bool visible);
    bool isGridVisible() const;
    void setGridSpacing(int px);
    int  gridSpacing() const;
    void setPrecisionConstraintEnabled(bool enabled);
    bool isPrecisionConstraintEnabled() const;
    void setSegmentStatusVisible(bool visible);
    bool isSegmentStatusVisible() const;
    bool hasActiveSpecialMode() const;
    void cancelActiveModes();

    bool isDeplacerMode()  const;
    bool isSupprimerMode() const;
    bool isGommeMode()     const;
    bool isConnectMode()   const;

    QString validationSummary() const;
    bool hasValidationIssues() const;
    QString buildSelectionSummary() const;

    std::vector<bool> getCollisionFlags(int *pairCount = nullptr) const;

signals:
    void zoomChanged(double newScale);
    void closeModeChanged(bool enabled);
    void shapeSelection(bool enabled);
    void smoothingLevelChanged(int level);
    void multiSelectionModeChanged(bool enabled);
    void deplacerModeChanged(bool enabled);
    void supprimerModeChanged(bool enabled);
    void gommeModeChanged(bool enabled);
    void drawModeChanged(DrawMode mode);
    void selectionStateChanged(bool hasSelection, const QString &summary);
    void canvasStatusChanged(const QString &modeLabel, const QString &hint, const QString &detail);
    void historyStateChanged(bool canUndo, const QString &undoText,
                             bool canRedo, const QString &redoText);

public slots:
    void startCloseMode();    void cancelCloseMode();
    void startDeplacerMode(); void cancelDeplacerMode();
    void startSupprimerMode();void cancelSupprimerMode();
    void startGommeMode();    void cancelGommeMode();
    void setSmoothingLevel(int level);
    void setTwoFingersOn(bool active);
    void handlePinchZoom(QPointF center, qreal factor);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void onDrawModeChanged(DrawMode mode);
    QPointF toLogical(const QPointF &widgetPoint) const;
    QPointF toWidget(const QPointF &logicalPoint) const;
    QPointF snapToGridIfNeeded(const QPointF &logicalPoint) const;
    QPointF constrainedPoint(const QPointF &logicalPoint) const;
    QPointF applyDrawingAids(const QPointF &logicalPoint) const;
    QPointF applyPlacementAssistToPoint(const QPointF &logicalPoint) const;
    void applyPlacementAssistToSelection();
    QVector<QRectF> placementTargetBounds(bool excludeSelection) const;
    QRectF visibleLogicalArea() const;
    int hitTestShape(const QPointF &logicalPoint, qreal tolerance = 40.0) const;

    enum class EndpointKind { None, Start, End };
    struct EndpointHit {
        int shapeIndex = -1;
        EndpointKind kind = EndpointKind::None;
        QPointF point;
        bool isValid() const { return shapeIndex >= 0 && kind != EndpointKind::None; }
    };

    qreal endpointTouchTolerance() const;
    EndpointHit hitTestOpenEndpoint(const QPointF &logicalPoint,
                                    int ignoredShapeIndex = -1) const;
    bool supportsEndpointResume(DrawMode mode) const;
    QPointF snapToDrawingAid(const QPointF &logicalPoint) const;
    void resetPointByPointResumeState();
    QPainterPath buildPointByPointPath(const QList<QPointF> &points, bool shouldClose) const;
    QPainterPath buildResumedPath(const QPainterPath &extensionPath) const;
    bool beginEndpointResumeIfNeeded(const QPointF &logicalPoint);
    void drawOpenEndpointHandles(QPainter &painter) const;

    enum class ResizeHandle { None, TopLeft, TopRight, BottomLeft, BottomRight };
    QRectF       selectionOverlayBounds() const;
    QTransform   selectionRotationTransform() const;
    QTransform   selectionInverseRotationTransform() const;
    ResizeHandle hitTestSelectionHandle(const QPointF &logicalPoint) const;
    QRectF       resizeRectFromHandle(const QPointF &logicalPoint) const;
    QPointF      rotationHandlePoint() const;
    bool         hitTestRotationHandle(const QPointF &logicalPoint) const;
    qreal        currentSelectionAngle() const;
    void         emitSelectionState();
    void         emitCanvasStatus();
    void         emitCanvasStatusThrottled();
    void         emitHistoryState();
    QString      currentModeLabel() const;
    QString      currentModeHint() const;
    QString      currentDetailText() const;
    QString      livePreviewMetrics() const;
    bool         isPointInsideSelectedShape(const QPointF &logicalPoint) const;
    void         drawCanvasHud(QPainter &painter) const;
    void         drawMachinePreview(QPainter &painter) const;
    void         drawValidationOverlay(QPainter &painter) const;
    void         drawSmartGuides(QPainter &painter) const;
    void         drawPointByPointAids(QPainter &painter) const;
    void         updateFreehandPreviewCache();
    bool         isInteractiveRenderingActive() const;
    void         commitSelectedTransform(const std::vector<ShapeManager::Shape> &updated,
                                         const QString &label);

    CanvasViewModel*                         m_viewModel = nullptr;
    std::unique_ptr<ShapeRenderer>           m_renderer;
    std::unique_ptr<DrawModeManager>         m_modeManager;
    std::unique_ptr<ViewTransformer>         m_transformer;
    std::unique_ptr<DrawingState>            m_drawingState;
    std::unique_ptr<MouseInteractionHandler> m_mouseHandler;
    std::unique_ptr<EraserTool>              m_eraserTool;
    std::unique_ptr<TextTool>                m_textTool;

    bool m_placementAssistEnabled = true;
    bool m_placementMagnetEnabled = true;
    mutable QVector<PlacementAssist::Guide> m_activePlacementGuides;

    bool m_drawing = false;
    bool m_twoFingersOn = false;
    bool m_canvasStatusThrottleStarted = false;
    QElapsedTimer m_canvasStatusThrottle;
    bool m_hasPointer = false;
    bool m_lastSnapApplied = false;
    mutable EndpointHit m_hoveredEndpoint;
    QPointF m_lastPointerLogical;
    QPointF m_lastRawPointerLogical;

    QPointF  m_lastSelectClick;

    std::vector<ShapeManager::Shape> m_moveStartState;
    bool                             m_moveInProgress = false;
    std::vector<ShapeManager::Shape> m_transformStartState;
    QRectF                           m_transformStartBounds;
    ResizeHandle                     m_activeResizeHandle = ResizeHandle::None;
    bool                             m_resizeInProgress = false;
    bool                             m_rotateInProgress = false;
    qreal                            m_rotateStartPointerAngle = 0.0;
};

#endif // CUSTOMDRAWAREA_H
