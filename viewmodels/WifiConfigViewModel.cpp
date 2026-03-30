#include "WifiConfigViewModel.h"
#include "WifiNmcliParsers.h"

#include <QSettings>

WifiConfigViewModel::WifiConfigViewModel(QObject *parent)
    : QObject(parent)
    , m_wifiProfileService(m_nmcli)
{}

// --- Device ---

QString WifiConfigViewModel::detectWifiDevice()
{
    return m_nmcli.detectWifiDevice();
}

bool WifiConfigViewModel::ensureWifiRadioOn()
{
    return m_nmcli.ensureWifiRadioOn();
}

// --- Scan ---

QList<WifiConfigViewModel::ScannedNetwork> WifiConfigViewModel::scanNetworks(QString &errorMsg)
{
    errorMsg.clear();

    if (!m_nmcli.ensureWifiRadioOn()) {
        errorMsg = tr("Wi\u2011Fi désactivé — tentative d'activation impossible.");
        return {};
    }

    const QString dev = m_nmcli.detectWifiDevice();
    if (dev.isEmpty()) {
        errorMsg = tr("Aucune interface Wi\u2011Fi détectée.");
        return {};
    }

    WifiNmcliClient::Result r = m_nmcli.run(
        { "-t", "-f", "SSID,SIGNAL,SECURITY", "device", "wifi", "list", "ifname", dev });

    if (!r.ok()) {
        const QString raw = r.err.trimmed().isEmpty() ? r.out.trimmed() : r.err.trimmed();
        errorMsg = tr("Échec du scan Wi\u2011Fi: %1").arg(raw);
        return {};
    }

    QList<ScannedNetwork> result;
    QStringList seen;
    const auto lines = r.out.split('\n', Qt::SkipEmptyParts);
    for (const auto &lnRaw : lines) {
        const QString ln   = lnRaw.trimmed();
        const QString ssid = WifiNmcliParsers::extractSsid(ln);
        if (ssid.isEmpty() || seen.contains(ssid))
            continue;
        seen << ssid;

        ScannedNetwork net;
        net.ssid          = ssid;
        net.signalPercent = WifiNmcliParsers::parseSignalPercent(ln);
        net.security      = WifiNmcliParsers::parseSecurity(ln);
        result << net;
    }
    return result;
}

// --- Connection status ---

WifiConfigViewModel::ConnectionStatus WifiConfigViewModel::getConnectionStatus()
{
    ConnectionStatus status;
    const QString dev = m_nmcli.detectWifiDevice();
    if (dev.isEmpty())
        return status;

    status.currentSsid = m_wifiProfileService.currentSsidFromScan(dev);

    WifiNmcliClient::Result ip = m_nmcli.run({ "-t", "-f", "IP4.ADDRESS", "device", "show", dev });
    status.ipAddress = WifiNmcliParsers::parseIpV4(ip.out);

    if (!status.currentSsid.isEmpty()) {
        WifiNmcliClient::Result scan = m_nmcli.run(
            { "-t", "-f", "SSID,SIGNAL,SECURITY,IN-USE", "device", "wifi", "list", "ifname", dev });
        if (scan.ok()) {
            for (const auto &ln : scan.out.split('\n', Qt::SkipEmptyParts)) {
                if (WifiNmcliParsers::extractSsid(ln) == status.currentSsid) {
                    status.signalPercent = WifiNmcliParsers::parseSignalPercent(ln);
                    break;
                }
            }
        }
    }
    return status;
}

// --- Connect ---

