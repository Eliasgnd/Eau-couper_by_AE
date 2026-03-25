#include "InventorySortFilterService.h"

#include <algorithm>

namespace {
int kindPriority(InventoryViewItem::Kind kind)
{
    return kind == InventoryViewItem::Kind::Folder ? 0 : 1;
}
}

void InventorySortFilterService::sortItems(QList<InventoryViewItem> &items, InventorySortMode sortMode)
{
    std::sort(items.begin(), items.end(), [sortMode](const InventoryViewItem &a, const InventoryViewItem &b) {
        const int aPriority = kindPriority(a.kind);
        const int bPriority = kindPriority(b.kind);
        if (aPriority != bPriority)
            return aPriority < bPriority;

        switch (sortMode) {
        case InventorySortMode::NameAsc:
            return a.displayName.toLower() < b.displayName.toLower();
        case InventorySortMode::NameDesc:
            return a.displayName.toLower() > b.displayName.toLower();
        case InventorySortMode::FrequentUsage:
            return a.usageCount > b.usageCount;
        case InventorySortMode::RecentToOld:
            return a.lastUsed > b.lastUsed;
        case InventorySortMode::OldToRecent:
            return a.lastUsed < b.lastUsed;
        }
        return false;
    });
}
