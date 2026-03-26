# Coding Conventions

**Analysis Date:** 2026-03-26

## Naming Patterns

**Files:**
- C++ headers use `.h` extension (Qt convention): `ShapeManager.h`, `GeometryUtils.h`
- C++ implementation use `.cpp` extension: `ShapeManager.cpp`, `GeometryUtils.cpp`
- Prefer `#pragma once` over `#ifndef` guards in headers (see `GeometryUtils.h`, `InventoryModel.h`)
- Some legacy files still use `#ifndef` guards (see `CustomDrawArea.h`, `ShapeModel.h`)
- Qt Designer UI files use `.ui` extension (generated headers via Qt): `mainwindow.ui`, `Inventory.ui`

**Classes:**
- Use PascalCase for all class names: `ShapeManager`, `CustomDrawArea`, `InventoryController`
- Private members use `m_` prefix: `m_shapes`, `m_selectedShapes`, `m_repository`, `m_customShapes`
- Struct members do not use prefix: `path`, `originalId`, `rotationAngle` (see `ShapeManager::Shape`)
- Enums use `enum class` syntax (C++11): `enum class Type { Circle, Rectangle, Triangle, Star, Heart }`

**Functions:**
- Use camelCase for function names: `addShape()`, `removeShape()`, `pathsOverlap()`, `buildProxyPath()`
- Use camelCase for getter/setter methods: `shapes()`, `setShapes()`, `selectedShapes()`, `setSelectedShapes()`
- Qt slot methods use camelCase: `startCloseMode()`, `startGommeMode()`, `setSmoothingLevel()`
- Getter methods are const and return by reference/value: `const std::vector<Shape> &shapes() const`
- Private helper functions use camelCase: `pathStart()`, `pathEnd()`, `simplifyForProxyInternal()`

**Variables:**
- Local variables use camelCase: `largeur`, `longueur`, `startX`, `startY`, `budgetMs`
- Boolean variables use descriptive names: `hasSelection`, `initialized`, `changed`, `gLowEndMode`, `gSafeMode`
- Private static/global variables use `g` prefix: `gSimplifiedCache`, `gMetrics`, `gLowEndMode`, `gSafeMode`, `gEpsilon`
- Function parameters use camelCase: `epsilon`, `tol`, `res`, `parentFolder`

**Types & Constants:**
- Type aliases and struct names use PascalCase: `PipelineMetrics`, `ShapeValidationResult`, `LayoutData`, `CustomShapeData`
- Constants use UPPER_CASE: `kMaxPathElements` (see `GeometryUtils.h`)
- Qualified type names when necessary: `QRectF`, `QPainterPath`, `QPolygonF`, `std::vector`

## Code Style

**Formatting:**
- Indentation: 4 spaces (consistent across all files)
- Line length: Generally kept reasonable, but some lines exceed 120 chars in domain logic
- Braces: Opening brace on same line for control structures: `if (condition) {`
- Blocks: Spaces around control flow keywords: `if (`, `for (`, `while (`

**Linting:**
- No formal linting configuration detected (no `.clang-format`, `.clang-tidy`, or `CMakeLists.txt`)
- Code follows Qt conventions implicitly through qmake project files

**Qt-Specific Patterns:**
- Use `Q_OBJECT` macro for classes that need signals/slots: `class ShapeManager : public QObject { Q_OBJECT ... }`
- Use `Q_LOGGING_CATEGORY` for debug output: `Q_LOGGING_CATEGORY(lcShapeManager, "ae.draw.shape_manager")`
- Use `qDebug()` for debugging (commented out in production code): `//qDebug() << "..."`
- Prefer Qt container types: `QList<T>`, `QMap<K,V>`, `QSet<T>`, `QVector<T>` over STL when interacting with Qt
- Use `std::vector<T>` for internal domain logic when not interfacing with Qt (see `ShapeManager`): `std::vector<Shape> m_shapes`
- Mix of both STL and Qt containers in same codebase (pragmatic choice)

## Import Organization

**Order (C++ includes):**
1. Local headers from same directory: `#include "ShapeManager.h"`
2. Project headers (quoted): `#include "domain/shapes/ShapeModel.h"`
3. Qt headers (angled): `#include <QObject>`, `#include <QPolygonF>`
4. Standard library (angled): `#include <vector>`, `#include <cmath>`, `#include <functional>`
5. System headers (angled): (rarely used, project is Qt6-based)

**Example from `ShapeManager.cpp`:**
```cpp
#include "ShapeManager.h"
#include <QLineF>
#include <algorithm>
#include <limits>
#include <QLoggingCategory>
#include <QtMath>
```

**Path Aliases:**
- Project uses explicit include paths via `.pro` file `INCLUDEPATH` variable
- No C++17 namespace aliases detected
- Avoid deep relative paths; use absolute paths from `INCLUDEPATH`

## Error Handling

**Return Values:**
- Boolean returns for success/failure: `bool removeShape(int index)`, `bool validateAndProxyPolygons(...)`
- Functions that cannot fail return void: `void addShape(...)`, `void clearShapes()`
- Optional output parameters for extended information: `bool validateAndProxyPolygons(QList<QPolygonF> &polys, bool safeMode, QString *warning = nullptr, ...)`
- Use Qt's `const` reference returns for performance: `const std::vector<Shape> &shapes() const`

**Error Conditions:**
- Bounds checking with index validation: `if (index < 0 || index >= m_shapes.size()) return false;`
- Guard clauses for invalid input: `if (largeur <= 0 || longueur <= 0) { return shapes; }`
- Epsilon-based comparison for floating point: `double epsilon = 0.5` (configurable via `setGlobalEpsilon()`)
- Safe mode fallback for invalid shapes: Uses proxy geometry (bounding box) when shapes fail validation

