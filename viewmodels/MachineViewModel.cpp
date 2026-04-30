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
    connect(m_uart, &StmUartService::controlledStopReceived,
            this,   &MachineViewModel::onControlledStopReceived);
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
    connect(m_uart, &StmUartService::rawLineReceived,
            this,   &MachineViewModel::onRawLineReceived);
    connect(m_uart, &StmUartService::healthChanged,
            this,   &MachineViewModel::onHealthChanged);
    connect(m_uart, &StmUartService::prestartAccepted,
            this,   &MachineViewModel::onPrestartAccepted);
    connect(m_uart, &StmUartService::prestartRejected,
            this,   &MachineViewModel::onPrestartRejected);
    connect(m_uart, &StmUartService::safetyFault,
            this,   &MachineViewModel::onSafetyFault);
    connect(m_uart, &StmUartService::linkLost,
            this,   &MachineViewModel::onLinkLost);
    connect(m_uart, &StmUartService::valveOnConfirmed,
            this,   &MachineViewModel::onValveOnConfirmed);
    connect(m_uart, &StmUartService::valveOffConfirmed,
            this,   &MachineViewModel::onValveOffConfirmed);
    connect(m_uart, &StmUartService::posResetConfirmed,
            this,   &MachineViewModel::onPosResetConfirmed);
    connect(m_uart, &StmUartService::homeAckConfirmed,
            this,   &MachineViewModel::onHomeAckConfirmed);

    // Exécution réelle et contrôle pause/stop
    connect(m_uart, &StmUartService::segDoneReceived,
            this,   &MachineViewModel::onSegDoneReceived);
    connect(m_uart, &StmUartService::pausedConfirmed,
            this,   &MachineViewModel::onPausedConfirmed);
    connect(m_uart, &StmUartService::resumedConfirmed,
            this,   &MachineViewModel::onResumedConfirmed);

    connect(m_uart, &StmUartService::realPositionReceived,
            this, &MachineViewModel::realPositionReceived);
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
    updateSafetyReady();
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
    m_autoInitialize = false;
    m_uart->open(portName);
}

void MachineViewModel::connectAndInitialize(const QString& portName)
{
    m_autoInitialize = true;
    m_autoHomingRequested = false;
    m_uart->open(portName);
}

bool MachineViewModel::isSafetyReady() const
{
    return m_connected
        && m_linkHealthy
        && m_homed
        && !m_valveOpen
        && m_state == MachineState::READY;
}

void MachineViewModel::updateSafetyReady()
{
    const bool ready = isSafetyReady();
    if (m_safetyReady == ready)
        return;
    m_safetyReady = ready;
    emit safetyReadyChanged(ready);
}

void MachineViewModel::maybeStartAutomaticHoming()
{
    if (!m_autoInitialize || m_autoHomingRequested || !m_connected || !m_linkHealthy)
        return;
    if (m_homed || m_valveOpen)
        return;
    if (m_state != MachineState::READY && m_state != MachineState::BOOTING)
        return;

    m_autoHomingRequested = true;
    setStatus(tr("Initialisation machine : referencement automatique..."));
    sendHome();
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
    if (m_state == MachineState::CUTTING || m_state == MachineState::PAUSED || m_state == MachineState::STOPPING) {
        setStatus(tr("HOME refuse pendant une decoupe."));
        return;
    }
    m_homed = false;
    m_prestartAccepted = false;
    updateSafetyReady();
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
    if (m_state != MachineState::EMERGENCY && m_state != MachineState::FAULT) {
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
    m_prestartAccepted = false;
}

bool MachineViewModel::sendSegment(const StmSegment& seg)
{
    // 1. Vérification de la connexion
    if (!m_connected) return false;

    // 2. Vérification de l'état de la machine
    if (m_state != MachineState::READY && m_state != MachineState::CUTTING)
        return false;

    if (!m_prestartAccepted) {
        setStatus(tr("Découpe bloquée — préflight STM non validé."));
        return false;
    }

    // 3. Sécurité : refuser descente Z sans confirmation opérateur
    if (seg.dz < 0 && !m_zDescentConfirmed) {
        setStatus(tr("Descente Z bloquée — confirmation opérateur requise."));
        return false;
    }
    m_zDescentConfirmed = false; // Reset après usage

    // ==========================================
    // 4. CONTRÔLE DE FLUX — fenêtre glissante
    // ==========================================
    // On bloque si le buffer STM estimé est plein (isFull).
    // Le thread TrajetMotor patientera 10ms et réessayera tout seul.
    if (m_uart->isFull()) {
        return false;
    }

    // 5. Mise à jour de l'état et envoi
    setState(MachineState::CUTTING);
    m_uart->sendSegment(seg);

    // NB: On a retiré le ++m_sentSinceLastAck ici car c'est désormais
    // StmUartService qui gère son propre batch en interne.

    return true;
}

bool MachineViewModel::requestPrestart(int segmentCount)
{
    if (!m_connected) {
        setStatus(tr("Préflight impossible : STM32 non connecté."));
        return false;
    }
    if (!isSafetyReady()) {
        setStatus(tr("Préflight impossible : liaison sûre, READY et homing requis."));
        return false;
    }
    if (segmentCount <= 0) {
        setStatus(tr("Préflight impossible : aucune trajectoire à envoyer."));
        return false;
    }

    m_prestartAccepted = false;
    ++m_pendingSessionId;
    if (m_pendingSessionId == 0)
        m_pendingSessionId = 1;

    m_uart->requestPrestart(m_pendingSessionId, segmentCount);
    setStatus(tr("Préflight STM en cours..."));
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
        m_linkHealthy = false;
        m_prestartAccepted = false;
        m_autoHomingRequested = false;
        setState(MachineState::DISCONNECTED);
        setStatus(tr("Déconnecté du STM32."));
    } else {
        m_linkHealthy = false;
        m_prestartAccepted = false;
        setState(MachineState::BOOTING);
        setStatus(tr("Initialisation machine : attente du STM32..."));
    }
    emit connectionChanged(connected);
    updateSafetyReady();
}

