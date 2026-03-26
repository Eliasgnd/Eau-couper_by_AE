# Codebase Concerns

**Analysis Date:** 2026-03-26

## Technical Debt

### Motor Calibration TODO
- **Issue:** Hardcoded `mmPerStep` calibration constant with TODO comment indicating it's never been properly calibrated for the machine
- **Files:** `infrastructure/hardware/MotorControl.cpp:10`
- **Impact:** Motor movements may be inaccurate, potentially affecting cut quality and placement precision
- **Fix approach:** Implement calibration procedure with user interface to measure actual mm/step ratio on the machine and save to persistent config

### Debug Logging Left in Production Code
- **Issue:** Extensive `qDebug()` statements scattered throughout critical hardware execution paths, particularly in `TrajetMotor::executeTrajet()` which runs during active cutting
- **Files:**
  - `infrastructure/hardware/TrajetMotor.cpp:133, 237, 242, 303, 304`
  - `ui/widgets/CustomEditor.cpp:460`
  - `ui/widgets/ShapeVisualization.cpp:227`
- **Impact:** Performance degradation during cutting operations, unnecessary console output, code maintainability issues
- **Fix approach:** Remove debug output from production paths or wrap in proper debug-only compile flags (Q_DEBUG_OUTPUT)

### QApplication::topLevelWidgets() Widget Discovery Pattern
- **Issue:** Multiple UI widgets use unsafe pattern of iterating through all top-level widgets to find MainWindow instance at runtime
- **Files:**
  - `ui/widgets/CustomEditor.cpp:39-48`
  - `ui/widgets/Inventory.cpp:49-58`
- **Impact:** Fragile widget lookups that fail silently if MainWindow doesn't exist, no type safety, doesn't scale with multiple windows
- **Fix approach:** Pass MainWindow reference explicitly through constructor or use dependency injection container

## Known Bugs

### TrajetMotor MainWindow Null Pointer Check Inconsistency
- **Symptoms:** Code checks `if (m_mainWindow)` before invoking methods, but also later invokes methods without null check via lambda at line 252
- **Files:** `infrastructure/hardware/TrajetMotor.cpp:237-246, 252-271`
- **Trigger:** When cutting completes/stops and UI was closed or MainWindow was destroyed during operation
- **Workaround:** MainWindow stays alive during execution due to reference counting, but not guaranteed by architecture

### Singleton Pattern in Inventory Widget
- **Symptoms:** Static `Inventory::instance` member exists at line 64 but pattern is incomplete
- **Files:** `ui/widgets/Inventory.cpp:64`
- **Risk:** Only one global instance, no cleanup handling, potential memory leak if widget is recreated, no thread safety

### Dynamic Casting in Critical Path
- **Symptoms:** Multiple dynamic casts to `QGraphicsItem` subclasses in ShapeVisualization widget without null validation
- **Files:** `ui/widgets/ShapeVisualization.cpp:54-61`
- **Trigger:** If shape model returns unexpected item type
- **Workaround:** Currently works because ShapeModel is tightly controlled, but fragile to schema changes

## Architecture Violations (MVVM)

### Critical MVVM Violation: Direct UI to Service Access
- **Issue:** UI widgets directly instantiate and call business logic services, bypassing ViewModel
- **Files:**
  - `ui/widgets/Inventory.cpp:74-77` - Creates InventoryModel and InventoryController directly
  - `ui/dialogs/WifiConfigDialog.cpp` - Direct WiFi service calls
- **Impact:** Cannot unit test UI independently, business logic tightly coupled to UI layer
- **Fix approach:** Pass fully initialized services through constructor; ViewModel should be single point of contact from UI

### Cross-Widget Communication via Global Lookups
- **Issue:** Widgets reach across module boundaries using unsafe `qobject_cast` searches instead of proper dependency injection
- **Files:** CustomEditor.cpp, Inventory.cpp (see above)
- **Impact:** Hidden dependencies, fragile to refactoring, impossible to provide mock implementations for testing
- **Fix approach:** Inject MainWindow/coordinator references at construction time

### TrajetMotor Tight Coupling to UI
- **Issue:** Hardware control layer (`infrastructure/hardware/TrajetMotor`) directly holds references to ShapeVisualization UI widget and MainWindow
- **Files:** `infrastructure/hardware/TrajetMotor.h:21, TrajetMotor.cpp:11, 240-246`
- **Impact:** Hardware layer cannot be tested without UI; UI changes force hardware changes; violates layered architecture
- **Fix approach:** Introduce interface layer for visualization callbacks; hardware should not know about UI

### QMetaObject::invokeMethod Usage Pattern
- **Issue:** Code uses `QMetaObject::invokeMethod()` extensively (51 occurrences) for cross-thread communication with lambda captures, particularly in TrajetMotor
- **Files:**
  - `infrastructure/hardware/TrajetMotor.cpp:240, 244, 252`
  - Throughout `ui/widgets/` and coordinators
- **Impact:** Hard to debug, lambda captures can outlive referenced objects, no compile-time type safety
- **Fix approach:** Replace with proper signal/slot connections; use Qt signal forwarding for cross-thread communication

