#ifndef SHAPECONTROLLER_H
#define SHAPECONTROLLER_H

#include <QObject>

#include "ShapeModel.h"

class ShapeVisualization;

class ShapeController : public QObject
{
    Q_OBJECT

public:
    explicit ShapeController(ShapeVisualization *visualization, QObject *parent = nullptr);

    ShapeModel::Type selectedShapeType() const;
    void setSelectedShapeType(ShapeModel::Type type);

public slots:
    void updateShape(int largeur, int longueur);
    void setPredefinedShape(ShapeModel::Type type);
    void moveSelectedShape(int dx, int dy);
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onMoveLeftClicked();
    void onMoveRightClicked();
    void onOptimizePlacementClicked(bool checked, int largeur, int longueur);
    void onOptimizePlacement2Clicked(bool checked, int largeur, int longueur);

private:
    ShapeVisualization *m_visualization = nullptr;
    ShapeModel::Type m_selectedShapeType = ShapeModel::Type::Circle;
};

#endif // SHAPECONTROLLER_H
