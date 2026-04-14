#include "StmTestDialog.h"
#include "ui_StmTestDialog.h"
#include "StmProtocol.h"

#include <QDateTime>
#include <QClipboard>
#include <QApplication>
#include <QSerialPortInfo>

// ============================================================
//  StmTestDialog — implémentation
// ============================================================

StmTestDialog::StmTestDialog(MachineViewModel *vm, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StmTestDialog)
    , m_vm(vm)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::Window);
    resize(1000, 700);

    setupConnections();
    refreshPortList();

    // Initialiser l'état depuis le ViewModel courant
    onConnectionChanged(m_vm->isConnected());
    onStateChanged(m_vm->state());
    onBufferLevelChanged(m_vm->bufferLevel());
    onPositionChanged(m_vm->posX_mm(), m_vm->posY_mm(), m_vm->posZ_mm());

    appendLog(tr("=== Interface Test Moteurs STM32 prête ==="), "#a6e3a1");
    appendLog(tr("Sélectionnez un port et cliquez Connecter."), "#cba6f7");
}

StmTestDialog::~StmTestDialog()
{
    delete ui;
}

// ----------------------------------------------------------
//  Connexion des signaux
// ----------------------------------------------------------

void StmTestDialog::setupConnections()
{
    // Boutons UI → slots locaux
    connect(ui->connectBtn,       &QPushButton::clicked, this, &StmTestDialog::onConnectClicked);
    connect(ui->refreshPortsBtn,  &QPushButton::clicked, this, &StmTestDialog::onRefreshPortsClicked);
    connect(ui->homeBtn,          &QPushButton::clicked, this, &StmTestDialog::onHomeClicked);
    connect(ui->rearmBtn,         &QPushButton::clicked, this, &StmTestDialog::onRearmClicked);
    connect(ui->resetPosBtn,      &QPushButton::clicked, this, &StmTestDialog::onResetPosClicked);
    connect(ui->valveOnBtn,       &QPushButton::clicked, this, &StmTestDialog::onValveOnClicked);
    connect(ui->valveOffBtn,      &QPushButton::clicked, this, &StmTestDialog::onValveOffClicked);
    connect(ui->sendSegmentBtn,   &QPushButton::clicked, this, &StmTestDialog::onSendSegmentClicked);
    connect(ui->recoverBtn,       &QPushButton::clicked, this, &StmTestDialog::onRecoverClicked);
    connect(ui->goBtn,            &QPushButton::clicked, this, &StmTestDialog::onGoClicked);
    connect(ui->clearRecoveryBtn, &QPushButton::clicked, this, &StmTestDialog::onClearRecoveryClicked);
    connect(ui->clearLogBtn,      &QPushButton::clicked, this, &StmTestDialog::onClearLogClicked);
    connect(ui->copyLogBtn,       &QPushButton::clicked, this, &StmTestDialog::onCopyLogClicked);
    connect(ui->closeBtn,         &QPushButton::clicked, this, &QDialog::close);

    // MachineViewModel → slots locaux
    connect(m_vm, &MachineViewModel::stateChanged,
            this, &StmTestDialog::onStateChanged);
    connect(m_vm, &MachineViewModel::connectionChanged,
            this, &StmTestDialog::onConnectionChanged);
    connect(m_vm, &MachineViewModel::bufferLevelChanged,
            this, &StmTestDialog::onBufferLevelChanged);
    connect(m_vm, &MachineViewModel::positionChanged,
            this, &StmTestDialog::onPositionChanged);
    connect(m_vm, &MachineViewModel::statusMessageChanged,
            this, &StmTestDialog::onStatusMessageChanged);
    connect(m_vm, &MachineViewModel::homingProgress,
            this, &StmTestDialog::onHomingProgress);
    connect(m_vm, &MachineViewModel::errorOccurred,
            this, &StmTestDialog::onErrorOccurred);
    connect(m_vm, &MachineViewModel::recoveryAvailable,
            this, &StmTestDialog::onRecoveryAvailable);
    connect(m_vm, &MachineViewModel::doneReceived,
            this, &StmTestDialog::onDoneReceived);
}

// ----------------------------------------------------------
//  Rafraîchissement de la liste des ports
// ----------------------------------------------------------

void StmTestDialog::refreshPortList()
{
    const QString current = ui->portCombo->currentText();
    ui->portCombo->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports)
        ui->portCombo->addItem(info.portName());

    // Restaurer la sélection précédente si le port est encore disponible
    const int idx = ui->portCombo->findText(current);
    if (idx >= 0)
        ui->portCombo->setCurrentIndex(idx);

    appendLog(tr("→ %1 port(s) série détecté(s).").arg(ports.size()), "#cba6f7");
}

void StmTestDialog::onRefreshPortsClicked()
{
    refreshPortList();
}

// ----------------------------------------------------------
//  Boutons connexion
// ----------------------------------------------------------

