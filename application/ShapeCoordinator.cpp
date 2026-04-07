#include "ShapeCoordinator.h"

#include "ShapeVisualization.h"
#include "GeometryUtils.h"
#include "PlacementOptimizer.h"
#include "ShapeVisualizationViewModel.h"

#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QTransform>
#include <QMessageBox>

ShapeCoordinator::ShapeCoordinator(ShapeVisualization *visualization, QObject *parent)
    : QObject(parent), m_visualization(visualization)
{
}

namespace {
QPainterPath buildPrototypePath(ShapeModel::Type model,
                                int largeur,
                                int longueur,
                                bool isCustomMode,
                                const QList<QPolygonF> &customShapes)
{
    QPainterPath prototypePath;

    if (isCustomMode && !customShapes.isEmpty()) {
        for (const QPolygonF &poly : customShapes)
            prototypePath.addPolygon(poly);
        prototypePath = prototypePath.simplified();

        const QRectF customBounds = prototypePath.boundingRect();
        const double scaleX = (customBounds.width() > 0) ? (largeur / customBounds.width()) : 1.0;
        const double scaleY = (customBounds.height() > 0) ? (longueur / customBounds.height()) : 1.0;
        QTransform scaleTransform;
        scaleTransform.scale(scaleX, scaleY);
        return scaleTransform.map(prototypePath);
    }

    QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(model, largeur, longueur);
    if (shapesList.isEmpty())
        return {};

    QGraphicsItem *prototype = shapesList.first();
    if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(prototype))
        prototypePath = pathItem->path();
    else if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(prototype))
        prototypePath.addRect(rectItem->rect());
    else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(prototype))
        prototypePath.addEllipse(ellipseItem->rect());
    else if (auto polyItem = dynamic_cast<QGraphicsPolygonItem*>(prototype))
        prototypePath.addPolygon(polyItem->polygon());

    return prototypePath;
}
}

ShapeModel::Type ShapeCoordinator::selectedShapeType() const
{
    return m_selectedShapeType;
}

void ShapeCoordinator::setSelectedShapeType(ShapeModel::Type type)
{
    m_selectedShapeType = type;
}

void ShapeCoordinator::updateShape(int largeur, int longueur)
{
    if (!m_visualization)
        return;
    m_visualization->updateDimensions(largeur, longueur);
}

void ShapeCoordinator::updateShapeCount(int count, int largeur, int longueur)
{
    if (!m_visualization)
        return;
    m_visualization->setShapeCount(count, m_selectedShapeType, largeur, longueur);
}

void ShapeCoordinator::updateSpacing(int value)
{
    if (!m_visualization)
        return;
    m_visualization->setSpacing(value);
}

void ShapeCoordinator::setPredefinedShape(ShapeModel::Type type)
{
    if (!m_visualization)
        return;
    m_selectedShapeType = type;
    m_visualization->setPredefinedMode();
    m_visualization->setModel(type);
}

void ShapeCoordinator::moveSelectedShape(int dx, int dy)
{
    if (!m_visualization)
        return;
    m_visualization->moveSelectedShapes(dx, dy);
}

void ShapeCoordinator::onMoveUpClicked()
{
    moveSelectedShape(0, -1);
}

void ShapeCoordinator::onMoveDownClicked()
{
    moveSelectedShape(0, 1);
}

void ShapeCoordinator::onMoveLeftClicked()
{
    moveSelectedShape(-1, 0);
}

void ShapeCoordinator::onMoveRightClicked()
{
    moveSelectedShape(1, 0);
}

void ShapeCoordinator::rotateLeft()
{
    if (!m_visualization)
        return;
    m_visualization->rotateSelectedShapes(-90);
}

void ShapeCoordinator::rotateRight()
{
    if (!m_visualization)
        return;
    m_visualization->rotateSelectedShapes(90);
}

void ShapeCoordinator::addShape()
{
    if (!m_visualization)
        return;
    m_visualization->addShapeBottomRight();
}

void ShapeCoordinator::deleteShape()
{
    if (!m_visualization)
        return;
    m_visualization->deleteSelectedShapes();
}

void ShapeCoordinator::onOptimizePlacementClicked(bool checked, int largeur, int longueur)
{
    if (!m_visualization)
        return;
    if (checked) {
        runOptimization({0, 180, 90});
    } else {
        m_visualization->setOptimizationRunning(false);
        emit progressUpdated(0, 0);
        m_visualization->updateDimensions(largeur, longueur);
    }
}

void ShapeCoordinator::onOptimizePlacement2Clicked(bool checked, int largeur, int longueur)
{
    if (!m_visualization)
        return;
    if (checked) {
        runOptimization({0, 180});
    } else {
        m_visualization->setOptimizationRunning(false);
        emit progressUpdated(0, 0);
        m_visualization->updateDimensions(largeur, longueur);
    }
}

bool ShapeCoordinator::loadCustomShapes(const QList<QPolygonF> &polygons,
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

void ShapeCoordinator::runOptimization(const QList<int> &angles)
{
    if (!m_visualization || !m_visualization->projectModel())
        return;
    if (!m_visualization->isInteractionEnabled() || m_visualization->isOptimizationRunning())
        return;

    auto *model = m_visualization->projectModel();
    m_visualization->setSpacing(0);
    m_visualization->setOptimizationRunning(true);

    const QPainterPath prototypePath = buildPrototypePath(model->currentModel(),
                                                          model->currentLargeur(),
                                                          model->currentLongueur(),
                                                          m_visualization->isCustomMode(),
                                                          model->customShapes());

    if (prototypePath.isEmpty()) {
        m_visualization->setOptimizationRunning(false);
        emit progressUpdated(0, 0);
        return;
    }

    PlacementParams params;
    params.prototypePath = prototypePath;
    params.containerRect = m_visualization->getScene()->sceneRect();
    params.shapeCount = model->shapeCount();
    params.spacing = model->spacing();
    //params.step = 5;
    params.angles = angles;

    PlacementOptimizer optimizer;
    const PlacementResult result = optimizer.run(
        params,
        [this](int current, int total) {
            emit progressUpdated(current, total);
            // qApp->processEvents() supprimé : causait une ré-entrance
            // si le bouton était cliqué pendant l'optimisation
            return true;
        });

    if (result.cancelled) {
        m_visualization->setOptimizationRunning(false);
        emit optimizationStateChanged(false);
        emit progressUpdated(0, 0);
        return;
    }

    m_visualization->setOptimizationResult(result.placedPaths, true);
    m_visualization->setOptimizationRunning(false);
    emit optimizationStateChanged(true);
    emit progressUpdated(result.totalPositions, result.totalPositions);
}
