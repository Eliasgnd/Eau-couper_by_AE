#ifndef NAVIGATIONCONTROLLER_H
#define NAVIGATIONCONTROLLER_H

#include <QObject>
#include <QList>
#include <QPolygonF>
#include <QString>
#include "Language.h"
#include "inventaire.h"
#include "ShapeModel.h"

class QWidget;
class custom;
class WifiTransferWidget;
class WifiConfigDialog;
class BluetoothReceiverDialog;
class TestGpio;
class DossierWidget;
class Dispositions;

class NavigationController : public QObject
{
    Q_OBJECT
public:
    explicit NavigationController(QObject *parent = nullptr);

    void showInventaire(QWidget *from, QWidget *inventaire);
    custom *openCustomEditor(QWidget *from, Language language);
    WifiTransferWidget *openWifiTransfer(QWidget *from);
    WifiConfigDialog *openWifiSettings(QWidget *from);
    BluetoothReceiverDialog *openBluetoothReceiver(QWidget *from);
    TestGpio *openTestGpio(QWidget *from);
    DossierWidget *openDossier(QWidget *from, Language language);
    Dispositions *openDispositions(QWidget *from,
                                   const QString &shapeName,
                                   const QList<LayoutData> &layouts,
                                   const QList<QPolygonF> &shapePolygons,
                                   Language language,
                                   bool isBaseShape = false,
                                   ShapeModel::Type baseType = ShapeModel::Type::Circle);
};

#endif // NAVIGATIONCONTROLLER_H