void MachineViewModel::onAckReceived(int bufLevel, int segIndex)
{
    Q_UNUSED(segIndex)
    m_sentSinceLastAck = 0;   // le STM a confirmé la réception du batch
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
    setStatus(tr("Trajectoire terminee - referencement automatique requis."));
    m_homed = false;
    m_autoHomingRequested = false;
    m_bufferLevel      = 0;
    m_sentSinceLastAck = 0;
    m_prestartAccepted = false;
    emit bufferLevelChanged(0);
    emit doneReceived();
    updateSafetyReady();
    maybeStartAutomaticHoming();
}

void MachineViewModel::onControlledStopReceived()
{
    setState(MachineState::READY);
    setStatus(tr("Arret controle termine - referencement automatique requis."));
    m_homed = false;
    m_autoHomingRequested = false;
    m_bufferLevel = 0;
    m_sentSinceLastAck = 0;
    m_prestartAccepted = false;
    emit bufferLevelChanged(0);
    emit doneReceived();
    updateSafetyReady();
    maybeStartAutomaticHoming();
}

void MachineViewModel::onHomingMessage(const QString& msg)
{
    setState(MachineState::HOMING);
    if (msg == QLatin1String("HOMED")) {
        m_homed = true;
        setState(MachineState::READY);
        setStatus(tr("Homing terminé — machine prête."));
        // Reset de la position logique
        m_posX = 0; m_posY = 0; m_posZ = 0;
        emit positionChanged(0.0, 0.0, 0.0);
    } else {
        if (msg.startsWith(QLatin1String("HOMING")))
            m_homed = false;
        setStatus(tr("Homing : %1").arg(msg));
    }
    emit homingProgress(msg);
    updateSafetyReady();
}

void MachineViewModel::onHomingError(const QString& axis)
{
    m_homed = false;
    m_prestartAccepted = false;
    setState(MachineState::FAULT);
    setStatus(tr("Erreur homing axe %1 — vérifier la mécanique.").arg(axis));
    emit errorOccurred(QLatin1String("HOMING_") + axis);
}

void MachineViewModel::onEmergencyTriggered()
{
    m_prestartAccepted = false;
    setState(MachineState::EMERGENCY);
    setStatus(tr("ARRÊT D'URGENCE — relâcher l'AU puis envoyer R pour réarmer."));
}

void MachineViewModel::onAlarmTriggered(const QString& axis)
{
    m_prestartAccepted = false;
    setState(MachineState::FAULT);
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
    m_bufferLevel      = 0;
    m_sentSinceLastAck = 0;
    updateSafetyReady();
    maybeStartAutomaticHoming();
}

void MachineViewModel::onStartupBannerReceived()
{
    // Banner reçu hors boot initial → redémarrage watchdog STM
    if (m_connected && m_state != MachineState::DISCONNECTED)
        setStatus(tr("ATTENTION : Redémarrage inattendu du STM32 détecté (watchdog)."));
    m_linkHealthy = false;
    m_prestartAccepted = false;
    setState(MachineState::DISCONNECTED);
    updateSafetyReady();
}

