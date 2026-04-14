#include "MachineViewModel.h"
#include "StmUartService.h"
#include <QDebug>

// ============================================================
//  MachineViewModel — implémentation
// ============================================================

MachineViewModel::MachineViewModel(QObject* parent)
    : QObject(parent)
    , m_uart(new StmUartService(this))
{
    // --- Branchement des signaux StmUartService → slots locaux ---
    connect(m_uart, &StmUartService::connectionChanged,
            this,   &MachineViewModel::onConnectionChanged);
    connect(m_uart, &StmUartService::ackReceived,
            this,   &MachineViewModel::onAckReceived);
    connect(m_uart, &StmUartService::nakReceived,
            this,   &MachineViewModel::onNakReceived);
    connect(m_uart, &StmUartService::doneReceived,
            this,   &MachineViewModel::onDoneReceived);
    connect(m_uart, &StmUartService::homingMessage,
            this,   &MachineViewModel::onHomingMessage);
    connect(m_uart, &StmUartService::homingError,
            this,   &MachineViewModel::onHomingError);
    connect(m_uart, &StmUartService::emergencyTriggered,
            this,   &MachineViewModel::onEmergencyTriggered);
    connect(m_uart, &StmUartService::alarmTriggered,
            this,   &MachineViewModel::onAlarmTriggered);
    connect(m_uart, &StmUartService::recoveryAvailable,
            this,   &MachineViewModel::onRecoveryAvailable);
    connect(m_uart, &StmUartService::recoveryDataReceived,
            this,   &MachineViewModel::onRecoveryDataReceived);
    connect(m_uart, &StmUartService::recoveryOk,
            this,   &MachineViewModel::onRecoveryOk);
    connect(m_uart, &StmUartService::recoveryCleared,
            this,   &MachineViewModel::onRecoveryCleared);
    connect(m_uart, &StmUartService::readyReceived,
            this,   &MachineViewModel::onReadyReceived);
    connect(m_uart, &StmUartService::startupBannerReceived,
            this,   &MachineViewModel::onStartupBannerReceived);
    connect(m_uart, &StmUartService::errorReceived,
            this,   &MachineViewModel::onErrorReceived);
    connect(m_uart, &StmUartService::comError,
            this,   &MachineViewModel::onComError);
}

MachineViewModel::~MachineViewModel() = default;

// ----------------------------------------------------------
//  Helpers privés
// ----------------------------------------------------------

void MachineViewModel::setState(MachineState s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(s);
}

void MachineViewModel::setStatus(const QString& msg)
{
    if (m_statusMessage == msg) return;
    m_statusMessage = msg;
    emit statusMessageChanged(msg);
}

// ----------------------------------------------------------
//  Connexion
// ----------------------------------------------------------

void MachineViewModel::connectToStm(const QString& portName)
{
    m_uart->open(portName);
}

void MachineViewModel::disconnectFromStm()
{
    m_uart->close();
}

// ----------------------------------------------------------
//  Commandes machine
// ----------------------------------------------------------

void MachineViewModel::sendHome()
{
    if (!m_connected) { setStatus(tr("Non connecté.")); return; }
    m_uart->sendAsciiCommand(QStringLiteral("HOME"));
}

void MachineViewModel::sendPositionReset()
{
    if (!m_connected) { setStatus(tr("Non connecté.")); return; }
    m_uart->sendAsciiCommand(QStringLiteral("H"));
}

void MachineViewModel::sendRearm()
{
    if (!m_connected) { setStatus(tr("Non connecté.")); return; }
    if (m_state != MachineState::EMERGENCY && m_state != MachineState::ALARM) {
        setStatus(tr("Réarmement impossible hors état URGENCE/ALARME."));
        return;
    }
    m_uart->sendAsciiCommand(QStringLiteral("R"));
}

void MachineViewModel::sendValveOn()
{
    if (!m_connected) return;
    m_uart->sendAsciiCommand(QStringLiteral("V+"));
}

void MachineViewModel::sendValveOff()
{
    if (!m_connected) return;
    m_uart->sendAsciiCommand(QStringLiteral("V-"));
}

void MachineViewModel::sendRecover()
{
    if (!m_connected) { setStatus(tr("Non connecté.")); return; }
    if (m_state != MachineState::READY) {
        setStatus(tr("RECOVER impossible : la machine n'est pas en état READY."));
        return;
    }
    m_uart->sendAsciiCommand(QStringLiteral("RECOVER"));
}

void MachineViewModel::sendGo()
{
    if (!m_connected) { setStatus(tr("Non connecté.")); return; }
    if (m_state != MachineState::RECOVERY_WAIT) {
        setStatus(tr("GO impossible : la machine n'attend pas de confirmation de recovery."));
        return;
    }
    m_uart->sendAsciiCommand(QStringLiteral("GO"));
}

void MachineViewModel::sendClear()
{
    if (!m_connected) return;
    m_uart->sendAsciiCommand(QStringLiteral("CLEAR"));
    m_hasRecovery = false;
}

