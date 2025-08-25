#pragma once
#include <QObject>

class InventorySafetyTests : public QObject {
    Q_OBJECT
private slots:
    void malformedShapeUsesProxy();
    void corruptedFileHandled();
    void hugeInventoryHandled();
};
