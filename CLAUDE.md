# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

---

## Project Overview

Qt6/C++ GUI application for controlling a water-cutting machine ("Eau Couper").
Runs on a Raspberry Pi touchscreen.
Bilingual (French/English).

---

## Build Commands

```bash
# One-time dev environment setup (Ubuntu/Debian)
./scripts/setup_dev_ubuntu.sh

# Build the main application
qmake6 machineDecoupeIHM.pro
make -j$(nproc)

# Build and run tests
cd tests/
qmake6 tests.pro
make -j$(nproc)
./tests
```

Dependencies: Qt6 (widgets, svg, network, bluetooth, httpserver, openglwidgets), OpenCV 4, libgpiod.

---

## MVVM Architecture Rules (STRICT)

* View (ui/) must contain ONLY UI logic

* NO business logic allowed in QWidget or UI classes

* ViewModel must handle ALL logic and state

* Model must contain ONLY data (no logic)

* View → ViewModel communication: signals/slots only

* ViewModel → Model communication: direct or via services

* View MUST NOT directly access Model

* No direct calls from UI to domain/managers

* All interactions must go through ViewModel

* All new features MUST follow MVVM

* Existing code must be progressively refactored to MVVM

Violation of these rules is NOT allowed.

---

## Qt / qmake Rules

* ALWAYS update .pro file when adding new files

* NEVER break existing .pro structure

* Ensure all new .cpp/.h files are included in SOURCES/HEADERS

* Use qmake6 (not qmake)

* Maintain compatibility with Raspberry Pi environment

---

## Scope

Focus ONLY on:

* domain/          (business logic, models, geometry, interfaces)
* application/     (coordinators, services, factory)
* infrastructure/  (hardware, imaging, network, persistence)
* viewmodels/      (MVVM state management)
* ui/              (widgets, canvas, dialogs, mainwindow, utils)
* shared/          (cross-layer types: Language, PerformanceMode, ThemeManager)

Avoid unnecessary modifications outside these directories.

---

## Ignore Rules

Ignore the following files and directories:

* build/
* debug/
* release/
* cmake-build-*/
* Makefile*
* *.o
* *.obj
* *.log
* *.tmp
* .git/
* node_modules/

Do NOT read or modify these files.

---

## Code Quality

* Follow SOLID principles
* Prefer composition over inheritance
* Avoid duplicated logic
* Keep classes small and focused
* Use clear and explicit naming

---

## Refactoring Rules

* Do NOT break existing functionality
* Refactor step by step (incremental)
* Never refactor the entire project at once
* Always explain changes before applying them
* Ask for confirmation before major changes

---

## Build & Validation

After ANY modification:

1. Run:
   qmake6 machineDecoupeIHM.pro

2. Then:
   make -j$(nproc)

3. Fix ALL compilation errors

The project MUST always compile successfully.

---

## Testing

* Run tests after major changes
* Ensure no regression
* Maintain existing behavior

---

## AI Behavior Rules

* Always analyze before modifying code
* Always propose a plan before refactoring
* Work step by step
* Do NOT take destructive actions without confirmation
* Do NOT delete files unless explicitly approved
* Prefer minimal and safe changes

---

## Recommended Workflow

1. Analyze the project
2. Identify MVVM violations
3. Propose a refactoring plan
4. Refactor step by step
5. Build and fix errors
6. Validate behavior

---

## Important Notes

* This is a real industrial application (machine control)
* Stability and safety are critical
* Do NOT introduce risky or untested changes
* Maintain compatibility with existing hardware and logic

---
