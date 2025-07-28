#include "WifiConfigDialog.h"
#include "ui_WifiConfigDialog.h"
#include "MainWindow.h"

#include <QProcess>
#include <QMessageBox>
#include <QCloseEvent>
#include <QRegularExpression>
#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPalette>
#include <QColor>

WifiConfigDialog::WifiConfigDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WifiConfigDialog)
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
    connect(ui->hiddenSsidCheck,   &QCheckBox::toggled,   this, &WifiConfigDialog::onHiddenSsidToggled);
    connect(ui->diagnosticsButton, &QPushButton::clicked, this, &WifiConfigDialog::showDiagnostics);
    connect(ui->autoScanCheck,     &QCheckBox::toggled,   this, &WifiConfigDialog::onAutoScanToggled);

    connect(ui->backButton, &QPushButton::clicked, this, [this]{
        close();
        if (auto mw = MainWindow::getInstance()) mw->showFullScreen();
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

    // Timers
    _statusTimer = new QTimer(this);
    _statusTimer->setInterval(3000);
    connect(_statusTimer, &QTimer::timeout, this, &WifiConfigDialog::checkConnectionStatus);
    _statusTimer->start();

    _scanTimer = new QTimer(this);
    _scanTimer->setInterval(10000);
    connect(_scanTimer, &QTimer::timeout, this, &WifiConfigDialog::scanNetworks);
    if (ui->autoScanCheck) ui->autoScanCheck->setChecked(true); // auto-scan par défaut

    onHiddenSsidToggled(false);
    checkConnectionStatus();
    scanNetworks();
}

WifiConfigDialog::~WifiConfigDialog()
{
    delete ui;
}

void WifiConfigDialog::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    if (auto mw = MainWindow::getInstance())
        mw->showFullScreen();
}

// ================== NMCLI helpers ==================

WifiConfigDialog::NmResult WifiConfigDialog::runNmcli(const QStringList &args, int timeoutMs)
{
    QProcess p;
    p.start(QStringLiteral("nmcli"), args);
    if (!p.waitForStarted(3000)) {
        return {-1, QString(), tr("Impossible de démarrer nmcli")};
    }
    p.waitForFinished(timeoutMs);
    NmResult r;
    r.exitCode = p.exitCode();
    r.out = QString::fromLocal8Bit(p.readAllStandardOutput());
    r.err = QString::fromLocal8Bit(p.readAllStandardError());
    return r;
}

QString WifiConfigDialog::detectWifiDevice()
{
    if (!_wifiDev.isEmpty())
        return _wifiDev;

    NmResult r = runNmcli({ "-t", "-f", "DEVICE,TYPE,STATE", "device", "status" });
    if (!r.ok()) return QString();

    // format: DEVICE:TYPE:STATE
    const auto lines = r.out.split('\n', Qt::SkipEmptyParts);
    for (const auto &ln : lines) {
        const auto parts = ln.split(':');
        if (parts.size() >= 3 && parts[1] == "wifi") {
            _wifiDev = parts[0].trimmed();
            break;
        }
    }
    return _wifiDev;
}

bool WifiConfigDialog::ensureWifiRadioOn()
{
    NmResult r = runNmcli({ "radio", "wifi" });
    if (!r.ok()) return false;

    const QString st = r.out.trimmed().toLower();
    if (st == "enabled" || st == "on") return true;

    NmResult r2 = runNmcli({ "radio", "wifi", "on" });
    return r2.ok();
}

