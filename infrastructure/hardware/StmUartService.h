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
    void open(const QString& portName);
    void close();
    bool isOpen() const;

    // --- Envoi vers STM ---
    void sendSegment(const StmSegment& seg);
    void sendAsciiCommand(const QString& cmd);   // envoie cmd + "\r\n"

    // Retourne true si le buffer STM estimé est plein — TrajetMotor doit patienter.
    bool isFull() const;

    // Réinitialise les compteurs de fenêtre (appelé en début de coupe / arrêt d'urgence).
    void resetWindow();

signals:
    // Connexion
    void connectionChanged(bool connected);
    void comError(const QString& reason);

    // Progression trame binaire
    void ackReceived(int bufLevel, int segIndex);
    void nakReceived();
    void doneReceived();

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

private:
    QSerialPort  m_serial;
    QTimer       m_ackTimer;
    QByteArray   m_readBuffer;

    uint16_t m_globalSeqId = 0;

    // --- Fenêtre glissante : contrôle de flux vers le STM ---
    // m_stmBufLevel : dernier niveau buffer STM connu (champ buf=X de l'ACK)
    // m_sentSinceAck : segments envoyés depuis le dernier ACK reçu
    // isFull() bloque si (m_stmBufLevel + m_sentSinceAck) >= STM_SEND_AHEAD_MAX
    int  m_stmBufLevel  = 0;
    int  m_sentSinceAck = 0;

    QList<QByteArray> m_unackedBatch;  // trame courante pour retransmission NAK
    int  m_nakCount = 0;               // NAK consécutifs sur la trame courante

    // --- Encodage trame binaire ---
    QByteArray encodeFrame(const StmSegment& seg, uint16_t seqId);
    static uint8_t    calcChecksum(const QByteArray& frame);

    // --- Parsing ASCII ---
    void processLine(const QByteArray& line);
    static RecoveryData parseRecoveryPayload(const QByteArray& line);

    void retransmitLastFrame();
};
