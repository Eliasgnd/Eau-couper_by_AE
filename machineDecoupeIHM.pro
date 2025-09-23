QT += core gui widgets svg network bluetooth openglwidgets
DEFINES += DISABLE_GPIO

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET  = machineDecoupeIHM
!contains(DEFINES, DISABLE_GPIO) {
    SOURCES += raspberry.cpp
}

# ==== PLATEFORME macOS (M1/M2) ====
macx {
    INCLUDEPATH += /opt/homebrew/include/opencv4

INCLUDEPATH += /opt/homebrew/include/opencv4
LIBS += -L/opt/homebrew/lib \
    -lopencv_core \
    -lopencv_imgproc \
    -lopencv_highgui \
    -lopencv_imgcodecs \
    -lopencv_videoio \
    -lopencv_objdetect \
    -lopencv_photo \
    -lopencv_xphoto \
    -lopencv_ximgproc \
    -lopencv_features2d \
    -lopencv_flann \
    -lopencv_calib3d \
    -lopencv_dnn \
    -lopencv_ml \
    -lopencv_stitching \
    -lopencv_video
}

# ==== PLATEFORME LINUX / RASPBERRY PI ====
unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4 libgpiod Qt6HttpServer
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
    DossierWidget.h \
    ScreenUtils.h \
    AspectRatioWrapper.h \
    clavier.h claviernumerique.h custom.h inventaire.h Dispositions.h \
    keyboardeventfilter.h motorcontrol.h pathplanner.h \
    raspberry.h TestGpio.h \
    skeletonizer.h \
    touchgesturereader.h \
    trajetmotor.h \
    Language.h \
    AIImagePromptDialog.h\
    DossierWidget.h \
    AIImageProcessDialog.h\
    BluetoothReceiverDialog.h\
    WifiTransferWidget.h \
    qrcodegen.hpp \
    ImagePaths.h \
    WifiConfigDialog.h

SOURCES += \
    MainWindow.cpp FormeVisualization.cpp main.cpp \
    CustomDrawArea.cpp LogoImporter.cpp ImageEdgeImporter.cpp ShapeModel.cpp \
    DossierWidget.cpp \
    AspectRatioWrapper.cpp \
    clavier.cpp claviernumerique.cpp custom.cpp \
    inventaire.cpp Dispositions.cpp keyboardeventfilter.cpp motorcontrol.cpp \
    pathplanner.cpp trajetmotor.cpp \
    raspberry.cpp TestGpio.cpp \
    skeletonizer.cpp \
    touchgesturereader.cpp \
    AIImagePromptDialog.cpp \
    AIImageProcessDialog.cpp \
    BluetoothReceiverDialog.cpp \
    WifiTransferWidget.cpp \
    WifiConfigDialog.cpp \
    qrcodegen.cpp

FORMS += \
    BluetoothReceiverDialog.ui \
    mainwindow.ui custom.ui inventaire.ui Dispositions.ui DossierWidget.ui TestGpio.ui \
    WifiTransferWidget.ui \
    WifiConfigDialog.ui

RESOURCES += resources.qrc

TRANSLATIONS += \
    translations/machineDecoupeIHM_fr.ts \
    translations/machineDecoupeIHM_en.ts

!isEmpty(target.path): INSTALLS += target
