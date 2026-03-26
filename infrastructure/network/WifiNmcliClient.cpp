#include "WifiNmcliClient.h"

#include <QProcess>

WifiNmcliClient::Result WifiNmcliClient::run(const QStringList &args, int timeoutMs) const
{
    QProcess process;
    process.start(QStringLiteral("nmcli"), args);
    if (!process.waitForStarted(3000)) {
        return {-1, QString(), QStringLiteral("Impossible de démarrer nmcli")};
    }

    process.waitForFinished(timeoutMs);

    Result result;
    result.exitCode = process.exitCode();
    result.out = QString::fromLocal8Bit(process.readAllStandardOutput());
    result.err = QString::fromLocal8Bit(process.readAllStandardError());
    return result;
}

QString WifiNmcliClient::detectWifiDevice()
{
    if (!m_wifiDevice.isEmpty()) {
        return m_wifiDevice;
    }

    const Result result = run({"-t", "-f", "DEVICE,TYPE,STATE", "device", "status"});
    if (!result.ok()) {
        return {};
    }

    const auto lines = result.out.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        const auto parts = line.split(':');
        if (parts.size() >= 3 && parts[1] == QStringLiteral("wifi")) {
            m_wifiDevice = parts[0].trimmed();
            break;
        }
    }

    return m_wifiDevice;
}

bool WifiNmcliClient::ensureWifiRadioOn()
{
    const Result status = run({"radio", "wifi"});
    if (!status.ok()) {
        return false;
    }

    const QString radioState = status.out.trimmed().toLower();
    if (radioState == QStringLiteral("enabled") || radioState == QStringLiteral("on")) {
        return true;
    }

    const Result enable = run({"radio", "wifi", "on"});
    return enable.ok();
}
