#include <QApplication>
#include "MainWindow.h"
#include "keyboardeventfilter.h"

int main(int argc, char *argv[]) //gros caca boudin prout
{
    /*  On ne veut PAS désactiver la conversion touch → mouse,
        sinon le dessin au doigt ne marche plus.        */
    // QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);
    // QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    /* Optionnel : si vraiment tu tiens à désactiver l’autre sens (mouse → touch) : */
    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

    /*  IMPORTANT : on Laisse Qt synthétiser des événements souris
        pour les touchs non consommés (valeur par défaut = true). */
    // QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);

    QApplication app(argc, argv);

    KeyboardEventFilter filter(&app);
    app.installEventFilter(&filter);

    MainWindow *window = MainWindow::getInstance();
    window->showFullScreen();
    return app.exec();
}
