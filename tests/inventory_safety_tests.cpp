#include <QtTest>
#include "../GeometryUtils.h"
#include "inventory_safety_tests.h"

void InventorySafetyTests::malformedShapeProxied()
{
    setSafeMode(true);
    QList<QPolygonF> polys;
    QPolygonF p;
    p << QPointF(0,0) << QPointF(10,10) << QPointF(0,10) << QPointF(10,0) << QPointF(0,0);
    polys << p;
    QString warn;
    QVERIFY(!validateAndProxyPolygons(polys, true, &warn));
    QCOMPARE(warn, QStringLiteral("Invalid geometry replaced with proxy"));
    QCOMPARE(polys.size(), 1);
    setSafeMode(false);
}

void InventorySafetyTests::invalidShapeRejected()
{
    setSafeMode(false);
    QList<QPolygonF> polys;
    QPolygonF p;
    p << QPointF(0,0) << QPointF(10,10) << QPointF(0,10) << QPointF(10,0) << QPointF(0,0);
    polys << p;
    QString warn;
    QVERIFY(!validateAndProxyPolygons(polys, false, &warn));
    QVERIFY(warn.isEmpty());
}

void InventorySafetyTests::hugePolygonHandled()
{
    QList<QPolygonF> polys;
    QPolygonF p;
    p << QPointF(0,0);
    for (int i = 1; i < 20000; ++i)
        p << QPointF(i,0);
    p << QPointF(19999,1) << QPointF(0,1) << QPointF(0,0);
    polys << p;
    QVERIFY(sanitizePolygons(polys));
    QPainterPath path;
    path.addPolygon(polys.first());
    QVERIFY(isPathTooComplex(path, kMaxPathElements));
}