void StmTestDialog::onConnectClicked()
{
    if (!m_connected) {
        const QString port = ui->portCombo->currentText().trimmed();
        if (port.isEmpty()) return;
        appendLog(tr("→ Connexion sur %1...").arg(port), "#89b4fa");
        m_vm->connectToStm(port);
    } else {
        appendLog(tr("→ Déconnexion..."), "#f38ba8");
        m_vm->disconnectFromStm();
    }
}

// ----------------------------------------------------------
//  Boutons commandes rapides
// ----------------------------------------------------------

void StmTestDialog::onHomeClicked()
{
    appendLog(tr("→ TX : HOME"), "#89b4fa");
    m_vm->sendHome();
}

void StmTestDialog::onRearmClicked()
{
    appendLog(tr("→ TX : R (réarmement)"), "#fab387");
    m_vm->sendRearm();
}

void StmTestDialog::onResetPosClicked()
{
    appendLog(tr("→ TX : H (reset position logique)"), "#cba6f7");
    m_vm->sendPositionReset();
}

void StmTestDialog::onValveOnClicked()
{
    appendLog(tr("→ TX : V+ (vanne ON)"), "#a6e3a1");
    m_vm->sendValveOn();
}

void StmTestDialog::onValveOffClicked()
{
    appendLog(tr("→ TX : V- (vanne OFF)"), "#f38ba8");
    m_vm->sendValveOff();
}

void StmTestDialog::onSendSegmentClicked()
{
    // Construire le segment depuis les spinboxes
    StmSegment seg;
    seg.dx    = static_cast<int16_t>(mmToSteps(ui->dxSpin->value()));
    seg.dy    = static_cast<int16_t>(mmToSteps(ui->dySpin->value()));
    seg.dz    = static_cast<int16_t>(mmToSteps(ui->dzSpin->value()));
    seg.v_max = speedToArr(ui->vitesseSpin->value());
    seg.flags = 0;
    if (ui->flagValveCheck->isChecked())  seg.flags |= FLAG_VALVE_ON;
    if (ui->flagEndSeqCheck->isChecked()) seg.flags |= FLAG_END_SEQ;

    appendLog(tr("→ TX Segment : dX=%1 dY=%2 dZ=%3 pas | ARR=%4 | flags=0x%5")
              .arg(seg.dx).arg(seg.dy).arg(seg.dz)
              .arg(seg.v_max)
              .arg(seg.flags, 2, 16, QChar('0')).toUpper(),
              "#89dceb");

    if (!m_vm->sendSegment(seg)) {
        appendLog(tr("  ✗ Segment refusé (état incompatible ou buffer plein)"), "#f38ba8");
    }
}

// ----------------------------------------------------------
//  Boutons recovery
// ----------------------------------------------------------

void StmTestDialog::onRecoverClicked()
{
    appendLog(tr("→ TX : RECOVER"), "#f9e2af");
    m_vm->sendRecover();
}

void StmTestDialog::onGoClicked()
{
    appendLog(tr("→ TX : GO (confirmation reprise)"), "#a6e3a1");
    m_vm->sendGo();
}

void StmTestDialog::onClearRecoveryClicked()
{
    appendLog(tr("→ TX : CLEAR (annulation recovery)"), "#f38ba8");
    m_vm->sendClear();
    ui->grpRecovery->setVisible(false);
}

// ----------------------------------------------------------
//  Boutons log
// ----------------------------------------------------------

void StmTestDialog::onClearLogClicked()
{
    ui->logText->clear();
    appendLog(tr("Log effacé."), "#6c7086");
}

void StmTestDialog::onCopyLogClicked()
{
    QApplication::clipboard()->setText(ui->logText->toPlainText());
}

// ----------------------------------------------------------
//  Réactions MachineViewModel
// ----------------------------------------------------------

void StmTestDialog::onStateChanged(MachineState state)
{
    ui->stateLabel->setText(stateToString(state));
    ui->stateLabel->setStyleSheet(
        QString("font-weight: bold; font-size: 13px; color: %1;").arg(stateToColor(state)));
    updateButtonStates(state, m_connected);
}

void StmTestDialog::onConnectionChanged(bool connected)
{
    m_connected = connected;
    if (connected) {
        ui->connectBtn->setText(tr("Déconnecter"));
        ui->connectBtn->setStyleSheet(
            "QPushButton { background-color: #c0392b; color: white; border-radius: 4px; "
            "padding: 4px 10px; font-weight: bold; } QPushButton:hover { background-color: #e74c3c; }");
        ui->connexionStatusLabel->setText(tr("● Connecté"));
        ui->connexionStatusLabel->setStyleSheet("color: #a6e3a1; font-weight: bold;");
        appendLog(tr("✓ Connexion établie."), "#a6e3a1");
    } else {
        ui->connectBtn->setText(tr("Connecter"));
        ui->connectBtn->setStyleSheet(
            "QPushButton { background-color: #27ae60; color: white; border-radius: 4px; "
            "padding: 4px 10px; font-weight: bold; } QPushButton:hover { background-color: #2ecc71; }");
        ui->connexionStatusLabel->setText(tr("● Déconnecté"));
        ui->connexionStatusLabel->setStyleSheet("color: #f38ba8; font-weight: bold;");
    }
    updateButtonStates(m_vm->state(), connected);
}

