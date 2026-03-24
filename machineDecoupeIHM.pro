QT += core gui widgets svg network bluetooth httpserver openglwidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET  = machineDecoupeIHM

# ==== PLATEFORME LINUX / RASPBERRY PI ====
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4 libgpiod
}

# ==== PLATEFORME WINDOWS ====
win32 {
    INCLUDEPATH += C:/opencv/build/include \
                   C:/opencv/build/include/opencv2
    LIBS += -LC:/opencv/build/x64/vc16/lib
    CONFIG(debug, debug|release) {
        LIBS += -lopencv_world4120d
    } else {
        LIBS += -lopencv_world4120
    }
}

# ==== FICHIERS (HEADERS) ====
HEADERS += \
    # --- MainWindow et UI principale ---
    MainWindow.h \
    ui/widgets/FormeVisualization.h \
    # --- Drawing ---
    drawing/shapes/CustomDrawArea.h \
    drawing/tools/LogoImporter.h \
    drawing/tools/ImageEdgeImporter.h \
    models/ShapeModel.h \
    # --- UI (Widgets) ---
    ui/widgets/DossierWidget.h \
    drawing/utils/ScreenUtils.h \
    drawing/utils/AspectRatioWrapper.h \
    ui/widgets/clavier.h \
    ui/widgets/claviernumerique.h \
    ui/widgets/custom.h \
    models/inventaire.h \
    ui/widgets/Dispositions.h \
    # --- Drawing (Tools/Managers) ---
    drawing/DrawingState.h \
    ui/widgets/keyboardeventfilter.h \
    managers/system/motorcontrol.h \
    drawing/tools/pathplanner.h \
    managers/system/raspberry.h \
    ui/dialogs/TestGpio.h \
    drawing/tools/skeletonizer.h \
    drawing/tools/touchgesturereader.h \
    drawing/tools/trajetmotor.h \
    # --- Language ---
    Language.h \
    # --- UI (Dialogs) ---
    ui/dialogs/AIImagePromptDialog.h \
    ui/dialogs/AIImageProcessDialog.h \
    ui/dialogs/BluetoothReceiverDialog.h \
    ui/widgets/WifiTransferWidget.h \
    drawing/tools/qrcodegen.hpp \
    drawing/tools/ImagePaths.h \
    ui/dialogs/WifiConfigDialog.h \
    # --- Managers ---
    managers/ai/OpenAIService.h \
    managers/navigation/AppController.h \
    managers/navigation/NavigationController.h \
    managers/ai/AIServiceManager.h \
    drawing/tools/ImportedImageGeometryHelper.h \
    drawing/utils/GeometryUtils.h \
    drawing/ShapeManager.h \
    drawing/ShapeRenderer.h \
    drawing/DrawModeManager.h \
    drawing/HistoryManager.h \
    drawing/MouseInteractionHandler.h \
    drawing/ViewTransformer.h \
    drawing/EraserTool.h \
    managers/system/GestureHandler.h \
    drawing/TextTool.h \
    drawing/PathGenerator.h

# ==== FICHIERS (SOURCES) ====
SOURCES += \
    # --- MainWindow et UI principale ---
    MainWindow.cpp \
    ui/widgets/FormeVisualization.cpp \
    main.cpp \
    # --- Drawing ---
    drawing/shapes/CustomDrawArea.cpp \
    drawing/tools/LogoImporter.cpp \
    drawing/tools/ImageEdgeImporter.cpp \
    models/ShapeModel.cpp \
    # --- UI (Widgets) ---
    ui/widgets/DossierWidget.cpp \
    drawing/utils/AspectRatioWrapper.cpp \
    ui/widgets/clavier.cpp \
    ui/widgets/claviernumerique.cpp \
    ui/widgets/custom.cpp \
    models/inventaire.cpp \
    ui/widgets/Dispositions.cpp \
    ui/widgets/keyboardeventfilter.cpp \
    # --- Managers/System ---
    managers/system/motorcontrol.cpp \
    drawing/tools/pathplanner.cpp \
    managers/system/raspberry.cpp \
    ui/dialogs/TestGpio.cpp \
    drawing/tools/skeletonizer.cpp \
    drawing/tools/touchgesturereader.cpp \
    drawing/tools/trajetmotor.cpp \
    # --- UI (Dialogs) ---
    ui/dialogs/AIImagePromptDialog.cpp \
    ui/dialogs/AIImageProcessDialog.cpp \
    ui/dialogs/BluetoothReceiverDialog.cpp \
    ui/widgets/WifiTransferWidget.cpp \
    ui/dialogs/WifiConfigDialog.cpp \
    drawing/tools/qrcodegen.cpp \
    # --- Managers ---
    managers/ai/OpenAIService.cpp \
    managers/navigation/AppController.cpp \
    managers/navigation/NavigationController.cpp \
    managers/ai/AIServiceManager.cpp \
    drawing/tools/ImportedImageGeometryHelper.cpp \
    drawing/utils/GeometryUtils.cpp \
    drawing/ShapeManager.cpp \
    drawing/ShapeRenderer.cpp \
    drawing/DrawModeManager.cpp \
    drawing/HistoryManager.cpp \
    drawing/MouseInteractionHandler.cpp \
    drawing/ViewTransformer.cpp \
    drawing/EraserTool.cpp \
    managers/system/GestureHandler.cpp \
    drawing/TextTool.cpp \
    drawing/PathGenerator.cpp

# ==== FICHIERS (FORMS) ====
FORMS += \
    ui/dialogs/BluetoothReceiverDialog.ui \
    mainwindow.ui \
    ui/widgets/custom.ui \
    ui/widgets/inventaire.ui \
    ui/widgets/Dispositions.ui \
    ui/widgets/DossierWidget.ui \
    ui/dialogs/TestGpio.ui \
    ui/widgets/WifiTransferWidget.ui \
    ui/dialogs/WifiConfigDialog.ui



# ==== RESSOURCES ====
RESOURCES += resources.qrc

# ==== TRADUCTIONS ====
TRANSLATIONS += \
    translations/machineDecoupeIHM_fr.ts \
    translations/machineDecoupeIHM_en.ts

!isEmpty(target.path): INSTALLS += target
