QT += core gui widgets svg network bluetooth
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET  = machineDecoupeIHM

# ==== PLATEFORME LINUX / RASPBERRY PI ====
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
}

# ==== PLATEFORME WINDOWS ====
win32 {
    INCLUDEPATH += C:/opencv/build/include
                   C:/opencv/build/include/opencv2

    LIBS += -LC:/opencv/build/x64/vc16/lib

    CONFIG(debug, debug|release) {
        LIBS += -lopencv_world4120d
    } else {
        LIBS += -lopencv_world4120
    }
}

# ==== FICHIERS ====

HEADERS += \
    MainWindow.h FormeVisualization.h \
    CustomDrawArea.h LogoImporter.h ImageEdgeImporter.h ShapeModel.h \
    ScreenUtils.h \
    clavier.h claviernumerique.h custom.h inventaire.h Dispositions.h \
    keyboardeventfilter.h motorcontrol.h pathplanner.h \
    skeletonizer.h \
    touchgesturereader.h \
    trajetmotor.h \
    Language.h

SOURCES += \
    MainWindow.cpp FormeVisualization.cpp main.cpp \
    CustomDrawArea.cpp LogoImporter.cpp ImageEdgeImporter.cpp ShapeModel.cpp \
    clavier.cpp claviernumerique.cpp custom.cpp \
    inventaire.cpp Dispositions.cpp keyboardeventfilter.cpp motorcontrol.cpp \
    pathplanner.cpp trajetmotor.cpp \
    skeletonizer.cpp \
    touchgesturereader.cpp

FORMS += \
    mainwindow.ui custom.ui inventaire.ui Dispositions.ui

RESOURCES += resources.qrc

TRANSLATIONS += \
    translations/machineDecoupeIHM_fr.ts \
    translations/machineDecoupeIHM_en.ts

!isEmpty(target.path): INSTALLS += target
