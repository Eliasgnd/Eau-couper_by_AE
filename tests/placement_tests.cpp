#include <QtTest>
#include "../GeometryUtils.h"
#include "../motorcontrol.h"

class PlacementTests : public QObject {
    Q_OBJECT
private slots:
    void borderContactNotOverlap();
    void interiorIntersection();
    void motorControlSteps();
    void normalizationRemovesDuplicates();
    void complexityGuardRejects();
};

void PlacementTests::borderContactNotOverlap()
{
    QPainterPath a; a.addRect(0,0,10,10);
    QPainterPath b; b.addRect(10,0,10,10);
    QVERIFY(!pathsOverlap(a,b));
}

void PlacementTests::interiorIntersection()
{
    QPainterPath a; a.addRect(0,0,10,10);
    QPainterPath b; b.addRect(5,5,10,10);
    QVERIFY(pathsOverlap(a,b));
}

void PlacementTests::motorControlSteps()
{
    MotorControl mc;
    mc.moveRapid(1.0,2.0);
    QCOMPARE(mc.getStepsX(), 10);
    QCOMPARE(mc.getStepsY(), 20);
}

void PlacementTests::normalizationRemovesDuplicates()
{
    QPainterPath p;
    p.addRect(0,0,1,1);
    p.lineTo(1,1); // duplicate point
    QPainterPath n = normalizePath(p);
    QVERIFY(n.elementCount() < p.elementCount());
}

void PlacementTests::complexityGuardRejects()
{
    QPainterPath p;
    p.moveTo(0,0);
    for (int i = 0; i < kMaxPathElements + 1; ++i)
        p.lineTo(i+1,0);
    QVERIFY(isPathTooComplex(p, kMaxPathElements));
}

QTEST_MAIN(PlacementTests)
#include "placement_tests.moc"
