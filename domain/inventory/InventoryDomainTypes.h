#pragma once

#include <QDateTime>
#include <QList>
#include <QPolygonF>
#include <QString>

#include "ShapeModel.h"

// Single item in a saved layout
struct LayoutItem {
    double x {0};
    double y {0};
    double rotation {0};
};

// Complete layout for a custom shape
struct LayoutData {
    QString name;
    int largeur  {0};
    int longueur {0};
    int spacing  {0};
    QList<LayoutItem> items;
    int usageCount {0};
    QDateTime lastUsed;
};

// Custom shape definition (can contain multiple polygons) and its saved layouts
struct CustomShapeData {
    QList<QPolygonF> polygons;
    QString name;
    QString folder;
    QList<LayoutData> layouts;
    int usageCount {0};
    QDateTime lastUsed;
    QList<QDateTime> cutHistory;
};

// Folder in inventory tree
struct InventoryFolder {
    QString name;
    QString parentFolder;  // empty if root folder
    int usageCount {0};
    QDateTime lastUsed;
};

struct QuickShapeEntry {
    QString name;
    bool isBaseShape {false};
    ShapeModel::Type baseType {ShapeModel::Type::Circle};
    int recentCutCount {0};
    QDateTime lastCutAt;
};
