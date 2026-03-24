#pragma once

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>

#include "Inventory.h"

class InventoryModel
{
public:
    InventoryModel() = default;

    void load();
    void save() const;
    QString customShapesFilePath() const;

    QList<CustomShapeData> &customShapes() { return m_customShapes; }
    const QList<CustomShapeData> &customShapes() const { return m_customShapes; }

    QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts() { return m_baseShapeLayouts; }
    const QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts() const { return m_baseShapeLayouts; }

    QMap<ShapeModel::Type, QString> &baseShapeFolders() { return m_baseShapeFolders; }
    const QMap<ShapeModel::Type, QString> &baseShapeFolders() const { return m_baseShapeFolders; }

    QMap<ShapeModel::Type, int> &baseUsageCount() { return m_baseUsageCount; }
    const QMap<ShapeModel::Type, int> &baseUsageCount() const { return m_baseUsageCount; }

    QMap<ShapeModel::Type, QDateTime> &baseLastUsed() { return m_baseLastUsed; }
    const QMap<ShapeModel::Type, QDateTime> &baseLastUsed() const { return m_baseLastUsed; }

    QList<ShapeModel::Type> &baseShapeOrder() { return m_baseShapeOrder; }
    const QList<ShapeModel::Type> &baseShapeOrder() const { return m_baseShapeOrder; }

    QList<InventoryFolder> &folders() { return m_folders; }
    const QList<InventoryFolder> &folders() const { return m_folders; }

    Language &languageRef() { return m_currentLanguage; }
    const Language &languageRef() const { return m_currentLanguage; }

private:
    QList<CustomShapeData> m_customShapes;
    QMap<ShapeModel::Type, QList<LayoutData>> m_baseShapeLayouts;
    QMap<ShapeModel::Type, QString> m_baseShapeFolders;
    QMap<ShapeModel::Type, int> m_baseUsageCount;
    QMap<ShapeModel::Type, QDateTime> m_baseLastUsed;
    QList<ShapeModel::Type> m_baseShapeOrder;
    QList<InventoryFolder> m_folders;
    Language m_currentLanguage {Language::French};
};
