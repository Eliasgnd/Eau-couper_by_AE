#pragma once

#include "InventoryDomainTypes.h"
#include "ShapeModel.h"

namespace InventoryStorage {

QString customShapesFilePath();

void loadInventoryData(
    QList<CustomShapeData> &customShapes,
    QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
    QMap<ShapeModel::Type, QString> &baseShapeFolders,
    QMap<ShapeModel::Type, int> &baseUsageCount,
    QMap<ShapeModel::Type, QDateTime> &baseLastUsed,
    QList<ShapeModel::Type> &baseShapeOrder,
    QList<InventoryFolder> &folders);

void saveInventoryData(
    const QList<CustomShapeData> &customShapes,
    const QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
    const QMap<ShapeModel::Type, QString> &baseShapeFolders,
    const QMap<ShapeModel::Type, int> &baseUsageCount,
    const QMap<ShapeModel::Type, QDateTime> &baseLastUsed,
    const QList<InventoryFolder> &folders);

} // namespace InventoryStorage
