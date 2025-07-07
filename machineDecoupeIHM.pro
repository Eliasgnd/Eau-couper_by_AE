QT += core gui widgets svg network bluetooth
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET  = machineDecoupeIHM
CONFIG += c++17


HEADERS += \
    MainWindow.h FormeVisualization.h \
    CustomDrawArea.h LogoImporter.h ShapeModel.h \
    ScreenUtils.h \
    clavier.h claviernumerique.h custom.h inventaire.h \
    keyboardeventfilter.h motorcontrol.h pathplanner.h \
    touchgesturereader.h \
    trajetmotor.h \
    Language.h

SOURCES += \
    MainWindow.cpp FormeVisualization.cpp main.cpp \
    CustomDrawArea.cpp LogoImporter.cpp ShapeModel.cpp \
    clavier.cpp claviernumerique.cpp custom.cpp \
    inventaire.cpp keyboardeventfilter.cpp motorcontrol.cpp \
    pathplanner.cpp trajetmotor.cpp \
    touchgesturereader.cpp

FORMS += \
    mainwindow.ui custom.ui inventaire.ui

# Qt Resource Collection
RESOURCES += resources.qrc

# Installation (déploiement)
!isEmpty(target.path): INSTALLS += target
