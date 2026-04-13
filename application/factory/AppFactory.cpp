#include "AppFactory.h"

#include "InventoryModel.h"
#include "InventoryController.h"
#include "InventoryViewModel.h"
#include "Inventory.h"

Inventory *AppFactory::createInventory()
{
    auto *model      = new InventoryModel();
    auto *controller = new InventoryController(*model);
    controller->initialize();
    auto *vm         = new InventoryViewModel(*controller);
    return new Inventory(vm);
}
