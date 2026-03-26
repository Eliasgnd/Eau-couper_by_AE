#pragma once

#include "IInventoryRepository.h"
#include "InventorySnapshot.h"

class InventoryRepository : public IInventoryRepository
{
public:
    InventorySnapshot load() const override;
    void save(const InventorySnapshot &snapshot) const override;
    QString customShapesFilePath() const;
};
