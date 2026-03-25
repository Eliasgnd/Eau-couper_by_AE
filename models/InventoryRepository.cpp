#include "InventoryRepository.h"

#include "InventoryStorage.h"

InventorySnapshot InventoryRepository::load() const
{
    InventorySnapshot snapshot;
    InventoryStorage::loadInventoryData(snapshot.customShapes,
                                        snapshot.baseShapeLayouts,
                                        snapshot.baseShapeFolders,
                                        snapshot.baseUsageCount,
                                        snapshot.baseLastUsed,
                                        snapshot.baseShapeOrder,
                                        snapshot.folders);
    return snapshot;
}

void InventoryRepository::save(const InventorySnapshot &snapshot) const
{
    InventoryStorage::saveInventoryData(snapshot.customShapes,
                                        snapshot.baseShapeLayouts,
                                        snapshot.baseShapeFolders,
                                        snapshot.baseUsageCount,
                                        snapshot.baseLastUsed,
                                        snapshot.folders);
}

QString InventoryRepository::customShapesFilePath() const
{
    return InventoryStorage::customShapesFilePath();
}
