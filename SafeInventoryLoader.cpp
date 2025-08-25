#include "SafeInventoryLoader.h"
#include "GeometryUtils.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

bool loadInventoryFileSafe(const QString &path, QList<SafeShape> &out)
{
    out.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open inventory" << path;
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Invalid inventory JSON" << err.errorString();
        return false;
    }
    const QJsonArray shapesArr = doc.object().value("shapes").toArray();
    for (const QJsonValue &val : shapesArr) {
        if (!val.isObject())
            continue;
        const QJsonObject obj = val.toObject();
        SafeShape shape;
        shape.name = obj.value("name").toString();
        const QJsonArray polyArr = obj.value("polygons").toArray();
        for (const QJsonValue &polyVal : polyArr) {
            const QJsonArray ptArrList = polyVal.toArray();
            QPolygonF poly;
            for (const QJsonValue &ptVal : ptArrList) {
                const QJsonArray p = ptVal.toArray();
                if (p.size() >= 2)
                    poly.append(QPointF(p.at(0).toDouble(), p.at(1).toDouble()));
            }
            shape.polygons.append(poly);
        }
        QString warn;
        bool ok = false;
        try {
            ok = validateAndProxyPolygons(shape.polygons, safeModeEnabled(), &warn);
        } catch (...) {
            ok = false;
        }
        if (!ok) {
            qWarning() << "Skipping invalid shape" << shape.name;
            continue;
        }
        shape.proxyUsed = !warn.isEmpty();
        if (!warn.isEmpty())
            qWarning() << warn << shape.name;
        out.append(shape);
    }
    return true;
}
