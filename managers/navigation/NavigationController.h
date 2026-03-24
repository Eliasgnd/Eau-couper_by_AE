#ifndef NAVIGATIONCONTROLLER_H
#define NAVIGATIONCONTROLLER_H

#include <QObject>
#include <QList>
#include <QPolygonF>
#include <QString>
#include "Language.h"
#include "Inventory.h"
#include "ShapeModel.h"

class QWidget;
class CustomEditor;
class WifiTransferWidget;
class WifiConfigDialog;
class BluetoothReceiverDialog;
class TestGpio;
class FolderWidget;
class LayoutsDialog;

class NavigationController : public QObject
{
    Q_OBJECT
public:
    explicit NavigationController(QObject *parent = nullptr);

    void showInventory(QWidget *from, QWidget *inventory);
    CustomEditor *openCustomEditor(QWidget *from, Language language);
    WifiTransferWidget *openWifiTransfer(QWidget *from);
    WifiConfigDialog *openWifiSettings(QWidget *from);
    BluetoothReceiverDialog *openBluetoothReceiver(QWidget *from);
    TestGpio *openTestGpio(QWidget *from);
    FolderWidget *openFolder(QWidget *from, Language language);
    LayoutsDialog *openLayoutsDialog(QWidget *from,
                                   const QString &shapeName,
                                   const QList<LayoutData> &layouts,
                                   const QList<QPolygonF> &shapePolygons,
                                   Language language,
                                   bool isBaseShape = false,
                                   ShapeModel::Type baseType = ShapeModel::Type::Circle);
};

#endif // NAVIGATIONCONTROLLER_H
