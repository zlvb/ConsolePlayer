@echo off
chcp 65001 >nul

REM ============================================================
REM  ConsolePlayer Build Script
REM  Usage: build.bat [Release|Debug]
REM  Default: Release x64
REM ============================================================

setlocal enabledelayedexpansion

REM --- Parse arguments ---
set CONFIG=Release
if /i "%~1"=="Debug" set CONFIG=Debug
if /i "%~1"=="Release" set CONFIG=Release

set PLATFORM=x64
set SLN=%~dp0ConsolePlayer.sln

echo ============================================================
echo  ConsolePlayer Build - %CONFIG% ^| %PLATFORM%
echo ============================================================

REM --- Locate MSBuild ---
set MSBUILD=
for /f "delims=" %%i in ('dir /s /b "C:\Program Files\Microsoft Visual Studio\*MSBuild.exe" 2^>nul ^| findstr /i "Current\\Bin\\amd64"') do (
    set MSBUILD=%%i
)
if not defined MSBUILD (
    for /f "delims=" %%i in ('dir /s /b "C:\Program Files (x86)\Microsoft Visual Studio\*MSBuild.exe" 2^>nul ^| findstr /i "Current\\Bin\\amd64"') do (
        set MSBUILD=%%i
    )
)
if not defined MSBUILD (
    echo [ERROR] MSBuild not found. Please install Visual Studio with C++ workload.
    exit /b 1
)

echo  MSBuild: !MSBUILD!
echo  Solution: %SLN%
echo ============================================================
echo.

REM --- Build ---
"!MSBUILD!" "%SLN%" /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m /nologo /verbosity:minimal

if %ERRORLEVEL% neq 0 (
    echo.
    echo [FAILED] Build failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================================
echo  [SUCCESS] Build completed: %~dp0%PLATFORM%\%CONFIG%\ConsolePlayer.exe
echo ============================================================

endlocal
