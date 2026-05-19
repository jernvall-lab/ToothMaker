@echo off
REM Internal helper. Sourced by dev-shell.bat / run-build.bat / run-deploy.bat
REM with `call _setup-vcvars.bat`. Sets up MSVC + Qt environment for the host.
REM
REM Host arch selection:
REM   AMD64 host: vcvars64.bat              — native x64 build (the common case).
REM   ARM64 host: vcvarsarm64_amd64.bat     — cross-compile from ARM64 to x64
REM                                          (binaries run under Prism emulation).
REM
REM vcvars internally calls vswhere.exe, which Microsoft does NOT put on PATH by
REM default. We prepend the Installer dir so the call resolves.
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"

if /i "%PROCESSOR_ARCHITECTURE%"=="AMD64" set "VCVARS_SCRIPT=vcvars64.bat"
if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" set "VCVARS_SCRIPT=vcvarsarm64_amd64.bat"
if not defined VCVARS_SCRIPT (
    echo ERROR: Unsupported host architecture: %PROCESSOR_ARCHITECTURE%
    exit /b 1
)

REM Standard Build Tools install locations. If you have Community/Professional/
REM Enterprise editions of Visual Studio, add their paths here.
set "VCVARS_X86=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\%VCVARS_SCRIPT%"
set "VCVARS_PF=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\%VCVARS_SCRIPT%"
if exist "%VCVARS_X86%" (
    call "%VCVARS_X86%"
) else if exist "%VCVARS_PF%" (
    call "%VCVARS_PF%"
) else (
    echo ERROR: %VCVARS_SCRIPT% not found in standard Build Tools locations
    exit /b 1
)

set "PATH=C:\Qt\6.10.3\msvc2022_64\bin;%PATH%"
