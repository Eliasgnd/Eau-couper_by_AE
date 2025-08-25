#ifndef FORMEVISUALIZATION_H
#define FORMEVISUALIZATION_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QProgressBar>
#include <QLabel>
#include <QPainterPath>
#include <QGraphicsRectItem>
#include <QSizeF>                // <<< AJOUT
#include "ShapeModel.h"
#include "inventaire.h"
#include <QHash>
#include <QTransform>

// -----------------------------------------------------------------------------
// Classe permettant la visualisation des formes dessinées
// -----------------------------------------------------------------------------
class FormeVisualization : public QWidget {
    Q_OBJECT

    // <<< AJOUT : propriété "taille plateau en mm" (X=largeur, Y=hauteur)
    Q_PROPERTY(QSizeF sheetSizeMm READ sheetSizeMm WRITE setSheetSizeMm NOTIFY sheetSizeMmChanged)

public:
    explicit FormeVisualization(QWidget *parent = nullptr);

    // --- API existante ---
    void setModel(ShapeModel::Type model);
    void updateDimensions(int largeur, int longueur);
    void setShapeCount(int count, ShapeModel::Type, int, int);
    void setSpacing(int newSpacing);
    void setPredefinedMode();
    void optimizePlacement();
    void optimizePlacement2();
    void optimizePlacementComplex();
    double evaluateWasteArea(const QList<QPainterPath>& placedPaths,
                             int drawingWidth, int drawingHeight);
    void colorPositionRed (const QPoint& position);
    void colorPositionBlue(const QPoint& position);
    QGraphicsScene* getScene() const;
    void setCustomMode();
    void setEditingEnabled(bool enabled);
    bool isEditingEnabled() const;
    void setDecoupeEnCours(bool etat);
    bool isDecoupeEnCours() const;
    void cancelOptimization();
    bool isOptimizationRunning() const { return m_optimizationRunning; }
    QGraphicsView* getGraphicsView() const;

    bool isCustomMode() const { return m_isCustomMode; }
    void setCurrentCustomShapeName(const QString &name) { m_currentCustomShapeName = name; }
    QString currentCustomShapeName() const { return m_currentCustomShapeName; }
    QList<QPolygonF> currentCustomShapes() const { return m_customShapes; }
    void applyLayout(const LayoutData &layout);
    LayoutData captureCurrentLayout(const QString &name) const;

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
    void startDecoupeProgress(int maxSteps);
    void updateDecoupeProgress(int currentStep);
    void endDecoupeProgress();
    void resetCutMarkers();

    // <<< AJOUT : setter taille en mm
    void setSheetSizeMm(const QSizeF& mm);

signals:
    void optimizationStateChanged(bool optimized);
    void shapesPlacedCount(int);
    void spacingChanged(int newSpacing);
    void blackPixelsProgress(int remaining, int total);

    // <<< AJOUT
    void sheetSizeMmChanged(const QSizeF&);

protected:
    // --- API existante ---
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    QPainterPath bufferedPath(const QPainterPath &path, int spacing);
    void handleSelectionChanged();

private:
    int countPlacedShapes() const;
    void redraw();

    // --- Membres existants ---
    QGraphicsView       *graphicsView {};
    QGraphicsScene      *scene {};
    QProgressBar        *progressBar {};
    ShapeModel::Type     currentModel {ShapeModel::Type::Circle};
    int  currentLargeur  {0};
    int  currentLongueur {0};
    int  shapeCount      {0};
    int  spacing         {0};
    bool                 m_isCustomMode {false};
    QList<QPolygonF>     m_customShapes;
    QString              m_currentCustomShapeName;
    QList<QGraphicsItem*> m_cutMarkers;
    bool editingEnabled = true;
    bool m_decoupeEnCours = false;
    bool m_optimizationRunning = false;
    bool m_cancelOptimization = false;
    QPointF m_rotationPivot;
    bool m_rotationPivotValid {false};

    struct CachedShape {
        QPainterPath base;      // simplified local path (cache)
        QPainterPath path;      // transformed path
        QRectF       bbox;      // cached bounding box
        QTransform   transform; // last transform used
    };
    QHash<QAbstractGraphicsShapeItem*, CachedShape> m_cache;

    // Bordure représentant visuellement la limite du plateau
    // (placée au-dessus des formes pour rester visible)
    QGraphicsRectItem *m_sheetBorder {nullptr};

    // <<< AJOUT : taille plateau et ratio (w/h) pour l'Option B
    // Dimensions logiques du plateau (par défaut 600x400)
    QSizeF m_sheetMm {600.0, 400.0};        // X = largeur(mm), Y = hauteur(mm)
    double m_aspect = m_sheetMm.width() / m_sheetMm.height();
};

#endif // FORMEVISUALIZATION_H
