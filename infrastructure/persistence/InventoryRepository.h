#pragma once

#include "InventorySnapshot.h"

class InventoryRepository
{
public:
    InventorySnapshot load() const;
    void save(const InventorySnapshot &snapshot) const;
    QString customShapesFilePath() const;
};
