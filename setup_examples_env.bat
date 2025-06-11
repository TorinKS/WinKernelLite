@echo off
:: This script sets up the environment for building the examples
:: It will:
:: 1. Create the correct include directory structure in the build tree
:: 2. Copy the headers to the right locations
:: 3. Set up environment variables for building examples

echo Setting up environment for WinKernelLite examples...

:: Detect paths
set SOURCE_DIR=%~dp0
set INCLUDE_DIR=%SOURCE_DIR%include
set BUILD_DIR=%SOURCE_DIR%build
set BUILD_INCLUDE_DIR=%BUILD_DIR%\include
set INSTALL_DIR=C:\Program Files (x86)\WinKernelLite

:: Create directory structure
if not exist "%BUILD_INCLUDE_DIR%" mkdir "%BUILD_INCLUDE_DIR%"
if not exist "%BUILD_INCLUDE_DIR%\WinKernelLite" mkdir "%BUILD_INCLUDE_DIR%\WinKernelLite"

:: Copy headers to the build directory
echo Copying headers to build tree...
for %%F in ("%INCLUDE_DIR%\*.h") do (
    copy "%%F" "%BUILD_INCLUDE_DIR%\WinKernelLite\" > NUL
)

:: Copy generated Version.h if it exists
if exist "%BUILD_DIR%\include\WinKernelLite\Version.h" (
    copy "%BUILD_DIR%\include\WinKernelLite\Version.h" "%BUILD_INCLUDE_DIR%\WinKernelLite\" > NUL
)

:: Set environment variables
set CMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%;%INSTALL_DIR%;%BUILD_DIR%
set CMAKE_MODULE_PATH=%CMAKE_MODULE_PATH%;%SOURCE_DIR%examples
set INCLUDE=%INCLUDE%;%BUILD_INCLUDE_DIR%;%INSTALL_DIR%\include

echo Environment setup complete.
echo.
echo Added to CMAKE_PREFIX_PATH:
echo - %INSTALL_DIR%
echo - %BUILD_DIR%
echo.
echo Added to CMAKE_MODULE_PATH:
echo - %SOURCE_DIR%examples
echo.
echo Added to INCLUDE:
echo - %BUILD_INCLUDE_DIR%
echo - %INSTALL_DIR%\include
echo.
echo You can now build examples using:
echo   cd examples\DevicesExample
echo   .\build_and_run.bat
