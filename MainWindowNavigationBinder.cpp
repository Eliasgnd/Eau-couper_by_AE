#include "MainWindowNavigationBinder.h"

#include "ui_mainwindow.h"
#include "MainWindowCoordinator.h"
#include "Inventory.h"

#include <QPushButton>

namespace MainWindowNavigationBinder {

void bindNavigationButtons(Ui::MainWindow *ui,
                           MainWindowCoordinator *coordinator,
                           QWidget *mainWindow,
                           const std::function<Language()> &languageProvider)
{
    QObject::connect(ui->buttonInventory, &QPushButton::clicked, mainWindow,
                     [coordinator, mainWindow]() {
                         coordinator->openInventory(mainWindow, Inventory::getInstance());
                     });

    QObject::connect(ui->buttonCustom, &QPushButton::clicked, mainWindow,
                     [coordinator, mainWindow, languageProvider]() {
                         coordinator->openCustomEditor(mainWindow, languageProvider());
                     });

    QObject::connect(ui->buttonTestGpio, &QPushButton::clicked, mainWindow,
                     [coordinator, mainWindow, languageProvider]() {
                         coordinator->openFolder(mainWindow, languageProvider());
                     });

    QObject::connect(ui->buttonViewGeneratedImages, &QPushButton::clicked, mainWindow,
                     [coordinator, mainWindow]() {
                         coordinator->openTestGpio(mainWindow);
                     });

    QObject::connect(ui->buttonFileReceiver, &QPushButton::clicked, mainWindow,
                     [coordinator, mainWindow]() {
                         coordinator->openBluetoothReceiver(mainWindow);
                     });

    QObject::connect(ui->buttonWifiTransfer, &QPushButton::clicked, mainWindow,
                     [coordinator, mainWindow]() {
                         coordinator->openWifiTransfer(mainWindow);
                     });
}

} // namespace MainWindowNavigationBinder
