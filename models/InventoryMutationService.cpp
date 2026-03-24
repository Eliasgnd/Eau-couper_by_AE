#include "InventoryMutationService.h"

#include <algorithm>

namespace InventoryMutationService {

bool addLayoutToShape(QList<CustomShapeData> &customShapes, const QString &shapeName, const LayoutData &layout)
{
    for (CustomShapeData &data : customShapes) {
        if (data.name == shapeName) {
            data.layouts.append(layout);
            return true;
        }
    }
    return false;
}

bool renameLayout(QList<CustomShapeData> &customShapes, const QString &shapeName, int index, const QString &newName)
{
    for (CustomShapeData &data : customShapes) {
        if (data.name == shapeName && index >= 0 && index < data.layouts.size()) {
            data.layouts[index].name = newName;
            return true;
        }
    }
    return false;
}

bool deleteLayout(QList<CustomShapeData> &customShapes, const QString &shapeName, int index)
{
    for (CustomShapeData &data : customShapes) {
        if (data.name == shapeName && index >= 0 && index < data.layouts.size()) {
            data.layouts.removeAt(index);
            return true;
        }
    }
    return false;
}

QList<LayoutData> getLayoutsForShape(const QList<CustomShapeData> &customShapes, const QString &shapeName)
{
    for (const CustomShapeData &data : customShapes) {
        if (data.name == shapeName)
            return data.layouts;
    }
    return {};
}

bool addLayoutToBaseShape(QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                          ShapeModel::Type type,
                          const LayoutData &layout)
{
    baseShapeLayouts[type].append(layout);
    return true;
}

bool renameBaseLayout(QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                      ShapeModel::Type type,
                      int index,
                      const QString &newName)
{
    auto it = baseShapeLayouts.find(type);
    if (it != baseShapeLayouts.end() && index >= 0 && index < it.value().size()) {
        it.value()[index].name = newName;
        return true;
    }
    return false;
}

bool deleteBaseLayout(QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                      ShapeModel::Type type,
                      int index)
{
    auto it = baseShapeLayouts.find(type);
    if (it != baseShapeLayouts.end() && index >= 0 && index < it.value().size()) {
        it.value().removeAt(index);
        return true;
    }
    return false;
}

QList<LayoutData> getLayoutsForBaseShape(const QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                                         ShapeModel::Type type)
{
    return baseShapeLayouts.value(type);
}

bool incrementLayoutUsage(QList<CustomShapeData> &customShapes,
                          const QString &shapeName,
                          int index,
                          const QDateTime &when)
{
    for (CustomShapeData &data : customShapes) {
        if (data.name == shapeName && index >= 0 && index < data.layouts.size()) {
            data.layouts[index].usageCount++;
            data.layouts[index].lastUsed = when;
            return true;
        }
    }
    return false;
}

bool incrementBaseLayoutUsage(QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
                              ShapeModel::Type type,
                              int index,
                              const QDateTime &when)
{
    auto it = baseShapeLayouts.find(type);
    if (it != baseShapeLayouts.end() && index >= 0 && index < it.value().size()) {
        it.value()[index].usageCount++;
        it.value()[index].lastUsed = when;
        return true;
    }
    return false;
}


bool renameFolder(QList<InventoryFolder> &folders,
                  const QString &oldName,
                  const QString &newName)
{
    for (InventoryFolder &folder : folders) {
        if (folder.name == oldName) {
            folder.name = newName;
            return true;
        }
    }
    return false;
}

bool deleteFolder(QList<InventoryFolder> &folders,
                  QList<CustomShapeData> &customShapes,
                  const QString &folderName)
{
    const auto oldSize = folders.size();
    folders.erase(std::remove_if(folders.begin(), folders.end(),
                                 [&](const InventoryFolder &f) { return f.name == folderName; }),
                  folders.end());

    for (CustomShapeData &shape : customShapes) {
        if (shape.folder == folderName)
            shape.folder.clear();
    }

    return folders.size() != oldSize;
}

bool incrementFolderUsage(QList<InventoryFolder> &folders,
                          const QString &folderName,
                          const QDateTime &when)
{
    for (InventoryFolder &folder : folders) {
        if (folder.name == folderName) {
            folder.usageCount++;
            folder.lastUsed = when;
            return true;
        }
    }
    return false;
}

void incrementBaseShapeUsage(QMap<ShapeModel::Type, int> &baseUsageCount,
                             QMap<ShapeModel::Type, QDateTime> &baseLastUsed,
                             ShapeModel::Type type,
                             const QDateTime &when)
{
    baseUsageCount[type]++;
    baseLastUsed[type] = when;
}

int incrementCustomShapeUsageByName(QList<CustomShapeData> &customShapes,
                                    const QString &shapeName,
                                    const QDateTime &when)
{
    for (int i = 0; i < customShapes.size(); ++i) {
        CustomShapeData &shape = customShapes[i];
        if (shape.name == shapeName) {
            shape.usageCount++;
            shape.lastUsed = when;
            return i;
        }
    }
    return -1;
}


bool createFolder(QList<InventoryFolder> &folders,
                  const QString &name,
                  const QString &parentFolder)
{
    if (name.trimmed().isEmpty())
        return false;

    folders.append({name.trimmed(), parentFolder});
    return true;
}

} // namespace InventoryMutationService
