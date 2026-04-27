#include "InventoryStorage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace InventoryStorage {

QString customShapesFilePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    QDir().mkpath(dir);
    return dir + "/custom_shapes.json";
}

void loadInventoryData(
    QList<CustomShapeData> &customShapes,
    QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
    QMap<ShapeModel::Type, QString> &baseShapeFolders,
    QMap<ShapeModel::Type, int> &baseUsageCount,
    QMap<ShapeModel::Type, QDateTime> &baseLastUsed,
    QMap<ShapeModel::Type, QList<QDateTime>> &baseShapeCutHistory,
    QList<ShapeModel::Type> &baseShapeOrder,
    QList<InventoryFolder> &folders)
{
    customShapes.clear();
    baseShapeLayouts.clear();
    baseShapeFolders.clear();
    baseShapeCutHistory.clear();
    baseShapeOrder.clear();
    folders.clear();

    QFile file(customShapesFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();

    const QJsonArray foldersArr = root.value("folders").toArray();
    for (const QJsonValue &val : foldersArr) {
        if (!val.isObject())
            continue;
        const QJsonObject obj = val.toObject();
        InventoryFolder f;
        f.name = obj.value("name").toString();
        f.parentFolder = obj.value("parentFolder").toString();
        f.usageCount = obj.value("usageCount").toInt();
        qint64 ts = static_cast<qint64>(obj.value("lastUsed").toDouble());
        if (ts > 0)
            f.lastUsed = QDateTime::fromSecsSinceEpoch(ts);
        folders.append(f);
    }

    const QJsonArray shapesArr = root.value("shapes").toArray();
    for (const QJsonValue &val : shapesArr) {
        if (!val.isObject())
            continue;

        const QJsonObject obj = val.toObject();
        CustomShapeData data;
        data.name = obj.value("name").toString();
        data.folder = obj.value("folder").toString();
        data.usageCount = obj.value("usageCount").toInt();
        qint64 tsShape = static_cast<qint64>(obj.value("lastUsed").toDouble());
        if (tsShape > 0)
            data.lastUsed = QDateTime::fromSecsSinceEpoch(tsShape);
        const QJsonArray cutHistoryArr = obj.value("cutHistory").toArray();
        for (const QJsonValue &cutValue : cutHistoryArr) {
            const qint64 cutTs = static_cast<qint64>(cutValue.toDouble());
            if (cutTs > 0)
                data.cutHistory.append(QDateTime::fromSecsSinceEpoch(cutTs));
        }

        const QJsonArray polyArr = obj.value("polygons").toArray();
        for (const QJsonValue &polyVal : polyArr) {
            const QJsonArray ptArrList = polyVal.toArray();
            QPolygonF poly;
            for (const QJsonValue &ptVal : ptArrList) {
                const QJsonArray p = ptVal.toArray();
                if (p.size() >= 2)
                    poly.append(QPointF(p.at(0).toDouble(), p.at(1).toDouble()));
            }
            data.polygons.append(poly);
        }

        const QJsonArray layoutsArr = obj.value("layouts").toArray();
        for (const QJsonValue &layoutVal : layoutsArr) {
            if (!layoutVal.isObject())
                continue;
            const QJsonObject lo = layoutVal.toObject();
            LayoutData ld;
            ld.name      = lo.value("name").toString();
            ld.largeur   = lo.value("largeur").toInt();
            ld.longueur  = lo.value("longueur").toInt();
            ld.spacing   = lo.value("spacing").toInt();
            ld.usageCount = lo.value("usageCount").toInt();
            qint64 tsLayout = static_cast<qint64>(lo.value("lastUsed").toDouble());
            if (tsLayout > 0)
                ld.lastUsed = QDateTime::fromSecsSinceEpoch(tsLayout);
            const QJsonArray itemsArr = lo.value("items").toArray();
            for (const QJsonValue &itemVal : itemsArr) {
                const QJsonObject io = itemVal.toObject();
                LayoutItem li;
                li.x        = io.value("x").toDouble();
                li.y        = io.value("y").toDouble();
                li.rotation = io.value("rotation").toDouble();
                ld.items.append(li);
            }
            data.layouts.append(ld);
        }

        customShapes.append(data);
    }

    const QJsonObject baseObj = root.value("baseLayouts").toObject();
    for (auto it = baseObj.begin(); it != baseObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        const QJsonArray layoutsArr = it.value().toArray();
        QList<LayoutData> list;
        for (const QJsonValue &layoutVal : layoutsArr) {
            if (!layoutVal.isObject())
                continue;
            const QJsonObject lo = layoutVal.toObject();
            LayoutData ld;
            ld.name     = lo.value("name").toString();
            ld.largeur  = lo.value("largeur").toInt();
            ld.longueur = lo.value("longueur").toInt();
            ld.spacing  = lo.value("spacing").toInt();
            ld.usageCount = lo.value("usageCount").toInt();
            qint64 tsLayout = static_cast<qint64>(lo.value("lastUsed").toDouble());
            if (tsLayout > 0)
                ld.lastUsed = QDateTime::fromSecsSinceEpoch(tsLayout);
            const QJsonArray itemsArr = lo.value("items").toArray();
            for (const QJsonValue &itemVal : itemsArr) {
                const QJsonObject io = itemVal.toObject();
                LayoutItem li;
                li.x        = io.value("x").toDouble();
                li.y        = io.value("y").toDouble();
                li.rotation = io.value("rotation").toDouble();
                ld.items.append(li);
            }
            list.append(ld);
        }
        baseShapeLayouts[type] = list;
    }

    const QJsonObject baseFoldersObj = root.value("baseFolders").toObject();
    for (auto it = baseFoldersObj.begin(); it != baseFoldersObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        baseShapeFolders[type] = it.value().toString();
    }

    const QJsonObject usageObj = root.value("baseUsageCount").toObject();
    for (auto it = usageObj.begin(); it != usageObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        baseUsageCount[type] = it.value().toInt();
    }

    const QJsonObject lastUsedObj = root.value("baseLastUsed").toObject();
    for (auto it = lastUsedObj.begin(); it != lastUsedObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        qint64 ts = static_cast<qint64>(it.value().toDouble());
        if (ts > 0)
            baseLastUsed[type] = QDateTime::fromSecsSinceEpoch(ts);
    }

    const QJsonObject baseCutHistoryObj = root.value("baseCutHistory").toObject();
    for (auto it = baseCutHistoryObj.begin(); it != baseCutHistoryObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        const QJsonArray historyArr = it.value().toArray();
        QList<QDateTime> history;
        for (const QJsonValue &value : historyArr) {
            const qint64 ts = static_cast<qint64>(value.toDouble());
            if (ts > 0)
                history.append(QDateTime::fromSecsSinceEpoch(ts));
        }
        baseShapeCutHistory[type] = history;
    }
}

void saveInventoryData(
    const QList<CustomShapeData> &customShapes,
    const QMap<ShapeModel::Type, QList<LayoutData>> &baseShapeLayouts,
    const QMap<ShapeModel::Type, QString> &baseShapeFolders,
    const QMap<ShapeModel::Type, int> &baseUsageCount,
    const QMap<ShapeModel::Type, QDateTime> &baseLastUsed,
    const QMap<ShapeModel::Type, QList<QDateTime>> &baseShapeCutHistory,
    const QList<InventoryFolder> &folders)
{
    QJsonArray shapesArr;
    for (const CustomShapeData &data : customShapes) {
        QJsonObject obj;
        obj["name"] = data.name;
        obj["folder"] = data.folder;
        obj["usageCount"] = data.usageCount;
        obj["lastUsed"] = data.lastUsed.isValid() ? static_cast<qint64>(data.lastUsed.toSecsSinceEpoch()) : 0;
        QJsonArray cutHistoryArr;
        for (const QDateTime &cutAt : data.cutHistory)
            cutHistoryArr.append(static_cast<qint64>(cutAt.toSecsSinceEpoch()));
        obj["cutHistory"] = cutHistoryArr;

        QJsonArray polyArr;
        for (const QPolygonF &poly : data.polygons) {
            QJsonArray pointsArr;
            for (const QPointF &pt : poly) {
                QJsonArray ptArr;
                ptArr.append(pt.x());
                ptArr.append(pt.y());
                pointsArr.append(ptArr);
            }
            polyArr.append(pointsArr);
        }
        obj["polygons"] = polyArr;

        QJsonArray layoutsArr;
        for (const LayoutData &ld : data.layouts) {
            QJsonObject lo;
            lo["name"]     = ld.name;
            lo["largeur"]  = ld.largeur;
            lo["longueur"] = ld.longueur;
            lo["spacing"]  = ld.spacing;
            lo["usageCount"] = ld.usageCount;
            lo["lastUsed"] = ld.lastUsed.isValid() ? static_cast<qint64>(ld.lastUsed.toSecsSinceEpoch()) : 0;
            QJsonArray itemsArr;
            for (const LayoutItem &li : ld.items) {
                QJsonObject io;
                io["x"]        = li.x;
                io["y"]        = li.y;
                io["rotation"] = li.rotation;
                itemsArr.append(io);
            }
            lo["items"] = itemsArr;
            layoutsArr.append(lo);
        }
        obj["layouts"] = layoutsArr;

        shapesArr.append(obj);
    }

    QJsonObject baseObj;
    for (auto it = baseShapeLayouts.constBegin(); it != baseShapeLayouts.constEnd(); ++it) {
        QJsonArray layoutsArr;
        for (const LayoutData &ld : it.value()) {
            QJsonObject lo;
            lo["name"]     = ld.name;
            lo["largeur"]  = ld.largeur;
            lo["longueur"] = ld.longueur;
            lo["spacing"]  = ld.spacing;
            lo["usageCount"] = ld.usageCount;
            lo["lastUsed"] = ld.lastUsed.isValid() ? static_cast<qint64>(ld.lastUsed.toSecsSinceEpoch()) : 0;
            QJsonArray itemsArr;
            for (const LayoutItem &li : ld.items) {
                QJsonObject io;
                io["x"]        = li.x;
                io["y"]        = li.y;
                io["rotation"] = li.rotation;
                itemsArr.append(io);
            }
            lo["items"] = itemsArr;
            layoutsArr.append(lo);
        }
        baseObj[QString::number(static_cast<int>(it.key()))] = layoutsArr;
    }

    QJsonObject baseFoldersObj;
    for (auto it = baseShapeFolders.constBegin(); it != baseShapeFolders.constEnd(); ++it) {
        baseFoldersObj[QString::number(static_cast<int>(it.key()))] = it.value();
    }

    QJsonObject usageObj;
    for (auto it = baseUsageCount.constBegin(); it != baseUsageCount.constEnd(); ++it) {
        usageObj[QString::number(static_cast<int>(it.key()))] = it.value();
    }

    QJsonObject lastUsedObj;
    for (auto it = baseLastUsed.constBegin(); it != baseLastUsed.constEnd(); ++it) {
        lastUsedObj[QString::number(static_cast<int>(it.key()))] = static_cast<qint64>(it.value().toSecsSinceEpoch());
    }

    QJsonObject baseCutHistoryObj;
    for (auto it = baseShapeCutHistory.constBegin(); it != baseShapeCutHistory.constEnd(); ++it) {
        QJsonArray historyArr;
        for (const QDateTime &cutAt : it.value())
            historyArr.append(static_cast<qint64>(cutAt.toSecsSinceEpoch()));
        baseCutHistoryObj[QString::number(static_cast<int>(it.key()))] = historyArr;
    }

    QJsonArray foldersArr;
    for (const InventoryFolder &f : folders) {
        QJsonObject fo;
        fo["name"] = f.name;
        fo["parentFolder"] = f.parentFolder;
        fo["usageCount"] = f.usageCount;
        fo["lastUsed"] = f.lastUsed.isValid() ? static_cast<qint64>(f.lastUsed.toSecsSinceEpoch()) : 0;
        foldersArr.append(fo);
    }

    QJsonObject rootObj;
    rootObj["shapes"]      = shapesArr;
    rootObj["baseLayouts"] = baseObj;
    rootObj["baseFolders"] = baseFoldersObj;
    rootObj["baseUsageCount"] = usageObj;
    rootObj["baseLastUsed"] = lastUsedObj;
    rootObj["baseCutHistory"] = baseCutHistoryObj;
    rootObj["folders"]     = foldersArr;

    QFile file(customShapesFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(rootObj).toJson());
        file.close();
    }
}

} // namespace InventoryStorage
