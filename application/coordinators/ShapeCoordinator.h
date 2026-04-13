#ifndef SHAPECOORDINATOR_H
#define SHAPECOORDINATOR_H

#include <QObject>
#include <QList>
#include <QPolygonF>
#include <QString>

#include "ShapeModel.h"

class ShapeVisualization;
class ShapeVisualizationViewModel;

class ShapeCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit ShapeCoordinator(ShapeVisualization *visualization, QObject *parent = nullptr);

    ShapeModel::Type selectedShapeType() const;
    void setSelectedShapeType(ShapeModel::Type type);

public slots:
    void updateShape(int largeur, int longueur);
    void updateShapeCount(int count, int largeur, int longueur);
    void updateSpacing(int value);
    void setPredefinedShape(ShapeModel::Type type);
    void moveSelectedShape(int dx, int dy);
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onMoveLeftClicked();
    void onMoveRightClicked();
    void rotateLeft();
    void rotateRight();
    void addShape();
    void deleteShape();
    void onOptimizePlacementClicked(bool checked, int largeur, int longueur);
    void onOptimizePlacement2Clicked(bool checked, int largeur, int longueur);
    bool loadCustomShapes(const QList<QPolygonF> &polygons,
                          const QString &name,
                          int currentUiLargeur,
                          int currentUiLongueur);

signals:
    void progressUpdated(int current, int total);
    void optimizationStateChanged(bool optimized);

private:
    void runOptimization(const QList<int> &angles);

    ShapeVisualization *m_visualization = nullptr;
    ShapeModel::Type m_selectedShapeType = ShapeModel::Type::Circle;
};

#endif // SHAPECOORDINATOR_H
