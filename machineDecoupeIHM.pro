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

# ==== CHEMINS DE RECHERCHE (INCLUDEPATH) ====
INCLUDEPATH += \
    . \
    models \
    ui/widgets \
    ui/dialogs \
    managers/system \
    managers/ai \
    managers/navigation \
    drawing \
    drawing/utils \
    drawing/tools \
    drawing/shapes

# ==== FICHIERS (HEADERS) ====
HEADERS += \
    # --- MainWindow et UI principale ---
    MainWindow.h \
    MainWindowSystemBinder.h \
    MainWindowMenuBuilder.h \
    MainWindowNavigationBinder.h \
    MainWindowCoordinator.h \
    ShapeController.h \
    ImageImportService.h \
    ui/widgets/ShapeVisualization.h \
    ui/widgets/shapevisualization/ShapeProjectModel.h \
    ui/widgets/shapevisualization/LayoutManager.h \
    # --- Drawing ---
    drawing/shapes/CustomDrawArea.h \
    drawing/tools/LogoImporter.h \
    drawing/tools/ImageEdgeImporter.h \
    models/ShapeModel.h \
    models/InventoryStorage.h \
    models/InventoryQueryService.h \
    models/InventoryMutationService.h \
    models/InventoryModel.h \
    models/InventoryController.h \
    # --- UI (Widgets) ---
    ui/widgets/FolderWidget.h \
    drawing/utils/ScreenUtils.h \
    drawing/utils/AspectRatioWrapper.h \
    ui/widgets/KeyboardDialog.h \
    ui/widgets/NumericKeyboardDialog.h \
    ui/widgets/CustomEditor.h \
    models/Inventory.h \
    ui/widgets/LayoutsDialog.h \
    # --- Drawing (Tools/Managers) ---
    drawing/DrawingState.h \
    ui/widgets/KeyboardEventFilter.h \
    managers/system/MotorControl.h \
    drawing/tools/pathplanner.h \
    managers/system/Raspberry.h \
    ui/dialogs/TestGpio.h \
    drawing/tools/skeletonizer.h \
    drawing/tools/TouchGestureReader.h \
    drawing/tools/TrajetMotor.h \
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
    drawing/utils/shapevisualization/GridPlacementService.h \
    drawing/utils/shapevisualization/GeometryTransformHelper.h \
    drawing/utils/shapevisualization/ShapeValidationService.h \
    drawing/utils/ImageExporter.h \
    drawing/utils/PlacementOptimizer.h \
    drawing/ShapeManager.h \
    drawing/ShapeRenderer.h \
    drawing/DrawModeManager.h \
    drawing/HistoryManager.h \
    drawing/MouseInteractionHandler.h \
    drawing/ViewTransformer.h \
    drawing/EraserTool.h \
    managers/system/GestureHandler.h \
    managers/system/WifiNmcliClient.h \
    managers/system/WifiNmcliParsers.h \
    managers/system/WifiProfileService.h \
    drawing/TextTool.h \
    drawing/PathGenerator.h

# ==== FICHIERS (SOURCES) ====
SOURCES += \
    # --- MainWindow et UI principale ---
    MainWindow.cpp \
    MainWindowSystemBinder.cpp \
    MainWindowMenuBuilder.cpp \
    MainWindowNavigationBinder.cpp \
    MainWindowCoordinator.cpp \
    ShapeController.cpp \
    ImageImportService.cpp \
    ui/widgets/ShapeVisualization.cpp \
    ui/widgets/shapevisualization/ShapeProjectModel.cpp \
    ui/widgets/shapevisualization/LayoutManager.cpp \
    main.cpp \
    # --- Drawing ---
    drawing/shapes/CustomDrawArea.cpp \
    drawing/tools/LogoImporter.cpp \
    drawing/tools/ImageEdgeImporter.cpp \
    models/ShapeModel.cpp \
    models/InventoryStorage.cpp \
    models/InventoryQueryService.cpp \
    models/InventoryMutationService.cpp \
    models/InventoryModel.cpp \
    models/InventoryController.cpp \
    # --- UI (Widgets) ---
    ui/widgets/FolderWidget.cpp \
    drawing/utils/AspectRatioWrapper.cpp \
    ui/widgets/KeyboardDialog.cpp \
    ui/widgets/NumericKeyboardDialog.cpp \
    ui/widgets/CustomEditor.cpp \
    models/Inventory.cpp \
    ui/widgets/LayoutsDialog.cpp \
    ui/widgets/KeyboardEventFilter.cpp \
    # --- Managers/System ---
    managers/system/MotorControl.cpp \
    drawing/tools/pathplanner.cpp \
    managers/system/Raspberry.cpp \
    ui/dialogs/TestGpio.cpp \
    drawing/tools/skeletonizer.cpp \
    drawing/tools/TouchGestureReader.cpp \
    drawing/tools/TrajetMotor.cpp \
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
    drawing/utils/shapevisualization/GridPlacementService.cpp \
    drawing/utils/shapevisualization/GeometryTransformHelper.cpp \
    drawing/utils/shapevisualization/ShapeValidationService.cpp \
    drawing/utils/ImageExporter.cpp \
    drawing/utils/PlacementOptimizer.cpp \
    drawing/ShapeManager.cpp \
    drawing/ShapeRenderer.cpp \
    drawing/DrawModeManager.cpp \
    drawing/HistoryManager.cpp \
    drawing/MouseInteractionHandler.cpp \
    drawing/ViewTransformer.cpp \
    drawing/EraserTool.cpp \
    managers/system/GestureHandler.cpp \
    managers/system/WifiNmcliClient.cpp \
    managers/system/WifiNmcliParsers.cpp \
    managers/system/WifiProfileService.cpp \
    drawing/TextTool.cpp \
    drawing/PathGenerator.cpp

# ==== FICHIERS (FORMS) ====
FORMS += \
    ui/dialogs/BluetoothReceiverDialog.ui \
    mainwindow.ui \
    ui/widgets/CustomEditor.ui \
    ui/widgets/Inventory.ui \
    ui/widgets/LayoutsDialog.ui \
    ui/widgets/FolderWidget.ui \
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
