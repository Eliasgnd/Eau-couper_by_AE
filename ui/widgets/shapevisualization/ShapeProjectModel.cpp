#include "ShapeProjectModel.h"

ShapeProjectModel::ShapeProjectModel(QObject *parent)
    : QObject(parent)
{
}

ShapeModel::Type ShapeProjectModel::currentModel() const { return m_currentModel; }
int ShapeProjectModel::currentLargeur() const { return m_currentLargeur; }
int ShapeProjectModel::currentLongueur() const { return m_currentLongueur; }
int ShapeProjectModel::shapeCount() const { return m_shapeCount; }
int ShapeProjectModel::spacing() const { return m_spacing; }
QList<QPolygonF> ShapeProjectModel::customShapes() const { return m_customShapes; }

void ShapeProjectModel::setCurrentModel(ShapeModel::Type model)
{
    if (m_currentModel == model)
        return;
    m_currentModel = model;
    emit dataChanged();
}

void ShapeProjectModel::setDimensions(int largeur, int longueur)
{
    if (m_currentLargeur == largeur && m_currentLongueur == longueur)
        return;
    m_currentLargeur = largeur;
    m_currentLongueur = longueur;
    emit dataChanged();
}

void ShapeProjectModel::setShapeCount(int count)
{
    if (m_shapeCount == count)
        return;
    m_shapeCount = count;
    emit dataChanged();
}

void ShapeProjectModel::setSpacing(int spacing)
{
    if (m_spacing == spacing)
        return;
    m_spacing = spacing;
    emit dataChanged();
}

void ShapeProjectModel::setCustomShapes(const QList<QPolygonF> &shapes)
{
    if (m_customShapes == shapes)
        return;
    m_customShapes = shapes;
    emit dataChanged();
}

void ShapeProjectModel::clearCustomShapes()
{
    if (m_customShapes.isEmpty())
        return;
    m_customShapes.clear();
    emit dataChanged();
}
