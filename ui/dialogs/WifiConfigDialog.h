#pragma once

#include <QWidget>
#include <QTimer>
#include <QString>
#include <QCloseEvent>

#include "WifiConfigViewModel.h"

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
    void scanNetworks();
    void connectSelected();
    void checkConnectionStatus();
    void onHiddenSsidToggled(bool on);
    void showDiagnostics();
    void onAutoScanToggled(bool on);
    void updateCredentialStateForCurrentSsid();
    void forgetCurrentNetwork();
    void disconnectFromSelected();

private:
    void populateNetworksFromScan(const QList<WifiConfigViewModel::ScannedNetwork> &networks);
    void setBusy(bool busy);
    void updateStatusLabel(const QString &msg, bool ok);

    Ui::WifiConfigDialog  *ui           = nullptr;
    WifiConfigViewModel   *m_vm         = nullptr;
    QTimer                *_statusTimer = nullptr;
    QTimer                *_scanTimer   = nullptr;
    bool                   _busy        = false;
    bool                   m_isDarkTheme = false;
};
