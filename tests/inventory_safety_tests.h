#pragma once
#include <QObject>

class InventorySafetyTests : public QObject {
    Q_OBJECT
private slots:
    void malformedShapeAccepted();
    void invalidShapeAccepted();
    void hugePolygonHandled();
};
