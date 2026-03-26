#pragma once

#include <QString>

namespace WifiNmcliParsers {
QString parseIpV4(const QString &deviceShowOut);
int parseSignalPercent(const QString &nmcliScanLine);
QString parseSecurity(const QString &nmcliScanLine);
QString extractSsid(const QString &nmcliScanLine);
}
