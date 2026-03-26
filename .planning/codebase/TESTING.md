# Testing Patterns

**Analysis Date:** 2026-03-26

## Test Framework

**Runner:**
- Qt Test Framework (QTestLib)
- Config: `tests/tests.pro` (qmake-based test project)
- Version: Qt6 (implicit from main project)
- Located in: `/home/gand/Eau-couper_by_AE/tests/`

**Assertion Library:**
- Qt Test macros: `QVERIFY()`, `QCOMPARE()`, `QTEST()` (Qt's built-in)
- No external assertion library (pure Qt Test)

**Run Commands:**
```bash
# Build tests
cd tests/
qmake6 tests.pro
make -j$(nproc)

# Run all tests
./tests

# Run specific test class
./tests PlacementTests
./tests ShapeManagerTests
./tests InventorySafetyTests

# Run with verbose output
./tests -v1
```

## Test File Organization

**Location:**
- Separate directory: `/home/gand/Eau-couper_by_AE/tests/`
- Not co-located with source code
- Tests compiled into single executable: `placement_tests` (output of `tests.pro`)

**Naming Convention:**
- Test files suffixed with `_tests`: `placement_tests.cpp`, `shape_manager_tests.cpp`, `inventory_safety_tests.cpp`
- Header files use same suffix: `placement_tests.h`, `shape_manager_tests.h`, `inventory_safety_tests.h`
- Main test entry point: `main.cpp` (registers all test classes)

**Project File Structure:**
```
tests/
├── tests.pro                    # qmake project file for tests
├── main.cpp                     # Test runner (instantiates and runs all test classes)
├── placement_tests.h            # Test class declaration
├── placement_tests.cpp          # Geometric/placement algorithm tests
├── shape_manager_tests.h        # Test class declaration
├── shape_manager_tests.cpp      # ShapeManager functionality tests
├── inventory_safety_tests.h     # Test class declaration
└── inventory_safety_tests.cpp   # Geometry validation & safety tests
```

## Test Structure

**Suite Organization:**
- Qt Test uses class-based model: Each test class inherits `QObject`
- Test methods declared as `private slots` in class
- Each method is one test case (not grouped by test suites)

**Test Class Example** (`placement_tests.h`):
```cpp
#pragma once
#include <QObject>

class PlacementTests : public QObject {
    Q_OBJECT
private slots:
    void borderContactNotOverlap();
    void interiorIntersection();
    void motorControlSteps();
    void normalizationRemovesDuplicates();
    void complexityGuardRejects();
    void stressWorstCase();
    void lowEndModeAdjusts();
    void selfIntersectingAccepted();
    void selfIntersectingOverlapDetected();
};
```

**Test Case Example** (`placement_tests.cpp`):
```cpp
void PlacementTests::borderContactNotOverlap()
{
    QPainterPath a; a.addRect(0,0,10,10);
    QPainterPath b; b.addRect(10,0,10,10);
    QVERIFY(!pathsOverlap(a,b));
}
```

**Test Runner** (`main.cpp`):
```cpp
#include <QtTest>
#include "placement_tests.h"
#include "inventory_safety_tests.h"
#include "shape_manager_tests.h"

int main(int argc, char **argv)
{
    int status = 0;
    {
        PlacementTests tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        InventorySafetyTests tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        ShapeManagerTests tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
```

**Patterns:**
- Setup: Inline initialization (no `setUp()` method)
- Teardown: Implicit (rely on Qt object lifecycle)
- Assertion: Qt Test macros immediately after action
- No test fixtures or factories in current implementation

## Mocking

**Framework:**
- No mocking library detected (Qt Test doesn't include mocks)
- Mocking done manually via direct object instantiation
- Tests instantiate real objects and verify behavior directly

**Mocking Patterns:**
```cpp
void ShapeManagerTests::addAndRemoveShape()
{
    ShapeManager manager;  // Create real object (not mocked)
    QPainterPath p;
    p.moveTo(0, 0);
    p.lineTo(10, 0);

    manager.addShape(p, 1);
    QCOMPARE(manager.shapes().size(), 1);

    QVERIFY(manager.removeShape(0));
    QCOMPARE(manager.shapes().size(), 0);
}
```

**What to Mock:**
- Minimal mocking strategy
- Direct instantiation of objects under test
- No external dependencies in test scope (geometry is pure logic)
- No database/file system mocking (tests use in-memory objects)

**What NOT to Mock:**
- Core geometry functions (`pathsOverlap()`, `normalizePath()`)
- Shape data structures (`QPainterPath`, `QPolygonF`)
- Motor control (uses real `MotorControl` class in tests - see `motorControlSteps()`)

## Fixtures and Factories

**Test Data:**
```cpp
void PlacementTests::borderContactNotOverlap()
{
    QPainterPath a; a.addRect(0,0,10,10);      // Fixture: simple rect
    QPainterPath b; b.addRect(10,0,10,10);     // Fixture: adjacent rect
    QVERIFY(!pathsOverlap(a,b));               // Assert: no overlap at border
}
```

**Common Fixtures:**
- `QPainterPath` shapes (rects, stars, complex polygons)
- `QPolygonF` collections for polygon validation
- `ShapeManager` instances with pre-loaded shapes
- Constants like `kMaxPathElements` for boundary testing

**Location:**
- Fixtures inline in test methods (no separate factory classes)
- No external test data files
- Deterministic data created programmatically

**Example Complex Fixture** (from `inventory_safety_tests.cpp`):
```cpp
void InventorySafetyTests::hugePolygonHandled()
{
    QList<QPolygonF> polys;
    QPolygonF p;
    p << QPointF(0,0);
    for (int i = 1; i < 20000; ++i)
        p << QPointF(i,0);
    p << QPointF(19999,1) << QPointF(0,1) << QPointF(0,0);
    polys << p;
    QVERIFY(sanitizePolygons(polys));
}
```

## Coverage

**Requirements:**
- No coverage target enforced (no `.coveragerc` or similar detected)
- No CI/CD instrumentation visible

**Current Coverage Status:**
- Tests cover critical paths:
  - Geometry overlap detection (tier 0-3 algorithms)
  - Path complexity limits
  - Polygon validation and sanitization
  - Shape manager add/remove/selection
  - Undo/redo state management
  - Motor control step calculation
- Untested areas: UI rendering, file I/O, network communication

**View Coverage:**
- Manual inspection of test output:
```bash
cd tests/
make
./tests -v2
```

## Test Types

**Unit Tests:**
- **Scope:** Individual function/class behavior
- **Approach:** Test in isolation with fixtures
- **Examples:**
  - `borderContactNotOverlap()` - tests `pathsOverlap()` with non-overlapping geometry
  - `addAndRemoveShape()` - tests `ShapeManager` add/remove operations
  - `motorControlSteps()` - tests step calculation in `MotorControl`

**Integration Tests:**
- **Scope:** Multiple components working together
- **Approach:** Real object composition, no mocks
- **Examples:**
  - `selectionBounds()` - tests `ShapeManager` selection + bounds calculation
  - `undoRestoresPreviousState()` - tests state management across operations
  - `malformedShapeUsesProxy()` - tests validation + fallback behavior

**E2E Tests:**
- **Framework:** Not used
- **Status:** No end-to-end tests detected
- **Rationale:** Hardware coupling (Raspberry Pi GPIO, motor control) makes E2E testing impractical in unit test suite

## Common Patterns

**Assertion Pattern:**
```cpp
// Arrange
QPainterPath a; a.addRect(0,0,10,10);
QPainterPath b; b.addRect(5,5,10,10);

// Act (implicit in pathsOverlap call below)

// Assert
QVERIFY(pathsOverlap(a,b));
```

**Async Testing:**
- No async operations in tests
- All geometry tests are synchronous
- Motor control tests use direct function calls (no event loop)

**Performance Testing:**
```cpp
void PlacementTests::stressWorstCase()
{
    QPainterPath a; a.moveTo(0,0);
    for(int i=0;i<20000;++i) a.lineTo(i+1, (i%2));  // Complex path
    QPainterPath b; b.addRect(20010,0,10,10);        // Far away
    QElapsedTimer t; t.start();
    QVERIFY(!pathsOverlap(a,b));
    QVERIFY(t.elapsed() < 50);                        // Must complete in <50ms
}
```

**Error Testing:**
```cpp
void PlacementTests::complexityGuardRejects()
{
    QPainterPath p;
    p.moveTo(0,0);
    for (int i = 0; i < kMaxPathElements + 1; ++i)
        p.lineTo(i+1,0);
    QVERIFY(isPathTooComplex(p, kMaxPathElements));   // Verify guard works
}
```

**Mode/Flag Testing:**
```cpp
void PlacementTests::lowEndModeAdjusts()
{
    setLowEndMode(true);                              // Enable low-end mode
    QPainterPath a; a.addRect(0,0,10,10);
    QPainterPath b; b.addRect(5,5,10,10);
    QVERIFY(pathsOverlap(a,b));
    PipelineMetrics m = lastPipelineMetrics();
    QVERIFY(m.rasterResolution <= 512);               // Check adaptive behavior
    setLowEndMode(false);                             // Restore
}
```

## Build Configuration

**Test Project File** (`tests/tests.pro`):
```pro
QT += testlib widgets
CONFIG += console testcase
TARGET = placement_tests
SOURCES += placement_tests.cpp \
           inventory_safety_tests.cpp \
           shape_manager_tests.cpp \
           main.cpp \
           ../MotorControl.cpp \
           ../GeometryUtils.cpp \
           ../drawing/ShapeManager.cpp
INCLUDEPATH += ..
HEADERS += ../MotorControl.h \
           placement_tests.h \
           inventory_safety_tests.h \
           shape_manager_tests.h \
           ../drawing/ShapeManager.h
```

**Key Points:**
- Includes actual source `.cpp` files to test (not dynamic linking)
- Qt test library linked via `QT += testlib`
- Console application (no GUI)
- Pulls in necessary headers via `INCLUDEPATH`
- Links geometry and motor control directly for unit testing

---

*Testing analysis: 2026-03-26*
