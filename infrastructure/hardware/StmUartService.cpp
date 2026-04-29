#include "StmUartService.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <QtGlobal>
#include <utility>
#include <algorithm>

// ============================================================
//  StmUartService — implémentation
// ============================================================

StmUartService::StmUartService(QObject* parent)
    : QObject(parent)
{
    m_ackTimer.setSingleShot(true);
    m_ackTimer.setInterval(ACK_TIMEOUT_MS);
    m_pingTimer.setInterval(STM_HEARTBEAT_MS);
    m_linkTimer.setSingleShot(true);
    m_linkTimer.setInterval(STM_LINK_TIMEOUT_MS);

    connect(&m_serial, &QSerialPort::readyRead,
            this, &StmUartService::onReadyRead);
    connect(&m_serial, &QSerialPort::errorOccurred,
            this, &StmUartService::onSerialError);
    connect(&m_ackTimer, &QTimer::timeout,
            this, &StmUartService::onAckTimeout);
    connect(&m_pingTimer, &QTimer::timeout,
            this, &StmUartService::sendHeartbeat);
    connect(&m_linkTimer, &QTimer::timeout,
            this, &StmUartService::onLinkTimeout);
}

StmUartService::~StmUartService()
{
    close();
}

// ----------------------------------------------------------
//  Connexion
// ----------------------------------------------------------

QString StmUartService::findPreferredPortName()
{
    constexpr quint16 kPreferredVendorId = 0x0483;
    constexpr quint16 kPreferredProductId = 0x374E;

    QString fallbackPortName;

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        if (!info.hasVendorIdentifier() || !info.hasProductIdentifier())
            continue;

        if (info.vendorIdentifier() != kPreferredVendorId
            || info.productIdentifier() != kPreferredProductId) {
            continue;
        }

        if (fallbackPortName.isEmpty())
            fallbackPortName = info.portName();

        const QString description = info.description();
        const QString manufacturer = info.manufacturer();

        if (description.contains(QStringLiteral("MI_02"), Qt::CaseInsensitive)
            || manufacturer.contains(QStringLiteral("STMicroelectronics"), Qt::CaseInsensitive)) {
            return info.portName();
        }
    }

    return fallbackPortName;
}

void StmUartService::open(const QString& portName)
{
    if (m_serial.isOpen())
        close();

    const QString resolvedPortName = portName.trimmed().isEmpty()
        ? findPreferredPortName()
        : portName.trimmed();

    if (resolvedPortName.isEmpty()) {
        emit comError(tr("Aucun port STM32 correspondant au VID/PID 0483:374E n'a été trouvé."));
        return;
    }

    m_serial.setPortName(resolvedPortName);
    m_serial.setBaudRate(UART_BAUDRATE);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial.open(QIODevice::ReadWrite)) {
        m_readBuffer.clear();
        m_nakCount      = 0;
        m_stmBufLevel   = 0;
        m_sentSinceAck  = 0;
        m_unackedBatch.clear();
        m_unackedSeqIds.clear();
        m_globalSeqId   = 0;
        m_linkHealthy   = false;
        m_heartbeatSuspended = false;
        m_streamingActive = false;
        m_health        = StmHealth{};
        emit connectionChanged(true);
        sendAsciiCommand(QStringLiteral("HELLO"));
        m_pingTimer.start();
        m_linkTimer.start();
        qDebug() << "[STM-UART] Port ouvert :" << resolvedPortName;
    } else {
        emit comError(tr("Impossible d'ouvrir le port %1 : %2")
                      .arg(resolvedPortName, m_serial.errorString()));
    }
}