void MachineViewModel::onErrorReceived(const QString& code)
{
    setStatus(tr("Erreur STM : %1").arg(code));
    emit errorOccurred(code);
}

void MachineViewModel::onComError(const QString& reason)
{
    m_linkHealthy = false;
    m_prestartAccepted = false;
    updateSafetyReady();
    setStatus(tr("Erreur communication : %1").arg(reason));
    emit errorOccurred(reason);
}

void MachineViewModel::onRawLineReceived(const QString& line)
{
    emit rxLine(line);
}

void MachineViewModel::onHealthChanged(StmHealth health)
{
    m_linkHealthy = health.valid && health.fault.isEmpty();
    m_homed = health.homed;
    m_valveOpen = health.valveOpen;
    if (health.bufferLevel >= 0 && m_bufferLevel != health.bufferLevel) {
        m_bufferLevel = health.bufferLevel;
        emit bufferLevelChanged(m_bufferLevel);
    }
    if (health.state != MachineState::DISCONNECTED)
        setState(health.state);
    updateSafetyReady();
    maybeStartAutomaticHoming();
}

void MachineViewModel::onPrestartAccepted(quint32 sessionId)
{
    if (sessionId != m_pendingSessionId) {
        setStatus(tr("Préflight ignoré : session STM inattendue."));
        return;
    }
    m_activeSessionId = sessionId;
    m_prestartAccepted = true;
    setStatus(tr("Préflight STM validé — découpe autorisée."));
    emit prestartAccepted();
}

void MachineViewModel::onPrestartRejected(const QString& reason)
{
    m_prestartAccepted = false;
    setStatus(tr("Préflight refusé : %1").arg(reason));
    emit prestartRejected(reason);
}

void MachineViewModel::onSafetyFault(const QString& reason)
{
    m_linkHealthy = false;
    m_prestartAccepted = false;
    setState(MachineState::FAULT);
    setStatus(tr("Défaut sécurité STM : %1").arg(reason));
    emit safetyFault(reason);
    updateSafetyReady();
}

void MachineViewModel::onLinkLost()
{
    m_linkHealthy = false;
    m_prestartAccepted = false;
    setState(MachineState::DISCONNECTED);
    setStatus(tr("Liaison STM perdue — découpe bloquée."));
    emit linkLost();
    updateSafetyReady();
}

void MachineViewModel::onValveOnConfirmed()  { emit valveOnConfirmed(); }
void MachineViewModel::onValveOffConfirmed() { emit valveOffConfirmed(); }
void MachineViewModel::onPosResetConfirmed() { emit posResetConfirmed(); }
void MachineViewModel::onHomeAckConfirmed()  { emit homeAckConfirmed(); }

void MachineViewModel::sendPause()
{
    if (!m_connected) return;
    if (m_state != MachineState::CUTTING) {
        setStatus(tr("Pause impossible : aucune decoupe en cours."));
        return;
    }
    m_uart->sendAsciiCommand(QStringLiteral("PAUSE"));
}

void MachineViewModel::sendResume()
{
    if (!m_connected) return;
    if (m_state != MachineState::PAUSED) {
        setStatus(tr("Reprise impossible : la machine n'est pas en pause."));
        return;
    }
    m_uart->sendAsciiCommand(QStringLiteral("RESUME"));
}

void MachineViewModel::sendStop()
{
    if (!m_connected) return;
    m_prestartAccepted = false;
    setState(MachineState::STOPPING);
    m_uart->sendControlledStop();
}

void MachineViewModel::sendEmergencyStop()
{
    if (!m_connected) return;
    m_prestartAccepted = false;
    m_uart->sendEmergencyStop();
}

void MachineViewModel::onSegDoneReceived(int seg, int x, int y)
{
    m_posX = x;
    m_posY = y;
    emit positionChanged(stepsToMm(x), stepsToMm(y), stepsToMm(m_posZ));
    emit segmentDone(seg, x, y);
}

void MachineViewModel::onPausedConfirmed()
{
    setState(MachineState::PAUSED);
    emit machinePaused();
}

void MachineViewModel::onResumedConfirmed()
{
    setState(MachineState::CUTTING);
    emit machineResumed();
}

void MachineViewModel::sendAsciiMove(int dx_steps, int dy_steps, int dz_steps, int arr)
{
    if (!m_connected || m_state != MachineState::READY) {
        setStatus(tr("Mouvement ASCII impossible : machine non prête."));
        return;
    }
    const QString cmd = QString("X%1 Y%2 Z%3 F%4")
        .arg(dx_steps).arg(dy_steps).arg(dz_steps).arg(arr);
    m_uart->sendAsciiCommand(cmd);
}