## Security Considerations

### Unchecked File Operations in Inventory
- **Issue:** Image import and file operations in CustomEditor and Inventory don't validate file contents before processing
- **Files:**
  - `ui/widgets/CustomEditor.cpp` - Image imports
  - `ui/widgets/Inventory.cpp` - Shape file operations
- **Risk:** Malformed image/shape files could crash app or cause undefined behavior
- **Current mitigation:** Qt's image loaders validate format, but custom parsers (SVG, shape definitions) may not
- **Recommendations:** Add file size limits, validate image dimensions, implement try-catch for shape parsing

### OpenAI API Key in Environment
- **Issue:** OpenAI integration reads API key from environment variable with no validation
- **Files:** `infrastructure/network/OpenAIService.cpp`
- **Risk:** Key could be logged, passed insecurely, or left in process memory
- **Current mitigation:** Key only in memory-based environment
- **Recommendations:** Implement secure credential storage, add key rotation support, sanitize logging

### GPIO Direct Access
- **Issue:** GPIO operations in Raspberry.cpp directly access hardware without permission validation
- **Files:** `infrastructure/hardware/Raspberry.cpp`
- **Risk:** On actual hardware, requires privileged access; no error recovery for permission denied
- **Current mitigation:** Runs as embedded application with expected permissions
- **Recommendations:** Add graceful degradation for non-privileged environments, implement hardware capability detection

## Performance Bottlenecks

### Segment Nearest Neighbor Search (O(n²))
- **Problem:** TrajetMotor's path optimization uses nested loop to find nearest unvisited segment for each cut
- **Files:** `infrastructure/hardware/TrajetMotor.cpp:86-100, 191-199`
- **Cause:** Linear search through segments on every iteration; no spatial indexing
- **Impact:** For designs with >100 segments, visible delay in cut planning phase
- **Improvement path:** Build spatial index (KD-tree or grid) at start; use efficient nearest-neighbor query

### Graphics Scene Rendering on Every Shape Change
- **Problem:** ShapeVisualization::redraw() regenerates entire scene graph for all shapes
- **Files:** `ui/widgets/ShapeVisualization.cpp`
- **Impact:** Laggy UI response when manipulating many shapes (>50); unnecessary redraws during animation
- **Improvement path:** Implement incremental updates, scene graph caching, dirty region tracking

### Floating Point Conversion Overhead
- **Problem:** Multiple conversions between mm/pixel/steps throughout geometry calculations
- **Files:** `infrastructure/hardware/TrajetMotor.cpp`, `drawing/tools/ImportedImageGeometryHelper.cpp`
- **Impact:** Precision loss accumulated through transform pipeline
- **Improvement path:** Use single coordinate system internally; convert only at boundaries

### KeyboardDialog Rendering
- **Problem:** Virtual keyboard redraws entire button grid on every interaction
- **Files:** `ui/widgets/KeyboardDialog.cpp` (~661 lines)
- **Impact:** Slow touch response on Raspberry Pi with weak GPU
- **Improvement path:** Cache button rendering, use sprite sheets, implement button state caching

## Fragile Areas

### CustomDrawArea State Management
- **Files:** `drawing/CustomDrawArea.cpp` (~656 lines)
- **Why fragile:** Large monolithic widget managing drawing, undo/redo, multiple tools, and input handling
- **Safe modification:** Changes to one tool (e.g., TextTool) can affect entire drawing state; test all tools after modifications
- **Test coverage:** Gaps in undo/redo edge cases, multi-touch scenarios
- **Recommendation:** Refactor into tool strategy pattern with separate state machines per tool

### InventoryController Complex State Transitions
- **Files:** `application/InventoryController.cpp` (~408 lines)
- **Why fragile:** Coordinates multiple services (Model, Storage, Mutation, Query) with implicit state assumptions
- **Safe modification:** Must maintain invariant that storage is always synchronized with model; test round-trip persistence
- **Test coverage:** Limited integration tests between services
- **Recommendation:** Implement event sourcing or transaction logging to track state changes

### Geometry Transform Pipeline
- **Files:**
  - `domain/geometry/GeometryUtils.cpp` (~349 lines)
  - `drawing/ViewTransformer.cpp`
  - `ui/widgets/shapevisualization/GeometryTransformHelper.cpp`
- **Why fragile:** Coordinate transforms applied at multiple layers (scene, view, graphics items); easy to apply twice or miss
- **Safe modification:** Always test that final rendered position matches expected mm on cutting table
- **Test coverage:** Unit tests for individual transforms exist, but integration tests limited
- **Recommendation:** Create CoordinateContext object to track all active transforms; add validation assertions

### PathPlanner Integration
- **Files:** `drawing/tools/pathplanner.h/cpp` (~200+ lines, complex algorithm)
- **Why fragile:** TSP-like path optimization algorithm with heuristic nearest-neighbor; small changes affect all path orderings
- **Safe modification:** Changes must maintain property that output paths don't cross; regression test with image atlas
- **Test coverage:** No automated path validation tests
- **Recommendation:** Add continuous integration test comparing path crossings before/after changes

