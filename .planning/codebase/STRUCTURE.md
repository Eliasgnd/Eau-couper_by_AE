# Codebase Structure

**Last Updated:** 2026-04-28

## Directory Layout

```
Eau-couper_by_AE/
├── main.cpp                          # Application entry point
├── machineDecoupeIHM.pro             # qmake project file
├── resources.qrc                     # Qt resource file (icons, styles, translations)
├── style.qss / style_light.qss      # Qt stylesheets (dark/light themes)
│
├── domain/                           # Pure business logic (minimal Qt dependencies)
│   ├── geometry/                     # Geometric utilities and algorithms
│   │   ├── GeometryUtils.h/cpp       # Geometric calculations (areas, overlaps, containment)
│   │   ├── pathplanner.h/cpp         # Path planning + TSP optimization for cutting
│   │   ├── PathOptimizer.h/cpp       # Cut ordering (inside-out, nearest neighbor)
│   │   └── optimization/
│   │       └── PlacementOptimizer.h/cpp  # Nesting algorithm (Clipper2-based NFP)
│   ├── interfaces/                   # Abstract interfaces for dependency inversion
│   │   ├── IInventoryRepository.h    # Persistence abstraction
│   │   └── IMotorControl.h           # Motor control abstraction
│   ├── inventory/                    # Inventory domain models
│   │   ├── InventoryDomainTypes.h    # Domain types (CustomShapeData, LayoutData, etc.)
│   │   ├── InventoryModel.h/cpp      # Inventory data model
│   │   └── InventorySnapshot.h       # Snapshot for persistence
│   └── shapes/                       # Shape models and generation
│       ├── BaseShapeNamingService.h/cpp # Localized shape names
│       ├── PathGenerator.h/cpp       # Convert shapes to cutting paths
│       ├── ShapeManager.h/cpp        # Shape collection, selection, proxy management
│       └── ShapeModel.h/cpp          # Predefined shape definitions (Circle, Rect, etc.)
│
├── application/                      # Orchestration, services, use cases
│   ├── coordinators/                 # Complex interaction orchestrators
│   │   ├── AIDialogCoordinator.h/cpp # AI dialog workflow orchestration
│   │   ├── DialogManager.h/cpp       # Dialog navigation and lifecycle
│   │   ├── MainWindowCoordinator.h/cpp # Main window interaction orchestrator
│   │   └── ShapeCoordinator.h/cpp    # Shape visualization orchestrator
│   ├── factory/                      # Dependency injection factories
│   │   └── AppFactory.h/cpp          # Creates and wires Inventory stack
│   └── services/                     # Business operation services
│       ├── CuttingService.h/cpp      # Cutting workflow management
│       ├── GeometryTransformHelper.h/cpp # Transform utilities for scene
│       ├── GridPlacementService.h/cpp # Grid layout calculation
│       ├── ImageImportService.h/cpp   # Image import workflow
│       ├── InventoryController.h/cpp  # Inventory use cases (CRUD, folders)
│       ├── InventoryMutationService.h/cpp # Inventory write operations
│       ├── InventoryQueryService.h/cpp # Inventory read operations
│       ├── InventorySortFilterService.h/cpp # Sorting and filtering
│       ├── InventoryStorage.h/cpp    # Inventory persistence coordination
│       ├── LayoutManager.h/cpp       # Layout save/load management
│       └── ShapeValidationService.h/cpp # Shape constraint validation
│
├── infrastructure/                   # External integrations, hardware, persistence
│   ├── hardware/                     # Motor control, GPIO, Raspberry Pi
│   │   ├── Raspberry.h/cpp           # Raspberry Pi hardware specifics (GPIO)
│   │   ├── StmProtocol.h             # STM32 binary protocol types
│   │   ├── StmUartService.h/cpp      # UART communication with STM32
│   │   └── TrajetMotor.h/cpp         # Motor trajectory execution
│   ├── imaging/                      # Image processing and import
│   │   ├── ImageEdgeImporter.h/cpp   # Extract edges from color images
│   │   ├── LogoImporter.h/cpp        # Import logos as vector shapes
│   │   └── skeletonizer.h/cpp        # Skeletonization algorithm (OpenCV)
│   ├── network/                      # External service communication
│   │   ├── OpenAIService.h/cpp       # OpenAI API client (image generation)
│   │   ├── WifiNmcliClient.h/cpp     # WiFi control via nmcli
│   │   ├── WifiNmcliParsers.h/cpp    # Parse nmcli output
│   │   └── WifiProfileService.h/cpp  # Manage WiFi profiles/credentials
│   └── persistence/                  # Data storage layer
│       └── InventoryRepository.h/cpp # File-based inventory storage
│
├── viewmodels/                       # MVVM state management (ViewModel layer)
│   ├── WorkspaceViewModel.h          # Central workspace state (dimensions, language)
│   ├── MainWindowViewModel.h/cpp     # Main window state (cutting, AI status)
│   ├── ShapeVisualizationViewModel.h/cpp # Shape display state
│   ├── InventoryViewModel.h/cpp      # Inventory presentation state
│   ├── InventoryViewState.h          # View state structure for Inventory display
│   ├── CustomEditorViewModel.h/cpp   # Editor state (logo import, save)
│   ├── FolderViewModel.h/cpp         # Folder browser state
│   ├── WifiConfigViewModel.h/cpp     # WiFi configuration state
│   └── MachineViewModel.h/cpp        # Machine control state (STM, position)
│
├── ui/                               # Qt Widgets — visual presentation only
│   ├── mainwindow/                   # Main application window
│   │   ├── MainWindow.h/cpp          # Main window widget
│   │   ├── MainWindowMenuBuilder.h/cpp # Menu construction
│   │   └── mainwindow.ui             # Qt Designer form
│   ├── canvas/                       # Drawing canvas and tools
│   │   ├── CustomDrawArea.h/cpp      # Main interactive drawing canvas
│   │   ├── DrawModeManager.h/cpp     # Drawing mode state machine
│   │   ├── DrawingState.h            # Active drawing session state
│   │   ├── ShapeRenderer.h/cpp       # Renders shapes to canvas
│   │   ├── MouseInteractionHandler.h/cpp # Mouse/touch input handling
│   │   ├── ViewTransformer.h/cpp     # Pan and zoom transforms
│   │   ├── HistoryManager.h/cpp      # Undo/redo functionality
│   │   ├── EraserTool.h/cpp          # Eraser tool implementation
│   │   ├── TextTool.h/cpp            # Text drawing tool
│   │   └── tools/                    # Specialized canvas tools
│   │       ├── TouchGestureReader.h/cpp # Touch gesture recognition
│   │       ├── ImportedImageGeometryHelper.h/cpp # Image geometry utils
│   │       └── ImagePaths.h          # Image file path constants
│   ├── widgets/                      # Reusable UI components
│   │   ├── ShapeVisualization.h/cpp  # Shape visualization (hosts QGraphicsView)
│   │   ├── CustomEditor.h/cpp + .ui  # Custom shape editor
│   │   ├── Inventory.h/cpp + .ui     # Inventory browser
│   │   ├── FolderWidget.h/cpp + .ui  # Folder browser
│   │   ├── LayoutsDialog.h/cpp + .ui # Layout editor dialog
│   │   ├── KeyboardDialog.h/cpp      # Text keyboard for touchscreen
│   │   ├── NumericKeyboardDialog.h/cpp # Numeric keyboard for touchscreen
│   │   ├── KeyboardEventFilter.h/cpp # Global keyboard event filtering
│   │   └── WifiTransferWidget.h/cpp + .ui # WiFi file transfer UI
│   ├── dialogs/                      # Dialog windows
│   │   ├── AIImagePromptDialog.h/cpp # AI image generation prompt
│   │   ├── AIImageProcessDialog.h/cpp # AI image processing options
│   │   ├── StmTestDialog.h/cpp + .ui # STM32 GPIO test dialog
│   │   └── WifiConfigDialog.h/cpp + .ui # WiFi configuration dialog
│   └── utils/                        # UI utilities
│       ├── AspectRatioWrapper.h/cpp   # Aspect ratio widget wrapper
│       ├── GestureHandler.h/cpp      # Gesture recognition
│       ├── ImageExporter.h/cpp       # Export canvas to image
│       ├── ScreenUtils.h             # Screen size utilities
│       └── UiScale.h                 # UI scaling utilities
│
├── shared/                           # Cross-layer shared types
│   ├── Language.h                    # Language enum (French, English)
│   ├── PerformanceMode.h             # Performance mode enum
│   ├── ShapeValidationResult.h       # Validation result type
│   └── ThemeManager.h/cpp            # Application theme management
│
├── external/                         # Third-party libraries (do not modify)
│   ├── clipper2/                     # Geometry clipping library
│   ├── lemon/                        # Graph optimization library (TSP)
│   └── qrcodegen/                    # QR code generation library
│
├── tests/                            # Unit and integration tests
│   ├── tests.pro                     # Test project file
│   ├── main.cpp                      # Test runner entry point
│   ├── inventory_safety_tests.h/cpp  # Inventory safety tests
│   ├── placement_tests.h/cpp         # Placement algorithm tests
│   └── shape_manager_tests.h/cpp     # Shape manager tests
│
├── translations/                     # .ts translation files
│   ├── machineDecoupeIHM_fr_FR.ts    # French translations
│   └── machineDecoupeIHM_en_US.ts    # English translations
│
├── icons/                            # Application icons
├── html/                             # HTML documentation/help
└── scripts/                          # Build scripts
    └── setup_dev_ubuntu.sh           # Dev environment setup
```