void StmUartService::close()
{
    if (m_serial.isOpen()) {
        m_ackTimer.stop();
        m_pingTimer.stop();
        m_linkTimer.stop();
        m_linkHealthy = false;
        m_heartbeatSuspended = false;
        m_streamingActive = false;
        m_health = StmHealth{};
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

QByteArray StmUartService::encodeFrame(const StmSegment& seg, uint16_t seqId)
{
    QByteArray frame(STM_FRAME_LEN, '\0');

    frame[0] = static_cast<char>(STM_SYNC_BYTE);

    // seq_id (16 bits)
    frame[1] = static_cast<char>((seqId >> 8) & 0xFF);
    frame[2] = static_cast<char>( seqId       & 0xFF);

    // dx (32 bits = 4 octets)
    frame[3] = static_cast<char>((seg.dx >> 24) & 0xFF);
    frame[4] = static_cast<char>((seg.dx >> 16) & 0xFF);
    frame[5] = static_cast<char>((seg.dx >> 8)  & 0xFF);
    frame[6] = static_cast<char>( seg.dx        & 0xFF);

    // dy (32 bits = 4 octets)
    frame[7] = static_cast<char>((seg.dy >> 24) & 0xFF);
    frame[8] = static_cast<char>((seg.dy >> 16) & 0xFF);
    frame[9] = static_cast<char>((seg.dy >> 8)  & 0xFF);
    frame[10] = static_cast<char>( seg.dy       & 0xFF);

    // dz (32 bits = 4 octets)
    frame[11] = static_cast<char>((seg.dz >> 24) & 0xFF);
    frame[12] = static_cast<char>((seg.dz >> 16) & 0xFF);
    frame[13] = static_cast<char>((seg.dz >> 8)  & 0xFF);
    frame[14] = static_cast<char>( seg.dz       & 0xFF);

    // v_max (16 bits)
    frame[15] = static_cast<char>((seg.v_max >> 8) & 0xFF);
    frame[16] = static_cast<char>( seg.v_max       & 0xFF);

    // flags
    frame[17] = static_cast<char>(seg.flags);

    const uint16_t crc = calcCrc16(frame);
    frame[18] = static_cast<char>((crc >> 8) & 0xFF);
    frame[19] = static_cast<char>( crc       & 0xFF);

    return frame;
}

uint16_t StmUartService::calcCrc16(const QByteArray& frame)
{
    return stmCrc16(reinterpret_cast<const uint8_t*>(frame.constData()),
                    qMin(STM_FRAME_LEN - 2, frame.size()));
}

bool StmUartService::isFull() const
{
    // BARRIÈRE 1 : L'entrepôt (STM32) est-il plein ?
    // m_stmBufLevel est mis à jour à chaque fois que la STM32 dit "ACK|buf=X"
    bool isWarehouseFull = (m_stmBufLevel >= STM_SEND_AHEAD_MAX);

    // BARRIÈRE 2 : Le camion (câble UART) est-il plein ?
    // m_sentSinceAck compte les trames envoyées qui n'ont pas encore reçu de ACK
    bool isTruckFull = (m_sentSinceAck >= STM_MAX_IN_FLIGHT);

    // On bloque l'envoi de nouvelles trames si l'une des deux limites est atteinte
    return isWarehouseFull || isTruckFull;
}

void StmUartService::resetWindow()
{
    m_stmBufLevel  = 0;
    m_sentSinceAck = 0;
    m_nakCount     = 0;
    m_unackedBatch.clear();
    m_unackedSeqIds.clear();
    m_ackTimer.stop();
    m_globalSeqId  = 0;
}

void StmUartService::sendSegment(const StmSegment& seg)
{
    if (!m_serial.isOpen()) return;

    // NOUVEAU : On génère l'ID pour ce segment précis, et on l'incrémente pour le prochain
    uint16_t currentSeqId = m_globalSeqId++;

    // On passe cet ID à l'encodeur
    QByteArray frame = encodeFrame(seg, currentSeqId);

    // Le reste ne change pas !
    // m_unackedBatch garde une copie de la trame AVEC son ID intégré.
    // Si on timeout, c'est cette trame identique qui sera re-balancée.
    m_unackedBatch.append(frame);
    m_unackedSeqIds.append(currentSeqId);
    ++m_sentSinceAck;

    m_serial.write(frame);
    m_ackTimer.start();

    const bool isEndSeq = (seg.flags & FLAG_END_SEQ) || (seg.flags & FLAG_HOME_SEQ);
    if (isEndSeq) {
        m_nakCount = 0;
    }
}

void StmUartService::requestPrestart(quint32 sessionId, int segmentCount)
{
    if (!m_serial.isOpen()) {
        emit prestartRejected(tr("Port série non ouvert."));
        return;
    }
    sendAsciiCommand(QStringLiteral("PRESTART|session=%1|segments=%2")
                         .arg(sessionId)
                         .arg(qMax(0, segmentCount)));
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
    if (cmd == QLatin1String("HOME") || cmd == QLatin1String("R")) {
        m_heartbeatSuspended = true;
        m_linkTimer.stop();
    }
    if (cmd != QLatin1String("PING"))
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
            if (m_readBuffer.size() < STM_FRAME_LEN)
                break;
            // Vérifier le CRC
            QByteArray frame = m_readBuffer.left(STM_FRAME_LEN);
            m_readBuffer.remove(0, STM_FRAME_LEN);
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
    markHostActivity();

    if (line.startsWith("HELLO|") || line.startsWith("PONG|")) {
        m_health = parseHealthPayload(line);
        m_health.valid = true;
        m_linkHealthy = m_health.fault.isEmpty();
        emit healthChanged(m_health);
        if (!m_health.fault.isEmpty()) {
            emit safetyFault(m_health.fault);
        }
        return;
    }

    if (line.startsWith("PRESTART_OK|session=")) {
        bool ok = false;
        const quint32 sessionId = line.mid(20).toUInt(&ok);
        if (ok) {
            m_streamingActive = true;
            m_linkTimer.start();
            emit prestartAccepted(sessionId);
        } else {
            emit prestartRejected(QStringLiteral("Réponse PRESTART invalide."));
        }
        return;
    }

    if (line.startsWith("PRESTART_REJECT|reason=")) {
        m_streamingActive = false;
        emit prestartRejected(QString::fromUtf8(line.mid(23)));
        return;
    }

    if (line.startsWith("FAULT|reason=")) {
        const QString reason = QString::fromUtf8(line.mid(13));
        m_streamingActive = false;
        markUnsafeLink(reason);
        emit safetyFault(reason);
        return;
    }

    // --- ACK|buf=<n>|seg=<s> ---
    if (line.startsWith("ACK|buf=")) {
        int bufLevel = 0, segIndex = 0;
        QByteArray payload = line.mid(8);
        int sep = payload.indexOf("|seg=");
        if (sep != -1) {
            bufLevel = payload.left(sep).toInt();
            segIndex = payload.mid(sep + 5).toInt();
        }

        m_stmBufLevel  = bufLevel;
        m_sentSinceAck = std::max(0, m_sentSinceAck - 1);
        m_nakCount     = 0;

        if (!m_unackedBatch.isEmpty()) {
            m_unackedBatch.removeFirst();
        }
        if (!m_unackedSeqIds.isEmpty()) {
            m_unackedSeqIds.removeFirst();
        }

        // NOUVEAU : Gestion du chronomètre
        if (m_sentSinceAck > 0) {
            m_ackTimer.start(); // Il reste des trames en vol, on relance le chrono
        } else {
            m_ackTimer.stop();  // Tout est validé, on arrête le chrono
        }

        emit ackReceived(bufLevel, segIndex);
        return;
    }

    if (line.startsWith("ACK|seq=")) {
        int seqId = -1, bufLevel = 0, rxCount = 0;
        const QList<QByteArray> parts = line.mid(4).split('|');
        for (const QByteArray& part : parts) {
            if      (part.startsWith("seq=")) seqId = part.mid(4).toInt();
            else if (part.startsWith("buf=")) bufLevel = part.mid(4).toInt();
            else if (part.startsWith("rx="))  rxCount = part.mid(3).toInt();
        }

        m_stmBufLevel  = bufLevel;
        m_sentSinceAck = std::max(0, m_sentSinceAck - 1);
        m_nakCount     = 0;

        const int seqIndex = m_unackedSeqIds.indexOf(static_cast<uint16_t>(seqId));
        if (seqIndex >= 0) {
            m_unackedSeqIds.removeAt(seqIndex);
            if (seqIndex < m_unackedBatch.size())
                m_unackedBatch.removeAt(seqIndex);
        } else if (!m_unackedBatch.isEmpty()) {
            m_unackedBatch.removeFirst();
            if (!m_unackedSeqIds.isEmpty())
                m_unackedSeqIds.removeFirst();
        }

        if (m_sentSinceAck > 0)
            m_ackTimer.start();
        else
            m_ackTimer.stop();

        emit ackReceived(bufLevel, rxCount);
        return;
    }

    // --- NAK ou ERREUR MATERIELLE UART ---
    if (line == "NAK" || line.endsWith("NAK") || line.startsWith("ERR:UART_HW_FAULT")) {

        m_ackTimer.stop();

        ++m_nakCount;
        emit nakReceived();
        m_streamingActive = false;
        m_nakCount = 0;
        m_unackedBatch.clear();
        m_unackedSeqIds.clear();
        m_sentSinceAck = 0;
        emit safetyFault(tr("Erreur UART/NAK pendant la découpe."));
        return;
    }

    // --- DONE ---
    if (line == "DONE" || line.endsWith("DONE")) {
        // NOUVEAU : On coupe le timer et on vide le batch pour éviter les envois fantômes !
        m_ackTimer.stop();
        m_unackedBatch.clear();
        m_unackedSeqIds.clear();
        m_sentSinceAck = 0;
        m_streamingActive = false;

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
        if (line.startsWith("HOMING") || line == "Z OK" || line == "X OK" || line == "Y OK") {
            m_heartbeatSuspended = true;
            m_linkTimer.stop();
        }
        if (line == "HOMED") {
            m_heartbeatSuspended = false;
            markHostActivity();
        }
        emit homingMessage(QString::fromUtf8(line));
        return;
    }

    // --- Homing errors ---
    if (line == "ERR:HOMING_TIMEOUT") {
        m_heartbeatSuspended = false;
        markHostActivity();
        emit homingError(QStringLiteral("TIMEOUT"));
        return;
    }
    if (line.startsWith("ERR:HOMING_")) {
        m_heartbeatSuspended = false;
        markHostActivity();
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
        markUnsafeLink(QStringLiteral("Redémarrage STM détecté."));
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

            // NOUVEAU : Lecture de l'état de la valve synchronisé avec la position
            if (parts.size() >= 4) {
                static bool lastValveState = false;
                bool currentValveState = (parts[3].toInt() == 1);

                // Si l'état a changé, on simule la réception d'un VALVE ON / OFF
                if (currentValveState != lastValveState) {
                    lastValveState = currentValveState;
                    if (currentValveState) {
                        emit valveOnConfirmed();
                    } else {
                        emit valveOffConfirmed();
                    }
                }
            }

            emit realPositionReceived(x_steps, y_steps);
        }
        return;
    }

    // --- SEG_DONE|seg=N|x=X|y=Y|buf=B ---
    if (line.startsWith("SEG_DONE|")) {
        int seg = 0, x = 0, y = 0;
        int bufLevel = -1; // NOUVEAU

        const QList<QByteArray> parts = line.mid(9).split('|');
        for (const QByteArray& part : parts) {
            if      (part.startsWith("seg=")) seg = part.mid(4).toInt();
            else if (part.startsWith("x="))   x   = part.mid(2).toInt();
            else if (part.startsWith("y="))   y   = part.mid(2).toInt();
            else if (part.startsWith("buf=")) bufLevel = part.mid(4).toInt(); // NOUVEAU
        }

        // NOUVEAU : On met à jour le buffer dynamiquement pour débloquer l'envoi !
        if (bufLevel != -1) {
            m_stmBufLevel = bufLevel;
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

MachineState StmUartService::parseMachineState(const QByteArray& value)
{
    if (value == "READY") return MachineState::READY;
    if (value == "MOVING") return MachineState::MOVING;
    if (value == "HOMING") return MachineState::HOMING;
    if (value == "RECOVERY_WAIT") return MachineState::RECOVERY_WAIT;
    if (value == "EMERGENCY") return MachineState::EMERGENCY;
    if (value == "ALARM") return MachineState::ALARM;
    return MachineState::DISCONNECTED;
}

StmHealth StmUartService::parseHealthPayload(const QByteArray& line)
{
    StmHealth health;
    const int payloadStart = line.indexOf('|');
    if (payloadStart < 0)
        return health;

    const QList<QByteArray> parts = line.mid(payloadStart + 1).split('|');
    for (const QByteArray& part : parts) {
        if (part.startsWith("state=")) {
            health.state = parseMachineState(part.mid(6));
        } else if (part.startsWith("armed=")) {
            health.armed = part.mid(6).toInt() != 0;
        } else if (part.startsWith("homed=")) {
            health.homed = part.mid(6).toInt() != 0;
        } else if (part.startsWith("fault=")) {
            const QByteArray fault = part.mid(6);
            if (!fault.isEmpty() && fault != "NONE")
                health.fault = QString::fromUtf8(fault);
        }
    }
    return health;
}

void StmUartService::markHostActivity()
{
    if (m_serial.isOpen() && !m_heartbeatSuspended)
        m_linkTimer.start();
}

void StmUartService::markUnsafeLink(const QString& reason)
{
    m_linkHealthy = false;
    m_streamingActive = false;
    m_health.valid = false;
    m_ackTimer.stop();
    m_unackedBatch.clear();
    m_unackedSeqIds.clear();
    m_sentSinceAck = 0;
    emit comError(reason);
}

// ----------------------------------------------------------
//  Retry et timeout
// ----------------------------------------------------------

void StmUartService::retransmitLastFrame()
{
    if (m_unackedBatch.isEmpty()) return;

    // Changement du texte du log pour plus de clarté
    qDebug() << "[STM-UART] Retransmission du batch complet suite à NAK/FAULT ("
             << m_nakCount << "/" << MAX_NAK_RETRY << ") - Trames en vol :" << m_unackedBatch.size();

    const int batchSize = m_unackedBatch.size();
    for (int i = 0; i < batchSize; ++i) {
        m_serial.write(m_unackedBatch[i]);
    }
    m_ackTimer.start();
}

void StmUartService::onAckTimeout()
{
    if (m_unackedBatch.isEmpty()) return;

    // AVERTISSEMENT : On ne retransmet plus le dernier segment à l'aveugle.
    // Cela créait des doublons physiques qui faussaient les coordonnées de la machine.
    qWarning() << "[STM-UART] Timeout : ACK en retard. On patiente sans retransmettre.";

    // On relance juste le timer pour laisser la machine finir son mouvement
    m_ackTimer.start(1000);
}

void StmUartService::sendHeartbeat()
{
    if (!m_serial.isOpen())
        return;
    if (m_heartbeatSuspended)
        return;
    sendAsciiCommand(QStringLiteral("PING"));
}

void StmUartService::onLinkTimeout()
{
    if (!m_serial.isOpen())
        return;
    if (m_heartbeatSuspended)
        return;

    markUnsafeLink(tr("Liaison STM perdue : aucun heartbeat reçu."));
    emit linkLost();
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
