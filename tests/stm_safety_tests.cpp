#include "stm_safety_tests.h"

#include "StmUartService.h"

#include <QSignalSpy>
#include <QtTest>

void StmSafetyTests::parsesHeartbeatHealth()
{
    qRegisterMetaType<StmHealth>("StmHealth");
    StmUartService service;
    QSignalSpy spy(&service, &StmUartService::healthChanged);

    service.processLineForTest("PONG|state=READY|armed=0|homed=1|fault=NONE");

    QCOMPARE(spy.count(), 1);
    const StmHealth health = spy.takeFirst().at(0).value<StmHealth>();
    QVERIFY(health.valid);
    QCOMPARE(health.state, MachineState::READY);
    QVERIFY(health.homed);
    QVERIFY(!health.armed);
    QVERIFY(health.fault.isEmpty());
    QVERIFY(service.hasHealthyLink());
}

void StmSafetyTests::parsesPrestartAccepted()
{
    StmUartService service;
    QSignalSpy spy(&service, &StmUartService::prestartAccepted);

    service.processLineForTest("PRESTART_OK|session=42");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toUInt(), 42u);
}

void StmSafetyTests::parsesSafetyFault()
{
    StmUartService service;
    QSignalSpy spy(&service, &StmUartService::safetyFault);

    service.processLineForTest("FAULT|reason=UART_TIMEOUT");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("UART_TIMEOUT"));
    QVERIFY(!service.hasHealthyLink());
}

void StmSafetyTests::parsesAckWithSequence()
{
    StmUartService service;
    QSignalSpy spy(&service, &StmUartService::ackReceived);

    service.processLineForTest("ACK|seq=7|buf=2|rx=3");

    QCOMPARE(spy.count(), 1);
    const QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 2);
    QCOMPARE(args.at(1).toInt(), 3);
}

void StmSafetyTests::parsesPausedAndResumed()
{
    StmUartService service;
    QSignalSpy pausedSpy(&service, &StmUartService::pausedConfirmed);
    QSignalSpy resumedSpy(&service, &StmUartService::resumedConfirmed);

    service.processLineForTest("PAUSE_HOLD");
    service.processLineForTest("RESUMED");

    QCOMPARE(pausedSpy.count(), 1);
    QCOMPARE(resumedSpy.count(), 1);
}

void StmSafetyTests::parsesControlledStop()
{
    StmUartService service;
    QSignalSpy spy(&service, &StmUartService::controlledStopReceived);

    service.processLineForTest("STOPPED|reason=CONTROLLED");

    QCOMPARE(spy.count(), 1);
}

void StmSafetyTests::parsesExtendedHealthDiagnostics()
{
    qRegisterMetaType<StmHealth>("StmHealth");
    StmUartService service;
    QSignalSpy spy(&service, &StmUartService::healthChanged);

    service.processLineForTest("PONG|state=CUTTING|armed=1|homed=1|valve=0|buf=12|seg_rx=22|seg_done=10|fault=NONE");

    QCOMPARE(spy.count(), 1);
    const StmHealth health = spy.takeFirst().at(0).value<StmHealth>();
    QCOMPARE(health.state, MachineState::CUTTING);
    QVERIFY(health.armed);
    QVERIFY(health.homed);
    QVERIFY(!health.valveOpen);
    QCOMPARE(health.bufferLevel, 12);
    QCOMPARE(health.segmentsReceived, 22);
    QCOMPARE(health.segmentsDone, 10);
}
