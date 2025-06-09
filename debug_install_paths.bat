@echo off
echo WinKernelLite Installation Path Debugging
echo ========================================
echo.

set INSTALL_PATH=C:/Program Files (x86)/WinKernelLite

echo Checking installed include directory...
if exist "%INSTALL_PATH%\include\WinKernelLite" (
    echo WinKernelLite include directory found
    dir "%INSTALL_PATH%\include\WinKernelLite"
) else (
    echo WinKernelLite include directory NOT found!
)

echo.
echo Checking CMake config files...
if exist "%INSTALL_PATH%\lib\cmake\WinKernelLite" (
    echo CMake config directory found
    dir "%INSTALL_PATH%\lib\cmake\WinKernelLite"
) else (
    echo CMake config directory NOT found!
)

echo.
echo Contents of WinKernelLiteConfig.cmake:
if exist "%INSTALL_PATH%\lib\cmake\WinKernelLite\WinKernelLiteConfig.cmake" (
    type "%INSTALL_PATH%\lib\cmake\WinKernelLite\WinKernelLiteConfig.cmake"
) else (
    echo WinKernelLiteConfig.cmake NOT found!
)

echo.
echo Contents of WinKernelLiteTargets.cmake:
if exist "%INSTALL_PATH%\lib\cmake\WinKernelLite\WinKernelLiteTargets.cmake" (
    type "%INSTALL_PATH%\lib\cmake\WinKernelLite\WinKernelLiteTargets.cmake"
) else (
    echo WinKernelLiteTargets.cmake NOT found!
)

pause
