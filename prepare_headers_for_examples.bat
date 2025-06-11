@echo off
:: This script links the library headers in a way that examples can find them
:: during the build and installation process.

echo Preparing WinKernelLite header paths for examples...

:: Detect required paths
set SOURCE_DIR=%~dp0
set INCLUDE_DIR=%SOURCE_DIR%include
set BUILD_DIR=%SOURCE_DIR%build
set BUILD_INCLUDE_DIR=%BUILD_DIR%\include

:: Ensure directory structure exists
if not exist "%BUILD_INCLUDE_DIR%\WinKernelLite" mkdir "%BUILD_INCLUDE_DIR%\WinKernelLite"

:: Copy all headers to the proper location
echo Copying headers to build tree...
for %%F in ("%INCLUDE_DIR%\*.h") do (
    copy "%%F" "%BUILD_INCLUDE_DIR%\WinKernelLite\" > NUL
)

:: Also copy the Version.h file if it exists
if exist "%BUILD_INCLUDE_DIR%\WinKernelLite\Version.h" (
    echo Found Version.h, no need to copy
) else if exist "%BUILD_DIR%\include\WinKernelLite\Version.h" (
    copy "%BUILD_DIR%\include\WinKernelLite\Version.h" "%BUILD_INCLUDE_DIR%\WinKernelLite\" > NUL
    echo Copied Version.h from build directory
)

echo Finished preparing header paths.
echo.
echo Headers are now available at:
echo - %BUILD_INCLUDE_DIR%\WinKernelLite\
echo.
echo You can now build examples with proper include paths.
