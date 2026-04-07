# ==== PROJET ====
TARGET = machineDecoupeIHM
TEMPLATE = app
QT += core gui widgets svg network bluetooth httpserver openglwidgets concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17

# ==== PLATEFORME LINUX / RASPBERRY PI ====
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4 libgpiod
    # Sur Linux, NLopt s'installe via : sudo apt install libnlopt-dev
    LIBS += -lnlopt
}

# ==== PLATEFORME WINDOWS ====
win32 {
    # Suppression de l'avertissement de caractères non conformes (C4828)
    QMAKE_CXXFLAGS += /wd4828

    # OpenCV
    INCLUDEPATH += C:/opencv/build/include
    LIBS += -LC:/opencv/build/x64/vc16/lib
    CONFIG(debug, debug|release) {
        LIBS += -lopencv_world4120d
    } else {
        LIBS += -lopencv_world4120
    }
}

# ==============================================================================
# ==== EXTERNAL LIBRARIES (Clipper1, Clipper2, Boost, libnest2d, NLopt) ====
# ==============================================================================

# --- 1. Clipper 1 (Version 6.4.2 - REQUIS par libnest2d) ---
# D'après tes erreurs, les fichiers sont dans le sous-dossier /cpp/
CLIPPER1_PATH = $$PWD/external/clipper1/cpp
INCLUDEPATH += $$CLIPPER1_PATH
SOURCES += $$CLIPPER1_PATH/clipper.cpp
HEADERS += $$CLIPPER1_PATH/clipper.hpp

# --- 2. Clipper 2 (Pour tes propres fonctions de marge) ---
CLIPPER2_PATH = $$PWD/external/clipper2
INCLUDEPATH += $$CLIPPER2_PATH/include
SOURCES += \
    $$CLIPPER2_PATH/src/clipper.engine.cpp \
    $$CLIPPER2_PATH/src/clipper.offset.cpp \
    $$CLIPPER2_PATH/src/clipper.rectclip.cpp

# --- 3. Boost (Moteur géométrique - Header-only) ---
# Important : Pointer vers la racine où se trouve le dossier /boost/ (après b2 headers)
INCLUDEPATH += $$PWD/external/boost

# --- 4. libnest2d (Moteur d'emboîtement - Header-only) ---
INCLUDEPATH += $$PWD/external/libnest2d/include
DEFINES += LIBNEST2D_GEOMETRIES_clipper
DEFINES += LIBNEST2D_OPTIMIZER_nlopt   # ← AJOUTER cette ligne

# NLopt Windows
# --- NLopt (Windows) ---
win32 {
    NLOPT_PATH = $$PWD/external/nlopt
    INCLUDEPATH += $$NLOPT_PATH/build      # nlopt.hpp
    INCLUDEPATH += $$NLOPT_PATH/src/api    # nlopt.h  ← c'est ici
    LIBS += -L$$NLOPT_PATH/build/Release -lnlopt
}

# ==============================================================================
# ==== PROJET SOURCES & HEADERS ====
# ==============================================================================

# Fichiers de l'optimization
HEADERS += domain/geometry/optimization/PlacementOptimizer.h
SOURCES += domain/geometry/optimization/PlacementOptimizer.cpp

# ==== INCLUDEPATH GÉNÉRAL ====
INCLUDEPATH += \
    . \
    ui/mainwindow \
    ui/widgets \
    ui/widgets/shapevisualization \
    ui/dialogs \
    ui/utils \
    application \
    viewmodels \
    drawing \
    drawing/tools \
    domain \
    domain/shapes \
    domain/geometry \
    domain/geometry/optimization \
    domain/inventory \
    domain/interfaces \
    infrastructure \
    infrastructure/persistence \
    infrastructure/hardware \
    infrastructure/network \
    infrastructure/imaging \
    shared

