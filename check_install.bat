@echo off
echo WinKernelLite Installation Check
echo ==============================
echo.

REM Use environment variables to detect the correct Program Files directory
if defined "ProgramFiles(x86)" (
    REM We're on a 64-bit system
    set "INSTALL_PATH=%ProgramFiles(x86)%\WinKernelLite"
) else (
    REM We're on a 32-bit system
    set "INSTALL_PATH=%ProgramFiles%\WinKernelLite"
)

REM Allow override via environment variable
if defined WINKERNELLITE_INSTALL_PATH (
    set "INSTALL_PATH=%WINKERNELLITE_INSTALL_PATH%"
)

echo Using installation path: %INSTALL_PATH%
echo.

echo Checking include directory...
if exist "%INSTALL_PATH%\include\gtest" (
    echo GTest headers found - NOT CORRECT!
) else (
    echo No GTest headers found - CORRECT!
)

echo.
echo Checking WinKernelLite headers...
if exist "%INSTALL_PATH%\include\WinKernelLite" (
    echo WinKernelLite headers found - CORRECT!
    dir "%INSTALL_PATH%\include\WinKernelLite"
) else (
    echo WinKernelLite headers NOT found - INCORRECT!
)

echo.
echo Checking CMake configuration...
if exist "%INSTALL_PATH%\lib\cmake\WinKernelLite" (
    echo WinKernelLite CMake config found - CORRECT!
    dir "%INSTALL_PATH%\lib\cmake\WinKernelLite"
) else (
    echo WinKernelLite CMake config NOT found - INCORRECT!
)

pause
