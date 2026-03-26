#include "ShapeVisualizationViewModel.h"

ShapeVisualizationViewModel::ShapeVisualizationViewModel(QObject *parent)
    : QObject(parent)
{
}

ShapeModel::Type ShapeVisualizationViewModel::currentModel() const { return m_currentModel; }
int ShapeVisualizationViewModel::currentLargeur() const { return m_currentLargeur; }
int ShapeVisualizationViewModel::currentLongueur() const { return m_currentLongueur; }
int ShapeVisualizationViewModel::shapeCount() const { return m_shapeCount; }
int ShapeVisualizationViewModel::spacing() const { return m_spacing; }
QList<QPolygonF> ShapeVisualizationViewModel::customShapes() const { return m_customShapes; }

void ShapeVisualizationViewModel::setCurrentModel(ShapeModel::Type model)
{
    if (m_currentModel == model)
        return;
    m_currentModel = model;
    emit dataChanged();
}

void ShapeVisualizationViewModel::setDimensions(int largeur, int longueur)
{
    if (m_currentLargeur == largeur && m_currentLongueur == longueur)
        return;
    m_currentLargeur = largeur;
    m_currentLongueur = longueur;
    emit dataChanged();
}

void ShapeVisualizationViewModel::setShapeCount(int count)
{
    if (m_shapeCount == count)
        return;
    m_shapeCount = count;
    emit dataChanged();
}

void ShapeVisualizationViewModel::setSpacing(int spacing)
{
    if (m_spacing == spacing)
        return;
    m_spacing = spacing;
    emit dataChanged();
}

void ShapeVisualizationViewModel::setCustomShapes(const QList<QPolygonF> &shapes)
{
    if (m_customShapes == shapes)
        return;
    m_customShapes = shapes;
    emit dataChanged();
}

void ShapeVisualizationViewModel::clearCustomShapes()
{
    if (m_customShapes.isEmpty())
        return;
    m_customShapes.clear();
    emit dataChanged();
}

void ShapeVisualizationViewModel::setCustomMode(bool custom)
{
    if (m_isCustomMode == custom)
        return;
    m_isCustomMode = custom;
    emit customModeChanged(custom);
}

void ShapeVisualizationViewModel::setOptimizationRunning(bool running)
{
    if (m_optimizationRunning == running)
        return;
    m_optimizationRunning = running;
    emit optimizationRunningChanged(running);
}

void ShapeVisualizationViewModel::setCustomShapeName(const QString &name)
{
    m_customShapeName = name;
}
