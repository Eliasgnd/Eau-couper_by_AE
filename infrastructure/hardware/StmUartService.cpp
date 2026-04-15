#include "StmUartService.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <utility>

// ============================================================
//  StmUartService — implémentation
// ============================================================

StmUartService::StmUartService(QObject* parent)
    : QObject(parent)
{
    m_ackTimer.setSingleShot(true);
    m_ackTimer.setInterval(ACK_TIMEOUT_MS);

    connect(&m_serial, &QSerialPort::readyRead,
            this, &StmUartService::onReadyRead);
    connect(&m_serial, &QSerialPort::errorOccurred,
            this, &StmUartService::onSerialError);
    connect(&m_ackTimer, &QTimer::timeout,
            this, &StmUartService::onAckTimeout);
}

StmUartService::~StmUartService()
{
    close();
}

// ----------------------------------------------------------
//  Connexion
// ----------------------------------------------------------

void StmUartService::open(const QString& portName)
{
    if (m_serial.isOpen())
        close();

    m_serial.setPortName(portName);
    m_serial.setBaudRate(UART_BAUDRATE);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial.open(QIODevice::ReadWrite)) {
        m_readBuffer.clear();
        m_nakCount          = 0;
        m_waitingAck        = false;
        m_segsSinceLastAck  = 0;
        m_unackedBatch.clear();
        emit connectionChanged(true);
        qDebug() << "[STM-UART] Port ouvert :" << portName;
    } else {
        emit comError(tr("Impossible d'ouvrir le port %1 : %2")
                      .arg(portName, m_serial.errorString()));
    }
}

void StmUartService::close()
{
    if (m_serial.isOpen()) {
        m_ackTimer.stop();
        m_serial.close();
        emit connectionChanged(false);
        qDebug() << "[STM-UART] Port fermé.";
    }
}

bool StmUartService::isOpen() const
{
    return m_serial.isOpen();
}

// ----------------------------------------------------------
//  Envoi — trame binaire
// ----------------------------------------------------------

QByteArray StmUartService::encodeFrame(const StmSegment& seg)
{
    QByteArray frame(11, '\0');

    frame[0] = static_cast<char>(STM_SYNC_BYTE);

    // dx — int16 BIG-endian (Poids FORT en premier pour correspondre au STM32)
    frame[1] = static_cast<char>((seg.dx >> 8)  & 0xFF);
    frame[2] = static_cast<char>( seg.dx        & 0xFF);

    // dy — int16 BIG-endian
    frame[3] = static_cast<char>((seg.dy >> 8)  & 0xFF);
    frame[4] = static_cast<char>( seg.dy        & 0xFF);

    // dz — int16 BIG-endian
    frame[5] = static_cast<char>((seg.dz >> 8)  & 0xFF);
    frame[6] = static_cast<char>( seg.dz        & 0xFF);

    // v_max — uint16 BIG-endian
    frame[7] = static_cast<char>((seg.v_max >> 8) & 0xFF);
    frame[8] = static_cast<char>( seg.v_max       & 0xFF);

    // flags
    frame[9] = static_cast<char>(seg.flags);

    // Checksum XOR de tous les octets précédents
    frame[10] = static_cast<char>(calcChecksum(frame));

    return frame;
}

uint8_t StmUartService::calcChecksum(const QByteArray& frame)
{
    uint8_t checksum = 0;
    // XOR des octets [0..9] (le checksum se trouve en [10])
    for (int i = 0; i < 10 && i < frame.size(); ++i)
        checksum ^= static_cast<uint8_t>(frame[i]);
    return checksum;
}

void StmUartService::sendSegment(const StmSegment& seg)
{
    if (!m_serial.isOpen()) return;

    QByteArray frame = encodeFrame(seg);
    m_unackedBatch.append(frame); // On stocke la trame dans le batch en cours
    ++m_segsSinceLastAck;

    m_serial.write(frame);

    const bool isBatchBoundary = (m_segsSinceLastAck >= STM_ACK_BATCH);
    const bool isEndSeq = (seg.flags & FLAG_END_SEQ) ||
                          (seg.flags & FLAG_HOME_SEQ) ||
                          (seg.flags & FLAG_VALVE_ON) ||
                          (seg.flags & FLAG_VALVE_OFF);

    if (isBatchBoundary || isEndSeq) {
        m_nakCount   = 0;
        m_waitingAck = true;
        m_ackTimer.start();
    }
}

// ----------------------------------------------------------
//  Envoi — commande ASCII
// ----------------------------------------------------------

void StmUartService::sendAsciiCommand(const QString& cmd)
{
    if (!m_serial.isOpen()) {
        emit comError(tr("Port série non ouvert."));
        return;
    }
    QByteArray data = cmd.toUtf8() + "\r\n";
    m_serial.write(data);
    qDebug() << "[STM-UART] TX ASCII :" << cmd;
}

