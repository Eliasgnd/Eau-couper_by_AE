#include <QtTest>
#include <QElapsedTimer>
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
    void stressWorstCase();
    void lowEndModeAdjusts();
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

void PlacementTests::stressWorstCase()
{
    QPainterPath a; a.moveTo(0,0);
    for(int i=0;i<20000;++i) a.lineTo(i+1, (i%2));
    QPainterPath b; b.addRect(20010,0,10,10); // far apart
    QElapsedTimer t; t.start();
    QVERIFY(!pathsOverlap(a,b));
    QVERIFY(t.elapsed() < 50); // should finish quickly
}

void PlacementTests::lowEndModeAdjusts()
{
    setLowEndMode(true);
    QPainterPath a; a.addRect(0,0,10,10);
    QPainterPath b; b.addRect(5,5,10,10);
    QVERIFY(pathsOverlap(a,b));
    PipelineMetrics m = lastPipelineMetrics();
    QVERIFY(m.rasterResolution <= 512);
    setLowEndMode(false);
}

QTEST_MAIN(PlacementTests)
#include "placement_tests.moc"
