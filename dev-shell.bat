@echo off
call "%~dp0_setup-vcvars.bat"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cmd /k
