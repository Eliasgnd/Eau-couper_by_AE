#include "shape_manager_tests.h"

#include "drawing/ShapeManager.h"

#include <QtTest>

void ShapeManagerTests::addAndRemoveShape()
{
    ShapeManager manager;
    QPainterPath p;
    p.moveTo(0, 0);
    p.lineTo(10, 0);

    manager.addShape(p, 1);
    QCOMPARE(manager.shapes().size(), 1);

    QVERIFY(manager.removeShape(0));
    QCOMPARE(manager.shapes().size(), 0);
}

void ShapeManagerTests::selectionBounds()
{
    ShapeManager manager;
    QPainterPath a;
    a.addRect(0, 0, 10, 10);
    QPainterPath b;
    b.addRect(20, 20, 5, 5);

    manager.addShape(a, 1);
    manager.addShape(b, 2);
    manager.setSelectedShapes({0, 1});

    QCOMPARE(manager.selectedShapesBounds(), QRectF(0, 0, 25, 25));
}

void ShapeManagerTests::undoRestoresPreviousState()
{
    ShapeManager manager;
    QPainterPath p1;
    p1.addRect(0, 0, 10, 10);
    QPainterPath p2;
    p2.addRect(10, 10, 5, 5);

    manager.addShape(p1, 1);
    manager.pushState();
    manager.addShape(p2, 2);
    QCOMPARE(manager.shapes().size(), 2);

    manager.undoLastAction();
    QCOMPARE(manager.shapes().size(), 1);
}
