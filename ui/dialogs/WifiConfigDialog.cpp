#include "WifiConfigDialog.h"
#include "ui_WifiConfigDialog.h"

#include <QMessageBox>
#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPalette>
#include <QColor>
#include <QSettings>
#include <QVariant>
#include <QFont>
#include <QtGlobal>
#include <QApplication>
#include <QProgressDialog>
#include <QLabel>
#include "ThemeManager.h"


static void showToast(QWidget *parent, const QString &text, int ms = 1800) {
    auto *lbl = new QLabel(text, parent);
    lbl->setObjectName("wifiToast");
    lbl->setStyleSheet(
        "QLabel#wifiToast {"
        "  background: rgba(0,0,0,190); color: white;"
        "  border-radius: 10px; padding: 6px 10px; font-weight: 500;"
        "}"
    );
    lbl->setAttribute(Qt::WA_ShowWithoutActivating);
    lbl->adjustSize();
    const int margin = 12;
    const QPoint pos(parent->width() - lbl->width() - margin, margin);
    lbl->move(pos);
    lbl->show();
    QTimer::singleShot(ms, lbl, &QLabel::deleteLater);
}

WifiConfigDialog::WifiConfigDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WifiConfigDialog)
    , m_vm(new WifiConfigViewModel(this))
{
    ui->setupUi(this);

    // Thème global — hérité de qApp, aucune action nécessaire
    m_isDarkTheme = ThemeManager::instance()->isDark();

    // --- Réglages UI pour grand écran (table) ---
    if (ui->networksTable) {
        ui->networksTable->setColumnCount(3);
        auto *hh = ui->networksTable->horizontalHeader();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        hh->setStretchLastSection(true);
        hh->setSectionResizeMode(0, QHeaderView::Stretch);
        hh->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);
#endif
        ui->networksTable->sortItems(1, Qt::DescendingOrder);
    }

    // --- Connexions UI ---
    connect(ui->refreshButton,     &QPushButton::clicked, this, &WifiConfigDialog::scanNetworks);
    connect(ui->connectButton,     &QPushButton::clicked, this, &WifiConfigDialog::connectSelected);
    connect(ui->disconnectButton,  &QPushButton::clicked, this, &WifiConfigDialog::disconnectFromSelected);
    connect(ui->hiddenSsidCheck,   &QCheckBox::toggled,   this, &WifiConfigDialog::onHiddenSsidToggled);
    connect(ui->diagnosticsButton, &QPushButton::clicked, this, &WifiConfigDialog::showDiagnostics);
    connect(ui->autoScanCheck,     &QCheckBox::toggled,   this, &WifiConfigDialog::onAutoScanToggled);
    connect(ui->forgetButton,      &QPushButton::clicked, this, &WifiConfigDialog::forgetCurrentNetwork);

    connect(ui->backButton, &QPushButton::clicked, this, [this]{ close(); });

    if (ui->networksTable && ui->selectedSsidLine) {
        connect(ui->networksTable, &QTableWidget::itemSelectionChanged, this, [this]{
            int row = ui->networksTable->currentRow();
            if (row >= 0) {
                auto *ssidItem = ui->networksTable->item(row, 0);
                ui->selectedSsidLine->setText(ssidItem ? ssidItem->text() : QString());
                ui->hiddenSsidCheck->setChecked(false);
            }
        });
        connect(ui->networksTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem*){
            connectSelected();
        });
    }

    if (ui->selectedSsidLine)
        connect(ui->selectedSsidLine, &QLineEdit::textChanged,
                this, &WifiConfigDialog::updateCredentialStateForCurrentSsid);
    if (ui->networkComboBox)
#if QT_VERSION >= QT_VERSION_CHECK(5,7,0)
        connect(ui->networkComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
                this, &WifiConfigDialog::updateCredentialStateForCurrentSsid);
#else
        connect(ui->networkComboBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(updateCredentialStateForCurrentSsid()));
