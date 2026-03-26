@echo off
setlocal enabledelayedexpansion
cd /d "C:\Users\elias\Desktop\codeQT\Eau-couper_by_AE"
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat" >/dev/null 2>&1
"C:\Qt\6.10.2\msvc2022_64\bin\qmake.exe" machineDecoupeIHM.pro 2>&1
exit /b %ERRORLEVEL%
