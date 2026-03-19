#include <QApplication>
#include "MainWindow.h"
#include "AppController.h"
#include "keyboardeventfilter.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);
    QApplication app(argc, argv);

    KeyboardEventFilter filter(&app);
    app.installEventFilter(&filter);

    AppController controller;
    MainWindow w;

    controller.setMainWindow(&w);
    controller.setFormeVisualization(w.getFormeVisualization());

    QObject::connect(&w, &MainWindow::requestStartCut,
                     &controller, &AppController::startCutting);
    QObject::connect(&w, &MainWindow::requestPauseCut,
                     &controller, &AppController::pauseCutting);
    QObject::connect(&w, &MainWindow::requestStopCut,
                     &controller, &AppController::stopCutting);
    QObject::connect(&w, &MainWindow::requestAiGeneration,
                     &controller, &AppController::requestAiGeneration);
    QObject::connect(&w, &MainWindow::requestLanguageChange,
                     &controller, &AppController::changeLanguage);
    QObject::connect(&w, &MainWindow::requestWifiConfig,
                     &controller, &AppController::openWifiSettings);
    QObject::connect(&w, &MainWindow::requestBluetoothReceiver,
                     &controller, &AppController::openBluetoothReceiver);
    QObject::connect(&w, &MainWindow::requestOpenTestGpio,
                     &controller, &AppController::openTestGpio);
    QObject::connect(&w, &MainWindow::requestOpenDossier,
                     &controller, &AppController::openDossier);

    QObject::connect(&controller, &AppController::cutProgressUpdated,
                     &w, &MainWindow::updateProgressBar);
    QObject::connect(&controller, &AppController::cutFinished,
                     &w, &MainWindow::onCutFinished);
    QObject::connect(&controller, &AppController::cutControlsEnabled,
                     &w, &MainWindow::setSpinboxSliderEnabled);
    QObject::connect(&controller, &AppController::aiGenerationStatus,
                     &w, &MainWindow::onAiGenerationStatus);
    QObject::connect(&controller, &AppController::aiImageReady,
                     &w, &MainWindow::onAiImageReady);
    QObject::connect(&controller, &AppController::languageApplied,
                     &w, &MainWindow::onLanguageApplied);

    w.showFullScreen();
    filter.setFormeVisualization(w.getFormeVisualization());

    return app.exec();
}
