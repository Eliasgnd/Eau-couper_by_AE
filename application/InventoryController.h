#pragma once

#include <QList>
#include <QPolygonF>
#include <QString>

#include "ShapeModel.h"
#include "InventoryViewState.h"

class InventoryModel;
struct LayoutData;

class InventoryController
{
public:
    explicit InventoryController(InventoryModel &model);
    void initialize();
    void addCustomShape(const QList<QPolygonF> &polygons, const QString &name);

    bool createFolder(const QString &name, const QString &parentFolder);
    bool renameFolder(const QString &oldName, const QString &newName);
    bool deleteFolder(const QString &folderName);
    void onFolderSelected(const QString &folderName);
    void onBaseShapeSelected(ShapeModel::Type type);
    bool onCustomShapeSelected(const QString &shapeName,
                               QList<QPolygonF> &polygonsOut,
                               QString &resolvedNameOut);
    bool renameCustomShape(int index, const QString &newName);
    bool deleteCustomShape(int index);
    bool moveCustomShapeToFolder(int index, const QString &folderName);
    bool removeCustomShapeFromFolderToParent(int index);
    bool moveBaseShapeToFolder(ShapeModel::Type type, const QString &folderName);
    bool removeBaseShapeFromFolderToParent(ShapeModel::Type type);

    bool addLayoutToCustomShape(const QString &shapeName, const LayoutData &layout);
    bool renameLayoutForCustomShape(const QString &shapeName, int index, const QString &newName);
    bool deleteLayoutForCustomShape(const QString &shapeName, int index);
    bool addLayoutToBaseShape(ShapeModel::Type type, const LayoutData &layout);
    bool renameLayoutForBaseShape(ShapeModel::Type type, int index, const QString &newName);
    bool deleteLayoutForBaseShape(ShapeModel::Type type, int index);
    bool incrementLayoutUsageForCustomShape(const QString &shapeName, int index);
    bool incrementLayoutUsageForBaseShape(ShapeModel::Type type, int index);

    InventoryViewState buildRootState(const QString &filterText,
                                      InventorySortMode sortMode,
                                      InventoryFilterMode filterMode) const;
    InventoryViewState buildFolderState(const QString &folderName,
                                        const QString &filterText,
                                        InventorySortMode sortMode,
                                        InventoryFilterMode filterMode) const;

private:
    InventoryModel &m_model;
};
