#pragma once

#include <QWidget>
#include <QTimer>
#include <QString>

namespace Ui { class WifiConfigDialog; }

class WifiConfigDialog : public QWidget
{
    Q_OBJECT
public:
    explicit WifiConfigDialog(QWidget *parent = nullptr);
    ~WifiConfigDialog() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void scanNetworks();                 // Scan et remplit la table + combo (compat)
    void connectSelected();              // Connexion au réseau choisi
    void checkConnectionStatus();        // Rafraîchit l'état (réseau courant, IP, signal)
    void onHiddenSsidToggled(bool on);   // Gère l'UI SSID caché
    void showDiagnostics();              // Affiche & copie les infos de diagnostic
    void onAutoScanToggled(bool on);     // Active/désactive l’auto-scan

private:
    struct NmResult {
        int exitCode = -1;
        QString out;
        QString err;
        bool ok() const { return exitCode == 0; }
    };

    // Helpers NMCLI
    NmResult runNmcli(const QStringList &args, int timeoutMs = 12000);
    QString detectWifiDevice();                  // Trouve l’interface Wi‑Fi (ex: wlan0, wlp2s0…)
    bool ensureWifiRadioOn();                    // Active la radio si OFF
    void populateNetworksFromScan(const QString &nmcliOutput);
    void setBusy(bool busy);                     // Spinner + disable UI
    void updateStatusLabel(const QString &msg, bool ok);
    void applyAutoConnectPolicy(const QString &connectionName, bool autoconnect);

    // Parsing utilitaires
    static QString parseCurrentSsid(const QString &devStatusOut, const QString &devName);
    static QString parseIpV4(const QString &deviceShowOut);
    static int     parseSignalPercent(const QString &nmcliScanLine); // renvoie 0..100
    static QString parseSecurity(const QString &nmcliScanLine);
    static QString extractSsid(const QString &nmcliScanLine);

private:
    Ui::WifiConfigDialog *ui = nullptr;
    QTimer *_statusTimer = nullptr;
    QTimer *_scanTimer   = nullptr;
    QString _wifiDev;         // cache de l’interface Wi‑Fi
    bool _busy = false;
};
