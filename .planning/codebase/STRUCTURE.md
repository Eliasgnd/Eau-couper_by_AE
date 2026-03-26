# Codebase Structure

**Analysis Date:** 2026-03-26

## Directory Layout

```
Eau-couper_by_AE/
├── application/          # Coordinators, services, use cases
├── domain/              # Business logic, models (no Qt dependencies)
│   ├── geometry/        # Geometric utilities, placement optimization
│   ├── interfaces/      # Abstract interfaces (IMotorControl, IInventoryRepository)
│   ├── inventory/       # Inventory domain models and types
│   └── shapes/          # Shape models, managers, path generation
├── drawing/             # Canvas interaction, rendering, drawing tools
│   └── tools/           # Pathplanner, image geometry helpers, QR code
├── infrastructure/      # External integrations, hardware, persistence
│   ├── hardware/        # Motor control, GPIO, Raspberry Pi specifics
│   ├── imaging/         # Image processing, logo/edge import, skeletonizer
│   ├── network/         # OpenAI service, WiFi management
│   └── persistence/     # Inventory repository implementation
├── ui/                  # Qt Widgets, dialogs, visual components
│   ├── dialogs/         # AI dialogs, GPIO test, Bluetooth, WiFi config
│   ├── mainwindow/      # MainWindow widget and menu builder
│   ├── utils/           # Screen utilities, image export, gesture handling
│   └── widgets/         # ShapeVisualization, custom editor, keyboards
│       └── shapevisualization/  # Shape visualization specific logic
├── viewmodels/          # MVVM state management (WorkspaceViewModel, etc.)
├── shared/              # Language enum
├── tests/               # Unit and integration tests
├── translations/        # .ts translation files (French, English)
├── icons/               # Application icons
├── html/                # HTML documentation/help
├── scripts/             # Build scripts (setup_dev_ubuntu.sh)
├── main.cpp             # Application entry point
├── machineDecoupeIHM.pro # qmake project file (Qt configuration)
└── resources.qrc        # Qt resource file (icons, styles)
```

## Directory Purposes

