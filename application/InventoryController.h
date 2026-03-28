#pragma once

#include <QList>
#include <QPolygonF>
#include <QString>

#include "ShapeModel.h"
#include "InventoryViewState.h"
#include "Language.h"
#include "InventoryDomainTypes.h"

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

    // Queries
    bool shapeNameExists(const QString &name) const;
    QStringList getAllShapeNames(Language lang) const;
    QList<LayoutData> getLayoutsForShape(const QString &shapeName) const;
    QList<LayoutData> getLayoutsForBaseShape(ShapeModel::Type type) const;
    bool folderIsEmpty(const QString &folderName) const;
    QString parentFolderOf(const QString &folderName) const;

    // Read-only data accessors (for ViewModel — View must not call these directly)
    const QList<CustomShapeData> &customShapes() const;
    const QList<InventoryFolder> &folders() const;
    const QMap<ShapeModel::Type, QString> &baseShapeFolders() const;
    Language language() const;
    void setLanguage(Language lang);

private:
    InventoryModel &m_model;
};
