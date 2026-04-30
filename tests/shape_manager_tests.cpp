#include "shape_manager_tests.h"

#include "ShapeManager.h"

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

void ShapeManagerTests::mergeShapesByNearestEndpointsUsesClosestEnds()
{
    ShapeManager manager;

    QPainterPath left;
    left.moveTo(0, 0);
    left.lineTo(10, 0);

    QPainterPath right;
    right.moveTo(30, 0);
    right.lineTo(20, 0);

    manager.addShape(left, 1);
    manager.addShape(right, 2);

    manager.mergeShapesByNearestEndpoints(0, 1);

    QCOMPARE(manager.shapes().size(), 1);

    const QPainterPath merged = manager.shapes().front().path;
    QCOMPARE(merged.elementCount(), 4);
    QCOMPARE(merged.elementAt(0).x, 0.0);
    QCOMPARE(merged.elementAt(0).y, 0.0);
    QCOMPARE(merged.elementAt(1).x, 10.0);
    QCOMPARE(merged.elementAt(2).x, 20.0);
    QCOMPARE(merged.elementAt(3).x, 30.0);
}

void ShapeManagerTests::geometryCacheUpdatesAfterMutations()
{
    ShapeManager manager;
    QPainterPath p;
    p.addRect(10, 20, 30, 40);

    manager.addShape(p, 1);
    QCOMPARE(manager.shapes().front().bounds, QRectF(10, 20, 30, 40));
    QCOMPARE(manager.shapes().front().elementCount, manager.shapes().front().path.elementCount());

    manager.setSelectedShapes({0});
    manager.translateSelectedShapes(QPointF(5, -10));
    QCOMPARE(manager.shapes().front().bounds, QRectF(15, 10, 30, 40));

    auto updated = manager.shapes();
    updated.front().path = QPainterPath();
    updated.front().path.addRect(0, 0, 5, 6);
    manager.setShapes(updated);
    QCOMPARE(manager.shapes().front().bounds, QRectF(0, 0, 5, 6));
}

void ShapeManagerTests::visibleShapeIndicesUsesCachedBounds()
{
    ShapeManager manager;
    QPainterPath left;
    left.addRect(0, 0, 10, 10);
    QPainterPath right;
    right.addRect(100, 100, 10, 10);

    manager.addShape(left, 1);
    manager.addShape(right, 2);

    const std::vector<int> visible = manager.visibleShapeIndices(QRectF(-5, -5, 30, 30));
    QCOMPARE(visible.size(), static_cast<size_t>(1));
    QCOMPARE(visible.front(), 0);
}

void ShapeManagerTests::visibleShapeIndicesIncludesAxisAlignedLines()
{
    ShapeManager manager;

    QPainterPath horizontal;
    horizontal.moveTo(0, 20);
    horizontal.lineTo(60, 20);

    QPainterPath vertical;
    vertical.moveTo(80, 0);
    vertical.lineTo(80, 60);

    manager.addShape(horizontal, 1);
    manager.addShape(vertical, 2);

    const std::vector<int> horizontalVisible = manager.visibleShapeIndices(QRectF(-5, 10, 80, 20));
    QCOMPARE(horizontalVisible.size(), static_cast<size_t>(1));
    QCOMPARE(horizontalVisible.front(), 0);

    const std::vector<int> verticalVisible = manager.visibleShapeIndices(QRectF(70, -5, 20, 80));
    QCOMPARE(verticalVisible.size(), static_cast<size_t>(1));
    QCOMPARE(verticalVisible.front(), 1);
}

void ShapeManagerTests::appendShapesEmitsSingleChange()
{
    ShapeManager manager;
    QSignalSpy spy(&manager, &ShapeManager::shapesChanged);

    QPainterPath a;
    a.addRect(0, 0, 10, 10);
    QPainterPath b;
    b.addRect(20, 20, 10, 10);

    std::vector<ShapeManager::Shape> shapes;
    shapes.push_back({a, 1, 0.0});
    shapes.push_back({b, 2, 0.0});
    manager.appendShapes(shapes);

    QCOMPARE(manager.shapes().size(), static_cast<size_t>(2));
    QCOMPARE(spy.count(), 1);
}
