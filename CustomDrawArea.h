#ifndef CUSTOMDRAWAREA_H
#define CUSTOMDRAWAREA_H
#include <QFont>
#include <QPainterPath>
#include <QPolygonF>
#include <QWidget>
#include "drawing/DrawModeManager.h"

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;
class EraserTool;
class HistoryManager;
class MouseInteractionHandler;
class ShapeManager;
class ShapeRenderer;
class TextTool;
class ViewTransformer;
class CustomDrawArea : public QWidget
{
    Q_OBJECT
public:
    using DrawMode = DrawModeManager::DrawMode;

    explicit CustomDrawArea(QWidget *parent = nullptr);
    ~CustomDrawArea() override = default;

    void setDrawMode(DrawMode mode);
    DrawMode getDrawMode() const;
    void restorePreviousMode();
    QList<QPolygonF> getCustomShapes() const;
    void clearDrawing();
    void undoLastAction();
    void setEraserRadius(qreal radius);

    void addImportedLogo(const QPainterPath &logoPath);
    void addImportedLogoSubpath(const QPainterPath &subpath);

    void setTextFont(const QFont &font);
    QFont getTextFont() const;
    void startShapeSelection();
    void cancelSelection();
    void toggleMultiSelectMode();
    bool hasSelection() const;
    void deleteSelectedShapes();
    void copySelectedShapes();
    void enablePasteMode();
    void pasteCopiedShapes(const QPointF &dest);
    void setSnapToGridEnabled(bool enabled);
    bool isSnapToGridEnabled() const;
    void setGridVisible(bool visible);
    bool isGridVisible() const;
    void setGridSpacing(int px);
    int gridSpacing() const;

    bool isDeplacerMode() const;
    bool isSupprimerMode() const;
    bool isGommeMode() const;
    bool isConnectMode() const;

    static QList<QPainterPath> separateIntoSubpaths(const QPainterPath &path);

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

public slots:
    void startCloseMode(); void cancelCloseMode();
    void startDeplacerMode(); void cancelDeplacerMode();
    void startSupprimerMode(); void cancelSupprimerMode();
    void startGommeMode(); void cancelGommeMode();
    void setSmoothingLevel(int level); void setTwoFingersOn(bool active);
    void handlePinchZoom(QPointF center, qreal factor);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QPointF toLogical(const QPointF &widgetPoint) const;

    ShapeManager *m_shapeManager; ShapeRenderer *m_renderer; DrawModeManager *m_modeManager;
    HistoryManager *m_historyManager; ViewTransformer *m_transformer;
    MouseInteractionHandler *m_mouseHandler; EraserTool *m_eraserTool; TextTool *m_textTool;
    QList<QPointF> m_strokePoints; QPointF m_startPoint; QPointF m_currentPoint;
    int m_nextShapeId = 1; int m_smoothingLevel = 1; bool m_multiSelect = false;
    bool m_pasteMode = false; bool m_drawing = false; bool m_twoFingersOn = false;
};
#endif // CUSTOMDRAWAREA_H
