#pragma once

#include <QObject>
#include <QString>
#include "StmProtocol.h"

class StmUartService;

// ============================================================
//  MachineViewModel — ViewModel MVVM pour le contrôle machine
//
//  Responsabilités :
//   - Maintenir l'état global de la machine (MachineState)
//   - Exposer la position courante en mm
//   - Fournir les commandes que la Vue peut déclencher
//   - Gérer le flow control (buffer STM)
//   - Protéger la sécurité Z (pas de descente sans confirmation)
//
//  La Vue ne parle JAMAIS à StmUartService directement.
// ============================================================
class MachineViewModel : public QObject
{
    Q_OBJECT

public:
    explicit MachineViewModel(QObject* parent = nullptr);
    ~MachineViewModel() override;

    // --- Accès à l'état (lecture seule) ---
    MachineState  state()        const { return m_state; }
    bool          isConnected()  const { return m_connected; }
    int           bufferLevel()  const { return m_bufferLevel; }
    double        posX_mm()      const { return stepsToMm(m_posX); }
    double        posY_mm()      const { return stepsToMm(m_posY); }
    double        posZ_mm()      const { return stepsToMm(m_posZ); }
    QString       statusMessage()const { return m_statusMessage; }
    bool          hasRecovery()  const { return m_hasRecovery; }
    RecoveryData  recoveryInfo() const { return m_recovery; }
    bool          isSafetyReady() const;
    bool          hasRecentHoming() const { return m_homed; }

    // --- Accès au service UART (pour injection dans TrajetMotor) ---
    StmUartService* uartService() const { return m_uart; }

public slots:
    // Connexion au port série
    void connectToStm(const QString& portName);
    void connectAndInitialize(const QString& portName = QString());
    void disconnectFromStm();

    // Commandes machine → STM (les préconditions d'état sont vérifiées ici)
    void sendHome();            // HOME — homing complet
    void sendPositionReset();   // H   — reset logique position
    void sendRearm();           // R   — réarmement après EMERGENCY/ALARM
    void sendValveOn();         // V+  — ouvre vanne manuellement
    void sendValveOff();        // V-  — ferme vanne manuellement
    void sendRecover();         // RECOVER — demande données recovery (si READY)
    void sendGo();              // GO — confirme reprise (si RECOVERY_WAIT)
    void sendClear();           // CLEAR — annule recovery

    // Envoi d'un segment de trajectoire (appelé par TrajetMotor)
    // Retourne false si le buffer est plein (flow control) ou état incompatible
    bool sendSegment(const StmSegment& seg);
    bool requestPrestart(int segmentCount);

    // Confirmation explicite de descente Z (sécurité opérateur)
    void confirmZDescent();

    // Mouvement ASCII debug : X<dx> Y<dy> Z<dz> F<arr>
    void sendAsciiMove(int dx_steps, int dy_steps, int dz_steps, int arr);

    // Commandes de contrôle découpe
    void sendPause();   // PAUSE  — stoppe après le segment courant
    void sendResume();  // RESUME — reprend depuis le buffer STM
    void sendStop();    // STOP_CONTROLLED - arret operateur controle
    void sendEmergencyStop(); // AU - arret d'urgence logiciel

signals:
    void stateChanged(MachineState state);
    void connectionChanged(bool connected);
    void bufferLevelChanged(int level);
    void positionChanged(double x_mm, double y_mm, double z_mm);
    void statusMessageChanged(const QString& message);
    void recoveryAvailable(RecoveryData data);
    void doneReceived();
    void homingProgress(const QString& message);
    void errorOccurred(const QString& code);
    void safetyReadyChanged(bool ready);
    void prestartAccepted();
    void prestartRejected(const QString& reason);
    void safetyFault(const QString& reason);
    void linkLost();

    // Exécution réelle — un segment physiquement terminé par le STM
    void segmentDone(int seg, int x_steps, int y_steps);

    // Pause / reprise confirmées par le STM
    void machinePaused();
    void machineResumed();

    // Relais brut UART — chaque ligne reçue du STM (pour logs test)
    void rxLine(const QString& line);

    // Confirmations reçues du STM
    void valveOnConfirmed();
    void valveOffConfirmed();
    void posResetConfirmed();
    void homeAckConfirmed();

    void realPositionReceived(int x_steps, int y_steps);
private slots:
    // Branchés sur StmUartService
    void onConnectionChanged(bool connected);
    void onAckReceived(int bufLevel, int segIndex);
    void onNakReceived();
    void onDoneReceived();
    void onHomingMessage(const QString& msg);
    void onHomingError(const QString& axis);
    void onEmergencyTriggered();
    void onAlarmTriggered(const QString& axis);
    void onRecoveryAvailable(RecoveryData data);
    void onRecoveryDataReceived(RecoveryData data);
    void onRecoveryOk();
    void onRecoveryCleared();
    void onReadyReceived();
    void onStartupBannerReceived();
    void onErrorReceived(const QString& code);
    void onComError(const QString& reason);
    void onRawLineReceived(const QString& line);
    void onHealthChanged(StmHealth health);
    void onPrestartAccepted(quint32 sessionId);
    void onPrestartRejected(const QString& reason);
    void onSafetyFault(const QString& reason);
    void onLinkLost();
    void onValveOnConfirmed();
    void onValveOffConfirmed();
    void onPosResetConfirmed();
    void onHomeAckConfirmed();
    void onControlledStopReceived();

    // Nouveaux slots — réponses STM
    void onSegDoneReceived(int seg, int x, int y);
    void onPausedConfirmed();
    void onResumedConfirmed();

private:
    void setState(MachineState s);
    void setStatus(const QString& msg);
    void updateSafetyReady();
    void maybeStartAutomaticHoming();

    StmUartService* m_uart          = nullptr;

    MachineState    m_state         = MachineState::DISCONNECTED;
    bool            m_connected     = false;
    int             m_bufferLevel   = 0;
    int             m_posX          = 0;   // en pas moteur
    int             m_posY          = 0;
    int             m_posZ          = 0;
    QString         m_statusMessage;

    bool            m_hasRecovery   = false;
    RecoveryData    m_recovery;

    bool            m_zDescentConfirmed  = false;  // sécurité descente Z
    int             m_sentSinceLastAck   = 0;      // segments envoyés depuis le dernier ACK (estimation buffer)
    bool            m_homed              = false;
    bool            m_valveOpen          = false;
    bool            m_linkHealthy        = false;
    bool            m_safetyReady        = false;
    bool            m_prestartAccepted   = false;
    bool            m_autoInitialize     = false;
    bool            m_autoHomingRequested = false;
    quint32         m_pendingSessionId   = 0;
    quint32         m_activeSessionId    = 0;
};
