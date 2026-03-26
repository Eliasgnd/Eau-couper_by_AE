#pragma once

#include <QString>
#include <QStringList>

class WifiNmcliClient
{
public:
    struct Result {
        int exitCode = -1;
        QString out;
        QString err;
        bool ok() const { return exitCode == 0; }
    };

    Result run(const QStringList &args, int timeoutMs = 12000) const;
    QString detectWifiDevice();
    bool ensureWifiRadioOn();

private:
    QString m_wifiDevice;
};