void WifiConfigDialog::populateNetworksFromScan(const QString &nmcliOutput)
{
    // nmcli -t -f SSID,SIGNAL,SECURITY dev wifi list
    if (ui->networkComboBox) ui->networkComboBox->clear();
    if (ui->networksTable)   ui->networksTable->setRowCount(0);

    QStringList seen; // éviter doublons SSID
    const auto lines = nmcliOutput.split('\n', Qt::SkipEmptyParts);
    for (const auto &lnRaw : lines) {
        const QString ln = lnRaw.trimmed();
        const QString ssid = extractSsid(ln);
        if (ssid.isEmpty() || seen.contains(ssid)) continue;
        seen << ssid;

        const int signal = parseSignalPercent(ln);
        const QString sec = parseSecurity(ln);
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
}

void WifiConfigDialog::setBusy(bool busy)
{
    _busy = busy;
    if (ui->connectButton)      ui->connectButton->setEnabled(!busy);
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

void WifiConfigDialog::applyAutoConnectPolicy(const QString &connectionName, bool autoconnect)
{
    if (connectionName.isEmpty()) return;
    // connection.autoconnect yes|no
    runNmcli({ "connection", "modify", connectionName,
               "connection.autoconnect", autoconnect ? "yes" : "no" });
}

// ================== Parsing utils ==================

QString WifiConfigDialog::parseCurrentSsid(const QString &devStatusOut, const QString &devName)
{
    // nmcli -t -f DEVICE,STATE,CONNECTION dev status
    // line: wlp2s0:connected:MonSSID
    const auto lines = devStatusOut.split('\n', Qt::SkipEmptyParts);
    for (const auto &ln : lines) {
        const auto parts = ln.split(':');
        if (parts.size() >= 3 && parts[0] == devName && parts[1] == "connected") {
            return parts[2].trimmed();
        }
    }
    return QString();
}

QString WifiConfigDialog::parseIpV4(const QString &deviceShowOut)
{
    // nmcli -t -f IP4.ADDRESS device show <dev>
    // returns e.g. "192.168.1.42/24"
    const auto lines = deviceShowOut.split('\n', Qt::SkipEmptyParts);
    for (const auto &ln : lines) {
        if (ln.startsWith("IP4.ADDRESS", Qt::CaseInsensitive)) {
            const int idx = ln.indexOf(':');
            if (idx >= 0) return ln.mid(idx+1).trimmed();
        }
    }
    return QString();
}

int WifiConfigDialog::parseSignalPercent(const QString &nmcliScanLine)
{
    // format: SSID:SIGNAL:SECURITY
    const auto parts = nmcliScanLine.split(':');
    if (parts.size() >= 2) {
        bool ok = false;
        int v = parts[1].toInt(&ok);
        return ok ? qBound(0, v, 100) : 0;
    }
    return 0;
}

QString WifiConfigDialog::parseSecurity(const QString &nmcliScanLine)
{
    const auto parts = nmcliScanLine.split(':');
    if (parts.size() >= 3) return parts[2].trimmed();
    return QString();
}

QString WifiConfigDialog::extractSsid(const QString &nmcliScanLine)
{
    // ATTENTION : SSID peut contenir des ":" => nmcli tronque/échappe rarement l'SSID.
    // nmcli -t garantit le séparateur ":", mais l'SSID vide -> réseau caché.
    const int firstColon = nmcliScanLine.indexOf(':');
    if (firstColon <= 0) return nmcliScanLine.trimmed(); // cas dégradé (ou SSID vide)
    return nmcliScanLine.left(firstColon).trimmed();
}

// ================== Slots ==================

void WifiConfigDialog::scanNetworks()
{
    if (!ensureWifiRadioOn()) {
        updateStatusLabel(tr("Wi‑Fi désactivé — tentative d’activation impossible."), false);
        return;
    }

    const QString dev = detectWifiDevice();
    if (dev.isEmpty()) {
        updateStatusLabel(tr("Aucune interface Wi‑Fi détectée."), false);
        return;
    }

    setBusy(true);
    // Liste SSID:SIGNAL:SECURITY
    NmResult r = runNmcli({ "-t", "-f", "SSID,SIGNAL,SECURITY", "device", "wifi", "list", "ifname", dev });
    setBusy(false);

    if (!r.ok()) {
        updateStatusLabel(tr("Échec du scan Wi‑Fi: %1").arg(r.err.trimmed().isEmpty() ? r.out.trimmed() : r.err.trimmed()), false);
        return;
    }

    populateNetworksFromScan(r.out);
}

void WifiConfigDialog::connectSelected()
{
    if (!ensureWifiRadioOn()) {
        updateStatusLabel(tr("Wi‑Fi désactivé — impossible de se connecter."), false);
        return;
    }

    const QString dev = detectWifiDevice();
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
        args << "hidden" << "yes"; // ✅ correction: deux arguments séparés
    }

    // UX
    setBusy(true);
    if (ui->statusLabel) ui->statusLabel->setText(tr("Connexion à %1…").arg(ssid));

    NmResult r = runNmcli(args, 20000);
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
        updateStatusLabel(tr("❌ Échec de la connexion à %1").arg(ssid), false);
        return;
    }

    // Option autoconnect
    const bool autoconnect = (ui->autoConnectCheck && ui->autoConnectCheck->isChecked());
    applyAutoConnectPolicy(ssid, autoconnect);

    updateStatusLabel(tr("✅ Connecté à %1").arg(ssid), true);
    if (ui->passwordEdit) ui->passwordEdit->clear();
    checkConnectionStatus();
}

