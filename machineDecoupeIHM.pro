QMAKE_MSC_VER = 1929  # Visual Studio 2022 (19.29+)

QT += core gui widgets svg network bluetooth
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET  = machineDecoupeIHM

# Chemins vers les fichiers d'en-tête OpenCV
INCLUDEPATH += C:\Users\elias\Desktop\opencv\build\include
INCLUDEPATH += C:\Users\elias\Desktop\opencv\build\include\opencv2

# Chemin vers les bibliothèques OpenCV
LIBS += -LC:\Users\elias\Desktop\opencv\build\x64\vc16\lib

# Nom exact de la bibliothèque à lier (vérifie le numéro de version dans le dossier lib)
LIBS += -lopencv_world4120d  # pour Debug
# LIBS += -lopencv_world4120   # pour Release



CONFIG(debug, debug|release) {
    LIBS += -lopencv_world4120d
} else {
    LIBS += -lopencv_world4120
}

HEADERS += \
    MainWindow.h FormeVisualization.h \
    CustomDrawArea.h LogoImporter.h ImageEdgeImporter.h ShapeModel.h \
    ScreenUtils.h \
    clavier.h claviernumerique.h custom.h inventaire.h Dispositions.h \
    keyboardeventfilter.h motorcontrol.h pathplanner.h \
    skeletonizer.h \
    touchgesturereader.h \
    trajetmotor.h \
    Language.h \
    AIImagePromptDialog.h \
    PageImagesGenerées.h

SOURCES += \
    MainWindow.cpp FormeVisualization.cpp main.cpp \
    CustomDrawArea.cpp LogoImporter.cpp ImageEdgeImporter.cpp ShapeModel.cpp \
    clavier.cpp claviernumerique.cpp custom.cpp \
    inventaire.cpp Dispositions.cpp keyboardeventfilter.cpp motorcontrol.cpp \
    pathplanner.cpp trajetmotor.cpp \
    skeletonizer.cpp \
    touchgesturereader.cpp \
    AIImagePromptDialog.cpp \
    PageImagesGenerées.cpp

FORMS += \
    mainwindow.ui custom.ui inventaire.ui Dispositions.ui PageImagesGenerées.ui

# Qt Resource Collection
RESOURCES += resources.qrc

TRANSLATIONS += \
    translations/machineDecoupeIHM_fr.ts \
    translations/machineDecoupeIHM_en.ts

# Installation (déploiement)
!isEmpty(target.path): INSTALLS += target
