#ifndef FORMEVISUALIZATION_H
#define FORMEVISUALIZATION_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QProgressBar>
#include <QLabel>
#include <QPainterPath>
#include "ShapeModel.h"
#include "inventaire.h"

// -----------------------------------------------------------------------------
// Classe permettant la visualisation des formes dessinées
// -----------------------------------------------------------------------------
class FormeVisualization : public QWidget {
    Q_OBJECT
public:
    explicit FormeVisualization(QWidget *parent = nullptr);

    void setModel(ShapeModel::Type model);                      // modèle prédéfini
    void updateDimensions(int largeur, int longueur);           // maj dimensions
    void setShapeCount(int count, ShapeModel::Type, int, int);  // nb formes
    void setSpacing(int newSpacing);                            // espacement
    void setPredefinedMode();                                   // repasser en mode prédéfini

    // Optimisations de placement
    void optimizePlacement();
    void optimizePlacement2();
    void optimizePlacementComplex();        // (libnest2d)

    double evaluateWasteArea(const QList<QPainterPath>& placedPaths,
                             int drawingWidth, int drawingHeight);

    // Points de visualisation de découpe
    void colorPositionRed (const QPoint& position);             // coupe
    void colorPositionBlue(const QPoint& position);             // déplacement

    QGraphicsScene* getScene() const;                           // accès scène
    void setCustomMode();                                       // forcer mode custom

    void setEditingEnabled(bool enabled);
    bool isEditingEnabled() const;
    void setDecoupeEnCours(bool etat);
    bool isDecoupeEnCours() const;
    QGraphicsView* getGraphicsView() const;             // accès à la vue

    bool isCustomMode() const { return m_isCustomMode; }
    void setCurrentCustomShapeName(const QString &name) { m_currentCustomShapeName = name; }
    QString currentCustomShapeName() const { return m_currentCustomShapeName; }
    void applyLayout(const LayoutData &layout);
    LayoutData captureCurrentLayout(const QString &name) const;


public slots:
    void displayCustomShapes(const QList<QPolygonF>& shapes);   // affichage custom
    void moveSelectedShapes(qreal dx, qreal dy);                // déplacement
    void rotateSelectedShapes(qreal angleDelta);                // rotation
    void deleteSelectedShapes();                                // suppression
    void addShapeBottomRight();                                 // ajout en bas à droite
    bool validateShapes();                                      // vérifie positions
    void resetAllShapeColors();                                 // remise à zéro des couleurs

    QList<QPoint> getBlackPixels();                             // pixels noirs

    void startDecoupeProgress(int maxSteps);                    // barre prog
    void updateDecoupeProgress(int currentStep);
    void endDecoupeProgress();

    void resetCutMarkers();     // <<< nouveau slot : effacer marqueurs découpe

signals:
    void optimizationStateChanged(bool optimized);
    void shapesPlacedCount(int);
    void spacingChanged(int newSpacing);
    void blackPixelsProgress(int remaining, int total);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    QPainterPath bufferedPath(const QPainterPath &path, int spacing);
    void handleSelectionChanged();

private:
    int countPlacedShapes() const;
    void redraw();                                              // redessin



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

    // liste des points / traces ajoutés pendant la découpe
    QList<QGraphicsItem*> m_cutMarkers;
    bool editingEnabled = true;
    bool m_decoupeEnCours = false;



};

#endif // FORMEVISUALIZATION_H
