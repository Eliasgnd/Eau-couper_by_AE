#include "placement_assist_tests.h"

#include "PlacementAssist.h"

#include <QtTest>

namespace {
PlacementAssist::Options options(qreal scale = 1.0)
{
    PlacementAssist::Options result;
    result.guideTolerance = 18.0 / scale;
    result.magnetTolerance = 12.0 / scale;
    result.includeViewportCenter = false;
    return result;
}
} // namespace

void PlacementAssistTests::snapsCenterToCenter()
{
    const QRectF moving(188.0, 100.0, 20.0, 20.0);
    const QVector<QRectF> targets{QRectF(190.0, 100.0, 20.0, 20.0)};

    const auto result = PlacementAssist::resolve(moving, targets, QRectF(), options());

    QVERIFY(result.hasSnap());
    QCOMPARE(result.correction.x(), 2.0);
    QCOMPARE(result.correction.y(), 0.0);
    QVERIFY(!result.guides.isEmpty());
}

void PlacementAssistTests::snapsLeftEdgeToRightEdge()
{
    const QRectF moving(109.0, 40.0, 20.0, 20.0);
    const QVector<QRectF> targets{QRectF(80.0, 40.0, 20.0, 20.0)};

    const auto result = PlacementAssist::resolve(moving, targets, QRectF(), options());

    QVERIFY(result.hasSnap());
    QCOMPARE(result.correction.x(), -9.0);
    QCOMPARE(result.correctedRect(moving).left(), 100.0);
}

void PlacementAssistTests::doesNotSnapOutsideTolerance()
{
    const QRectF moving(130.0, 40.0, 20.0, 20.0);
    const QVector<QRectF> targets{QRectF(80.0, 100.0, 20.0, 20.0)};

    const auto result = PlacementAssist::resolve(moving, targets, QRectF(), options());

    QVERIFY(!result.hasSnap());
    QVERIFY(result.guides.isEmpty());
}

void PlacementAssistTests::ignoresExcludedSelectionTargets()
{
    const QRectF moving(109.0, 40.0, 20.0, 20.0);
    const QVector<QRectF> targets;

    const auto result = PlacementAssist::resolve(moving, targets, QRectF(), options());

    QVERIFY(!result.hasSnap());
    QVERIFY(result.guides.isEmpty());
}

void PlacementAssistTests::toleranceScalesFromScreenPixels()
{
    const QRectF moving(105.5, 40.0, 20.0, 20.0);
    const QVector<QRectF> targets{QRectF(80.0, 40.0, 20.0, 20.0)};

    const auto zoomedIn = PlacementAssist::resolve(moving, targets, QRectF(), options(2.0));
    const auto zoomedOut = PlacementAssist::resolve(moving, targets, QRectF(), options(0.5));

    QVERIFY(zoomedIn.hasSnap());
    QVERIFY(zoomedOut.hasSnap());
    QCOMPARE(zoomedIn.correction.x(), -5.5);
    QCOMPARE(zoomedOut.correction.x(), -5.5);
}