## Scaling Limits

### QGraphicsScene Item Limit
- **Current capacity:** ~1000-2000 QGraphicsItems before visible slowdown on Pi
- **Limit:** QGraphicsScene uses index structures that degrade after ~5000 items
- **Trigger:** Projects with many small shapes or fine detail paths
- **Scaling path:** Implement shape instancing/batch rendering; use custom rendering instead of individual items

### Memory for Large Image Imports
- **Current capacity:** Images up to ~8MB load without freeze
- **Limit:** Raspberry Pi 4 has ~2-4GB RAM; large batch imports can cause swapping
- **Trigger:** Importing multiple high-resolution images or large SVG files
- **Scaling path:** Implement progressive loading with thumbnails; add memory usage warnings

### Shape Count Limitation
- **Current capacity:** UI responsive up to ~100 shapes per layout
- **Limit:** Grid layout algorithm is O(n) and renders all shapes each frame
- **Trigger:** User creates grid of small shapes
- **Scaling path:** Implement lazy rendering, viewport culling, LOD system

## Test Coverage Gaps

### No Motor Control Testing
- **What's not tested:** MotorControl, TrajetMotor execution, hardware simulation
- **Files:** `infrastructure/hardware/MotorControl.cpp`, `infrastructure/hardware/TrajetMotor.cpp`
- **Risk:** Changes to path execution could produce wrong output on real machine without detection until physical test
- **Priority:** **HIGH** - This is safety-critical code

### Limited UI State Testing
- **What's not tested:** Complex UI interactions (undo/redo with multiple tools, multi-touch), keyboard handling edge cases
- **Files:** `ui/widgets/CustomEditor.cpp`, `drawing/CustomDrawArea.cpp`
- **Risk:** UI freezes, crashes during normal operation go undetected
- **Priority:** **MEDIUM** - Affects user experience

### Inventory Persistence Inconsistency Testing
- **What's not tested:** Crash-safe persistence (e.g., crash during save, corrupted storage file recovery)
- **Files:** `infrastructure/persistence/InventoryRepository.cpp`, `application/InventoryStorage.cpp`
- **Risk:** Data loss if application crashes during save operation
- **Priority:** **HIGH** - Affects user data integrity

### Coordinate Transform Integration Tests
- **What's not tested:** End-to-end geometry transforms (canvas → motor → physical output)
- **Files:** `domain/geometry/`, `drawing/ViewTransformer.cpp`, `ui/widgets/shapevisualization/`
- **Risk:** Subtle positioning errors only visible when cutting real material
- **Priority:** **HIGH** - Safety and quality critical

### Wifi/Bluetooth Communication Testing
- **What's not tested:** Network failures, incomplete message handling, concurrent connections
- **Files:** `infrastructure/network/WifiNmcliClient.cpp`, `ui/dialogs/BluetoothReceiverDialog.cpp`
- **Risk:** Silent failures or incomplete data transfer
- **Priority:** **MEDIUM** - Affects reliability

## Refactoring Priorities

### Priority 1: Extract Hardware Abstraction Layer (BLOCKING)
**Why:** TrajetMotor currently couples hardware operations to UI rendering, violating architecture
- Remove ShapeVisualization dependency from `infrastructure/hardware/`
- Define interface for visualization callbacks (progress, position)
- Inject visualization service rather than storing UI reference
- **Enables:** Unit testing of motor control, hardware simulation, code reuse

### Priority 2: Implement Dependency Injection Container (FOUNDATION)
**Why:** Current manual wiring of dependencies is error-prone and blocks MVVM enforcement
- Create DI container to manage service lifecycle
- Inject all services through constructors
- Remove global widget lookups
- **Benefits:** Testability, loose coupling, clearer architecture

### Priority 3: Remove Debug Output from Production Paths (CLEANUP)
**Why:** Scattered qDebug() calls degrade performance and clutter output
- Audit all qDebug() occurrences (starts with 10+ in critical paths)
- Wrap non-essential logs in Q_DEBUG or logging framework
- Add metrics collection for performance analysis
- **Benefits:** Cleaner production, measurable performance

### Priority 4: Implement Proper ViewModel Layer (ARCH)
**Why:** Some widgets (Inventory) have ViewModel, others (CustomEditor) don't; inconsistent MVVM
- Audit all UI widgets for direct business logic calls
- Create ViewModel for each major widget (CustomEditor, ShapeVisualization)
- Connect VM to coordinator, not directly to services
- **Benefits:** Consistent architecture, testability

### Priority 5: Add Motor Control Tests (SAFETY)
**Why:** No automated tests for hardware critical path
- Create mock motor control implementation
- Add unit tests for path optimization algorithm
- Add integration tests for cut sequence validation
- **Benefits:** Confidence in machine behavior, easier hardware changes

### Priority 6: Refactor Graphics Rendering (PERFORMANCE)
**Why:** Scene graph regeneration causes lag on large projects
- Implement dirty tracking for incremental updates
- Cache transformed geometries
- Use display lists or batched rendering
- **Benefits:** Smoother UI, faster preview, larger projects viable

---

*Concerns audit: 2026-03-26*
