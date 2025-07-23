#include <QApplication>
#include "MainWindow.h"
#include "keyboardeventfilter.h"
#include <iostream>

int main(int argc, char *argv[])
{
    // Initialisation Qt
    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);
    QApplication app(argc, argv);

    // Filtrage des événements clavier
    KeyboardEventFilter filter(&app);
    app.installEventFilter(&filter);

    // Création de la fenêtre principale
    MainWindow *window = MainWindow::getInstance();
    window->showFullScreen();
    filter.setFormeVisualization(window->getFormeVisualization());

    return app.exec();
}
