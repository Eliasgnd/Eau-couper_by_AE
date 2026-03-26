#include "WifiProfileService.h"

#include "WifiNmcliClient.h"

WifiProfileService::WifiProfileService(WifiNmcliClient &nmcliClient)
    : m_nmcli(nmcliClient)
{
}

void WifiProfileService::applyAutoConnectPolicy(const QString &connectionName, bool autoconnect)
{
    if (connectionName.isEmpty()) {
        return;
    }

    m_nmcli.run({"connection", "modify", connectionName,
                 "connection.autoconnect", autoconnect ? "yes" : "no"});
}

QString WifiProfileService::findConnectionNameForSsid(const QString &ssid) const
{
    if (ssid.trimmed().isEmpty()) {
        return {};
    }

    const WifiNmcliClient::Result result = m_nmcli.run({"-t", "-f", "NAME,TYPE,802-11-wireless.ssid", "connection", "show"});
    if (!result.ok()) {
        return {};
    }

    const auto lines = result.out.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        const auto parts = line.split(':');
        if (parts.size() < 2) {
            continue;
        }

        const QString name = parts.value(0).trimmed();
        const QString type = parts.value(1).trimmed();
        const QString ssidValue = parts.size() >= 3 ? parts.mid(2).join(":").trimmed() : QString();

        if (type == QStringLiteral("wifi")) {
            if (!ssidValue.isEmpty() && ssidValue == ssid) {
                return name;
            }
            if (name == ssid) {
                return name;
            }
        }
    }

    return {};
}

void WifiProfileService::persistPsk(const QString &connectionName, const QString &password)
{
    if (connectionName.isEmpty() || password.isEmpty()) {
        return;
    }

    const WifiNmcliClient::Result security = m_nmcli.run({"-t", "-f", "802-11-wireless-security.key-mgmt",
                                                           "connection", "show", connectionName});
    if (!security.ok()) {
        return;
    }

    const QString keyMgmt = security.out.trimmed().toLower();
    const bool isPskLike = keyMgmt.contains("wpa-psk") || keyMgmt.contains("sae");
    if (!isPskLike) {
        return;
    }

    m_nmcli.run({"connection", "modify", connectionName,
                 "802-11-wireless-security.psk", password});
    m_nmcli.run({"connection", "modify", connectionName,
                 "802-11-wireless-security.psk-flags", "0"});
}

bool WifiProfileService::isPasswordSavedForConnection(const QString &connectionName) const
{
    if (connectionName.isEmpty()) {
        return false;
    }

    const WifiNmcliClient::Result result = m_nmcli.run({"-t", "-f",
                                                         "802-11-wireless-security.key-mgmt,"
                                                         "802-11-wireless-security.psk,"
                                                         "802-11-wireless-security.psk-flags",
                                                         "connection", "show", connectionName});
    if (!result.ok()) {
        return false;
    }

    bool hasPsk = false;
    int flags = -1;
    QString keyMgmt;

    const auto lines = result.out.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        if (line.startsWith("802-11-wireless-security.key-mgmt:", Qt::CaseInsensitive)) {
            keyMgmt = line.section(':', 1).trimmed();
        } else if (line.startsWith("802-11-wireless-security.psk:", Qt::CaseInsensitive)) {
            const QString password = line.section(':', 1);
            hasPsk = !password.trimmed().isEmpty();
        } else if (line.startsWith("802-11-wireless-security.psk-flags:", Qt::CaseInsensitive)) {
            bool ok = false;
            flags = line.section(':', 1).trimmed().toInt(&ok);
            if (!ok) {
                flags = -1;
            }
        }
    }

    if (keyMgmt.contains("wpa-psk", Qt::CaseInsensitive) || keyMgmt.contains("sae", Qt::CaseInsensitive)) {
        if (hasPsk || flags == 0) {
            return true;
        }
    }

    return false;
}

QString WifiProfileService::getSecurityForSsid(const QString &ssid, const QString &dev) const
{
    if (ssid.isEmpty() || dev.isEmpty()) {
        return {};
    }

    const WifiNmcliClient::Result scan = m_nmcli.run({"-t", "-f", "SSID,SECURITY", "device", "wifi", "list", "ifname", dev});
    if (!scan.ok()) {
        return {};
    }

    const auto lines = scan.out.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        const int separator = line.indexOf(':');
        if (separator <= 0) {
            continue;
        }

        const QString currentSsid = line.left(separator).trimmed();
        const QString security = line.mid(separator + 1).trimmed();
        if (currentSsid == ssid) {
            return security;
        }
    }

    return {};
}

QString WifiProfileService::activeWifiConnectionNameForDevice(const QString &dev) const
{
    if (dev.isEmpty()) {
        return {};
    }

    const WifiNmcliClient::Result result = m_nmcli.run({"-t", "-f", "NAME,TYPE,DEVICE", "connection", "show", "--active"});
    if (!result.ok()) {
        return {};
    }

    const auto lines = result.out.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        const auto parts = line.split(':');
        if (parts.size() >= 3) {
            const QString name = parts[0].trimmed();
            const QString type = parts[1].trimmed();
            const QString device = parts[2].trimmed();
            if (type == QStringLiteral("wifi") && device == dev) {
                return name;
            }
        }
    }

    return {};
}

QString WifiProfileService::currentSsidFromScan(const QString &dev) const
{
    if (dev.isEmpty()) {
        return {};
    }

    const WifiNmcliClient::Result result = m_nmcli.run({"-t", "-f", "IN-USE,SSID", "device", "wifi", "list", "ifname", dev});
    if (!result.ok()) {
        return {};
    }

    const auto lines = result.out.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        const int separator = line.indexOf(':');
        if (separator < 0) {
            continue;
        }

        const QString inUse = line.left(separator).trimmed();
        const QString ssid = line.mid(separator + 1).trimmed();
        if (inUse == "*" || inUse.compare("yes", Qt::CaseInsensitive) == 0 || inUse == "1") {
            return ssid;
        }
    }

    return {};
}
