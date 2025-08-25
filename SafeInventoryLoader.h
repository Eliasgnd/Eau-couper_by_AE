#pragma once
#include <QList>
#include <QPolygonF>
#include <QString>

struct SafeShape {
    QList<QPolygonF> polygons;
    QString name;
    bool proxyUsed{false};
};

// Load shapes from an inventory JSON file, validating each shape and
// substituting safe proxies when necessary. Returns false if the file could not
// be parsed. The output list is cleared before use.
bool loadInventoryFileSafe(const QString &path, QList<SafeShape> &out);
