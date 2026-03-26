@echo off
setlocal enabledelayedexpansion

REM Set up environment
cd /d "C:\Users\elias\Desktop\codeQT\Eau-couper_by_AE"
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat" > /dev/null 2>&1

REM Go to build directory
cd /d "C:\Users\elias\Desktop\codeQT\Eau-couper_by_AE\build\Desktop_Qt_6_10_2_MSVC2022_64bit-Debug"

REM Clean object files to ensure fresh build of modified files
del debug\Inventory.obj 2>/dev/null
del debug\InventoryViewModel.obj 2>/dev/null
del debug\InventoryRepository.obj 2>/dev/null
del debug\MainWindow*.obj 2>/dev/null
del debug\MainWindowCoordinator.obj 2>/dev/null
del debug\main.obj 2>/dev/null

REM Run nmake
nmake -f Makefile.Debug

REM Check if build succeeded
if exist debug\machineDecoupeIHM.exe (
    echo.
    echo ===== BUILD SUCCESSFUL =====
    echo Executable: %CD%\debug\machineDecoupeIHM.exe
    exit /b 0
) else (
    echo.
    echo ===== BUILD FAILED =====
    exit /b 1
)
