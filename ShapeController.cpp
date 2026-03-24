#include "ShapeController.h"

#include "ShapeVisualization.h"

ShapeController::ShapeController(ShapeVisualization *visualization, QObject *parent)
    : QObject(parent), m_visualization(visualization)
{
}

ShapeModel::Type ShapeController::selectedShapeType() const
{
    return m_selectedShapeType;
}

void ShapeController::setSelectedShapeType(ShapeModel::Type type)
{
    m_selectedShapeType = type;
}

void ShapeController::updateShape(int largeur, int longueur)
{
    if (!m_visualization)
        return;
    m_visualization->updateDimensions(largeur, longueur);
}

void ShapeController::setPredefinedShape(ShapeModel::Type type)
{
    if (!m_visualization)
        return;
    m_selectedShapeType = type;
    m_visualization->setPredefinedMode();
    m_visualization->setModel(type);
}

void ShapeController::moveSelectedShape(int dx, int dy)
{
    if (!m_visualization)
        return;
    m_visualization->moveSelectedShapes(dx, dy);
}

void ShapeController::onMoveUpClicked()
{
    moveSelectedShape(0, -1);
}

void ShapeController::onMoveDownClicked()
{
    moveSelectedShape(0, 1);
}

void ShapeController::onMoveLeftClicked()
{
    moveSelectedShape(-1, 0);
}

void ShapeController::onMoveRightClicked()
{
    moveSelectedShape(1, 0);
}

void ShapeController::onOptimizePlacementClicked(bool checked, int largeur, int longueur)
{
    if (!m_visualization)
        return;
    if (checked) {
        m_visualization->optimizePlacement();
    } else {
        m_visualization->updateDimensions(largeur, longueur);
    }
}

void ShapeController::onOptimizePlacement2Clicked(bool checked, int largeur, int longueur)
{
    if (!m_visualization)
        return;
    if (checked) {
        m_visualization->optimizePlacement2();
    } else {
        m_visualization->updateDimensions(largeur, longueur);
    }
}
