#pragma once

#include <QWidget>
#include <QTimer>
#include <QString>
#include <QCloseEvent>

#include "WifiNmcliClient.h"
#include "WifiProfileService.h"

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

    // État des identifiants / actions
    void updateCredentialStateForCurrentSsid();  // Met à jour l'UI selon SSID/sécurité/secret
    void forgetCurrentNetwork();                 // Supprime le profil NM (« oublier »)
    void disconnectFromSelected();               // Déconnecte du réseau sélectionné

private:
    // Helpers NMCLI/UI
    void populateNetworksFromScan(const QString &nmcliOutput);
    void setBusy(bool busy);                     // Spinner + disable UI
    void updateStatusLabel(const QString &msg, bool ok);
private:
    Ui::WifiConfigDialog *ui = nullptr;
    QTimer *_statusTimer = nullptr;
    QTimer *_scanTimer   = nullptr;
    bool _busy = false;

    WifiNmcliClient m_nmcli;
    WifiProfileService m_wifiProfileService;
};
