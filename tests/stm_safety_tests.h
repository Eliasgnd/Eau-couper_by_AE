#pragma once

#include <QObject>

class StmSafetyTests : public QObject
{
    Q_OBJECT

private slots:
    void parsesHeartbeatHealth();
    void parsesPrestartAccepted();
    void parsesSafetyFault();
    void parsesAckWithSequence();
    void parsesPausedAndResumed();
    void parsesControlledStop();
    void parsesExtendedHealthDiagnostics();
};
