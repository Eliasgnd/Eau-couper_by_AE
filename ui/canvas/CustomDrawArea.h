#ifndef CUSTOMDRAWAREA_H
#define CUSTOMDRAWAREA_H

#include <QFont>
#include <QPainterPath>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QWidget>
#include <memory>
#include <vector>

#include "DrawModeManager.h"
#include "DrawingState.h"
#include "domain/shapes/ShapeManager.h"

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;
class EraserTool;
class HistoryManager;
class MouseInteractionHandler;
class ShapeRenderer;
class TextTool;
class ViewTransformer;

class CustomDrawArea : public QWidget
{
    Q_OBJECT
public:
    using DrawMode = DrawModeManager::DrawMode;

    explicit CustomDrawArea(QWidget *parent = nullptr);
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

    void  setTextFont(const QFont &font);
    QFont getTextFont() const;

    void startShapeSelection();
    void cancelSelection();
    void toggleMultiSelectMode();
    bool hasSelection() const;
    void deleteSelectedShapes();
    void duplicateSelectedShapes();
    void copySelectedShapes();
    void enablePasteMode();
    void pasteCopiedShapes(const QPointF &dest);
    QRectF selectedShapesBounds() const;
    void resizeSelectedShapes(qreal targetWidth, qreal targetHeight);
    void rotateSelectedShapes(qreal angleDegrees);
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

    void setSnapToGridEnabled(bool enabled);
    bool isSnapToGridEnabled() const;
    void setGridVisible(bool visible);
    bool isGridVisible() const;
    void setGridSpacing(int px);
    int  gridSpacing() const;
    void setPrecisionConstraintEnabled(bool enabled);
    bool isPrecisionConstraintEnabled() const;
    void setMachinePreviewEnabled(bool enabled);
    bool isMachinePreviewEnabled() const;
    bool hasActiveSpecialMode() const;
    void cancelActiveModes();

    bool isDeplacerMode()  const;
    bool isSupprimerMode() const;
    bool isGommeMode()     const;
    bool isConnectMode()   const;

    QString validationSummary() const;
    bool hasValidationIssues() const;
    QString buildSelectionSummary() const;

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
    int hitTestShape(const QPointF &logicalPoint, qreal tolerance = 40.0) const;

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
    void         emitHistoryState();
    QString      currentModeLabel() const;
    QString      currentModeHint() const;
    QString      currentDetailText() const;
    QString      livePreviewMetrics() const;
    bool         isPointInsideSelectedShape(const QPointF &logicalPoint) const;
    bool         isPathClosed(const QPainterPath &path) const;
    void         drawCanvasHud(QPainter &painter) const;
    void         drawMachinePreview(QPainter &painter) const;
    void         commitSelectedTransform(const std::vector<ShapeManager::Shape> &updated,
                                         const QString &label);

    std::unique_ptr<ShapeManager>            m_shapeManager;
    std::unique_ptr<ShapeRenderer>           m_renderer;
    std::unique_ptr<DrawModeManager>         m_modeManager;
    std::unique_ptr<HistoryManager>          m_historyManager;
    std::unique_ptr<ViewTransformer>         m_transformer;
    std::unique_ptr<DrawingState>            m_drawingState;
    std::unique_ptr<MouseInteractionHandler> m_mouseHandler;
    std::unique_ptr<EraserTool>              m_eraserTool;
    std::unique_ptr<TextTool>                m_textTool;

    bool m_drawing = false;
    int  m_nextShapeId = 1;
    int  m_smoothingLevel = 1;
    bool m_twoFingersOn = false;
    bool m_precisionConstraintEnabled = false;
    bool m_machinePreviewEnabled = false;
    bool m_hasPointer = false;
    bool m_lastSnapApplied = false;
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