// ----------------------------------------------------------
//  Réception
// ----------------------------------------------------------

void StmUartService::onReadyRead()
{
    m_readBuffer += m_serial.readAll();

    // Traitement en boucle tant qu'il y a des données complètes
    while (!m_readBuffer.isEmpty()) {
        // Détection trame binaire : premier octet == 0xAA
        if (static_cast<uint8_t>(m_readBuffer[0]) == STM_SYNC_BYTE) {
            // Attendre les 11 octets complets
            if (m_readBuffer.size() < 11)
                break;
            // Vérifier le CRC
            QByteArray frame = m_readBuffer.left(11);
            m_readBuffer.remove(0, 11);
            // (Les trames binaires vont du RPi → STM, pas l'inverse.
            //  Si on reçoit 0xAA du STM, c'est inattendu — on ignore.)
            qWarning() << "[STM-UART] Trame binaire inattendue en réception, ignorée.";
            continue;
        }

        // Recherche de \r\n pour une ligne ASCII
        int idx = m_readBuffer.indexOf('\n');
        if (idx == -1) {
            // Ligne incomplète — on attend la suite
            break;
        }

        // Extraire la ligne (sans \r\n)
        QByteArray line = m_readBuffer.left(idx);
        m_readBuffer.remove(0, idx + 1);
        line.replace('\r', "");
        line = line.trimmed();

        if (!line.isEmpty())
            processLine(line);
    }
}

// ----------------------------------------------------------
//  Parser les lignes ASCII du STM
// ----------------------------------------------------------

void StmUartService::processLine(const QByteArray& line)
{
    emit rawLineReceived(QString::fromUtf8(line));
    qDebug() << "[STM-UART] RX :" << line;

    // --- ACK|buf=<n>|seg=<s> ---
    if (line.startsWith("ACK|buf=")) {
        m_ackTimer.stop();
        m_waitingAck        = false;
        m_nakCount          = 0;
        m_segsSinceLastAck  = 0;
        m_unackedBatch.clear();

        // Parse : ACK|buf=<n>|seg=<s>
        int bufLevel = 0, segIndex = 0;
        QByteArray payload = line.mid(8); // après "ACK|buf="
        int sep = payload.indexOf("|seg=");
        if (sep != -1) {
            bufLevel = payload.left(sep).toInt();
            segIndex = payload.mid(sep + 5).toInt();
        }
        emit ackReceived(bufLevel, segIndex);
        return;
    }

    // --- NAK ---
    if (line == "NAK" || line.endsWith("NAK")) {
        m_ackTimer.stop();
        m_waitingAck = false;
        ++m_nakCount;
        emit nakReceived();

        if (m_nakCount >= MAX_NAK_RETRY) {
            m_nakCount = 0;
            m_unackedBatch.clear();
            emit comError(tr("3 NAK consécutifs — anomalie de communication."));
        } else {
            retransmitLastFrame();
        }
        return;
    }

    // --- DONE ---
    if (line == "DONE" || line.endsWith("DONE")) {
        emit doneReceived();
        return;
    }

    // --- READY ---
    if (line == "READY" || line.endsWith("READY")) {
        emit readyReceived();
        return;
    }

    // --- Homing messages ---
    if (line.startsWith("HOMING") || line == "Z OK" || line == "X OK" || line == "Y OK"
        || line == "HOMED") {
        emit homingMessage(QString::fromUtf8(line));
        return;
    }

    // --- Homing errors ---
    if (line == "ERR:HOMING_TIMEOUT") {
        emit homingError(QStringLiteral("TIMEOUT"));
        return;
    }
    if (line.startsWith("ERR:HOMING_")) {
        QString axis = QString::fromUtf8(line.mid(11)); // après "ERR:HOMING_"
        emit homingError(axis);
        return;
    }

    // --- EMERGENCY ---
    if (line == "EMERGENCY") {
        emit emergencyTriggered();
        return;
    }

    // --- Alarmes ---
    if (line == "ALM_X" || line == "ALM_Y" || line == "ALM_Z") {
        QString axis = QString::fromUtf8(line.mid(4)); // X, Y ou Z
        emit alarmTriggered(axis);
        return;
    }
    if (line == "LIM_HIT") {
        emit alarmTriggered(QStringLiteral("LIM_HIT"));
        return;
    }

    // --- Recovery ---
    if (line.startsWith("RECOVERY_AVAILABLE|")) {
        emit recoveryAvailable(parseRecoveryPayload(line.mid(19)));
        return;
    }
    if (line.startsWith("RECOVERY|")) {
        emit recoveryDataReceived(parseRecoveryPayload(line.mid(9)));
        return;
    }
    if (line == "RECOVERY_OK") {
        emit recoveryOk();
        return;
    }
    if (line == "RECOVERY_CLEARED") {
        emit recoveryCleared();
        return;
    }

    // --- Confirmations diverses ---
    if (line == "VALVE ON") { emit valveOnConfirmed();  return; }
    if (line == "VALVE OFF") { emit valveOffConfirmed(); return; }
    if (line == "POS_RESET") { emit posResetConfirmed(); return; }
    if (line == "ACK:HOME")  { emit homeAckConfirmed();  return; }

    // --- Erreurs STM ---
    if (line.startsWith("ERR:")) {
        emit errorReceived(QString::fromUtf8(line.mid(4)));
        return;
    }

    // --- AU au démarrage ---
    if (line.startsWith("EMERGENCY AT STARTUP")) {
        emit emergencyTriggered();
        emit startupBannerReceived();
        return;
    }

    // --- Banner de démarrage STM (watchdog reset ou boot initial) ---
    if (line.startsWith("=== CNC")) {
        emit startupBannerReceived();
        return;
    }

    // Messages INFO (recovery disponible au démarrage, etc.)
    if (line.startsWith("INFO:")) {
        qDebug() << "[STM-UART] INFO :" << line;
        // Peut contenir "Recovery disponible seg=<s>..."
        // On émet le banner car c'est un contexte de démarrage
        return;
    }

    // --- POSITION RÉELLE EN DIRECT (Envoyée par le STM32) ---
    if (line.startsWith("POS|")) {
        QList<QByteArray> parts = line.split('|');
        if (parts.size() >= 3) {
            int x_steps = parts[1].toInt();
            int y_steps = parts[2].toInt();
            emit realPositionReceived(x_steps, y_steps);
        }
        return;
    }

    // --- SEG_DONE|seg=N|x=X|y=Y ---
    if (line.startsWith("SEG_DONE|")) {
        int seg = 0, x = 0, y = 0;

        // CORRECTION : On stocke le split dans une variable constante
        const QList<QByteArray> parts = line.mid(9).split('|');
        for (const QByteArray& part : parts) {
            if      (part.startsWith("seg=")) seg = part.mid(4).toInt();
            else if (part.startsWith("x="))   x   = part.mid(2).toInt();
            else if (part.startsWith("y="))   y   = part.mid(2).toInt();
        }
        emit segDoneReceived(seg, x, y);
        return;
    }

    // --- PAUSED / PAUSE_HOLD — machine en attente après segment courant ---
    if (line == "PAUSED" || line == "PAUSE_HOLD") {
        emit pausedConfirmed();
        return;
    }

    // --- RESUMED — machine reprend depuis son buffer ---
    if (line == "RESUMED") {
        emit resumedConfirmed();
        return;
    }

    // Message non reconnu — on logge sans émettre de signal d'erreur
    qDebug() << "[STM-UART] Message non parsé :" << line;
}

