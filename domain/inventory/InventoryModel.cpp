#include "InventoryModel.h"

#include "InventoryRepository.h"

InventoryModel::InventoryModel(InventoryRepository *repository)
    : m_repository(repository ? repository : new InventoryRepository()),
      m_ownsRepository(repository == nullptr)
{
}

InventoryModel::~InventoryModel()
{
    if (m_ownsRepository)
        delete m_repository;
}

void InventoryModel::load()
{
    const InventorySnapshot snapshot = m_repository->load();
    m_customShapes = snapshot.customShapes;
    m_baseShapeLayouts = snapshot.baseShapeLayouts;
    m_baseShapeFolders = snapshot.baseShapeFolders;
    m_baseUsageCount = snapshot.baseUsageCount;
    m_baseLastUsed = snapshot.baseLastUsed;
    m_baseShapeOrder = snapshot.baseShapeOrder;
    m_folders = snapshot.folders;
}

void InventoryModel::save() const
{
    InventorySnapshot snapshot;
    snapshot.customShapes = m_customShapes;
    snapshot.baseShapeLayouts = m_baseShapeLayouts;
    snapshot.baseShapeFolders = m_baseShapeFolders;
    snapshot.baseUsageCount = m_baseUsageCount;
    snapshot.baseLastUsed = m_baseLastUsed;
    snapshot.baseShapeOrder = m_baseShapeOrder;
    snapshot.folders = m_folders;
    m_repository->save(snapshot);
}

QString InventoryModel::customShapesFilePath() const
{
    return m_repository->customShapesFilePath();
}
