#pragma once

#include "InventorySnapshot.h"

class IInventoryRepository
{
public:
    virtual ~IInventoryRepository() = default;

    virtual InventorySnapshot load() const = 0;
    virtual void save(const InventorySnapshot &snapshot) const = 0;
};