void WifiConfigDialog::checkConnectionStatus()
{
    const QString dev = detectWifiDevice();
    if (dev.isEmpty()) {
        if (ui->currentNetworkValue) ui->currentNetworkValue->setText("-");
        if (ui->ipValue)             ui->ipValue->setText("-");
        if (ui->signalValue)         ui->signalValue->setText("-");
        return;
    }

    NmResult st = runNmcli({ "-t", "-f", "DEVICE,STATE,CONNECTION", "device", "status" });
    NmResult ip = runNmcli({ "-t", "-f", "IP4.ADDRESS", "device", "show", dev });

    const QString ssid = parseCurrentSsid(st.out, dev);
    const QString ip4 = parseIpV4(ip.out);

    if (ui->currentNetworkValue) ui->currentNetworkValue->setText(ssid.isEmpty() ? tr("(non connecté)") : ssid);
    if (ui->ipValue)             ui->ipValue->setText(ip4.isEmpty() ? "-" : ip4);

    // Pour la force du signal, on rescane juste la ligne correspondante si connecté
    int signal = -1;
    if (!ssid.isEmpty()) {
        NmResult scan = runNmcli({ "-t", "-f", "SSID,SIGNAL,SECURITY", "device", "wifi", "list", "ifname", dev });
        if (scan.ok()) {
            const auto lines = scan.out.split('\n', Qt::SkipEmptyParts);
            for (const auto &ln : lines) {
                if (extractSsid(ln) == ssid) {
                    signal = parseSignalPercent(ln);
                    break;
                }
            }
        }
    }
    if (ui->signalValue) ui->signalValue->setText(signal >= 0 ? QString::number(signal) + "%" : "-");

    // Option: refléter le réseau courant dans la table (sélection)
    if (ui->networksTable && !ssid.isEmpty()) {
        for (int r = 0; r < ui->networksTable->rowCount(); ++r) {
            auto *itm = ui->networksTable->item(r, 0);
            if (itm && itm->text() == ssid) {
                ui->networksTable->setCurrentCell(r, 0);
                if (ui->selectedSsidLine) ui->selectedSsidLine->setText(ssid);
                break;
            }
        }
    }
}

void WifiConfigDialog::onHiddenSsidToggled(bool on)
{
    if (ui->ssidLineEdit)      ui->ssidLineEdit->setVisible(on);
    // On garde la table visible; on laisse selectedSsidLine visible aussi (lecture seule)
}

void WifiConfigDialog::showDiagnostics()
{
    const QString dev = detectWifiDevice();

    QString diag;
    diag += "=== nmcli device status ===\n";
    diag += runNmcli({ "device", "status" }).out + "\n";
    diag += "=== nmcli connection show --active ===\n";
    diag += runNmcli({ "connection", "show", "--active" }).out + "\n";
    if (!dev.isEmpty()) {
        diag += QString("=== nmcli device show %1 ===\n").arg(dev);
        diag += runNmcli({ "device", "show", dev }).out + "\n";
        diag += QString("=== nmcli -t -f SSID,SIGNAL,SECURITY device wifi list ifname %1 ===\n").arg(dev);
        diag += runNmcli({ "-t", "-f", "SSID,SIGNAL,SECURITY", "device", "wifi", "list", "ifname", dev }).out + "\n";
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
