#include "NavigationController.h"

#include "CustomEditor.h"
#include "WifiTransferWidget.h"
#include "WifiConfigDialog.h"
#include "BluetoothReceiverDialog.h"
#include "TestGpio.h"
#include "FolderWidget.h"
#include "LayoutsDialog.h"

#include <QWidget>

NavigationController::NavigationController(QObject *parent)
    : QObject(parent)
{
}

void NavigationController::showInventory(QWidget *from, QWidget *inventory)
{
    if (from)
        from->hide();
    if (inventory)
        inventory->showFullScreen();
}

CustomEditor *NavigationController::openCustomEditor(QWidget *from, Language language)
{
    if (from)
        from->hide();

    CustomEditor *customWindow = new CustomEditor(language);
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

FolderWidget *NavigationController::openFolder(QWidget *from, Language language)
{
    if (from)
        from->hide();

    auto *dialog = new FolderWidget(language);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    return dialog;
}

LayoutsDialog *NavigationController::openLayoutsDialog(QWidget *from,
                                                     const QString &shapeName,
                                                     const QList<LayoutData> &layouts,
                                                     const QList<QPolygonF> &shapePolygons,
                                                     Language language,
                                                     bool isBaseShape,
                                                     ShapeModel::Type baseType)
{
    if (from)
        from->hide();

    auto *dialog = new LayoutsDialog(shapeName, layouts, shapePolygons, language, isBaseShape, baseType);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->showFullScreen();
    return dialog;
}
