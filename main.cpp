#include <QApplication>
#include "ThemeManager.h"
#include "MainWindow.h"
#include "WorkspaceViewModel.h"
#include "KeyboardEventFilter.h"
#include "AppFactory.h"
#include "Inventory.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);
    QApplication app(argc, argv);

    // Applique le thème global dès le démarrage via ThemeManager
    ThemeManager::instance()->applyToApp();

    KeyboardEventFilter filter(&app);
    app.installEventFilter(&filter);

    // La pile Inventory est construite par AppFactory et injectée dans MainWindow.
    // DialogManager, AIDialogCoordinator et ShapeCoordinator sont toujours créés
    // à l'intérieur de MainWindow car ShapeCoordinator dépend de ShapeVisualization
    // (widget instancié lors de ui->setupUi).
    // Toutes les connexions View ↔ Coordinator sont établies dans connectToView().
    WorkspaceViewModel model;
    Inventory *inventory = AppFactory::createInventory();
    MainWindow w(nullptr, nullptr, &model, inventory);

    filter.setShapeVisualization(w.getShapeVisualization());

    w.showFullScreen();
    return app.exec();
}