# ==== HEADERS ====
HEADERS += \
    ui/mainwindow/MainWindow.h \
    ui/mainwindow/MainWindowMenuBuilder.h \
    application/MainWindowCoordinator.h \
    application/ShapeCoordinator.h \
    application/CuttingService.h \
    application/ImageImportService.h \
    application/DialogManager.h \
    application/AIDialogCoordinator.h \
    application/InventoryController.h \
    application/InventoryStorage.h \
    application/InventoryQueryService.h \
    application/InventoryMutationService.h \
    application/InventorySortFilterService.h \
    application/InventoryViewState.h \
    viewmodels/WorkspaceViewModel.h \
    viewmodels/MainWindowViewModel.h \
    viewmodels/InventoryViewModel.h \
    viewmodels/CustomEditorViewModel.h \
    viewmodels/WifiConfigViewModel.h \
    viewmodels/FolderViewModel.h \
    ui/widgets/ShapeVisualization.h \
    ui/widgets/shapevisualization/ShapeVisualizationViewModel.h \
    ui/widgets/shapevisualization/LayoutManager.h \
    ui/widgets/shapevisualization/GridPlacementService.h \
    ui/widgets/shapevisualization/GeometryTransformHelper.h \
    ui/widgets/shapevisualization/ShapeValidationService.h \
    ui/widgets/FolderWidget.h \
    ui/widgets/KeyboardDialog.h \
    ui/widgets/NumericKeyboardDialog.h \
    ui/widgets/CustomEditor.h \
    ui/widgets/Inventory.h \
    ui/widgets/LayoutsDialog.h \
    ui/widgets/KeyboardEventFilter.h \
    ui/widgets/WifiTransferWidget.h \
    ui/dialogs/TestGpio.h \
    ui/dialogs/AIImagePromptDialog.h \
    ui/dialogs/AIImageProcessDialog.h \
    ui/dialogs/BluetoothReceiverDialog.h \
    ui/dialogs/WifiConfigDialog.h \
    ui/utils/AspectRatioWrapper.h \
    ui/utils/ScreenUtils.h \
    ui/utils/UiScale.h \
    ui/utils/ImageExporter.h \
    ui/utils/GestureHandler.h \
    drawing/CustomDrawArea.h \
    drawing/DrawingState.h \
    drawing/ShapeRenderer.h \
    drawing/DrawModeManager.h \
    drawing/HistoryManager.h \
    drawing/MouseInteractionHandler.h \
    drawing/ViewTransformer.h \
    drawing/EraserTool.h \
    drawing/TextTool.h \
    drawing/tools/pathplanner.h \
    drawing/tools/TouchGestureReader.h \
    drawing/tools/ImportedImageGeometryHelper.h \
    drawing/tools/qrcodegen.hpp \
    drawing/tools/ImagePaths.h \
    domain/shapes/ShapeModel.h \
    domain/shapes/BaseShapeNamingService.h \
    domain/shapes/ShapeManager.h \
    domain/shapes/PathGenerator.h \
    domain/geometry/GeometryUtils.h \
    domain/inventory/InventoryModel.h \
    domain/inventory/InventoryDomainTypes.h \
    domain/inventory/InventorySnapshot.h \
    domain/interfaces/IInventoryRepository.h \
    domain/interfaces/IMotorControl.h \
    infrastructure/persistence/InventoryRepository.h \
    infrastructure/hardware/MotorControl.h \
    infrastructure/hardware/Raspberry.h \
    infrastructure/hardware/TrajetMotor.h \
    infrastructure/network/OpenAIService.h \
    infrastructure/network/WifiNmcliClient.h \
    infrastructure/network/WifiNmcliParsers.h \
    infrastructure/network/WifiProfileService.h \
    infrastructure/imaging/LogoImporter.h \
    infrastructure/imaging/ImageEdgeImporter.h \
    infrastructure/imaging/skeletonizer.h \
    shared/Language.h