## Naming Conventions

**Files:**
- Headers: `.h` (Qt convention), implementation: `.cpp`
- Forms: `.ui` (Qt Designer), Resources: `.qrc`, Translations: `.ts`
- Pattern: `ClassName.h` / `ClassName.cpp` (PascalCase)

**Directories:**
- All lowercase: `application/`, `domain/`, `ui/`, `viewmodels/`
- Functional grouping: `domain/shapes/`, `infrastructure/hardware/`

**Classes:**
- PascalCase: `MainWindow`, `ShapeModel`, `CustomDrawArea`
- Services: suffix `Service` — `CuttingService`, `ImageImportService`
- ViewModels: suffix `ViewModel` — `WorkspaceViewModel`, `MachineViewModel`
- Managers: suffix `Manager` — `DrawModeManager`, `HistoryManager`
- Coordinators: suffix `Coordinator` — `MainWindowCoordinator`

**Functions/Methods:**
- camelCase: `getShapeVisualization()`, `updateShape()`
- Signals: descriptive — `shapeCountChanged()`, `dimensionsChanged()`
- Slots: verb prefix — `onCircleRequested()`, `onDimensionsChanged()`

**Variables:**
- Members: `m_` prefix — `m_largeur`, `m_visualization`
- Local: camelCase — `shapeCount`, `filePath`
- Constants: UPPERCASE — `DEFAULT_WIDTH`, `STEPS_PER_MM`

## Where to Add New Code

**New Feature (Complete Workflow):**
1. Domain logic → `domain/` (new model or utility)
2. Application workflow → `application/services/` (new service) or `application/coordinators/` (orchestration)
3. ViewModel → `viewmodels/` (new ViewModel for state)
4. UI component → `ui/widgets/` or `ui/dialogs/`
5. Tests → `tests/`
6. Update → `machineDecoupeIHM.pro` (HEADERS/SOURCES)

**New Drawing Tool/Mode:**
1. Tool implementation → `ui/canvas/` or `ui/canvas/tools/`
2. Mode definition → Update `DrawModeManager.h` enum
3. Rendering → Update `ShapeRenderer.h/cpp`
4. Interaction → Update `MouseInteractionHandler.h/cpp`

**New Domain Algorithm:**
1. Pure logic → `domain/geometry/` or `domain/shapes/`
2. Avoid Qt UI dependencies; prefer Qt containers + standard library
3. If infrastructure needed → Define interface in `domain/interfaces/`

**New Hardware Integration:**
1. Interface → `domain/interfaces/` (e.g., `INewService.h`)
2. Implementation → `infrastructure/hardware/`, `network/`, or `persistence/`
3. Inject via constructor or factory

---

*Structure updated: 2026-04-28*
