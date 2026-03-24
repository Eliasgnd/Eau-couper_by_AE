#pragma once

#include "Inventory.h"

namespace InventoryQueryService {

bool shapeNameExists(const QList<CustomShapeData> &customShapes, const QString &name);

QStringList getAllShapeNames(const QList<CustomShapeData> &customShapes, Language language);

bool folderIsEmpty(const QString &folderName,
                   const QList<InventoryFolder> &folders,
                   const QList<CustomShapeData> &customShapes,
                   const QMap<ShapeModel::Type, QString> &baseShapeFolders);

bool folderContainsMatchingShape(const QString &folderName,
                                 const QString &text,
                                 const QList<InventoryFolder> &folders,
                                 const QList<CustomShapeData> &customShapes,
                                 const QMap<ShapeModel::Type, QString> &baseShapeFolders,
                                 Language language);

QString parentFolderOf(const QList<InventoryFolder> &folders, const QString &folderName);

} // namespace InventoryQueryService
