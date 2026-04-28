# Architecture

**Last Updated:** 2026-04-28

## Pattern Overview

**Overall:** MVVM (Model-View-ViewModel) with Coordinator pattern and Clean Architecture layers

**Key Characteristics:**
- Strict separation between UI logic (View), state management (ViewModel), and business logic (Domain/Application services)
- Domain layer is isolated from Qt dependencies where possible
- Coordinator pattern for orchestrating complex interactions between components
- Interface-based dependency injection for infrastructure services
- Signal/slot communication between View and ViewModel

## Layers

### Domain Layer
- **Purpose:** Pure business logic with minimal dependencies
- **Location:** `domain/`
- **Contains:**
  - `shapes/` — ShapeModel, ShapeManager, PathGenerator, BaseShapeNamingService
  - `geometry/` — GeometryUtils, PathPlanner (TSP optimization), PathOptimizer (cut ordering)
  - `geometry/optimization/` — PlacementOptimizer (nesting algorithm using Clipper2)
  - `inventory/` — InventoryModel, InventoryDomainTypes, InventorySnapshot
  - `interfaces/` — IMotorControl, IInventoryRepository (abstract interfaces)
- **Depends on:** Standard library, Qt containers where needed (QList, QPolygonF, QPointF)
- **Used by:** Application services, ViewModels

### Application / Coordination Layer
- **Purpose:** High-level orchestration, use cases, and business workflows
- **Location:** `application/`
- **Subdivisions:**
  - `coordinators/` — MainWindowCoordinator, ShapeCoordinator, DialogManager, AIDialogCoordinator
  - `services/` — CuttingService, ImageImportService, InventoryController, InventoryQueryService, InventoryMutationService, InventorySortFilterService, InventoryStorage, LayoutManager, GridPlacementService, GeometryTransformHelper, ShapeValidationService
  - `factory/` — AppFactory (constructs and wires dependency stacks)
- **Depends on:** Domain, Infrastructure, ViewModels
- **Used by:** ViewModels, Coordinators

### Infrastructure Layer
- **Purpose:** External integrations and hardware/persistence details
- **Location:** `infrastructure/`
- **Subdivisions:**
  - `hardware/` — Raspberry (GPIO), StmUartService (UART protocol), StmProtocol (protocol types), TrajetMotor (trajectory execution)
  - `imaging/` — ImageEdgeImporter, LogoImporter, skeletonizer (OpenCV-based)
  - `network/` — OpenAIService (API client), WifiNmcliClient, WifiNmcliParsers, WifiProfileService
  - `persistence/` — InventoryRepository (implements IInventoryRepository)
- **Depends on:** Domain interfaces, Qt, external libraries (OpenCV, libgpiod)
- **Used by:** Application services, MachineViewModel

### ViewModel Layer
- **Purpose:** Bridge between View (UI) and Model (data/logic) — state + commands
- **Location:** `viewmodels/`
- **Contains:**
  - WorkspaceViewModel — Central workspace state (dimensions, shape count, spacing, language)
  - MainWindowViewModel — Main window UI state (cutting progress, AI status)
  - ShapeVisualizationViewModel — Shape display state (model, dimensions, custom mode)
  - InventoryViewModel — Inventory presentation state (folders, shapes, search/sort/filter)
  - CustomEditorViewModel — Editor state (logo import, shape save)
  - FolderViewModel — Folder browser state (file list, filter, sort)
  - WifiConfigViewModel — WiFi configuration state (scan, connect, profiles)
  - MachineViewModel — Machine control state (STM connection, position, commands)
  - InventoryViewState — View state structure for Inventory display
- **Depends on:** Application services, Domain models
- **Used by:** Views (UI layer) via signals/slots

### View / UI Layer
- **Purpose:** Qt Widgets and visual presentation only — NO business logic
- **Location:** `ui/`
- **Subdivisions:**
  - `mainwindow/` — MainWindow, MainWindowMenuBuilder, mainwindow.ui
  - `widgets/` — ShapeVisualization, CustomEditor, Inventory, FolderWidget, LayoutsDialog, KeyboardDialog, NumericKeyboardDialog, KeyboardEventFilter, WifiTransferWidget
  - `canvas/` — CustomDrawArea (drawing canvas), DrawModeManager, DrawingState, ShapeRenderer, MouseInteractionHandler, ViewTransformer, HistoryManager, EraserTool, TextTool
  - `canvas/tools/` — TouchGestureReader, ImportedImageGeometryHelper, ImagePaths
  - `dialogs/` — StmTestDialog, AIImagePromptDialog, AIImageProcessDialog, WifiConfigDialog
  - `utils/` — AspectRatioWrapper, GestureHandler, ImageExporter, ScreenUtils, UiScale
- **Rules:** NO business logic allowed. Views connect to ViewModels via signals/slots only.

### Shared Layer
- **Purpose:** Cross-cutting enumerations and types used by all layers
- **Location:** `shared/`
- **Contains:** Language enum, PerformanceMode enum, ShapeValidationResult, ThemeManager

