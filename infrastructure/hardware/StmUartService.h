#pragma once

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>
#include "StmProtocol.h"

// ============================================================
//  StmUartService — Couche UART pure RPi ↔ STM32
//
//  Responsabilités :
//   - Ouvrir/fermer le port série (115200 bps, 8N1)
//   - Encoder et envoyer les trames binaires 11 octets
//   - Envoyer les commandes ASCII
//   - Parser les réponses STM ligne par ligne
//   - Gérer le retry NAK (max 3) et le timeout ACK (2 s)
//
//  N'implémente AUCUNE logique métier — elle émet des signaux.
//  C'est MachineViewModel qui réagit à ces signaux.
// ============================================================
class StmUartService : public QObject
{
    Q_OBJECT

public:
    explicit StmUartService(QObject* parent = nullptr);
    ~StmUartService() override;

    // --- Connexion ---
    void open(const QString& portName = QString());
    void close();
    bool isOpen() const;
    static QString findPreferredPortName();

    // --- Envoi vers STM ---
    void sendSegment(const StmSegment& seg);
    void sendAsciiCommand(const QString& cmd);   // envoie cmd + "\r\n"
    void requestPrestart(quint32 sessionId, int segmentCount);
    void sendControlledStop();
    void sendEmergencyStop();

    // Retourne true si le buffer STM estimé est plein — TrajetMotor doit patienter.
    bool isFull() const;

    // Réinitialise les compteurs de fenêtre (appelé en début de coupe / arrêt d'urgence).
    void resetWindow();
    bool hasHealthyLink() const { return m_linkHealthy; }
    StmHealth lastHealth() const { return m_health; }
    void processLineForTest(const QByteArray& line) { processLine(line.trimmed()); }

signals:
    // Connexion
    void connectionChanged(bool connected);
    void comError(const QString& reason);
    void healthChanged(StmHealth health);
    void prestartAccepted(quint32 sessionId);
    void prestartRejected(const QString& reason);
    void safetyFault(const QString& reason);
    void linkLost();

    // Progression trame binaire
    void ackReceived(int bufLevel, int segIndex);
    void nakReceived();
    void doneReceived();
    void controlledStopReceived();

    // Homing
    void homingMessage(const QString& msg);   // HOMING..., Z OK, X OK, Y OK, HOMED
    void homingError(const QString& axis);    // ERR:HOMING_Z / _X / _Y / _TIMEOUT

    // Sécurité
    void emergencyTriggered();
    void alarmTriggered(const QString& axis); // ALM_X, ALM_Y, ALM_Z, LIM_HIT

    // Recovery
    void recoveryAvailable(RecoveryData data);
    void recoveryDataReceived(RecoveryData data);
    void recoveryOk();
    void recoveryCleared();

    // États généraux
    void readyReceived();
    void startupBannerReceived();             // re-démarrage watchdog STM détecté

    // Erreurs STM
    void errorReceived(const QString& code);  // ERR:ESTOP, ERR:NOT_READY, etc.

    // Confirmations diverses
    void valveOnConfirmed();
    void valveOffConfirmed();
    void posResetConfirmed();
    void homeAckConfirmed();

    // Relais brut — chaque ligne ASCII reçue du STM (pour logs test)
    void rawLineReceived(const QString& line);

    // Progression d'exécution réelle (SEG_DONE)
    void segDoneReceived(int seg, int x_steps, int y_steps);

    // Pause / Reprise confirmées par le STM
    void pausedConfirmed();
    void resumedConfirmed();

    void realPositionReceived(int x_steps, int y_steps);
private slots:
    void onReadyRead();
    void onAckTimeout();
    void onSerialError(QSerialPort::SerialPortError error);
    void sendHeartbeat();
    void onLinkTimeout();

private:
    QSerialPort  m_serial;
    QTimer       m_ackTimer;
    QTimer       m_pingTimer;
    QTimer       m_linkTimer;
    QByteArray   m_readBuffer;

    uint16_t m_globalSeqId = 0;

    // --- Fenêtre glissante : contrôle de flux vers le STM ---
    // m_stmBufLevel : dernier niveau buffer STM connu (champ buf=X de l'ACK)
    // m_sentSinceAck : segments envoyés depuis le dernier ACK reçu
    // isFull() bloque si (m_stmBufLevel + m_sentSinceAck) >= STM_SEND_AHEAD_MAX
    int  m_stmBufLevel  = 0;
    int  m_sentSinceAck = 0;

    QList<QByteArray> m_unackedBatch;  // trame courante pour retransmission NAK
    QList<uint16_t>   m_unackedSeqIds;
    int  m_nakCount = 0;               // NAK consécutifs sur la trame courante
    bool m_linkHealthy = false;
    bool m_heartbeatSuspended = false;
    bool m_streamingActive = false;
    qint64 m_lastTxMs = 0;
    qint64 m_lastRxMs = 0;
    QString m_portName;
    QString m_lastRxLine;
    StmHealth m_health;

    // --- Encodage trame binaire ---
    QByteArray encodeFrame(const StmSegment& seg, uint16_t seqId);
    static uint16_t   calcCrc16(const QByteArray& frame);

    // --- Parsing ASCII ---
    void processLine(const QByteArray& line);
    static RecoveryData parseRecoveryPayload(const QByteArray& line);
    static MachineState parseMachineState(const QByteArray& value);
    static StmHealth parseHealthPayload(const QByteArray& line);

    void retransmitLastFrame();
    void markHostActivity();
    void markHostTransmit(const QString& label);
    void markUnsafeLink(const QString& reason);
    static qint64 nowMs();
};
