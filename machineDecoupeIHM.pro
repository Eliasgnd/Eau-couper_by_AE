QT += core gui widgets svg network bluetooth
QT += httpserver
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET  = machineDecoupeIHM


# ==== PLATEFORME LINUX / RASPBERRY PI ====
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4 libgpiod
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
    PageImagesGenerees.h \
    ScreenUtils.h \
    clavier.h claviernumerique.h custom.h inventaire.h Dispositions.h \
    keyboardeventfilter.h motorcontrol.h pathplanner.h \
    raspberry.h TestGpio.h \
    skeletonizer.h \
    touchgesturereader.h \
    trajetmotor.h \
    Language.h \
    AIImagePromptDialog.h\
    PageImagesGenerees.h \
    AIImageProcessDialog.h\
    BluetoothReceiverDialog.h\
    WifiTransferWidget.h \
    WifiConfigDialog.h \
    qrcodegen.hpp

SOURCES += \
    MainWindow.cpp FormeVisualization.cpp main.cpp \
    CustomDrawArea.cpp LogoImporter.cpp ImageEdgeImporter.cpp ShapeModel.cpp \
    PageImagesGenerees.cpp \
    clavier.cpp claviernumerique.cpp custom.cpp \
    inventaire.cpp Dispositions.cpp keyboardeventfilter.cpp motorcontrol.cpp \
    pathplanner.cpp trajetmotor.cpp \
    raspberry.cpp TestGpio.cpp \
    skeletonizer.cpp \
    touchgesturereader.cpp \
    AIImagePromptDialog.cpp \
    AIImageProcessDialog.cpp \
    BluetoothReceiverDialog.cpp\
    WifiTransferWidget.cpp \
    WifiConfigDialog.cpp \
    qrcodegen.cpp

FORMS += \
    mainwindow.ui custom.ui inventaire.ui Dispositions.ui PageImagesGenerees.ui TestGpio.ui \
    WifiTransferWidget.ui
    WifiConfigDialog.ui

RESOURCES += ressources.qrc resources.qrc

TRANSLATIONS += \
    translations/machineDecoupeIHM_fr.ts \
    translations/machineDecoupeIHM_en.ts

!isEmpty(target.path): INSTALLS += target