WifiConfigViewModel::ActionResult WifiConfigViewModel::connectToNetwork(
    const QString &ssid, const QString &password, bool hidden)
{
    if (!m_nmcli.ensureWifiRadioOn())
        return { false, tr("Wi\u2011Fi désactivé — impossible de se connecter.") };

    const QString dev = m_nmcli.detectWifiDevice();
    if (dev.isEmpty())
        return { false, tr("Aucune interface Wi\u2011Fi détectée.") };

    QStringList args = { "device", "wifi", "connect", ssid, "ifname", dev };
    if (!password.isEmpty())
        args << "password" << password;
    if (hidden)
        args << "hidden" << "yes";

    WifiNmcliClient::Result r = m_nmcli.run(args, 20000);
    if (!r.ok()) {
        const QString raw = r.err.trimmed().isEmpty() ? r.out.trimmed() : r.err.trimmed();
        QString msg = raw;
        if (raw.contains("No network with SSID", Qt::CaseInsensitive))
            msg = tr("Réseau introuvable. Vérifiez le SSID (ou cochez « SSID caché »).");
        else if (raw.contains("incorrect password", Qt::CaseInsensitive)
                 || raw.contains("failed to activate connection", Qt::CaseInsensitive))
            msg = tr("Mot de passe incorrect ou authentification refusée.");
        else if (raw.contains("Device not ready", Qt::CaseInsensitive))
            msg = tr("Interface Wi\u2011Fi indisponible (mode avion ? pilote ?).");
        else if (raw.contains("Timeout", Qt::CaseInsensitive))
            msg = tr("Délai dépassé. Réessayez en vous rapprochant du point d'accès.");
        return { false, msg };
    }

    return { true, {} };
}

void WifiConfigViewModel::persistConnectionSettings(
    const QString &ssid, const QString &password, bool autoconnect)
{
    QSettings s;
    s.setValue("wifi/last_ssid", ssid);

    const QString connName = m_wifiProfileService.findConnectionNameForSsid(ssid);
    if (!connName.isEmpty()) {
        m_wifiProfileService.applyAutoConnectPolicy(connName, autoconnect);
        if (!password.isEmpty())
            m_wifiProfileService.persistPsk(connName, password);
    }
}

// --- Disconnect ---

WifiConfigViewModel::ActionResult WifiConfigViewModel::disconnectFromNetwork(const QString &ssid)
{
    const QString dev = m_nmcli.detectWifiDevice();
    if (dev.isEmpty())
        return { false, tr("Aucune interface Wi\u2011Fi détectée.") };

    QString connName = m_wifiProfileService.findConnectionNameForSsid(ssid);
    if (connName.isEmpty())
        connName = m_wifiProfileService.activeWifiConnectionNameForDevice(dev);

    WifiNmcliClient::Result r = connName.isEmpty()
        ? m_nmcli.run({ "device", "disconnect", dev })
        : m_nmcli.run({ "connection", "down", connName });

    if (!r.ok()) {
        const QString raw = r.err.trimmed().isEmpty() ? r.out.trimmed() : r.err.trimmed();
        return { false, tr("Impossible de se déconnecter : %1").arg(raw) };
    }
    return { true, {} };
}

// --- Forget ---

WifiConfigViewModel::ActionResult WifiConfigViewModel::forgetNetwork(const QString &connName)
{
    WifiNmcliClient::Result del = m_nmcli.run({ "connection", "delete", connName });
    if (!del.ok()) {
        const QString raw = del.err.trimmed().isEmpty() ? del.out.trimmed() : del.err.trimmed();
        return { false, tr("Impossible de supprimer le profil : %1").arg(raw) };
    }
    return { true, {} };
}

// --- Per-SSID queries ---

QString WifiConfigViewModel::currentConnectedSsid()
{
    const QString dev = m_nmcli.detectWifiDevice();
    return dev.isEmpty() ? QString() : m_wifiProfileService.currentSsidFromScan(dev);
}

QString WifiConfigViewModel::getSecurityForSsid(const QString &ssid)
{
    const QString dev = m_nmcli.detectWifiDevice();
    return m_wifiProfileService.getSecurityForSsid(ssid, dev);
}

QString WifiConfigViewModel::findConnectionNameForSsid(const QString &ssid)
{
    return m_wifiProfileService.findConnectionNameForSsid(ssid);
}

QString WifiConfigViewModel::activeWifiConnectionName()
{
    const QString dev = m_nmcli.detectWifiDevice();
    return dev.isEmpty() ? QString() : m_wifiProfileService.activeWifiConnectionNameForDevice(dev);
}

bool WifiConfigViewModel::isPasswordSavedForConnection(const QString &connName)
{
    return m_wifiProfileService.isPasswordSavedForConnection(connName);
}

// --- Diagnostics ---

QString WifiConfigViewModel::getDiagnosticsInfo()
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
    return diag;
}

// --- Policy ---

void WifiConfigViewModel::applyAutoConnectPolicy(const QString &connName, bool autoconnect)
{
    m_wifiProfileService.applyAutoConnectPolicy(connName, autoconnect);
}
