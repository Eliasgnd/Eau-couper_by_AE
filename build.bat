@echo off
cd /d "C:\Users\elias\Desktop\codeQT\Eau-couper_by_AE"
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
"C:\Qt\6.10.2\msvc2022_64\bin\qmake.exe" machineDecoupeIHM.pro
if exist Makefile (
    echo QMAKE_SUCCESS
    nmake
) else (
    echo QMAKE_FAILED
)
