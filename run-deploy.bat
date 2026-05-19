@echo off
setlocal
call "%~dp0_setup-vcvars.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0build\interface\release"
windeployqt --release --no-translations ToothMaker.exe
echo ===DEPLOY_EXIT=%errorlevel%===
