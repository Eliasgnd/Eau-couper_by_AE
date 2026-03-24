#include "NavigationController.h"

#include "Custom.h"
#include "WifiTransferWidget.h"
#include "WifiConfigDialog.h"
#include "BluetoothReceiverDialog.h"
#include "TestGpio.h"
#include "DossierWidget.h"
#include "Dispositions.h"

#include <QWidget>

NavigationController::NavigationController(QObject *parent)
    : QObject(parent)
{
}

void NavigationController::showInventaire(QWidget *from, QWidget *inventaire)
{
    if (from)
        from->hide();
    if (inventaire)
        inventaire->showFullScreen();
}

Custom *NavigationController::openCustomEditor(QWidget *from, Language language)
{
    if (from)
        from->hide();

    Custom *customWindow = new Custom(language);
    customWindow->setAttribute(Qt::WA_DeleteOnClose);
    customWindow->showFullScreen();
    return customWindow;
}

WifiTransferWidget *NavigationController::openWifiTransfer(QWidget *from)
{
    if (from)
        from->hide();

    WifiTransferWidget *window = new WifiTransferWidget();
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->showFullScreen();
    return window;
}

WifiConfigDialog *NavigationController::openWifiSettings(QWidget *from)
{
    if (from)
        from->hide();

    auto *dialog = new WifiConfigDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    return dialog;
}

BluetoothReceiverDialog *NavigationController::openBluetoothReceiver(QWidget *from)
{
    if (from)
        from->hide();

    auto *dialog = new BluetoothReceiverDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    return dialog;
}

TestGpio *NavigationController::openTestGpio(QWidget *from)
{
    if (from)
        from->hide();

    auto *dialog = new TestGpio();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    return dialog;
}

DossierWidget *NavigationController::openDossier(QWidget *from, Language language)
{
    if (from)
        from->hide();

    auto *dialog = new DossierWidget(language);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    return dialog;
}

Dispositions *NavigationController::openDispositions(QWidget *from,
                                                     const QString &shapeName,
                                                     const QList<LayoutData> &layouts,
                                                     const QList<QPolygonF> &shapePolygons,
                                                     Language language,
                                                     bool isBaseShape,
                                                     ShapeModel::Type baseType)
{
    if (from)
        from->hide();

    auto *dialog = new Dispositions(shapeName, layouts, shapePolygons, language, isBaseShape, baseType);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    return dialog;
}
