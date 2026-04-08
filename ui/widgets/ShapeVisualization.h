#ifndef FORMEVISUALIZATION_H
#define FORMEVISUALIZATION_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QPainterPath>
#include <QGraphicsRectItem>
#include <QSizeF>                // <<< AJOUT
#include "Inventory.h"
#include "shapevisualization/ShapeVisualizationViewModel.h"

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
    void cancelOptimization();
    bool isOptimizationRunning() const { return m_projectModel && m_projectModel->isOptimizationRunning(); }
    void setOptimizationRunning(bool running);
    void setOptimizationResult(const QList<QPainterPath> &placedPaths, bool optimized);
    QGraphicsView* getGraphicsView() const;

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

    // <<< AJOUT : setter taille en mm
    void setSheetSizeMm(const QSizeF& mm);

signals:
    void optimizationStateChanged(bool optimized);
    void shapesPlacedCount(int);
    void spacingChanged(int newSpacing);
    void blackPixelsProgress(int remaining, int total);
    void actionRefused(const QString &reason);
    void shapeSelectionChanged(int selectedCount);
    void shapeMoved(const QPointF &delta);
    void shapesDeleted(int deletedCount);

    // <<< AJOUT
    void sheetSizeMmChanged(const QSizeF&);

protected:
    // --- API existante ---
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void handleSelectionChanged();

private:
    int countPlacedShapes() const;
    void redraw();

    // --- Membres existants ---
    QGraphicsView       *graphicsView {};
    QGraphicsScene      *scene {};
    ShapeVisualizationViewModel    *m_projectModel {nullptr};
    QList<QGraphicsItem*> m_cutMarkers;
    bool editingEnabled = true;
    bool m_interactionEnabled = true;
    QPointF m_rotationPivot;
    bool m_rotationPivotValid {false};

    // Bordure représentant visuellement la limite du plateau
    // (placée au-dessus des formes pour rester visible)
    QGraphicsRectItem *m_sheetBorder {nullptr};

    // <<< AJOUT : taille plateau et ratio (w/h) pour l'Option B
    // Dimensions logiques du plateau (par défaut 600x400)
    QSizeF m_sheetMm {600.0, 400.0};        // X = largeur(mm), Y = hauteur(mm)
    double m_aspect = m_sheetMm.width() / m_sheetMm.height();
};

#endif // FORMEVISUALIZATION_H
