# ==== PROJET ====
TARGET = machineDecoupeIHM
TEMPLATE = app
QT += core gui widgets svg network httpserver openglwidgets concurrent serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++14

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
    QMAKE_CXXFLAGS += /wd4828 /wd4267

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
# ==== EXTERNAL LIBRARIES ====
# ==============================================================================

# --- Clipper 2 ---
CLIPPER2_PATH = $$PWD/external/clipper2
INCLUDEPATH += $$CLIPPER2_PATH/include
SOURCES += \
    $$CLIPPER2_PATH/src/clipper.engine.cpp \
    $$CLIPPER2_PATH/src/clipper.offset.cpp \
    $$CLIPPER2_PATH/src/clipper.rectclip.cpp \
    ui/canvas/tools/PathOptimizer.cpp

# --- CONFIGURATION LEMON (Solution Finale - Structure Plate) ---

# 1. On pointe sur 'external' pour que <lemon/random.h> devienne 'external/lemon/random.h'
INCLUDEPATH += $$PWD/external

# 2. Chemin vers tes fichiers .cc
LEMON_ROOT = $$PWD/external/lemon

# 3. FIX POUR VISUAL STUDIO 2022+ (Correction de l'erreur register)
win32-msvc* {
    # Autorise la redéfinition des mots-clés (indispensable pour MSVC)
    DEFINES += _ALLOW_KEYWORD_MACROS
    # Transforme le vieux 'register' en vide pour le C++ moderne
    DEFINES += "register="
    # Supprime l'avertissement de redéfinition qui en découle
    QMAKE_CXXFLAGS += /wd4005
}

# 4. Paramètres LEMON standards
DEFINES += LEMON_HAVE_LONG_LONG
DEFINES += _CRT_SECURE_NO_WARNINGS

# 5. Liste des sources (basée sur ton 'ls')
SOURCES += \
    $$LEMON_ROOT/arg_parser.cc \
    $$LEMON_ROOT/base.cc \
    $$LEMON_ROOT/color.cc \
    $$LEMON_ROOT/lp_base.cc \
    $$LEMON_ROOT/lp_skeleton.cc \
    $$LEMON_ROOT/random.cc \
    $$LEMON_ROOT/bits/windows.cc

# On ignore les alertes de conversion mineures
win32: QMAKE_CXXFLAGS += /wd4267 /wd4828 /wd4244

# ==============================================================================
# ==== PROJET SOURCES & HEADERS ====
# ==============================================================================

# Fichiers de l'optimization
HEADERS += domain/geometry/optimization/PlacementOptimizer.h \
    ui/canvas/tools/PathOptimizer.h
SOURCES += domain/geometry/optimization/PlacementOptimizer.cpp

# ==== INCLUDEPATH GÉNÉRAL ====
INCLUDEPATH += \
    . \
    ui/mainwindow \
    ui/widgets \
    ui/canvas \
    ui/canvas/tools \
    ui/dialogs \
    ui/utils \
    application/coordinators \
    application/services \
    application/factory \
    viewmodels \
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
    application/coordinators/MainWindowCoordinator.h \
    application/coordinators/ShapeCoordinator.h \
    application/coordinators/DialogManager.h \
    application/coordinators/AIDialogCoordinator.h \
    application/services/CuttingService.h \
    application/services/ImageImportService.h \
    application/services/InventoryController.h \
    application/services/InventoryStorage.h \
    application/services/InventoryQueryService.h \
    application/services/InventoryMutationService.h \
    application/services/InventorySortFilterService.h \
    application/services/InventoryViewState.h \
    application/services/GridPlacementService.h \
    application/services/LayoutManager.h \
    application/services/GeometryTransformHelper.h \
    application/services/ShapeValidationService.h \
    application/factory/AppFactory.h \
    viewmodels/WorkspaceViewModel.h \
    viewmodels/MainWindowViewModel.h \
    viewmodels/InventoryViewModel.h \
    viewmodels/CustomEditorViewModel.h \
    viewmodels/WifiConfigViewModel.h \
    viewmodels/FolderViewModel.h \
    viewmodels/ShapeVisualizationViewModel.h \
    ui/widgets/ShapeVisualization.h \
    ui/widgets/FolderWidget.h \
    ui/widgets/KeyboardDialog.h \
    ui/widgets/NumericKeyboardDialog.h \
    ui/widgets/CustomEditor.h \
    ui/widgets/Inventory.h \
    ui/widgets/LayoutsDialog.h \
    ui/widgets/KeyboardEventFilter.h \
    ui/widgets/WifiTransferWidget.h \
    ui/dialogs/StmTestDialog.h \
    ui/dialogs/AIImagePromptDialog.h \
    ui/dialogs/AIImageProcessDialog.h \
    ui/dialogs/WifiConfigDialog.h \
    ui/utils/AspectRatioWrapper.h \
    ui/utils/ScreenUtils.h \
    ui/utils/UiScale.h \
    ui/utils/ImageExporter.h \
    ui/utils/GestureHandler.h \
    ui/canvas/CustomDrawArea.h \
    ui/canvas/DrawingState.h \
    ui/canvas/ShapeRenderer.h \
    ui/canvas/DrawModeManager.h \
    ui/canvas/HistoryManager.h \
    ui/canvas/MouseInteractionHandler.h \
    ui/canvas/ViewTransformer.h \
    ui/canvas/EraserTool.h \
    ui/canvas/TextTool.h \
    ui/canvas/tools/pathplanner.h \
    ui/canvas/tools/TouchGestureReader.h \
    ui/canvas/tools/ImportedImageGeometryHelper.h \
    ui/canvas/tools/qrcodegen.hpp \
    ui/canvas/tools/ImagePaths.h \
    domain/shapes/ShapeModel.h \
    domain/shapes/BaseShapeNamingService.h \
    domain/shapes/ShapeManager.h \
    domain/shapes/PathGenerator.h \
    domain/geometry/GeometryUtils.h \
    domain/inventory/InventoryModel.h \
    domain/inventory/InventoryDomainTypes.h \
    domain/inventory/InventorySnapshot.h \
    domain/interfaces/IInventoryRepository.h \
    infrastructure/persistence/InventoryRepository.h \
    infrastructure/hardware/Raspberry.h \
    infrastructure/hardware/TrajetMotor.h \
    infrastructure/hardware/StmProtocol.h \
    infrastructure/hardware/StmUartService.h \
    viewmodels/MachineViewModel.h \
    infrastructure/network/OpenAIService.h \
    infrastructure/network/WifiNmcliClient.h \
    infrastructure/network/WifiNmcliParsers.h \
    infrastructure/network/WifiProfileService.h \
    infrastructure/imaging/LogoImporter.h \
    infrastructure/imaging/ImageEdgeImporter.h \
    infrastructure/imaging/skeletonizer.h \
    shared/Language.h \
    shared/ShapeValidationResult.h \
    shared/ThemeManager.h