#endif

    // Timers
    _statusTimer = new QTimer(this);
    _statusTimer->setInterval(3000);
    connect(_statusTimer, &QTimer::timeout, this, &WifiConfigDialog::checkConnectionStatus);
    _statusTimer->start();

    _scanTimer = new QTimer(this);
    _scanTimer->setInterval(10000);
    connect(_scanTimer, &QTimer::timeout, this, &WifiConfigDialog::scanNetworks);
    if (ui->autoScanCheck) ui->autoScanCheck->setChecked(true);

    // Préremplir avec le dernier SSID connu
    {
        QSettings s;
        const QString last = s.value("wifi/last_ssid").toString();
        if (!last.isEmpty() && ui->selectedSsidLine)
            ui->selectedSsidLine->setText(last);
    }

    onHiddenSsidToggled(false);
    checkConnectionStatus();
    scanNetworks();
    updateCredentialStateForCurrentSsid();
}

WifiConfigDialog::~WifiConfigDialog()
{
    delete ui;
}

void WifiConfigDialog::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
}

// ================== UI helpers ==================

void WifiConfigDialog::populateNetworksFromScan(
    const QList<WifiConfigViewModel::ScannedNetwork> &networks)
{
    if (ui->networkComboBox) ui->networkComboBox->clear();
    if (ui->networksTable)   ui->networksTable->setRowCount(0);

    for (const auto &net : networks) {
        const QString secDisplay = net.security.isEmpty() ? tr("Ouvert") : net.security;

        if (ui->networkComboBox) {
            const QString display = QString("%1  (%2%)  [%3]")
                .arg(net.ssid).arg(net.signalPercent).arg(secDisplay);
            ui->networkComboBox->addItem(display, net.ssid);
        }

        if (ui->networksTable) {
            int r = ui->networksTable->rowCount();
            ui->networksTable->insertRow(r);
            ui->networksTable->setItem(r, 0, new QTableWidgetItem(net.ssid));
            ui->networksTable->setItem(r, 1, new QTableWidgetItem(
                QString::number(net.signalPercent) + "%"));
            ui->networksTable->setItem(r, 2, new QTableWidgetItem(secDisplay));
        }
    }

    if (ui->networksTable) ui->networksTable->sortItems(1, Qt::DescendingOrder);

    if (ui->selectedSsidLine && ui->selectedSsidLine->text().trimmed().isEmpty()
        && ui->networksTable && ui->networksTable->rowCount() > 0)
    {
        ui->networksTable->setCurrentCell(0, 0);
        auto *ssidItem = ui->networksTable->item(0, 0);
        if (ssidItem) ui->selectedSsidLine->setText(ssidItem->text());
    }

    if (ui->statusLabel)
        updateStatusLabel(tr("Réseaux trouvés : %1").arg(networks.size()), true);

    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::setBusy(bool busy)
{
    _busy = busy;
    if (ui->connectButton)     ui->connectButton->setEnabled(!busy);
    if (ui->disconnectButton)  ui->disconnectButton->setEnabled(!busy);
    if (ui->refreshButton)     ui->refreshButton->setEnabled(!busy);
    if (ui->backButton)        ui->backButton->setEnabled(!busy);
    if (ui->diagnosticsButton) ui->diagnosticsButton->setEnabled(!busy);
    if (ui->networksTable)     ui->networksTable->setEnabled(!busy);
    if (ui->busyLabel)         ui->busyLabel->setVisible(busy);
}

void WifiConfigDialog::updateStatusLabel(const QString &msg, bool ok)
{
    if (!ui->statusLabel) return;
    ui->statusLabel->setText(msg);
    QPalette pal = ui->statusLabel->palette();
    pal.setColor(QPalette::WindowText, ok ? QColor("#1b8a5a") : QColor("#bb1e10"));
    ui->statusLabel->setPalette(pal);
}

// ================== Slots ==================

void WifiConfigDialog::scanNetworks()
{
    setBusy(true);
    QString errorMsg;
    const QList<WifiConfigViewModel::ScannedNetwork> networks = m_vm->scanNetworks(errorMsg);
    setBusy(false);

    if (!errorMsg.isEmpty()) {
        updateStatusLabel(errorMsg, false);
        return;
    }

    populateNetworksFromScan(networks);
}

void WifiConfigDialog::connectSelected()
{
    // Choix du SSID
    QString ssid;
    const bool hidden = ui->hiddenSsidCheck && ui->hiddenSsidCheck->isChecked();
    if (hidden) {
        ssid = ui->ssidLineEdit ? ui->ssidLineEdit->text().trimmed() : QString();
    } else if (ui->selectedSsidLine && !ui->selectedSsidLine->text().trimmed().isEmpty()) {
        ssid = ui->selectedSsidLine->text().trimmed();
    } else if (ui->networkComboBox) {
        ssid = ui->networkComboBox->currentData().toString();
    }

    const QString password = ui->passwordEdit ? ui->passwordEdit->text() : QString();

    if (ssid.isEmpty()) {
        QMessageBox::warning(this, tr("Connexion Wi\u2011Fi"),
                             tr("Veuillez sélectionner ou saisir un SSID."));
        return;
    }

    setBusy(true);
    if (ui->statusLabel) ui->statusLabel->setText(tr("Connexion à %1…").arg(ssid));

    QProgressDialog *progress = new QProgressDialog(
        tr("Connexion au Wi\u2011Fi en cours…"), QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(nullptr);
    progress->setMinimumDuration(0);
    progress->setAutoClose(true);
    progress->setAutoReset(true);
    progress->show();

    QString oldBtnText;
    if (ui->connectButton) {
        oldBtnText = ui->connectButton->text();
        ui->connectButton->setText(tr("Connexion…"));
    }
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    showToast(this, tr("Connexion à %1 en cours…").arg(ssid), 1600);

    const WifiConfigViewModel::ActionResult result = m_vm->connectToNetwork(ssid, password, hidden);

    if (ui->connectButton) ui->connectButton->setText(oldBtnText);
    if (progress) { progress->close(); progress->deleteLater(); }
    setBusy(false);

    if (!result.ok) {
        QMessageBox::warning(this, tr("Connexion Wi\u2011Fi"),
                             QString("❌ %1").arg(result.errorMsg));
        showToast(this, tr("❌ Échec de connexion à %1").arg(ssid), 1600);
        updateStatusLabel(tr("❌ Échec de la connexion à %1").arg(ssid), false);
        return;
    }

    const bool autoconnect = (ui->autoConnectCheck && ui->autoConnectCheck->isChecked());
    m_vm->persistConnectionSettings(ssid, password, autoconnect);

    updateStatusLabel(tr("✅ Connecté à %1").arg(ssid), true);
    if (ui->passwordEdit) ui->passwordEdit->clear();
    checkConnectionStatus();
    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::checkConnectionStatus()
{
    const WifiConfigViewModel::ConnectionStatus status = m_vm->getConnectionStatus();

    if (status.currentSsid.isEmpty() && status.ipAddress.isEmpty()) {
        if (ui->currentNetworkValue) ui->currentNetworkValue->setText("-");
        if (ui->ipValue)             ui->ipValue->setText("-");
        if (ui->signalValue)         ui->signalValue->setText("-");
        updateStatusLabel(tr("⚪ Non connecté"), true);
        updateCredentialStateForCurrentSsid();
        return;
    }

    if (ui->currentNetworkValue) {
        ui->currentNetworkValue->setText(
            status.currentSsid.isEmpty() ? tr("(non connecté)") : status.currentSsid);
        QPalette p = ui->currentNetworkValue->palette();
        p.setColor(QPalette::WindowText,
                   status.currentSsid.isEmpty() ? QColor("#666666") : QColor("#1b8a5a"));
        ui->currentNetworkValue->setPalette(p);
        QFont f = ui->currentNetworkValue->font();
        f.setBold(!status.currentSsid.isEmpty());
        ui->currentNetworkValue->setFont(f);
    }
    if (ui->ipValue)
        ui->ipValue->setText(status.ipAddress.isEmpty() ? "-" : status.ipAddress);
    if (ui->signalValue)
        ui->signalValue->setText(status.signalPercent >= 0
                                  ? QString::number(status.signalPercent) + "%"
                                  : "-");

    // Gras sur la ligne connectée dans la table
    if (ui->networksTable) {
        for (int r = 0; r < ui->networksTable->rowCount(); ++r) {
            auto *itm = ui->networksTable->item(r, 0);
            if (!itm) continue;
            QFont f = itm->font();
            f.setBold(!status.currentSsid.isEmpty() && itm->text() == status.currentSsid);
            itm->setFont(f);
        }
    }

    if (!status.currentSsid.isEmpty()) {
        updateStatusLabel(
            tr("🟢 Connecté à %1 — %2").arg(
                status.currentSsid,
                status.ipAddress.isEmpty() ? tr("IP inconnue") : status.ipAddress),
            true);
    } else {
        updateStatusLabel(tr("⚪ Non connecté"), true);
    }

    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::onHiddenSsidToggled(bool on)
{
    if (ui->ssidLineEdit) ui->ssidLineEdit->setVisible(on);
    Q_UNUSED(on);
    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::showDiagnostics()
{
    const QString diag = m_vm->getDiagnosticsInfo();
    QGuiApplication::clipboard()->setText(diag);
    QMessageBox::information(this, tr("Diagnostic Wi\u2011Fi"),
                             tr("Diagnostic copié dans le presse‑papiers."));
}

void WifiConfigDialog::onAutoScanToggled(bool on)
{
    if (on) _scanTimer->start();
    else    _scanTimer->stop();
}

void WifiConfigDialog::updateCredentialStateForCurrentSsid()
{
    const bool hidden = ui->hiddenSsidCheck && ui->hiddenSsidCheck->isChecked();

    QString ssid;
    if (hidden) {
        ssid = ui->ssidLineEdit ? ui->ssidLineEdit->text().trimmed() : QString();
    } else if (ui->selectedSsidLine && !ui->selectedSsidLine->text().trimmed().isEmpty()) {
        ssid = ui->selectedSsidLine->text().trimmed();
    } else if (ui->networkComboBox) {
        ssid = ui->networkComboBox->currentData().toString();
        if (ssid.isEmpty()) ssid = ui->networkComboBox->currentText().trimmed();
    }

    if (ssid.isEmpty()) {
        if (ui->passwordEdit)     { ui->passwordEdit->setVisible(true); ui->passwordEdit->setEnabled(true); }
        if (ui->savedPwdLabel)    ui->savedPwdLabel->setVisible(false);
        if (ui->forgetButton)     { ui->forgetButton->setVisible(false); ui->forgetButton->setProperty("connName", QVariant()); }
        if (ui->connectButton)    ui->connectButton->setVisible(true);
        if (ui->disconnectButton) ui->disconnectButton->setVisible(false);
        return;
    }

    // Reset password field when SSID changes
    const QString prev = this->property("prevSsid").toString();
    if (prev != ssid) {
        if (ui->passwordEdit) ui->passwordEdit->clear();
        this->setProperty("prevSsid", ssid);
    }

    const QString currentSsid      = m_vm->currentConnectedSsid();
    const bool    selectedIsCurrent = (!ssid.isEmpty() && !currentSsid.isEmpty() && ssid == currentSsid);
    if (ui->connectButton)    ui->connectButton->setVisible(!selectedIsCurrent);
    if (ui->disconnectButton) ui->disconnectButton->setVisible(selectedIsCurrent);

    const QString sec      = m_vm->getSecurityForSsid(ssid);
    const bool    isOpen   = sec.trimmed().isEmpty();
    QString connName       = m_vm->findConnectionNameForSsid(ssid);
    if (connName.isEmpty() && selectedIsCurrent)
        connName = m_vm->activeWifiConnectionName();

    const bool hasProfile  = !connName.isEmpty();
    const bool hasSavedPwd = hasProfile && m_vm->isPasswordSavedForConnection(connName);
    const bool shouldHidePwd = isOpen || hasSavedPwd;

    if (ui->passwordEdit) {
        if (shouldHidePwd) ui->passwordEdit->clear();
        ui->passwordEdit->setVisible(!shouldHidePwd);
        ui->passwordEdit->setEnabled(!shouldHidePwd);
        if (!shouldHidePwd && ui->passwordEdit->text().isEmpty())
            ui->passwordEdit->setPlaceholderText(tr("Mot de passe"));
    }

    if (ui->savedPwdLabel) {
        ui->savedPwdLabel->setVisible(shouldHidePwd || hasProfile);
        if (isOpen)
            ui->savedPwdLabel->setText(tr("🔓 Réseau ouvert — pas de mot de passe"));
        else if (hasSavedPwd)
            ui->savedPwdLabel->setText(tr("🔒 Mot de passe mémorisé pour « %1 »").arg(ssid));
        else if (hasProfile)
            ui->savedPwdLabel->setText(tr("🗂️ Profil enregistré — saisissez le mot de passe si nécessaire"));
        else {
            ui->savedPwdLabel->clear();
            ui->savedPwdLabel->setVisible(false);
        }
    }

    if (ui->forgetButton) {
        ui->forgetButton->setVisible(hasProfile);
        ui->forgetButton->setEnabled(hasProfile);
        ui->forgetButton->setProperty("connName", connName);
    }
}

void WifiConfigDialog::forgetCurrentNetwork()
{
    if (!ui->forgetButton) return;
    const QString connName = ui->forgetButton->property("connName").toString();
    if (connName.isEmpty()) return;

    auto ret = QMessageBox::question(this, tr("Oublier ce réseau"),
                                     tr("Supprimer le profil « %1 » et oublier le mot de passe ?").arg(connName),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const WifiConfigViewModel::ActionResult result = m_vm->forgetNetwork(connName);
    if (!result.ok) {
        QMessageBox::warning(this, tr("Oublier le réseau"), result.errorMsg);
        return;
    }

    if (ui->passwordEdit)  { ui->passwordEdit->clear(); ui->passwordEdit->setEnabled(true);
                             ui->passwordEdit->setPlaceholderText(tr("Mot de passe")); }
    if (ui->savedPwdLabel) ui->savedPwdLabel->setVisible(false);
    if (ui->forgetButton)  { ui->forgetButton->setVisible(false);
                             ui->forgetButton->setProperty("connName", QVariant()); }

    updateStatusLabel(tr("Profil supprimé. Vous devrez ressaisir le mot de passe."), true);
    scanNetworks();
    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::disconnectFromSelected()
{
    QString ssid;
    const bool hidden = ui->hiddenSsidCheck && ui->hiddenSsidCheck->isChecked();
    if (hidden) {
        ssid = ui->ssidLineEdit ? ui->ssidLineEdit->text().trimmed() : QString();
    } else if (ui->selectedSsidLine && !ui->selectedSsidLine->text().trimmed().isEmpty()) {
        ssid = ui->selectedSsidLine->text().trimmed();
    } else if (ui->networkComboBox) {
        ssid = ui->networkComboBox->currentData().toString();
        if (ssid.isEmpty()) ssid = ui->networkComboBox->currentText().trimmed();
    }

    const WifiConfigViewModel::ActionResult result = m_vm->disconnectFromNetwork(ssid);
    if (!result.ok) {
        QMessageBox::warning(this, tr("Déconnexion Wi\u2011Fi"), result.errorMsg);
        return;
    }

    updateStatusLabel(
        tr("⛔ Déconnecté de %1").arg(ssid.isEmpty() ? tr("le réseau courant") : ssid),
        true);
    checkConnectionStatus();
    updateCredentialStateForCurrentSsid();
}
