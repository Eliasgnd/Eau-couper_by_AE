#include "ShapeController.h"

#include "ShapeVisualization.h"
#include "GeometryUtils.h"

#include <QMessageBox>

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

void ShapeController::updateShapeCount(int count, int largeur, int longueur)
{
    if (!m_visualization)
        return;
    m_visualization->setShapeCount(count, m_selectedShapeType, largeur, longueur);
}

void ShapeController::updateSpacing(int value)
{
    if (!m_visualization)
        return;
    m_visualization->setSpacing(value);
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

void ShapeController::rotateLeft()
{
    if (!m_visualization)
        return;
    m_visualization->rotateSelectedShapes(-90);
}

void ShapeController::rotateRight()
{
    if (!m_visualization)
        return;
    m_visualization->rotateSelectedShapes(90);
}

void ShapeController::addShape()
{
    if (!m_visualization)
        return;
    m_visualization->addShapeBottomRight();
}

void ShapeController::deleteShape()
{
    if (!m_visualization)
        return;
    m_visualization->deleteSelectedShapes();
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

bool ShapeController::loadCustomShapes(const QList<QPolygonF> &polygons,
                                       const QString &name,
                                       int currentUiLargeur,
                                       int currentUiLongueur)
{
    if (!m_visualization)
        return false;

    if (!m_visualization->isInteractionEnabled()) {
        QMessageBox::warning(nullptr,
                             tr("Interaction verrouillée"),
                             tr("Impossible de modifier la forme lorsque l'édition est verrouillée."));
        return false;
    }

    m_visualization->setCustomMode();

    const QRectF bounds = combinedBoundingRect(polygons);
    int largeur = currentUiLargeur;
    int hauteur = currentUiLongueur;

    if (largeur <= 0)
        largeur = bounds.width() > 0 ? qRound(bounds.width()) : 100;
    if (hauteur <= 0)
        hauteur = bounds.height() > 0 ? qRound(bounds.height()) : 100;

    m_visualization->updateDimensions(largeur, hauteur);
    m_visualization->displayCustomShapes(polygons);
    m_visualization->setCurrentCustomShapeName(name);
    return true;
}