# ==== SOURCES ====
SOURCES += \
    main.cpp \
    ui/mainwindow/MainWindow.cpp \
    ui/mainwindow/MainWindowMenuBuilder.cpp \
    application/factory/AppFactory.cpp \
    application/coordinators/MainWindowCoordinator.cpp \
    application/coordinators/ShapeCoordinator.cpp \
    application/coordinators/DialogManager.cpp \
    application/coordinators/AIDialogCoordinator.cpp \
    application/services/CuttingService.cpp \
    application/services/ImageImportService.cpp \
    application/services/InventoryController.cpp \
    application/services/InventoryStorage.cpp \
    application/services/InventoryQueryService.cpp \
    application/services/InventoryMutationService.cpp \
    application/services/InventorySortFilterService.cpp \
    application/services/GridPlacementService.cpp \
    application/services/LayoutManager.cpp \
    application/services/GeometryTransformHelper.cpp \
    application/services/ShapeValidationService.cpp \
    viewmodels/InventoryViewModel.cpp \
    viewmodels/CustomEditorViewModel.cpp \
    viewmodels/WifiConfigViewModel.cpp \
    viewmodels/FolderViewModel.cpp \
    viewmodels/MainWindowViewModel.cpp \
    viewmodels/ShapeVisualizationViewModel.cpp \
    ui/widgets/ShapeVisualization.cpp \
    ui/widgets/FolderWidget.cpp \
    ui/widgets/KeyboardDialog.cpp \
    ui/widgets/NumericKeyboardDialog.cpp \
    ui/widgets/CustomEditor.cpp \
    ui/widgets/Inventory.cpp \
    ui/widgets/LayoutsDialog.cpp \
    ui/widgets/KeyboardEventFilter.cpp \
    ui/widgets/WifiTransferWidget.cpp \
    ui/dialogs/StmTestDialog.cpp \
    ui/dialogs/AIImagePromptDialog.cpp \
    ui/dialogs/AIImageProcessDialog.cpp \
    ui/dialogs/WifiConfigDialog.cpp \
    ui/utils/AspectRatioWrapper.cpp \
    ui/utils/ImageExporter.cpp \
    ui/utils/GestureHandler.cpp \
    ui/canvas/CustomDrawArea.cpp \
    ui/canvas/ShapeRenderer.cpp \
    ui/canvas/DrawModeManager.cpp \
    ui/canvas/HistoryManager.cpp \
    ui/canvas/MouseInteractionHandler.cpp \
    ui/canvas/ViewTransformer.cpp \
    ui/canvas/EraserTool.cpp \
    ui/canvas/TextTool.cpp \
    ui/canvas/tools/pathplanner.cpp \
    ui/canvas/tools/TouchGestureReader.cpp \
    ui/canvas/tools/ImportedImageGeometryHelper.cpp \
    ui/canvas/tools/qrcodegen.cpp \
    domain/shapes/ShapeModel.cpp \
    domain/shapes/BaseShapeNamingService.cpp \
    domain/shapes/ShapeManager.cpp \
    domain/shapes/PathGenerator.cpp \
    domain/geometry/GeometryUtils.cpp \
    domain/inventory/InventoryModel.cpp \
    infrastructure/persistence/InventoryRepository.cpp \
    infrastructure/hardware/Raspberry.cpp \
    infrastructure/hardware/TrajetMotor.cpp \
    infrastructure/hardware/StmUartService.cpp \
    viewmodels/MachineViewModel.cpp \
    infrastructure/network/OpenAIService.cpp \
    infrastructure/network/WifiNmcliClient.cpp \
    infrastructure/network/WifiNmcliParsers.cpp \
    infrastructure/network/WifiProfileService.cpp \
    infrastructure/imaging/LogoImporter.cpp \
    infrastructure/imaging/ImageEdgeImporter.cpp \
    infrastructure/imaging/skeletonizer.cpp \
    shared/ThemeManager.cpp

# ==== FORMS ====
FORMS += \
    ui/mainwindow/mainwindow.ui \
    ui/widgets/CustomEditor.ui \
    ui/widgets/Inventory.ui \
    ui/widgets/LayoutsDialog.ui \
    ui/widgets/FolderWidget.ui \
    ui/dialogs/StmTestDialog.ui \
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