**application/**
- Purpose: High-level orchestration, use cases, business workflows
- Contains: Coordinators orchestrating View-ViewModel-Service interactions, service classes for business operations
- Key files:
  - `MainWindowCoordinator.h/cpp` - Orchestrates main window interactions
  - `ShapeCoordinator.h/cpp` - Orchestrates shape visualization and manipulation
  - `CuttingService.h/cpp` - Manages cutting workflow
  - `InventoryController.h/cpp` - Inventory use cases
  - `InventoryQueryService.h/cpp` - Inventory queries
  - `InventoryMutationService.h/cpp` - Inventory mutations
  - `InventorySortFilterService.h/cpp` - Sorting and filtering
  - `DialogManager.h/cpp` - Dialog navigation
  - `AIDialogCoordinator.h/cpp` - AI dialog orchestration
  - `ImageImportService.h/cpp` - Image import workflow

**domain/**
- Purpose: Pure business logic, free from Qt framework dependencies
- Contains: Domain models (ShapeModel, InventoryModel), geometry utilities, shape/path generation
- Key files:
  - `shapes/ShapeModel.h` - Predefined shape definitions
  - `shapes/ShapeManager.h/cpp` - Shape collection and selection management
  - `shapes/PathGenerator.h/cpp` - Convert shapes to cutting paths
  - `geometry/GeometryUtils.h` - Geometric calculations (areas, overlaps, containment)
  - `geometry/PlacementOptimizer.h/cpp` - Optimize shape placement
  - `inventory/InventoryModel.h/cpp` - Inventory domain model
  - `inventory/InventoryDomainTypes.h` - Domain types (CustomShapeData, LayoutData, InventoryFolder)
  - `inventory/InventorySnapshot.h` - Snapshot for persistence
  - `interfaces/IMotorControl.h` - Motor control abstraction
  - `interfaces/IInventoryRepository.h` - Persistence abstraction

**drawing/**
- Purpose: Canvas interaction, shape rendering, drawing tools
- Contains: Main drawing widget, mode management, interaction handlers, rendering
- Key files:
  - `CustomDrawArea.h/cpp` - Main interactive drawing canvas widget
  - `DrawModeManager.h/cpp` - Drawing mode state (Freehand, Line, Circle, Eraser, etc.)
  - `DrawingState.h` - Current drawing session state
  - `HistoryManager.h/cpp` - Undo/redo functionality
  - `ShapeRenderer.h/cpp` - Renders shapes to canvas
  - `MouseInteractionHandler.h/cpp` - Mouse/touch input handling
  - `ViewTransformer.h/cpp` - Pan and zoom
  - `EraserTool.h/cpp` - Eraser tool implementation
  - `TextTool.h/cpp` - Text drawing tool
  - `tools/pathplanner.h/cpp` - Path planning for cutting
  - `tools/TouchGestureReader.h/cpp` - Touch gesture recognition
  - `tools/ImportedImageGeometryHelper.h/cpp` - Imported image geometry utilities
  - `tools/qrcodegen.hpp/cpp` - QR code generation (third-party)

**infrastructure/**
- Purpose: External integrations, hardware communication, data persistence
- Contains:
  - **hardware/** - Motor control (GPIO via libgpiod), Raspberry Pi specifics, trajectory execution
    - `MotorControl.h/cpp` - Implements IMotorControl, manages cutting motor
    - `Raspberry.h/cpp` - Raspberry Pi hardware specifics
    - `TrajetMotor.h/cpp` - Motor trajectory execution, G-code generation
  - **imaging/** - Image processing and import
    - `LogoImporter.h/cpp` - Import logos as shapes
    - `ImageEdgeImporter.h/cpp` - Extract edges from images
    - `skeletonizer.h/cpp` - Skeletonization algorithm for images
  - **network/** - External service communication
    - `OpenAIService.h/cpp` - OpenAI API client for image generation
    - `WifiNmcliClient.h/cpp` - WiFi control via nmcli
    - `WifiNmcliParsers.h/cpp` - Parse nmcli output
    - `WifiProfileService.h/cpp` - Manage WiFi profiles
  - **persistence/** - Data storage layer
    - `InventoryRepository.h/cpp` - Implements IInventoryRepository, file/database operations

**ui/**
- Purpose: Qt Widgets, visual components, user interaction
- Contains:
  - **mainwindow/** - Main application window
    - `MainWindow.h/cpp` - Main window widget, creates internal UI components and coordinators
    - `MainWindowMenuBuilder.h/cpp` - Menu construction
    - `mainwindow.ui` - Qt Designer form (MainWindow layout)
  - **widgets/** - Reusable UI components
    - `ShapeVisualization.h/cpp` - Main shape visualization widget (hosts CustomDrawArea)
    - `shapevisualization/ShapeVisualizationViewModel.h/cpp` - Shape visualization state
    - `shapevisualization/LayoutManager.h/cpp` - Layout management
    - `shapevisualization/GridPlacementService.h/cpp` - Grid layout service
    - `shapevisualization/GeometryTransformHelper.h/cpp` - Transform utilities
    - `shapevisualization/ShapeValidationService.h/cpp` - Shape constraint validation
    - `FolderWidget.h/cpp` - Folder browser widget
    - `Inventory.h/cpp` - Inventory browser widget
    - `InventoryViewModel.h` - Inventory widget state (note: file missing from .pro, likely in cpp only)
    - `CustomEditor.h/cpp` - Custom shape editor
    - `LayoutsDialog.h/cpp` - Layout editor dialog
    - `KeyboardDialog.h/cpp` - Text keyboard for touchscreen
    - `NumericKeyboardDialog.h/cpp` - Numeric keyboard for touchscreen
    - `WifiTransferWidget.h/cpp` - WiFi file transfer UI
    - `KeyboardEventFilter.h/cpp` - Global keyboard event filtering
  - **dialogs/** - Dialog windows
    - `AIImagePromptDialog.h/cpp` - AI image generation prompt
    - `AIImageProcessDialog.h/cpp` - AI image processing options
    - `BluetoothReceiverDialog.h/cpp` - Bluetooth connection
    - `WifiConfigDialog.h/cpp` - WiFi configuration
    - `TestGpio.h/cpp` - GPIO testing utility
    - Plus corresponding .ui files
  - **utils/** - UI utilities
    - `AspectRatioWrapper.h/cpp` - Aspect ratio widget wrapper
    - `ScreenUtils.h` - Screen size utilities
    - `UiScale.h` - UI scaling utilities
    - `ImageExporter.h/cpp` - Export canvas to image
    - `GestureHandler.h/cpp` - Gesture recognition

**viewmodels/**
- Purpose: MVVM state management, ViewModel layer
- Contains: State objects that bridge View and business logic via signals/slots
- Key files:
  - `WorkspaceViewModel.h` - Central workspace state (dimensions, shape count, spacing, language)
  - `WorkspaceViewModel.cpp` - Implementation
  - `MainWindowViewModel.h/cpp` - Main window state (cutting progress, status)
  - `InventoryViewModel.h/cpp` - Inventory widget state

**shared/**
- Purpose: Cross-layer shared types and enumerations
- Contains:
  - `Language.h` - Language enum (French, English)

**tests/**
- Purpose: Unit and integration testing
- Contains: Test files (exact structure varies, see build configuration)
- Build: `qmake6 tests.pro && make`

**translations/**
- Purpose: Internationalization translation files
- Contains:
  - `machineDecoupeIHM_fr_FR.ts` - French translations
  - `machineDecoupeIHM_en_US.ts` - English translations

**scripts/**
- Purpose: Build and setup automation
- Contains:
  - `setup_dev_ubuntu.sh` - Development environment setup for Ubuntu/Debian

**Root Level**
- `main.cpp` - Application entry point
- `machineDecoupeIHM.pro` - Qt project file (qmake configuration, dependencies)
- `resources.qrc` - Qt resource file (embeds icons, styles, translations)
- `style.qss` - Qt stylesheet (application theming)

## Key File Locations

**Entry Points:**
- `main.cpp` - Creates QApplication, WorkspaceViewModel, MainWindow; starts event loop
- `ui/mainwindow/MainWindow.h/cpp` - Main window widget; creates internal UI and coordinators
- `ui/mainwindow/mainwindow.ui` - MainWindow layout designed in Qt Designer

**Configuration:**
- `machineDecoupeIHM.pro` - qmake configuration (Qt modules, includes, compiler flags, platform specifics)
- `style.qss` - Qt stylesheet for theming
- `resources.qrc` - Qt resource file (embeds resources at compile time)

**Core Logic:**
- `domain/shapes/ShapeModel.h` - Shape definitions
- `domain/geometry/GeometryUtils.h` - Geometric algorithms
- `domain/inventory/InventoryModel.h` - Inventory data model
- `application/MainWindowCoordinator.h` - Central orchestrator
- `application/CuttingService.h` - Cutting workflow
- `infrastructure/hardware/MotorControl.h` - Hardware interface

**Drawing/Visualization:**
- `drawing/CustomDrawArea.h` - Main canvas widget
- `ui/widgets/ShapeVisualization.h` - Shape visualization container
- `drawing/ShapeRenderer.h` - Shape rendering
- `drawing/DrawModeManager.h` - Drawing mode state

**State Management:**
- `viewmodels/WorkspaceViewModel.h` - Central workspace state
- `viewmodels/MainWindowViewModel.h` - Main window state
- `viewmodels/ShapeVisualizationViewModel.h` - Shape visualization state

**Testing:**
- `tests/` - Test directory (check tests.pro for structure)

## Naming Conventions

**Files:**
- Headers: `.h` (older Qt style) or `.hpp` (modern C++); both used in this codebase
- Source: `.cpp`
- Forms (Qt Designer): `.ui`
- Resources: `.qrc`
- Translation: `.ts`
- Pattern: `ClassName.h` / `ClassName.cpp` (PascalCase)

**Directories:**
- All lowercase: `application/`, `domain/`, `drawing/`, `ui/`, etc.
- Functional grouping: `domain/shapes/`, `domain/geometry/`, `infrastructure/hardware/`, etc.
- Pattern: functional purpose, singular or plural based on content

**Classes:**
- PascalCase: `MainWindow`, `ShapeModel`, `InventoryController`, `CustomDrawArea`
- Services: Suffix with "Service": `CuttingService`, `ImageImportService`
- Viewmodels: Suffix with "ViewModel": `WorkspaceViewModel`, `MainWindowViewModel`
- Managers: Suffix with "Manager": `DrawModeManager`, `HistoryManager`, `DialogManager`
- Coordinators: Suffix with "Coordinator": `MainWindowCoordinator`, `ShapeCoordinator`
- Utilities/Helpers: Suffix with "Utils" or "Helper": `GeometryUtils`, `ImportedImageGeometryHelper`

**Functions/Methods:**
- camelCase: `getShapeVisualization()`, `updateShape()`, `onDimensionsChanged()`
- Signals: Past tense or descriptive: `shapeCountChanged()`, `dimensionsChanged()`, `imageReadyForImport()`
- Slots: Prefix with "on": `onCircleRequested()`, `onDimensionsChanged()`

**Variables:**
- Member variables: Prefix with `m_`: `m_largeur`, `m_visualization`, `m_model`
- Local variables: camelCase: `shapeCount`, `filePath`, `isValid`
- Constants: UPPERCASE: `DEFAULT_WIDTH`, `MAX_SHAPES`

## Where to Add New Code

**New Feature (Complete Workflow):**
- Domain logic: `domain/` (new service or model as needed)
- Application workflow: `application/` (new service class or extend existing coordinator)
- UI component: `ui/widgets/` or `ui/dialogs/` (new widget or dialog)
- ViewModel: `viewmodels/` (new ViewModel for feature state)
- Tests: `tests/` (matching test for domain/app logic)
- Update: `machineDecoupeIHM.pro` (add new .h/.cpp files to HEADERS/SOURCES)

**New UI Component/Module:**
- Implementation: `ui/widgets/` (for general widgets) or `ui/dialogs/` (for dialogs)
- ViewModel (if managing state): `viewmodels/`
- Form file (.ui): Same location as .h/.cpp
- Coordinator connection: Add slots/signals in `application/MainWindowCoordinator.h/cpp` if integrating with main flow
- Update: `machineDecoupeIHM.pro`

**New Drawing Tool/Mode:**
- Tool implementation: `drawing/tools/` (for specialized tools) or `drawing/` (for core tools)
- Mode definition: Update `drawing/DrawModeManager.h` enum
- Rendering: Update `drawing/ShapeRenderer.h/cpp` if new visual style
- Interaction: Update `drawing/MouseInteractionHandler.h/cpp` if new input handling
- Update: `machineDecoupeIHM.pro`

**New Domain Logic:**
- Pure logic: `domain/shapes/` (shape-related), `domain/geometry/` (geometry-related), `domain/inventory/` (inventory-related)
- Avoid Qt dependencies; prefer standard library
- Provide abstract interfaces in `domain/interfaces/` if infrastructure implementation needed
- Concrete implementation: `infrastructure/` (hardware, network, persistence)
- Update: `machineDecoupeIHM.pro`

**New Hardware/Infrastructure Integration:**
- Interface: Define in `domain/interfaces/` (e.g., `INewService.h`)
- Implementation: `infrastructure/hardware/` (GPIO), `infrastructure/network/` (API), `infrastructure/imaging/` (image), or `infrastructure/persistence/` (storage)
- Dependency injection: Pass to services via constructor or initialize() method
- Update: `machineDecoupeIHM.pro`

**Utilities/Helpers:**
- Geometry utilities: `domain/geometry/`
- UI utilities: `ui/utils/`
- Drawing utilities: `drawing/tools/`
- Shared utilities: `shared/`

## Special Directories

**build/**
- Purpose: Build output directory (created by qmake/make)
- Generated: Yes (do not commit)
- Committed: No (.gitignore'd)

**translations/**
- Purpose: .ts translation files for i18n
- Generated: No (manually edited)
- Committed: Yes (source translations)
- Files are compiled to .qm at build time by Qt build system

**icons/, html/**
- Purpose: Static assets (icons, documentation)
- Generated: No
- Committed: Yes

**tests/**
- Purpose: Unit and integration tests
- Generated: No (tests.pro is template, test files are source)
- Committed: Yes (test source code)
- Built separately: `cd tests && qmake6 tests.pro && make && ./tests`

---

*Structure analysis: 2026-03-26*
