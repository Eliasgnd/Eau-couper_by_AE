#pragma once

#include <QList>
#include <QPolygonF>
#include <QString>

#include "ShapeModel.h"

class InventoryModel;

class InventoryController
{
public:
    explicit InventoryController(InventoryModel &model);

    bool createFolder(const QString &name, const QString &parentFolder);
    void onFolderSelected(const QString &folderName);
    void onBaseShapeSelected(ShapeModel::Type type);
    bool onCustomShapeSelected(const QString &shapeName,
                               QList<QPolygonF> &polygonsOut,
                               QString &resolvedNameOut);

private:
    InventoryModel &m_model;
};
