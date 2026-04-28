#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QVariant>

enum class InventorySortMode {
    NameAsc = 0,
    NameDesc = 1,
    FrequentUsage = 2,
    RecentToOld = 3,
    OldToRecent = 4
};

enum class InventoryFilterMode {
    All = 0,
    FoldersOnly = 1,
    ShapesOnly = 2
};

struct InventoryViewItem
{
    enum class Kind { Folder, BaseShape, CustomShape };

    Kind kind {Kind::Folder};
    QString id;
    QString displayName;
    QString iconKey;
    int usageCount {0};
    QDateTime lastUsed;
    bool isInFolder {false};
    QString folderName;
    QVariant payload;
};

struct InventoryViewState
{
    QList<InventoryViewItem> items;
    bool inFolderView {false};
    QString currentFolder;
    QString searchText;
    InventorySortMode sortMode {InventorySortMode::NameAsc};
    InventoryFilterMode filterMode {InventoryFilterMode::All};
};
