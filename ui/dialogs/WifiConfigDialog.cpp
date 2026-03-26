#include "WifiConfigDialog.h"
#include "ui_WifiConfigDialog.h"
#include "MainWindow.h"

#include "WifiNmcliParsers.h"
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

namespace {
MainWindow* resolveMainWindow()
{
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget *w : widgets) {
        if (auto *mw = qobject_cast<MainWindow*>(w)) {
            return mw;
        }
    }
    return nullptr;
}
}

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
    , m_wifiProfileService(m_nmcli)
{
    ui->setupUi(this);

    // --- Réglages UI pour grand écran (table) ---
    if (ui->networksTable) {
        ui->networksTable->setColumnCount(3); // SSID, Signal, Sécurité (déjà défini dans le .ui)
        auto *hh = ui->networksTable->horizontalHeader();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        hh->setStretchLastSection(true);
        hh->setSectionResizeMode(0, QHeaderView::Stretch);           // SSID
        hh->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Signal
        hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Sécurité
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

    connect(ui->backButton, &QPushButton::clicked, this, [this]{
        close();
        if (auto mw = resolveMainWindow()) mw->showFullScreen();
    });

    // Sélection dans la table -> remplit selectedSsidLine
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

    // Changement d’entrée -> on met à jour l’état des identifiants
    if (ui->selectedSsidLine)
        connect(ui->selectedSsidLine, &QLineEdit::textChanged, this, &WifiConfigDialog::updateCredentialStateForCurrentSsid);
    if (ui->networkComboBox)
#if QT_VERSION >= QT_VERSION_CHECK(5,7,0)
        connect(ui->networkComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &WifiConfigDialog::updateCredentialStateForCurrentSsid);
#else
        connect(ui->networkComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCredentialStateForCurrentSsid()));
#endif

    // Timers
    _statusTimer = new QTimer(this);
    _statusTimer->setInterval(3000);
    connect(_statusTimer, &QTimer::timeout, this, &WifiConfigDialog::checkConnectionStatus);
    _statusTimer->start();

    _scanTimer = new QTimer(this);
    _scanTimer->setInterval(10000);
    connect(_scanTimer, &QTimer::timeout, this, &WifiConfigDialog::scanNetworks);
    if (ui->autoScanCheck) ui->autoScanCheck->setChecked(true); // auto-scan par défaut

    // Préremplir avec le dernier SSID connu (si présent)
    {
        QSettings s;
        const QString last = s.value("wifi/last_ssid").toString();
        if (!last.isEmpty() && ui->selectedSsidLine) {
            ui->selectedSsidLine->setText(last);
        }
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
    if (auto mw = resolveMainWindow())
        mw->showFullScreen();
}

// ================== NMCLI helpers ==================

void WifiConfigDialog::populateNetworksFromScan(const QString &nmcliOutput)
{
    // nmcli -t -f SSID,SIGNAL,SECURITY dev wifi list
    if (ui->networkComboBox) ui->networkComboBox->clear();
    if (ui->networksTable)   ui->networksTable->setRowCount(0);

    QStringList seen; // éviter doublons SSID
    const auto lines = nmcliOutput.split('\n', Qt::SkipEmptyParts);
    for (const auto &lnRaw : lines) {
        const QString ln = lnRaw.trimmed();
        const QString ssid = WifiNmcliParsers::extractSsid(ln);
        if (ssid.isEmpty() || seen.contains(ssid)) continue;
        seen << ssid;

        const int signal = WifiNmcliParsers::parseSignalPercent(ln);
        const QString sec = WifiNmcliParsers::parseSecurity(ln);
        const QString secDisplay = sec.isEmpty() ? tr("Ouvert") : sec;

        // Combo (compat)
        if (ui->networkComboBox) {
            const QString display = QString("%1  (%2%)  [%3]").arg(ssid).arg(signal).arg(secDisplay);
            ui->networkComboBox->addItem(display, ssid);
        }

        // Table
        if (ui->networksTable) {
            int r = ui->networksTable->rowCount();
            ui->networksTable->insertRow(r);
            ui->networksTable->setItem(r, 0, new QTableWidgetItem(ssid));
            ui->networksTable->setItem(r, 1, new QTableWidgetItem(QString::number(signal) + "%"));
            ui->networksTable->setItem(r, 2, new QTableWidgetItem(secDisplay));
        }
    }

    // Tri par signal desc si dispo
    if (ui->networksTable) ui->networksTable->sortItems(1, Qt::DescendingOrder);

    // Si rien de sélectionné, remplir le champ lecture seule avec le 1er SSID
    if (ui->selectedSsidLine && ui->selectedSsidLine->text().trimmed().isEmpty() && ui->networksTable && ui->networksTable->rowCount() > 0) {
        ui->networksTable->setCurrentCell(0, 0);
        auto *ssidItem = ui->networksTable->item(0, 0);
        if (ssidItem) ui->selectedSsidLine->setText(ssidItem->text());
    }

    if (ui->statusLabel) updateStatusLabel(tr("Réseaux trouvés : %1").arg(seen.size()), true);

    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::setBusy(bool busy)
{
    _busy = busy;
    if (ui->connectButton)      ui->connectButton->setEnabled(!busy);
    if (ui->disconnectButton)   ui->disconnectButton->setEnabled(!busy);
    if (ui->refreshButton)      ui->refreshButton->setEnabled(!busy);
    if (ui->backButton)         ui->backButton->setEnabled(!busy);
    if (ui->diagnosticsButton)  ui->diagnosticsButton->setEnabled(!busy);
    if (ui->networksTable)      ui->networksTable->setEnabled(!busy);
    if (ui->busyLabel)          ui->busyLabel->setVisible(busy);
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
    if (!m_nmcli.ensureWifiRadioOn()) {
        updateStatusLabel(tr("Wi‑Fi désactivé — tentative d’activation impossible."), false);
        return;
    }

    const QString dev = m_nmcli.detectWifiDevice();
    if (dev.isEmpty()) {
        updateStatusLabel(tr("Aucune interface Wi‑Fi détectée."), false);
        return;
    }

    setBusy(true);
    // Liste SSID:SIGNAL:SECURITY
    WifiNmcliClient::Result r = m_nmcli.run({ "-t", "-f", "SSID,SIGNAL,SECURITY", "device", "wifi", "list", "ifname", dev });
    setBusy(false);

    if (!r.ok()) {
        updateStatusLabel(tr("Échec du scan Wi‑Fi: %1").arg(r.err.trimmed().isEmpty() ? r.out.trimmed() : r.err.trimmed()), false);
        return;
    }

    populateNetworksFromScan(r.out);
}

void WifiConfigDialog::connectSelected()
{
    if (!m_nmcli.ensureWifiRadioOn()) {
        updateStatusLabel(tr("Wi‑Fi désactivé — impossible de se connecter."), false);
        return;
    }

    const QString dev = m_nmcli.detectWifiDevice();
    if (dev.isEmpty()) {
        updateStatusLabel(tr("Aucune interface Wi‑Fi détectée."), false);
        return;
    }

    // Choix du SSID : SSID caché > sélection dans la table > fallback combo (cachée)
    QString ssid;
    if (ui->hiddenSsidCheck && ui->hiddenSsidCheck->isChecked()) {
        ssid = ui->ssidLineEdit ? ui->ssidLineEdit->text().trimmed() : QString();
    } else if (ui->selectedSsidLine && !ui->selectedSsidLine->text().trimmed().isEmpty()) {
        ssid = ui->selectedSsidLine->text().trimmed();
    } else if (ui->networkComboBox) {
        ssid = ui->networkComboBox->currentData().toString();
    }

    const QString password = ui->passwordEdit ? ui->passwordEdit->text() : QString();

    if (ssid.isEmpty()) {
        QMessageBox::warning(this, tr("Connexion Wi‑Fi"), tr("Veuillez sélectionner ou saisir un SSID."));
        return;
    }

    // Prépare la commande
    QStringList args = { "device", "wifi", "connect", ssid, "ifname", dev };
    if (!password.isEmpty()) {
        args << "password" << password;
    }
    if (ui->hiddenSsidCheck && ui->hiddenSsidCheck->isChecked()) {
        args << "hidden" << "yes";
    }

    // UX
    setBusy(true);
    if (ui->statusLabel) ui->statusLabel->setText(tr("Connexion à %1…").arg(ssid));

    /* --- NOUVEAU : mini indicateur de progression + feedback immédiat --- */
    QProgressDialog *progress = new QProgressDialog(tr("Connexion au Wi‑Fi en cours…"), QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(nullptr);
    progress->setMinimumDuration(0);  // s'affiche tout de suite
    progress->setAutoClose(true);
    progress->setAutoReset(true);
    progress->show();

    // Changer temporairement le libellé du bouton
    QString oldBtnText;
    if (ui->connectButton) {
        oldBtnText = ui->connectButton->text();
        ui->connectButton->setText(tr("Connexion…"));
    }

    // Force l'affichage immédiat (sinon tout apparaît après nmcli)
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    /* --- FIN NOUVEAU --- */

    showToast(this, tr("Connexion à %1 en cours…").arg(ssid), 1600);

    WifiNmcliClient::Result r = m_nmcli.run(args, 20000);

    /* --- NOUVEAU : restaurer l'état UI --- */
    if (ui->connectButton) ui->connectButton->setText(oldBtnText);
    if (progress) { progress->close(); progress->deleteLater(); }
    /* --- FIN NOUVEAU --- */

    setBusy(false);

    if (!r.ok()) {
        // Mapping erreurs nmcli -> messages lisibles
        const QString raw = (r.err.trimmed().isEmpty() ? r.out.trimmed() : r.err.trimmed());
        QString msg = raw;

        if (raw.contains("No network with SSID", Qt::CaseInsensitive))
            msg = tr("Réseau introuvable. Vérifiez le SSID (ou cochez « SSID caché »).");
        else if (raw.contains("incorrect password", Qt::CaseInsensitive)
                 || raw.contains("failed to activate connection", Qt::CaseInsensitive))
            msg = tr("Mot de passe incorrect ou authentification refusée.");
        else if (raw.contains("Device not ready", Qt::CaseInsensitive))
            msg = tr("Interface Wi‑Fi indisponible (mode avion ? pilote ?).");
        else if (raw.contains("Timeout", Qt::CaseInsensitive))
            msg = tr("Délai dépassé. Réessayez en vous rapprochant du point d’accès.");

        QMessageBox::warning(this, tr("Connexion Wi‑Fi"), QString("❌ %1").arg(msg));
        showToast(this, tr("❌ Échec de connexion à %1").arg(ssid), 1600);
        updateStatusLabel(tr("❌ Échec de la connexion à %1").arg(ssid), false);
        return;
    }

    // Enregistrer le dernier SSID (pour présélection)
    {
        QSettings s;
        s.setValue("wifi/last_ssid", ssid);
    }

    // Trouver le vrai nom de profil et appliquer les politiques
    const QString connName = m_wifiProfileService.findConnectionNameForSsid(ssid);
    const bool autoconnect = (ui->autoConnectCheck && ui->autoConnectCheck->isChecked());
    if (!connName.isEmpty()) {
        m_wifiProfileService.applyAutoConnectPolicy(connName, autoconnect);
        if (!password.isEmpty()) {
            m_wifiProfileService.persistPsk(connName, password); // mémorise le mot de passe
        }
    }

    updateStatusLabel(tr("✅ Connecté à %1").arg(ssid), true);
    if (ui->passwordEdit) ui->passwordEdit->clear();
    checkConnectionStatus();
    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::checkConnectionStatus()
{
    const QString dev = m_nmcli.detectWifiDevice();
    if (dev.isEmpty()) {
        if (ui->currentNetworkValue) ui->currentNetworkValue->setText("-");
        if (ui->ipValue)             ui->ipValue->setText("-");
        if (ui->signalValue)         ui->signalValue->setText("-");
        updateStatusLabel(tr("⚪ Non connecté"), true);
        return;
    }

    // SSID actuel (fiable) + IP
    const QString currentSsid = m_wifiProfileService.currentSsidFromScan(dev);

    WifiNmcliClient::Result ip = m_nmcli.run({ "-t", "-f", "IP4.ADDRESS", "device", "show", dev });
    const QString ip4 = WifiNmcliParsers::parseIpV4(ip.out);

    if (ui->currentNetworkValue) {
        ui->currentNetworkValue->setText(currentSsid.isEmpty() ? tr("(non connecté)") : currentSsid);
        QPalette p = ui->currentNetworkValue->palette();
        p.setColor(QPalette::WindowText, currentSsid.isEmpty() ? QColor("#666666") : QColor("#1b8a5a"));
        ui->currentNetworkValue->setPalette(p);
        QFont f = ui->currentNetworkValue->font();
        f.setBold(!currentSsid.isEmpty());
        ui->currentNetworkValue->setFont(f);
    }
    if (ui->ipValue)     ui->ipValue->setText(ip4.isEmpty() ? "-" : ip4);

    // Force du signal (si connecté)
    int signal = -1;
    if (!currentSsid.isEmpty()) {
        WifiNmcliClient::Result scan = m_nmcli.run({ "-t", "-f", "SSID,SIGNAL,SECURITY,IN-USE", "device", "wifi", "list", "ifname", dev });
        if (scan.ok()) {
            const auto lines = scan.out.split('\n', Qt::SkipEmptyParts);
            for (const auto &ln : lines) {
                const QString ssid = WifiNmcliParsers::extractSsid(ln);
                if (ssid == currentSsid) {
                    signal = WifiNmcliParsers::parseSignalPercent(ln);
                    break;
                }
            }
        }
    }
    if (ui->signalValue) ui->signalValue->setText(signal >= 0 ? QString::number(signal) + "%" : "-");

    // Mettre en gras la ligne connectée SANS écraser la sélection utilisateur
    if (ui->networksTable) {
        for (int r = 0; r < ui->networksTable->rowCount(); ++r) {
            auto *itm = ui->networksTable->item(r, 0);
            if (!itm) continue;
            QFont f = itm->font();
            f.setBold(!currentSsid.isEmpty() && itm->text() == currentSsid);
            itm->setFont(f);
        }
    }

    // Message de statut persistant
    if (!currentSsid.isEmpty()) {
        updateStatusLabel(tr("🟢 Connecté à %1 — %2").arg(currentSsid, ip4.isEmpty() ? tr("IP inconnue") : ip4), true);
    } else {
        updateStatusLabel(tr("⚪ Non connecté"), true);
    }

    // Rafraîchir l'état des boutons (Se déconnecter / Oublier, etc.)
    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::onHiddenSsidToggled(bool on)
{
    if (ui->ssidLineEdit)      ui->ssidLineEdit->setVisible(on);
    // On garde la table visible; selectedSsidLine visible aussi (lecture seule)
    Q_UNUSED(on);
    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::showDiagnostics()
{
    const QString dev = m_nmcli.detectWifiDevice();

    QString diag;
    diag += "=== nmcli device status ===\n";
    diag += m_nmcli.run({ "device", "status" }).out + "\n";
    diag += "=== nmcli connection show --active ===\n";
    diag += m_nmcli.run({ "connection", "show", "--active" }).out + "\n";
    if (!dev.isEmpty()) {
        diag += QString("=== nmcli device show %1 ===\n").arg(dev);
        diag += m_nmcli.run({ "device", "show", dev }).out + "\n";
        diag += QString("=== nmcli -t -f SSID,SIGNAL,SECURITY device wifi list ifname %1 ===\n").arg(dev);
        diag += m_nmcli.run({ "-t", "-f", "SSID,SIGNAL,SECURITY", "device", "wifi", "list", "ifname", dev }).out + "\n";
    }

    QGuiApplication::clipboard()->setText(diag);
    QMessageBox::information(this, tr("Diagnostic Wi‑Fi"),
                             tr("Diagnostic copié dans le presse‑papiers."));
}

void WifiConfigDialog::onAutoScanToggled(bool on)
{
    if (on) _scanTimer->start();
    else    _scanTimer->stop();
}

void WifiConfigDialog::updateCredentialStateForCurrentSsid()
{
    const QString dev  = m_nmcli.detectWifiDevice();
    const bool hidden  = ui->hiddenSsidCheck && ui->hiddenSsidCheck->isChecked();

    // --- Quel SSID est sélectionné ?
    QString ssid;
    if (hidden) {
        ssid = ui->ssidLineEdit ? ui->ssidLineEdit->text().trimmed() : QString();
    } else if (ui->selectedSsidLine && !ui->selectedSsidLine->text().trimmed().isEmpty()) {
        ssid = ui->selectedSsidLine->text().trimmed();
    } else if (ui->networkComboBox) {
        ssid = ui->networkComboBox->currentData().toString();
        if (ssid.isEmpty()) ssid = ui->networkComboBox->currentText().trimmed();
    }

    // --- Aucun SSID
    if (ssid.isEmpty()) {
        if (ui->passwordEdit)  { ui->passwordEdit->setVisible(true); ui->passwordEdit->setEnabled(true); }
        if (ui->savedPwdLabel) { ui->savedPwdLabel->setVisible(false); }
        if (ui->forgetButton)  { ui->forgetButton->setVisible(false); ui->forgetButton->setProperty("connName", QVariant()); }
        if (ui->connectButton) ui->connectButton->setVisible(true);
        if (ui->disconnectButton) ui->disconnectButton->setVisible(false);
        return;
    }

    // --- Effacer la saisie UNIQUEMENT si l’SSID a changé
    const QString prev = this->property("prevSsid").toString();
    const bool ssidChanged = (prev != ssid);
    if (ssidChanged) {
        if (ui->passwordEdit) ui->passwordEdit->clear();
        this->setProperty("prevSsid", ssid);
    }

    // --- Boutons Connexion / Déconnexion
    const QString currentSsid = m_wifiProfileService.currentSsidFromScan(dev);
    const bool selectedIsCurrent = (!ssid.isEmpty() && !currentSsid.isEmpty() && ssid == currentSsid);
    if (ui->connectButton)    ui->connectButton->setVisible(!selectedIsCurrent);
    if (ui->disconnectButton) ui->disconnectButton->setVisible(selectedIsCurrent);

    // --- Détection sécurité + profil
    const QString sec = m_wifiProfileService.getSecurityForSsid(ssid, dev);
    const bool isOpen = sec.trimmed().isEmpty();

    QString connName = m_wifiProfileService.findConnectionNameForSsid(ssid);
    if (connName.isEmpty() && selectedIsCurrent)
        connName = m_wifiProfileService.activeWifiConnectionNameForDevice(dev);

    const bool hasProfile  = !connName.isEmpty();
    const bool hasSavedPwd = hasProfile && m_wifiProfileService.isPasswordSavedForConnection(connName);

    const bool shouldHidePwd = isOpen || hasSavedPwd;

    // --- Champ mot de passe
    if (ui->passwordEdit) {
        if (shouldHidePwd) ui->passwordEdit->clear(); // on efface seulement quand on cache
        ui->passwordEdit->setVisible(!shouldHidePwd);
        ui->passwordEdit->setEnabled(!shouldHidePwd);
        if (!shouldHidePwd && ui->passwordEdit->text().isEmpty())
            ui->passwordEdit->setPlaceholderText(tr("Mot de passe"));
    }

    // --- Label d’état sous le champ
    if (ui->savedPwdLabel) {
        ui->savedPwdLabel->setVisible(shouldHidePwd || hasProfile);
        if (isOpen)
            ui->savedPwdLabel->setText(tr("🔓 Réseau ouvert — pas de mot de passe"));
        else if (hasSavedPwd)
            ui->savedPwdLabel->setText(tr("🔒 Mot de passe mémorisé pour « %1 »").arg(ssid));
        else if (hasProfile)
            ui->savedPwdLabel->setText(tr("🗂️ Profil enregistré — saisissez le mot de passe si nécessaire"));
        else {
            ui->savedPwdLabel->clear();
            ui->savedPwdLabel->setVisible(false);
        }
    }

    // --- Bouton « Oublier »
    if (ui->forgetButton) {
        ui->forgetButton->setVisible(hasProfile);
        ui->forgetButton->setEnabled(hasProfile);
        ui->forgetButton->setProperty("connName", connName);
    }
}

void WifiConfigDialog::forgetCurrentNetwork()
{
    if (!ui->forgetButton) return;
    const QVariant v = ui->forgetButton->property("connName");
    const QString connName = v.toString();
    if (connName.isEmpty()) return;

    auto ret = QMessageBox::question(this, tr("Oublier ce réseau"),
                                     tr("Supprimer le profil « %1 » et oublier le mot de passe ?").arg(connName),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    WifiNmcliClient::Result del = m_nmcli.run({ "connection", "delete", connName });
    if (!del.ok()) {
        QMessageBox::warning(this, tr("Oublier le réseau"),
                             tr("Impossible de supprimer le profil : %1").arg(del.err.trimmed().isEmpty() ? del.out.trimmed() : del.err.trimmed()));
        return;
    }

    if (ui->passwordEdit)  { ui->passwordEdit->clear(); ui->passwordEdit->setEnabled(true); ui->passwordEdit->setPlaceholderText(tr("Mot de passe")); }
    if (ui->savedPwdLabel) { ui->savedPwdLabel->setVisible(false); }
    if (ui->forgetButton)  { ui->forgetButton->setVisible(false); ui->forgetButton->setProperty("connName", QVariant()); }

    updateStatusLabel(tr("Profil supprimé. Vous devrez ressaisir le mot de passe."), true);
    scanNetworks();
    updateCredentialStateForCurrentSsid();
}

void WifiConfigDialog::disconnectFromSelected()
{
    const QString dev  = m_nmcli.detectWifiDevice();
    if (dev.isEmpty()) return;

    // Quel SSID est sélectionné ?
    QString ssid;
    if (ui->hiddenSsidCheck && ui->hiddenSsidCheck->isChecked()) {
        ssid = ui->ssidLineEdit ? ui->ssidLineEdit->text().trimmed() : QString();
    } else if (ui->selectedSsidLine && !ui->selectedSsidLine->text().trimmed().isEmpty()) {
        ssid = ui->selectedSsidLine->text().trimmed();
    } else if (ui->networkComboBox) {
        ssid = ui->networkComboBox->currentData().toString();
        if (ssid.isEmpty()) ssid = ui->networkComboBox->currentText().trimmed();
    }

    // On tente "connection down" sur le bon profil; sinon "device disconnect"
    QString connName = m_wifiProfileService.findConnectionNameForSsid(ssid);
    if (connName.isEmpty())
        connName = m_wifiProfileService.activeWifiConnectionNameForDevice(dev);

    WifiNmcliClient::Result r = connName.isEmpty()
               ? m_nmcli.run({ "device", "disconnect", dev })
               : m_nmcli.run({ "connection", "down", connName });

    if (!r.ok()) {
        QMessageBox::warning(this, tr("Déconnexion Wi‑Fi"),
                             tr("Impossible de se déconnecter : %1").arg(r.err.trimmed().isEmpty() ? r.out.trimmed() : r.err.trimmed()));
        return;
    }

    updateStatusLabel(tr("⛔ Déconnecté de %1").arg(ssid.isEmpty() ? tr("le réseau courant") : ssid), true);
    checkConnectionStatus();
    updateCredentialStateForCurrentSsid();
}
