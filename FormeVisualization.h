#ifndef FORMEVISUALIZATION_H
#define FORMEVISUALIZATION_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QProgressBar>
#include <QPainterPath>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QPolygonF>
#include <QSizeF>
#include "ShapeModel.h"
#include "inventaire.h"

class FormeVisualization : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QSizeF sheetSizeMm READ sheetSizeMm WRITE setSheetSizeMm NOTIFY sheetSizeMmChanged)
public:
    explicit FormeVisualization(QWidget *parent = nullptr);
    ~FormeVisualization();

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
    void colorPositionRed(const QPoint& position);
    void colorPositionBlue(const QPoint& position);
    QGraphicsScene* getScene() const { return scene; }
    void setCustomMode();
    void setEditingEnabled(bool enabled) { editingEnabled = enabled; }
    bool isEditingEnabled() const { return editingEnabled; }
    void setDecoupeEnCours(bool etat);
    bool isDecoupeEnCours() const { return m_decoupeEnCours; }
    void cancelOptimization() { }
    bool isOptimizationRunning() const { return false; }
    QGraphicsView* getGraphicsView() const { return graphicsView; }

    bool isCustomMode() const { return m_isCustomMode; }
    void setCurrentCustomShapeName(const QString &name) { m_currentCustomShapeName = name; }
    QString currentCustomShapeName() const { return m_currentCustomShapeName; }
    QList<QPolygonF> currentCustomShapes() const { return m_customShapes; }
    void applyLayout(const LayoutData &layout);
    LayoutData captureCurrentLayout(const QString &name) const;

    QSizeF sheetSizeMm() const { return m_sheetMm; }

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
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
    void setSheetSizeMm(const QSizeF& mm);

signals:
    void optimizationStateChanged(bool optimized);
    void shapesPlacedCount(int);
    void spacingChanged(int newSpacing);
    void blackPixelsProgress(int remaining, int total);
    void sheetSizeMmChanged(const QSizeF&);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    QPainterPath bufferedPath(const QPainterPath &path, int spacing);
    void handleSelectionChanged();

private:
    bool warnIfCutting();
    void addCutMarker(const QPoint& p, const QColor& color, bool center = false);
    int countPlacedShapes() const;
    void redraw();

    void addPathWithLOD(const QPainterPath &path, const QPointF &pos);
    void applySize(QGraphicsPathItem *item, qreal W, qreal H);
    void cleanup();
    static void messageForwarder(QtMsgType, const QMessageLogContext&, const QString&);
    QtMessageHandler previousHandler {nullptr};

    QGraphicsView  *graphicsView {};
    QGraphicsScene *scene {};
    QProgressBar   *progressBar {};
    ShapeModel::Type currentModel {ShapeModel::Type::Circle};
    int  currentLargeur  {0};
    int  currentLongueur {0};
    int  shapeCount      {0};
    int  spacing         {0};
    bool m_isCustomMode {false};
    QList<QPolygonF> m_customShapes;
    QString m_currentCustomShapeName;
    QList<QGraphicsItem*> m_cutMarkers;
    bool editingEnabled = true;
    bool m_decoupeEnCours = false;
    QGraphicsRectItem *m_sheetBorder {nullptr};
    QSizeF m_sheetMm {600.0, 400.0};
    double m_aspect = m_sheetMm.width() / m_sheetMm.height();
};

#endif // FORMEVISUALIZATION_H
