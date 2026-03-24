#pragma once

#include "Inventory.h"

namespace InventoryMutationService {

bool addLayoutToShape(QList<CustomShapeData> &customShapes, const QString &shapeName, const LayoutData &layout);
bool renameLayout(QList<CustomShapeData> &customShapes, const QString &shapeName, int index, const QString &newName);
bool deleteLayout(QList<CustomShapeData> &customShapes, const QString &shapeName, int index);
QList<LayoutData> getLayoutsForShape(const QList<CustomShapeData> &customShapes, const QString &shapeName);

bool addLayoutToBaseShape(QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                          ShapeModel::Type type,
                          const LayoutData &layout);
bool renameBaseLayout(QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                      ShapeModel::Type type,
                      int index,
                      const QString &newName);
bool deleteBaseLayout(QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                      ShapeModel::Type type,
                      int index);
QList<LayoutData> getLayoutsForBaseShape(const QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                                         ShapeModel::Type type);

bool incrementLayoutUsage(QList<CustomShapeData> &customShapes,
                          const QString &shapeName,
                          int index,
                          const QDateTime &when);
bool incrementBaseLayoutUsage(QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                              ShapeModel::Type type,
                              int index,
                              const QDateTime &when);


bool renameFolder(QList<InventoryFolder> &folders,
                  const QString &oldName,
                  const QString &newName);
bool deleteFolder(QList<InventoryFolder> &folders,
                  QList<CustomShapeData> &customShapes,
                  const QString &folderName);

bool incrementFolderUsage(QList<InventoryFolder> &folders,
                          const QString &folderName,
                          const QDateTime &when);
void incrementBaseShapeUsage(QMap<ShapeModel::Type, int> &baseUsageCount,
                             QMap<ShapeModel::Type, QDateTime> &baseLastUsed,
                             ShapeModel::Type type,
                             const QDateTime &when);
int incrementCustomShapeUsageByName(QList<CustomShapeData> &customShapes,
                                    const QString &shapeName,
                                    const QDateTime &when);

bool createFolder(QList<InventoryFolder> &folders,
                  const QString &name,
                  const QString &parentFolder);

} // namespace InventoryMutationService
