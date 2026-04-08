#include "ShapeCoordinator.h"

#include "ShapeVisualization.h"
#include "GeometryUtils.h"
#include "PlacementOptimizer.h"
#include "ShapeVisualizationViewModel.h"

#include <QApplication>
#include <QMessageBox>

ShapeCoordinator::ShapeCoordinator(ShapeVisualization *visualization, QObject *parent)
    : QObject(parent), m_visualization(visualization)
{
}

namespace {
QPainterPath normalizePathToOrigin(const QPainterPath &path)
{
    const QRectF bounds = path.boundingRect();
    if (!bounds.isValid())
        return path;
    return path.translated(-bounds.left(), -bounds.top());
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
    auto *model = m_visualization->projectModel();
    if (!model)
        return;
    model->setShapeConfig(static_cast<int>(m_selectedShapeType), largeur, longueur, count);
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
    m_visualization->setPredefinedMode();  // remet le ViewModel en mode prédéfini, déjà appelle redraw
    auto *model = m_visualization->projectModel();
    if (model)
        model->setCurrentModel(static_cast<int>(type));  // déclenche dataChanged → redraw avec le bon type
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

    const QPainterPath prototypePath = normalizePathToOrigin(model->prototypeShapePath());

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