# ==== SOURCES ====
SOURCES += \
    main.cpp \
    ui/mainwindow/MainWindow.cpp \
    ui/mainwindow/MainWindowMenuBuilder.cpp \
    application/MainWindowCoordinator.cpp \
    application/ShapeCoordinator.cpp \
    application/CuttingService.cpp \
    application/ImageImportService.cpp \
    application/DialogManager.cpp \
    application/AIDialogCoordinator.cpp \
    application/InventoryController.cpp \
    application/InventoryStorage.cpp \
    application/InventoryQueryService.cpp \
    application/InventoryMutationService.cpp \
    application/InventorySortFilterService.cpp \
    ui/widgets/ShapeVisualization.cpp \
    ui/widgets/shapevisualization/ShapeVisualizationViewModel.cpp \
    ui/widgets/shapevisualization/LayoutManager.cpp \
    ui/widgets/shapevisualization/GridPlacementService.cpp \
    ui/widgets/shapevisualization/GeometryTransformHelper.cpp \
    ui/widgets/shapevisualization/ShapeValidationService.cpp \
    ui/widgets/FolderWidget.cpp \
    ui/widgets/KeyboardDialog.cpp \
    ui/widgets/NumericKeyboardDialog.cpp \
    ui/widgets/CustomEditor.cpp \
    ui/widgets/Inventory.cpp \
    ui/widgets/LayoutsDialog.cpp \
    ui/widgets/KeyboardEventFilter.cpp \
    ui/widgets/WifiTransferWidget.cpp \
    ui/dialogs/TestGpio.cpp \
    ui/dialogs/AIImagePromptDialog.cpp \
    ui/dialogs/AIImageProcessDialog.cpp \
    ui/dialogs/BluetoothReceiverDialog.cpp \
    ui/dialogs/WifiConfigDialog.cpp \
    ui/utils/AspectRatioWrapper.cpp \
    ui/utils/ImageExporter.cpp \
    ui/utils/GestureHandler.cpp \
    drawing/CustomDrawArea.cpp \
    drawing/ShapeRenderer.cpp \
    drawing/DrawModeManager.cpp \
    drawing/HistoryManager.cpp \
    drawing/MouseInteractionHandler.cpp \
    drawing/ViewTransformer.cpp \
    drawing/EraserTool.cpp \
    drawing/TextTool.cpp \
    drawing/tools/pathplanner.cpp \
    drawing/tools/TouchGestureReader.cpp \
    drawing/tools/ImportedImageGeometryHelper.cpp \
    drawing/tools/qrcodegen.cpp \
    domain/shapes/ShapeModel.cpp \
    domain/shapes/BaseShapeNamingService.cpp \
    domain/shapes/ShapeManager.cpp \
    domain/shapes/PathGenerator.cpp \
    domain/geometry/GeometryUtils.cpp \
    domain/inventory/InventoryModel.cpp \
    infrastructure/persistence/InventoryRepository.cpp \
    viewmodels/InventoryViewModel.cpp \
    viewmodels/CustomEditorViewModel.cpp \
    viewmodels/WifiConfigViewModel.cpp \
    viewmodels/FolderViewModel.cpp \
    viewmodels/MainWindowViewModel.cpp \
    infrastructure/hardware/MotorControl.cpp \
    infrastructure/hardware/Raspberry.cpp \
    infrastructure/hardware/TrajetMotor.cpp \
    infrastructure/network/OpenAIService.cpp \
    infrastructure/network/WifiNmcliClient.cpp \
    infrastructure/network/WifiNmcliParsers.cpp \
    infrastructure/network/WifiProfileService.cpp \
    infrastructure/imaging/LogoImporter.cpp \
    infrastructure/imaging/ImageEdgeImporter.cpp \
    infrastructure/imaging/skeletonizer.cpp

# ==== FORMS ====
FORMS += \
    ui/mainwindow/mainwindow.ui \
    ui/dialogs/BluetoothReceiverDialog.ui \
    ui/widgets/CustomEditor.ui \
    ui/widgets/Inventory.ui \
    ui/widgets/LayoutsDialog.ui \
    ui/widgets/FolderWidget.ui \
    ui/dialogs/TestGpio.ui \
    ui/widgets/WifiTransferWidget.ui \
    ui/dialogs/WifiConfigDialog.ui

# ==== RESSOURCES ====
RESOURCES += \
    resources.qrc

# ==== TRADUCTIONS ====
TRANSLATIONS += \
    translations/machineDecoupeIHM_fr_FR.ts \
    translations/machineDecoupeIHM_en_US.ts

# ==== INSTALLATION ====
!isEmpty(target.path): INSTALLS += target