**Assertions:**
- No explicit `assert()` calls in production code
- Validation functions return boolean with optional warning output instead
- Guard conditions are preferred over assertions for safety-critical machine control code

## Comments & Documentation

**When to Comment:**
- Explain the "why" not the "what": Commented code shows intent
- Complex algorithms documented with step numbers: See `ShapeModel.cpp` lines 59-96 (star shape scaling algorithm)
- French comments for application domain: `// Erreur: dimensions invalides, largeur ou longueur <= 0`
- English comments for technical/infrastructure code: `// Adaptive multi-tier overlap test` in `GeometryUtils.h`

**JSDoc/TSDoc Style (Qt):**
- Use `@class`, `@brief`, `@note`, `@see` Doxygen-style comments
- Single-line brief followed by `@note` for additional context:
```cpp
/**
 * @class ShapeManager
 * @brief Gère la liste des formes dessinées, la sélection et l'historique d'édition.
 *
 * @note Cette classe émet des signaux pour notifier les changements de formes
 * et de sélection.
 * @see CustomDrawArea
 */
```
- Function-level comments inline: `//! Déplacement à vide (jet OFF) – coordonnées absolues en mm`
- Parameter comments in French when part of domain model: `// vitesse de coupe`, `// vitesse de déplacement à vide`

**Block Comments:**
- Use `// ` for single-line comments with space after `//`
- Separate logical sections with comment blocks: `// ====== PLATEFORME LINUX / RASPBERRY PI ======`
- Disabled code left in as comments for reference (see `ShapeModel.cpp` lines 10-11): `//qDebug() << "generateShapes appelé..."`

## Function Design

**Size:**
- Small, focused functions preferred
- Most functions 10-30 lines
- Complex logic like `simplifyForProxyInternal` can be 30+ lines but is well-documented
- Lambdas used for inline comparison/sorting logic: `std::remove_if(m_selectedShapes.begin(), m_selectedShapes.end(), [index](int i) { return i == index; })`

**Parameters:**
- Pass by value for small types: `int index`, `bool enabled`
- Pass by const reference for complex types: `const QPainterPath &path`, `const std::vector<int> &indices`
- Output parameters use non-const reference: `QList<QPolygonF> &polys`, `QString *warning = nullptr`
- Avoid mutable reference parameters when possible (see `setSelectedShapes` copies indices)

**Return Values:**
- Single value types return by value: `double pathArea(...)`, `bool pathsOverlap(...)`
- Complex types return by const reference when performance critical: `const std::vector<Shape> &shapes() const`
- Use output parameters for multiple return values: `bool onCustomShapeSelected(..., QList<QPolygonF> &polygonsOut, QString &resolvedNameOut)`

## Module Design

**Exports:**
- Each module has clear public interface in `.h` file
- All public functions/classes declared in header
- Implementation details in `.cpp` (anonymous namespace used for private helpers):
```cpp
namespace {
// Private helpers
QPainterPath simplifyForProxyInternal(...) { ... }
} // namespace
```

**Barrel Files:**
- No barrel/index files detected
- Each header imports only what it needs (minimal coupling)

**Namespace Usage:**
- No explicit namespaces used (relies on file structure for organization)
- Anonymous namespaces for file-local helpers: `namespace { ... } // namespace` in `GeometryUtils.cpp`

**Initialization Patterns:**
- Qt objects initialized via parent chain: `explicit ShapeManager(QObject *parent = nullptr)`
- Default member initialization (C++11): `QPainterPath path;`, `int originalId = -1;`, `qreal rotationAngle = 0.0;`
- In-class initialization for private members: `bool m_ownsRepository {false};` (see `InventoryModel.h`)

## Architecture Rules (MVVM)

**Strict MVVM Adherence:**
- View (UI) classes in `ui/` directory must not contain business logic
- ViewModel classes in `viewmodels/` handle all state and emit signals
- Model classes in `domain/` contain only data structures
- View communicates to ViewModel only through signals/slots
- ViewModel never directly references View
- Domain logic separated from Qt dependencies where possible

**Example MVVM Flow:**
- `ui/widgets/CustomDrawArea.h` (View): Emits `void drawModeChanged(DrawMode mode)` signal
- `viewmodels/WorkspaceViewModel.h` (ViewModel): Holds state, emits `void languageChanged(Language lang)`
- `domain/shapes/ShapeManager.h` (Model): Pure data container with signal support for Qt integration

**Signal/Slot Convention:**
- Signals use descriptive names: `shapesChanged()`, `selectionChanged()`, `drawModeChanged()`
- Slot methods use verb form: `startCloseMode()`, `setSmoothingLevel(int level)`, `setTextFont(const QFont &font)`
- Use Qt's auto-connect when possible (signal name maps to slot name)

## Performance Patterns

**Caching:**
- Hash-based cache for simplified paths: `QHash<CacheKey, QPainterPath> gSimplifiedCache` in `GeometryUtils.cpp`
- Pointer-based cache keys for path identity: `struct CacheKey { const QPainterPath *ptr; }`

**Adaptive Algorithms:**
- Multi-tier overlap detection with early-exit optimization
- Low-end mode flag for Raspberry Pi performance tuning: `setLowEndMode(bool enabled)`
- Epsilon tolerance configurable per-device: `setGlobalEpsilon(double eps)`

**Memory Management:**
- Qt object ownership via parent chain (automatic deletion)
- Smart pointers not used (relies on Qt's parent/child relationship)
- Manual `delete` for heap-allocated items when necessary
- STL containers used for performance-critical sections: `std::vector<Shape>` for shape storage

---

*Convention analysis: 2026-03-26*
