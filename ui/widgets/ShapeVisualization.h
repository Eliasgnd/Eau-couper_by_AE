#ifndef FORMEVISUALIZATION_H
#define FORMEVISUALIZATION_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QPainterPath>
#include <QGraphicsRectItem>
#include <QPointer>
#include <QSizeF>                // <<< AJOUT
#include "Inventory.h"
#include "ShapeVisualizationViewModel.h"

// -----------------------------------------------------------------------------
// Classe permettant la visualisation des formes dessinées
// -----------------------------------------------------------------------------
class ShapeVisualization : public QWidget {
    Q_OBJECT

    // <<< AJOUT : propriété "taille plateau en mm" (X=largeur, Y=hauteur)
    Q_PROPERTY(QSizeF sheetSizeMm READ sheetSizeMm WRITE setSheetSizeMm NOTIFY sheetSizeMmChanged)

public:
    explicit ShapeVisualization(QWidget *parent = nullptr);

    // --- API existante ---
    void updateDimensions(int largeur, int longueur);
    void setSpacing(int newSpacing);
    void setPredefinedMode();
    void colorPositionRed (const QPoint& position);
    void colorPositionBlue(const QPoint& position);
    QGraphicsScene* getScene() const;
    void setCustomMode();
    void setEditingEnabled(bool enabled);
    bool isEditingEnabled() const;
    void setInteractionEnabled(bool enabled);
    bool isInteractionEnabled() const;
    bool isSheetEditingEnabled() const { return m_sheetEditingEnabled; }
    void cancelOptimization();
    bool isOptimizationRunning() const { return m_projectModel && m_projectModel->isOptimizationRunning(); }
    void setOptimizationRunning(bool running);
    void setOptimizationResult(const QList<QPainterPath> &placedPaths, bool optimized);
    QGraphicsView* getGraphicsView() const;
    QPointF logicalPointFromScenePoint(const QPointF &scenePoint) const;
    QRectF placementRect() const;

    bool isCustomMode() const { return m_projectModel && m_projectModel->isCustomMode(); }
    void setCurrentCustomShapeName(const QString &name) { if (m_projectModel) m_projectModel->setCustomShapeName(name); }
    QString currentCustomShapeName() const { return m_projectModel ? m_projectModel->customShapeName() : QString(); }
    QList<QPolygonF> currentCustomShapes() const;
    void applyLayout(const LayoutData &layout);
    LayoutData captureCurrentLayout(const QString &name) const;

    ShapeVisualizationViewModel* projectModel() const { return m_projectModel; }
    void setProjectModel(ShapeVisualizationViewModel *model);

    // <<< AJOUT : accès à la taille en mm
    QSizeF sheetSizeMm() const { return m_sheetMm; }
    QPointF sheetOriginMm() const { return m_sheetOriginMm; }

    bool hasHeightForWidth() const override { return true; }
    int  heightForWidth(int w) const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    // --- API existante ---
    void displayCustomShapes(const QList<QPolygonF>& shapes);
    void moveSelectedShapes(qreal dx, qreal dy);
    void rotateSelectedShapes(qreal angleDelta);
    void deleteSelectedShapes();
    void addShapeBottomRight();
    bool validateShapes();
    void resetAllShapeColors();
    QList<QPoint> getBlackPixels();
    void resetCutMarkers();
    void setDecoupeEnCours(bool running);
    void addCutMarker(QGraphicsItem* item);
    void updateHeadLogicalPositionFromScene(const QPointF &scenePoint);

    // <<< AJOUT : setter taille en mm
    void setSheetSizeMm(const QSizeF& mm);
    void setSheetOriginMm(const QPointF &origin);
    void setSheetEditingEnabled(bool enabled);

signals:
    void optimizationStateChanged(bool optimized);
    void shapesPlacedCount(int);
    void spacingChanged(int newSpacing);
    void blackPixelsProgress(int remaining, int total);
    void actionRefused(const QString &reason);
    void shapeSelectionChanged(int selectedCount);
    void shapeMoved(const QPointF &delta);
    void shapesDeleted(int deletedCount);
    void headLogicalPositionChanged(double x, double y);

    // <<< AJOUT
    void sheetSizeMmChanged(const QSizeF&);
    void sheetOriginMmChanged(const QPointF &origin);

protected:
    // --- API existante ---
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void handleSelectionChanged();

private:
    int countPlacedShapes() const;
    void redraw();
    QRectF renderedSheetRectInViewport() const;
    void updateRulers() const;
    QRectF machineRect() const;
    QRectF clampedPlacementRect(const QPointF &origin, const QSizeF &size) const;
    void rebuildSheetOverlay();
    void translatePlacedItems(const QPointF &delta);
    bool isPlacedShapeItem(QGraphicsItem *item) const;

    // --- Membres existants ---
    QGraphicsView       *graphicsView {};
    QGraphicsScene      *scene {};
    QPointer<QWidget>    m_rulerCorner;
    QPointer<QWidget>    m_horizontalRuler;
    QPointer<QWidget>    m_verticalRuler;
    ShapeVisualizationViewModel    *m_projectModel {nullptr};
    QList<QGraphicsItem*> m_cutMarkers;
    bool editingEnabled = true;
    bool m_interactionEnabled = true;
    bool m_sheetEditingEnabled = false;
    QPointF m_rotationPivot;
    bool m_rotationPivotValid {false};

    // Bordure représentant visuellement la limite du plateau
    // (placée au-dessus des formes pour rester visible)
    QGraphicsRectItem *m_sheetBorder {nullptr};
    QGraphicsRectItem *m_machineBorder {nullptr};
    QGraphicsRectItem *m_machineBackground {nullptr};

    // <<< AJOUT : taille plateau et ratio (w/h) pour l'Option B
    // Dimensions logiques du plateau (par défaut 600x400)
    QSizeF m_sheetMm {600.0, 400.0};
    QPointF m_sheetOriginMm {0.0, 0.0};
    double m_aspect = 600.0 / 400.0;
    bool m_draggingSheet = false;
    QPointF m_dragStartScenePos;
    QPointF m_dragStartSheetOrigin;
};

#endif // FORMEVISUALIZATION_H