void StmTestDialog::onBufferLevelChanged(int level)
{
    ui->bufferBar->setValue(level);
}

void StmTestDialog::onPositionChanged(double x, double y, double z)
{
    ui->posXLabel->setText(QString("%1 mm").arg(x, 0, 'f', 2));
    ui->posYLabel->setText(QString("%1 mm").arg(y, 0, 'f', 2));
    ui->posZLabel->setText(QString("%1 mm").arg(z, 0, 'f', 2));
}

void StmTestDialog::onStatusMessageChanged(const QString &msg)
{
    appendLog(tr("  ℹ %1").arg(msg), "#cba6f7");
}

void StmTestDialog::onHomingProgress(const QString &msg)
{
    if (msg == QLatin1String("HOMED"))
        appendLog(tr("✓ HOMED — Tous les axes sont référencés."), "#a6e3a1");
    else
        appendLog(tr("  ⟳ %1").arg(msg), "#f9e2af");
}

void StmTestDialog::onErrorOccurred(const QString &code)
{
    appendLog(tr("✗ ERREUR : %1").arg(code), "#f38ba8");
}

void StmTestDialog::onRecoveryAvailable(RecoveryData data)
{
    ui->grpRecovery->setVisible(true);
    ui->recoveryInfoLabel->setText(
        tr("Recovery disponible\nSeg : %1\nPos : X=%2 Y=%3 Z=%4 mm")
        .arg(data.seg)
        .arg(stepsToMm(data.x), 0, 'f', 1)
        .arg(stepsToMm(data.y), 0, 'f', 1)
        .arg(stepsToMm(data.z), 0, 'f', 1));
    appendLog(tr("⚠ Recovery disponible — seg=%1 X=%2mm Y=%3mm")
              .arg(data.seg)
              .arg(stepsToMm(data.x), 0, 'f', 1)
              .arg(stepsToMm(data.y), 0, 'f', 1),
              "#fab387");
}

void StmTestDialog::onDoneReceived()
{
    appendLog(tr("✓ DONE — Trajectoire terminée. Machine READY."), "#a6e3a1");
}

// ----------------------------------------------------------
//  Helpers
// ----------------------------------------------------------

void StmTestDialog::appendLog(const QString &text, const QString &color)
{
    const QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString html;
    if (color.isEmpty()) {
        html = QString("<span style='color:#cdd6f4;'>[%1] %2</span>").arg(time, text.toHtmlEscaped());
    } else {
        html = QString("<span style='color:#6c7086;'>[%1]</span> <span style='color:%2;'>%3</span>")
               .arg(time, color, text.toHtmlEscaped());
    }
    ui->logText->appendHtml(html);

    // Auto-scroll vers le bas
    QTextCursor c = ui->logText->textCursor();
    c.movePosition(QTextCursor::End);
    ui->logText->setTextCursor(c);
}

void StmTestDialog::updateButtonStates(MachineState state, bool connected)
{
    const bool ready   = connected && (state == MachineState::READY);
    const bool moving  = connected && (state == MachineState::MOVING);
    const bool alarm   = connected && (state == MachineState::EMERGENCY
                                       || state == MachineState::ALARM);

    ui->homeBtn->setEnabled(ready);
    ui->rearmBtn->setEnabled(alarm);
    ui->resetPosBtn->setEnabled(connected);
    ui->valveOnBtn->setEnabled(connected);
    ui->valveOffBtn->setEnabled(connected);
    ui->sendSegmentBtn->setEnabled(ready || moving);
    ui->recoverBtn->setEnabled(ready && m_vm->hasRecovery());
    ui->goBtn->setEnabled(connected && state == MachineState::RECOVERY_WAIT);
    ui->clearRecoveryBtn->setEnabled(connected);
}

QString StmTestDialog::stateToString(MachineState state) const
{
    switch (state) {
    case MachineState::DISCONNECTED:   return tr("DÉCONNECTÉ");
    case MachineState::READY:          return tr("READY");
    case MachineState::MOVING:         return tr("EN MOUVEMENT");
    case MachineState::HOMING:         return tr("HOMING...");
    case MachineState::RECOVERY_WAIT:  return tr("ATTENTE GO");
    case MachineState::EMERGENCY:      return tr("URGENCE");
    case MachineState::ALARM:          return tr("ALARME");
    }
    return tr("INCONNU");
}

QString StmTestDialog::stateToColor(MachineState state) const
{
    switch (state) {
    case MachineState::READY:         return "#a6e3a1"; // vert
    case MachineState::MOVING:        return "#89b4fa"; // bleu
    case MachineState::HOMING:        return "#f9e2af"; // jaune
    case MachineState::RECOVERY_WAIT: return "#fab387"; // orange
    case MachineState::EMERGENCY:     return "#f38ba8"; // rouge
    case MachineState::ALARM:         return "#f38ba8"; // rouge
    case MachineState::DISCONNECTED:  return "#6c7086"; // gris
    }
    return "#6c7086";
}