### External Libraries
- **Location:** `external/`
- **Contains:**
  - `clipper2/` — Geometry clipping library (used by PlacementOptimizer)
  - `lemon/` — Graph optimization library (TSP solver)
  - `qrcodegen/` — QR code generation library (used by WifiTransferWidget)

## Data Flow

### Shape Creation and Visualization Flow

1. User interacts with ShapeVisualization (UI)
2. ShapeCoordinator receives action via slots (e.g., `setPredefinedShape()`)
3. ShapeCoordinator updates ShapeVisualizationViewModel
4. ViewModel emits signal; ShapeVisualization responds with `updateShape()`
5. CustomDrawArea renders shapes via ShapeRenderer
6. Drawing state stored in ShapeManager (domain) and DrawingState
7. HistoryManager maintains undo/redo stack

### Cutting Execution Flow

1. User clicks "Start Cutting" → MainWindowCoordinator.startCutting()
2. CuttingService initialized with ShapeVisualization reference
3. CuttingService creates TrajetMotor, which uses MachineViewModel (UART)
4. PathPlanner extracts paths from scene, PathOptimizer orders them
5. TrajetMotor sends segments via MachineViewModel → StmUartService
6. Progress signals emitted back to MainWindow via MainWindowViewModel

### Inventory Management Flow

1. InventoryModel holds data (custom shapes, layouts, folders)
2. InventoryRepository (implements IInventoryRepository) handles persistence
3. InventoryController provides use-case methods (addCustomShape, createFolder, etc.)
4. InventoryQueryService, InventoryMutationService provide filtered query/mutation
5. InventorySortFilterService sorts and filters results
6. InventoryViewModel adapts for UI consumption
7. Inventory widget displays and responds to user actions

### AI Image Generation Flow

1. AIDialogCoordinator orchestrates AI dialogs
2. AIImagePromptDialog, AIImageProcessDialog collect user input
3. OpenAIService (infrastructure) calls OpenAI API
4. ImageImportService processes returned image
5. Result passed to CustomDrawArea via signal

## Key Abstractions

### Coordinator Pattern
- **Purpose:** Orchestrate complex interactions between components
- **Examples:** MainWindowCoordinator, ShapeCoordinator, AIDialogCoordinator
- **Pattern:** Central class receives signals from View, updates ViewModel, manages services
- **Benefits:** Decouples View from complex business logic, centralizes interaction logic

### ViewModel Pattern (Qt)
- **Purpose:** Bridge between View (UI) and Model (data/logic)
- **Examples:** WorkspaceViewModel, MainWindowViewModel, MachineViewModel, InventoryViewModel
- **Pattern:** Inherits QObject, emits signals on state changes, provides properties with getters/setters
- **Benefits:** Enables signal/slot bindings, testable without UI, centralized state

### AppFactory
- **Purpose:** Constructs and wires dependency stacks (DI container)
- **Example:** `AppFactory::createInventory()` builds InventoryModel → InventoryController → InventoryViewModel → Inventory widget
- **Benefits:** Centralizes construction logic, avoids manual wiring in MainWindow

## Entry Points

### Main Application (`main.cpp`)
1. Creates QApplication
2. Applies global theme via ThemeManager
3. Installs KeyboardEventFilter for touchscreen
4. Creates WorkspaceViewModel (central state)
5. Creates Inventory stack via AppFactory
6. Creates MainWindow (which internally creates Coordinators via setupUi)
7. Shows full screen, enters event loop

### MainWindow (`ui/mainwindow/MainWindow.h`)
1. Hosts main UI layout (via mainwindow.ui)
2. Creates ShapeVisualization (main canvas)
3. Creates internal Coordinators (MainWindowCoordinator, DialogManager, AIDialogCoordinator, ShapeCoordinator)
4. Connects all View signals to Coordinator slots
5. Responds to ViewModel signals (progress, language, status)

## Known MVVM Violations (to be fixed)

### CustomDrawArea (CRITICAL)
- **Issue:** 86KB monolithic widget that owns ShapeManager directly, contains business logic
- **Fix planned:** Create CanvasViewModel to extract logic from the widget (Phase 3)

### TrajetMotor → ShapeVisualization coupling
- **Issue:** Infrastructure layer directly references UI widget
- **Fix:** Introduce interface for scene data extraction

## Error Handling

**Strategy:** Return codes and Qt signals preferred over exceptions

**Patterns:**
- Service methods return `bool` for success/failure
- Error messages passed via UI update slots or status signals
- DialogManager shows user-facing errors
- Validation services check constraints before operations
- CuttingService signals error state via `finished(bool success)`

## Cross-Cutting Concerns

**Logging:** qDebug(), qWarning() via Qt logging framework
**Validation:** Domain validation services + UI input validation
**Authentication:** None (embedded system, local only)
**Internationalization:** Language enum (FR/EN), QTranslator, .ts translation files

---

*Architecture updated: 2026-04-28*
