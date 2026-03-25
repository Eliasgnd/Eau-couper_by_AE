#pragma once

#include <QDateTime>
#include <QList>
#include <QMap>

#include "InventoryDomainTypes.h"
#include "ShapeModel.h"

struct InventorySnapshot
{
    QList<CustomShapeData> customShapes;
    QMap<ShapeModel::Type, QList<LayoutData>> baseShapeLayouts;
    QMap<ShapeModel::Type, QString> baseShapeFolders;
    QMap<ShapeModel::Type, int> baseUsageCount;
    QMap<ShapeModel::Type, QDateTime> baseLastUsed;
    QList<ShapeModel::Type> baseShapeOrder;
    QList<InventoryFolder> folders;
};
