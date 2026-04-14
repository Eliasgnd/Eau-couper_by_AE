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

private slots:
    void onReadyRead();
    void onAckTimeout();
    void onSerialError(QSerialPort::SerialPortError error);

private:
    QSerialPort  m_serial;
    QTimer       m_ackTimer;
    QByteArray   m_readBuffer;

    QByteArray   m_lastFrame;    // dernière trame binaire envoyée (pour retry)
    int          m_nakCount = 0; // NAK consécutifs sur la trame courante

    bool         m_waitingAck = false;

    // --- Encodage trame binaire ---
    static QByteArray encodeFrame(const StmSegment& seg);
    static uint8_t    calcChecksum(const QByteArray& frame);

    // --- Parsing ASCII ---
    void processLine(const QByteArray& line);
    static RecoveryData parseRecoveryPayload(const QByteArray& line);

    void retransmitLastFrame();
};
