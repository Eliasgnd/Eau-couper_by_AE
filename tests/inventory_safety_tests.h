#pragma once
#include <QObject>

class InventorySafetyTests : public QObject {
    Q_OBJECT
private slots:
    void malformedShapeProxied();
    void invalidShapeRejected();
    void hugePolygonHandled();
};
