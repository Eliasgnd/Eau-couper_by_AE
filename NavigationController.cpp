#include "NavigationController.h"

#include "custom.h"
#include "WifiTransferWidget.h"

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

custom *NavigationController::openCustomEditor(QWidget *from, Language language)
{
    if (from)
        from->hide();

    custom *customWindow = new custom(language);
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
