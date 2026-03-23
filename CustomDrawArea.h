#ifndef CUSTOMDRAWAREA_H
#define CUSTOMDRAWAREA_H
#include <QFont>
#include <QPainterPath>
#include <QPolygonF>
#include <QWidget>
#include <memory>
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

/**
 * @class CustomDrawArea
 * @brief Surface de dessin interactive qui orchestre les managers de rendu, d'édition et d'historique.
 *
 * @note Cette classe centralise les interactions souris/clavier et relaie les signaux d'état de dessin.
 * @see ShapeManager
 */
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
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void onDrawModeChanged(DrawMode mode);
    QPointF toLogical(const QPointF &widgetPoint) const;
    QPointF snapToGridIfNeeded(const QPointF &logicalPoint) const;
    int hitTestShape(const QPointF &logicalPoint, qreal tolerance = 25.0) const;

    std::unique_ptr<ShapeManager> m_shapeManager;
    std::unique_ptr<ShapeRenderer> m_renderer;
    std::unique_ptr<DrawModeManager> m_modeManager;
    std::unique_ptr<HistoryManager> m_historyManager;
    std::unique_ptr<ViewTransformer> m_transformer;
    std::unique_ptr<MouseInteractionHandler> m_mouseHandler;
    std::unique_ptr<EraserTool> m_eraserTool;
    std::unique_ptr<TextTool> m_textTool;
    QList<QPointF> m_strokePoints; QPointF m_startPoint; QPointF m_currentPoint;
    QList<QPointF> m_pointByPointPoints;
    int m_nextShapeId = 1; int m_smoothingLevel = 1; bool m_multiSelect = false;
    bool m_pasteMode = false; bool m_drawing = false; bool m_twoFingersOn = false;
    bool m_selectMode = false; bool m_connectSelectionMode = false; bool m_closeMode = false;
    bool m_gommeErasing = false;
    QPointF m_gommeCenter;
    QPointF m_lastEraserPos;
    QPointF m_lastSelectClick;
};
#endif // CUSTOMDRAWAREA_H
