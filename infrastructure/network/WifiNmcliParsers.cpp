#include "WifiNmcliParsers.h"

#include <QtGlobal>
#include <QStringList>

namespace WifiNmcliParsers {

QString parseIpV4(const QString &deviceShowOut)
{
    const auto lines = deviceShowOut.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        if (line.startsWith(QStringLiteral("IP4.ADDRESS"), Qt::CaseInsensitive)) {
            const int idx = line.indexOf(':');
            if (idx >= 0) {
                return line.mid(idx + 1).trimmed();
            }
        }
    }
    return {};
}

int parseSignalPercent(const QString &nmcliScanLine)
{
    const auto parts = nmcliScanLine.split(':');
    if (parts.size() >= 2) {
        bool ok = false;
        const int value = parts[1].toInt(&ok);
        return ok ? qBound(0, value, 100) : 0;
    }
    return 0;
}

QString parseSecurity(const QString &nmcliScanLine)
{
    const auto parts = nmcliScanLine.split(':');
    if (parts.size() >= 3) {
        return parts[2].trimmed();
    }
    return {};
}

QString extractSsid(const QString &nmcliScanLine)
{
    const int firstColon = nmcliScanLine.indexOf(':');
    if (firstColon <= 0) {
        return nmcliScanLine.trimmed();
    }
    return nmcliScanLine.left(firstColon).trimmed();
}

} // namespace WifiNmcliParsers
