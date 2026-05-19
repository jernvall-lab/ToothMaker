@echo off
setlocal
call "%~dp0_setup-vcvars.bat"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
cd build
echo ===QMAKE===
qmake ..\ToothMaker.pro CONFIG+=release
if errorlevel 1 (echo QMAKE FAILED & exit /b 1)
echo ===NMAKE===
nmake
echo ===NMAKE_EXIT=%errorlevel%===
