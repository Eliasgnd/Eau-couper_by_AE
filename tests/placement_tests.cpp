#include <QtTest>
#include "../GeometryUtils.h"
#include "../motorcontrol.h"

class PlacementTests : public QObject {
    Q_OBJECT
private slots:
    void borderContactNotOverlap();
    void interiorIntersection();
    void motorControlSteps();
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

QTEST_MAIN(PlacementTests)
#include "placement_tests.moc"
