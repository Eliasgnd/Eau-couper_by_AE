# Architecture

**Analysis Date:** 2026-03-26

## Pattern Overview

**Overall:** MVVM (Model-View-ViewModel) with Coordinator pattern and Clean Architecture layers

**Key Characteristics:**
- Strict separation between UI logic (View), state management (ViewModel), and business logic (Domain/Application services)
- Domain layer is isolated from Qt dependencies where possible
- Coordinator pattern for orchestrating complex interactions between components
- Interface-based dependency injection for infrastructure services
- Signal/slot communication between View and ViewModel

## Layers

**Domain Layer:**
- Purpose: Pure business logic with minimal dependencies
- Location: `domain/`
- Contains: Shapes (ShapeModel, ShapeManager), Geometry utilities, Inventory models
- Depends on: Standard library only (Qt containers where needed)
- Used by: Application services, ViewModels, Drawing layer

**Drawing Layer:**
- Purpose: Canvas interaction, drawing tools, shape rendering, and undo/redo
- Location: `drawing/`
- Contains: CustomDrawArea (main canvas), DrawModeManager, HistoryManager, ShapeRenderer, MouseInteractionHandler, tools (EraserTool, TextTool, PathPlanner)
- Depends on: Domain (ShapeManager), Qt Widgets
- Used by: UI (ShapeVisualization), Application services

**UI Layer:**
- Purpose: Qt Widgets and visual presentation only
- Location: `ui/`, `viewmodels/`
- Contains: MainWindow, ShapeVisualization, dialogs, utility widgets, ViewModels
- Depends on: Domain, Application services via ViewModels/Coordinators
- Used by: Application coordinator (main wiring)
- Rules: NO business logic allowed. Views connect to ViewModels via signals/slots only.

**Application/Coordination Layer:**
- Purpose: High-level orchestration, use cases, and business workflows
- Location: `application/`
- Contains: Coordinators (MainWindowCoordinator, ShapeCoordinator, AIDialogCoordinator), Services (CuttingService, ImageImportService, InventoryController, InventoryQueryService, InventoryMutationService, InventorySortFilterService)
- Depends on: Domain, Infrastructure, UI components
- Used by: ViewModels, Coordinators

**Infrastructure Layer:**
- Purpose: External integrations and hardware/persistence details
- Location: `infrastructure/`
- Subdivisions:
  - `persistence/` - InventoryRepository (implements IInventoryRepository)
  - `hardware/` - MotorControl (implements IMotorControl), Raspberry GPIO, TrajetMotor
  - `network/` - OpenAIService, WiFi clients, WiFi configuration
  - `imaging/` - Image processing, logo/edge importers, skeletonizer, QR code generation
- Depends on: Domain interfaces (IMotorControl, IInventoryRepository), Qt, external libraries (OpenCV, libgpiod)
- Used by: Application services, ViewModels

**Shared Layer:**
- Purpose: Cross-cutting enumerations and types
- Location: `shared/`
- Contains: Language enum (French/English)
- Used by: All layers

## Data Flow

**Shape Creation and Visualization Flow:**

1. User interacts with ShapeVisualization (UI)
2. ShapeCoordinator receives action via slots (e.g., `setPredefinedShape()`)
3. ShapeCoordinator updates ShapeVisualizationViewModel
4. ViewModel emits signal; ShapeVisualization responds with `updateShape()`
5. CustomDrawArea renders shapes via ShapeRenderer
6. Drawing state stored in ShapeManager (domain) and DrawingState
7. HistoryManager maintains undo/redo stack

**Cutting Execution Flow:**

1. User clicks "Start Cutting" → MainWindowCoordinator.startCutting()
2. CuttingService initialized with ShapeVisualization reference
3. CuttingService creates TrajetMotor, which uses MotorControl (implements IMotorControl)
4. TrajetMotor generates G-code from shapes in ShapeVisualization
5. MotorControl sends commands to Raspberry GPIO via libgpiod
6. Progress signals emitted back to MainWindow

**Inventory Management Flow:**

1. InventoryModel holds data (custom shapes, layouts, folders)
2. InventoryRepository (implements IInventoryRepository) handles persistence
3. InventoryController provides use-case methods (addCustomShape, createFolder, etc.)
4. InventoryQueryService, InventoryMutationService provide filtered query/mutation
5. InventorySortFilterService sorts and filters results
6. InventoryViewModel adapts for UI consumption
7. Inventory widget displays and responds to user actions

**AI Image Generation Flow:**

1. AIDialogCoordinator orchestrates AI dialogs
2. AIImagePromptDialog, AIImageProcessDialog collect user input
3. OpenAIService (infrastructure) calls OpenAI API
4. ImageImportService processes returned image
5. Result passed to CustomDrawArea via signal

**State Management:**

