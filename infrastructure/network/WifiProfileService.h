#pragma once

#include <QString>

class WifiNmcliClient;

class WifiProfileService
{
public:
    explicit WifiProfileService(WifiNmcliClient &nmcliClient);

    void applyAutoConnectPolicy(const QString &connectionName, bool autoconnect);
    QString findConnectionNameForSsid(const QString &ssid) const;
    void persistPsk(const QString &connectionName, const QString &password);
    bool isPasswordSavedForConnection(const QString &connectionName) const;
    QString getSecurityForSsid(const QString &ssid, const QString &dev) const;
    QString activeWifiConnectionNameForDevice(const QString &dev) const;
    QString currentSsidFromScan(const QString &dev) const;

private:
    WifiNmcliClient &m_nmcli;
};
