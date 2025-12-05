@echo off
REM ========================================
REM WinKernelLite Build Script
REM ========================================
REM This script builds the WinKernelLite library
REM Usage: _build.cmd [Debug|Release]
REM Default: Debug
REM ========================================

set BUILD_CONFIG=Debug

REM Parse arguments
if not "%1"=="" (
    if /i "%1"=="Debug" (
        set BUILD_CONFIG=Debug
    ) else if /i "%1"=="Release" (
        set BUILD_CONFIG=Release
    ) else (
        echo ERROR: Invalid build configuration. Use Debug or Release.
        echo Usage: _build.cmd [Debug^|Release]
        exit /b 1
    )
)

echo ========================================
echo Building WinKernelLite library in %BUILD_CONFIG% mode...
echo ========================================

call _run_all.cmd %BUILD_CONFIG% --build-only

if %ERRORLEVEL% neq 0 (
    echo ERROR: Execution failed!
    exit /b %ERRORLEVEL%
)

echo ========================================
echo Build, test, and installation completed successfully!
echo Configuration: %BUILD_CONFIG%
echo ========================================