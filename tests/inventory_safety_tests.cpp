#include <QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "../SafeInventoryLoader.h"
#include "../GeometryUtils.h"
#include "inventory_safety_tests.h"

void InventorySafetyTests::malformedShapeUsesProxy()
{
    setSafeMode(true);
    QTemporaryDir dir;
    QString path = dir.filePath("custom_shapes.json");
    QJsonArray points;
    // self-intersecting polygon
    QJsonArray p1; p1.append(0); p1.append(0); points.append(p1);
    QJsonArray p2; p2.append(10); p2.append(10); points.append(p2);
    QJsonArray p3; p3.append(0); p3.append(10); points.append(p3);
    QJsonArray p4; p4.append(10); p4.append(0); points.append(p4);
    points.append(p1); // close
    QJsonArray polys; polys.append(points);
    QJsonObject shape; shape["name"] = "bad"; shape["polygons"] = polys;
    QJsonArray shapes; shapes.append(shape);
    QJsonObject root; root["version"] = 2; root["shapes"] = shapes;
    QJsonDocument doc(root);
    QFile f(path); f.open(QIODevice::WriteOnly); f.write(doc.toJson()); f.close();

    QList<SafeShape> loaded;
    QVERIFY(loadInventoryFileSafe(path, loaded));
    QCOMPARE(loaded.size(), 1);
    QVERIFY(loaded[0].proxyUsed);
    setSafeMode(false);
}

void InventorySafetyTests::corruptedFileHandled()
{
    setSafeMode(true);
    QTemporaryDir dir;
    QString path = dir.filePath("custom_shapes.json");
    QFile f(path); f.open(QIODevice::WriteOnly); f.write("not json"); f.close();
    QList<SafeShape> loaded;
    QVERIFY(!loadInventoryFileSafe(path, loaded));
    setSafeMode(false);
}

void InventorySafetyTests::hugeInventoryHandled()
{
    setSafeMode(true);
    QTemporaryDir dir;
    QString path = dir.filePath("custom_shapes.json");
    QJsonArray points; QJsonArray first;
    first.append(0); first.append(0); points.append(first);
    for (int i = 1; i < 20000; ++i) {
        QJsonArray pt; pt.append(i); pt.append((i%2)?1:0); points.append(pt);
    }
    points.append(first);
    QJsonArray polys; polys.append(points);
    QJsonObject shape; shape["name"] = "huge"; shape["polygons"] = polys;
    QJsonArray shapes; shapes.append(shape);
    QJsonObject root; root["version"] = 2; root["shapes"] = shapes;
    QJsonDocument doc(root);
    QFile f(path); f.open(QIODevice::WriteOnly); f.write(doc.toJson()); f.close();
    QList<SafeShape> loaded;
    QVERIFY(loadInventoryFileSafe(path, loaded));
    QCOMPARE(loaded.size(), 1);
    setSafeMode(false);
}

