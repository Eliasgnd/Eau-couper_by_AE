#include "InventoryModel.h"

#include "InventoryStorage.h"

void InventoryModel::load()
{
    InventoryStorage::loadInventoryData(m_customShapes,
                                        m_baseShapeLayouts,
                                        m_baseShapeFolders,
                                        m_baseUsageCount,
                                        m_baseLastUsed,
                                        m_baseShapeOrder,
                                        m_folders);
}

void InventoryModel::save() const
{
    InventoryStorage::saveInventoryData(m_customShapes,
                                        m_baseShapeLayouts,
                                        m_baseShapeFolders,
                                        m_baseUsageCount,
                                        m_baseLastUsed,
                                        m_folders);
}

QString InventoryModel::customShapesFilePath() const
{
    return InventoryStorage::customShapesFilePath();
}