RecoveryData StmUartService::parseRecoveryPayload(const QByteArray& payload)
{
    // Format : seg=<s>|x=<x>|y=<y>|z=<z>
    RecoveryData data;

    // CORRECTION : On stocke le split dans une variable constante
    const QList<QByteArray> parts = payload.split('|');
    for (const QByteArray& part : parts) {
        if (part.startsWith("seg=")) data.seg = part.mid(4).toInt();
        else if (part.startsWith("x=")) data.x = part.mid(2).toInt();
        else if (part.startsWith("y=")) data.y = part.mid(2).toInt();
        else if (part.startsWith("z=")) data.z = part.mid(2).toInt();
    }
    return data;
}

// ----------------------------------------------------------
//  Retry et timeout
// ----------------------------------------------------------

void StmUartService::retransmitLastFrame()
{
    if (m_unackedBatch.isEmpty()) return;
    qDebug() << "[STM-UART] Retransmission du batch complet suite à NAK (" << m_nakCount << "/" << MAX_NAK_RETRY << ")";
    m_waitingAck = true;

    // Remplacement de la boucle range-based par un for classique
    const int batchSize = m_unackedBatch.size();
    for (int i = 0; i < batchSize; ++i) {
        m_serial.write(m_unackedBatch[i]);
    }
    m_ackTimer.start();
}

void StmUartService::onAckTimeout()
{
    if (!m_waitingAck) return;
    m_waitingAck = false;
    m_nakCount   = 0;
    m_unackedBatch.clear();
    emit comError(tr("Timeout ACK — pas de réponse STM en %1 ms.").arg(ACK_TIMEOUT_MS));
}

void StmUartService::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    const QString msg = tr("Erreur port série : %1").arg(m_serial.errorString());
    qWarning() << "[STM-UART]" << msg;
    emit comError(msg);

    if (error != QSerialPort::TimeoutError) {
        close();
    }
}
