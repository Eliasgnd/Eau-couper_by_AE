#include <QApplication>
#include "MainWindow.h"
#include "WorkspaceViewModel.h"
#include "KeyboardEventFilter.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);
    QApplication app(argc, argv);

    KeyboardEventFilter filter(&app);
    app.installEventFilter(&filter);

    // WorkspaceViewModel est le seul objet injectable avant la création de MainWindow.
    // DialogManager, AIDialogCoordinator et ShapeCoordinator sont créés
    // à l'intérieur de MainWindow car ShapeCoordinator dépend de ShapeVisualization
    // (widget instancié lors de ui->setupUi).
    // Toutes les connexions View ↔ Coordinator sont établies dans connectToView().
    WorkspaceViewModel model;
    MainWindow w(nullptr, nullptr, &model);

    filter.setShapeVisualization(w.getShapeVisualization());

    w.showFullScreen();
    return app.exec();
}
