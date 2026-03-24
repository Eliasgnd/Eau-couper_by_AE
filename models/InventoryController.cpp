#include "InventoryController.h"

#include "InventoryModel.h"
#include "InventoryMutationService.h"

#include <QDateTime>

InventoryController::InventoryController(InventoryModel &model)
    : m_model(model)
{
}

bool InventoryController::createFolder(const QString &name, const QString &parentFolder)
{
    if (!InventoryMutationService::createFolder(m_model.folders(), name, parentFolder)) {
        return false;
    }

    m_model.save();
    return true;
}

void InventoryController::onFolderSelected(const QString &folderName)
{
    if (InventoryMutationService::incrementFolderUsage(m_model.folders(),
                                                       folderName,
                                                       QDateTime::currentDateTime())) {
        m_model.save();
    }
}

void InventoryController::onBaseShapeSelected(ShapeModel::Type type)
{
    InventoryMutationService::incrementBaseShapeUsage(m_model.baseUsageCount(),
                                                      m_model.baseLastUsed(),
                                                      type,
                                                      QDateTime::currentDateTime());
    m_model.save();
}

bool InventoryController::onCustomShapeSelected(const QString &shapeName,
                                                QList<QPolygonF> &polygonsOut,
                                                QString &resolvedNameOut)
{
    const int shapeIndex = InventoryMutationService::incrementCustomShapeUsageByName(m_model.customShapes(),
                                                                                      shapeName,
                                                                                      QDateTime::currentDateTime());
    if (shapeIndex < 0) {
        return false;
    }

    m_model.save();
    polygonsOut = m_model.customShapes()[shapeIndex].polygons;
    resolvedNameOut = m_model.customShapes()[shapeIndex].name;
    return true;
}