- WorkspaceViewModel: Centralized state for machine dimensions, shape count, spacing, language (injected in main.cpp)
- MainWindowViewModel: Additional UI state (cutting progress, cutting status)
- ShapeVisualizationViewModel: Shape-specific state (selected type, optimization flags)
- InventoryViewState: Current inventory view (folder, filter mode, sort mode)
- DrawingState: Temporary state during active drawing (in-progress path, points)

## Key Abstractions

**IMotorControl (Interface):**
- Purpose: Abstraction over physical motor hardware
- Examples: `infrastructure/hardware/MotorControl.h`
- Pattern: Abstract interface with pure virtuals (startJet, stopJet, moveRapid, moveCut)
- Implementation: MotorControl (GPIO operations via Raspberry or stub)

**IInventoryRepository (Interface):**
- Purpose: Abstraction over persistence storage
- Examples: `domain/interfaces/IInventoryRepository.h`, `infrastructure/persistence/InventoryRepository.h`
- Pattern: Abstract interface with save/load operations
- Implementation: InventoryRepository (file/database operations)

**ShapeModel:**
- Purpose: Domain model for predefined shapes (Circle, Rectangle, Triangle, Star, Heart)
- Pattern: Static factory methods (generateShapes, shapePolygons)
- Used by: ShapeCoordinator, domain geometry utilities

**Coordinator Pattern:**
- Purpose: Orchestrate complex interactions between components
- Examples: MainWindowCoordinator, ShapeCoordinator, AIDialogCoordinator
- Pattern: Central class receives signals from View, updates ViewModel, manages services
- Benefits: Decouples View from complex business logic, centralizes interaction logic

**ViewModel Pattern (Qt):**
- Purpose: Bridge between View (UI) and Model (data/logic)
- Examples: WorkspaceViewModel, MainWindowViewModel, ShapeVisualizationViewModel, InventoryViewModel
- Pattern: Inherits QObject, emits signals on state changes, provides properties with getters/setters
- Benefits: Enables signal/slot bindings, testable without UI, centralized state

## Entry Points

**Main Application Entry:**
- Location: `main.cpp`
- Triggers: Application startup
- Responsibilities:
  1. Creates QApplication
  2. Installs KeyboardEventFilter for touchscreen
  3. Creates WorkspaceViewModel (central state)
  4. Creates MainWindow (which internally creates Coordinators via setupUi)
  5. Shows full screen
  6. Enters event loop

**MainWindow:**
- Location: `ui/mainwindow/MainWindow.h`
- Triggers: Application startup, created by main()
- Responsibilities:
  1. Hosts main UI layout (via ui/mainwindow/mainwindow.ui)
  2. Creates ShapeVisualization (main canvas)
  3. Creates internal Coordinators (MainWindowCoordinator, DialogManager, AIDialogCoordinator, ShapeCoordinator)
  4. Connects all View signals to Coordinator slots
  5. Responds to Coordinator signals (updateProgressBar, applyLayout, etc.)

**ShapeVisualization (Main Canvas Widget):**
- Location: `ui/widgets/ShapeVisualization.h`
- Triggers: Part of MainWindow layout, user interaction
- Responsibilities:
  1. Hosts CustomDrawArea (drawing canvas)
  2. Displays shapes and grid
  3. Emits user interaction signals
  4. Updates visual state on ViewModel changes

**CustomDrawArea (Drawing Canvas):**
- Location: `drawing/CustomDrawArea.h`
- Triggers: User mouse/touch events
- Responsibilities:
  1. Renders shapes via ShapeRenderer
  2. Handles drawing input via MouseInteractionHandler
  3. Manages draw mode via DrawModeManager
  4. Manages undo/redo via HistoryManager
  5. Returns shape data for export

## Error Handling

**Strategy:** Exceptions not heavily used; return codes and Qt signals preferred

**Patterns:**
- Service methods (InventoryController, etc.) return `bool` for success/failure
- Error messages passed via UI update slots or status signals
- DialogManager shows user-facing errors
- Validation services (ShapeValidationService) check constraints before operations
- CuttingService signals error state via `finished(bool success)`

## Cross-Cutting Concerns

**Logging:**
- Approach: qDebug(), qWarning() via Qt logging framework
- No centralized logging service; direct output in critical paths

**Validation:**
- Approach: Domain layer provides validation services (ShapeValidationService, GeometryUtils overlap detection)
- UI layer validates input (NumericKeyboardDialog, KeyboardDialog)
- Application layer validates pre-conditions (CuttingService checks shapes exist)

**Authentication:**
- Approach: None required (embedded system, local only)
- WiFi credentials stored via WifiProfileService

**Internationalization:**
- Approach: Language enum (FR/EN) passed through system
- QTranslator used for .ts translation files in `translations/`
- UI updates on language change via MainWindowCoordinator.changeLanguage()

---

*Architecture analysis: 2026-03-26*
