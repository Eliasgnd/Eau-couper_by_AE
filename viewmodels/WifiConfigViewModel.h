#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "WifiNmcliClient.h"
#include "WifiProfileService.h"

class WifiConfigViewModel : public QObject
{
    Q_OBJECT

public:
    struct ScannedNetwork {
        QString ssid;
        int     signalPercent = 0;
        QString security;
    };

    struct ConnectionStatus {
        QString currentSsid;
        QString ipAddress;
        int     signalPercent = -1;
    };

    // Result of a connect/disconnect/forget action: empty errorMsg = success
    struct ActionResult {
        bool    ok       = false;
        QString errorMsg;
    };

    explicit WifiConfigViewModel(QObject *parent = nullptr);

    // --- Device ---
    QString detectWifiDevice();
    bool    ensureWifiRadioOn();

    // --- Scan ---
    // Returns parsed list; fills errorMsg on failure
    QList<ScannedNetwork> scanNetworks(QString &errorMsg);

    // --- Connection status ---
    ConnectionStatus getConnectionStatus();

    // --- Connect ---
    ActionResult connectToNetwork(const QString &ssid,
                                  const QString &password,
                                  bool           hidden);
    void persistConnectionSettings(const QString &ssid,
                                   const QString &password,
                                   bool           autoconnect);

    // --- Disconnect ---
    ActionResult disconnectFromNetwork(const QString &ssid);

    // --- Forget ---
    ActionResult forgetNetwork(const QString &connName);

    // --- Per-SSID queries ---
    QString currentConnectedSsid();
    QString getSecurityForSsid(const QString &ssid);
    QString findConnectionNameForSsid(const QString &ssid);
    QString activeWifiConnectionName();
    bool    isPasswordSavedForConnection(const QString &connName);

    // --- Diagnostics ---
    QString getDiagnosticsInfo();

    // --- Policy ---
    void applyAutoConnectPolicy(const QString &connName, bool autoconnect);

private:
    WifiNmcliClient    m_nmcli;
    WifiProfileService m_wifiProfileService;
};