bool MachineViewModel::sendSegment(const StmSegment& seg)
{
    if (!m_connected) return false;

    // Vérification de l'état
    if (m_state != MachineState::READY && m_state != MachineState::MOVING)
        return false;

    // Sécurité : refuser descente Z sans confirmation opérateur
    if (seg.dz < 0 && !m_zDescentConfirmed) {
        setStatus(tr("Descente Z bloquée — confirmation opérateur requise."));
        return false;
    }
    m_zDescentConfirmed = false; // Reset après usage

    // Flow control : vérifier l'espace disponible dans le buffer STM
    if ((STM_BUFFER_MAX - m_bufferLevel) < 1) {
        setStatus(tr("Buffer STM plein — attente DONE."));
        return false;
    }

    setState(MachineState::MOVING);
    m_uart->sendSegment(seg);
    return true;
}

void MachineViewModel::confirmZDescent()
{
    m_zDescentConfirmed = true;
}

// ----------------------------------------------------------
//  Slots — réponses StmUartService
// ----------------------------------------------------------

void MachineViewModel::onConnectionChanged(bool connected)
{
    m_connected = connected;
    if (!connected) {
        setState(MachineState::DISCONNECTED);
        setStatus(tr("Déconnecté du STM32."));
    } else {
        setState(MachineState::DISCONNECTED); // sera mis à jour au 1er READY
        setStatus(tr("Connecté — en attente du STM32..."));
    }
    emit connectionChanged(connected);
}

void MachineViewModel::onAckReceived(int bufLevel, int segIndex)
{
    Q_UNUSED(segIndex)
    if (m_bufferLevel != bufLevel) {
        m_bufferLevel = bufLevel;
        emit bufferLevelChanged(bufLevel);
    }
}

void MachineViewModel::onNakReceived()
{
    setStatus(tr("NAK reçu — retransmission en cours..."));
}

void MachineViewModel::onDoneReceived()
{
    setState(MachineState::READY);
    setStatus(tr("Trajectoire terminée."));
    m_bufferLevel = 0;
    emit bufferLevelChanged(0);
    emit doneReceived();
}

void MachineViewModel::onHomingMessage(const QString& msg)
{
    setState(MachineState::HOMING);
    if (msg == QLatin1String("HOMED")) {
        setState(MachineState::READY);
        setStatus(tr("Homing terminé — machine prête."));
        // Reset de la position logique
        m_posX = 0; m_posY = 0; m_posZ = 0;
        emit positionChanged(0.0, 0.0, 0.0);
    } else {
        setStatus(tr("Homing : %1").arg(msg));
    }
    emit homingProgress(msg);
}

void MachineViewModel::onHomingError(const QString& axis)
{
    setState(MachineState::ALARM);
    setStatus(tr("Erreur homing axe %1 — vérifier la mécanique.").arg(axis));
    emit errorOccurred(QLatin1String("HOMING_") + axis);
}

void MachineViewModel::onEmergencyTriggered()
{
    setState(MachineState::EMERGENCY);
    setStatus(tr("ARRÊT D'URGENCE — relâcher l'AU puis envoyer R pour réarmer."));
}

void MachineViewModel::onAlarmTriggered(const QString& axis)
{
    setState(MachineState::ALARM);
    if (axis == QLatin1String("LIM_HIT"))
        setStatus(tr("Fin de course atteint — envoyer R pour réarmer."));
    else
        setStatus(tr("Alarme driver axe %1 — envoyer R pour réarmer.").arg(axis));
}

void MachineViewModel::onRecoveryAvailable(RecoveryData data)
{
    m_hasRecovery = true;
    m_recovery    = data;
    setStatus(tr("Recovery disponible — segment %1, pos X=%2 Y=%3 Z=%4 mm")
              .arg(data.seg)
              .arg(stepsToMm(data.x), 0, 'f', 1)
              .arg(stepsToMm(data.y), 0, 'f', 1)
              .arg(stepsToMm(data.z), 0, 'f', 1));
    emit recoveryAvailable(data);
}

void MachineViewModel::onRecoveryDataReceived(RecoveryData data)
{
    m_recovery = data;
    setState(MachineState::RECOVERY_WAIT);
    setStatus(tr("Repositionner la tête puis confirmer GO."));
    // Mise à jour position depuis la recovery
    m_posX = data.x; m_posY = data.y; m_posZ = data.z;
    emit positionChanged(stepsToMm(data.x), stepsToMm(data.y), stepsToMm(data.z));
}

void MachineViewModel::onRecoveryOk()
{
    setState(MachineState::READY);
    setStatus(tr("Recovery acceptée — reprise trajectoire autorisée."));
    m_hasRecovery = false;
}

void MachineViewModel::onRecoveryCleared()
{
    m_hasRecovery = false;
    setStatus(tr("Recovery effacée."));
}

void MachineViewModel::onReadyReceived()
{
    setState(MachineState::READY);
    setStatus(tr("Machine prête."));
    m_bufferLevel = 0;
}

void MachineViewModel::onStartupBannerReceived()
{
    // Banner reçu hors boot initial → redémarrage watchdog STM
    if (m_connected && m_state != MachineState::DISCONNECTED)
        setStatus(tr("ATTENTION : Redémarrage inattendu du STM32 détecté (watchdog)."));
    setState(MachineState::DISCONNECTED);
}

void MachineViewModel::onErrorReceived(const QString& code)
{
    setStatus(tr("Erreur STM : %1").arg(code));
    emit errorOccurred(code);
}

void MachineViewModel::onComError(const QString& reason)
{
    setStatus(tr("Erreur communication : %1").arg(reason));
    emit errorOccurred(reason);
}
