@echo off
echo Building and running WinKernelLite version example...
echo.

REM Check if the library is installed
if not exist "C:\Program Files (x86)\WinKernelLite\lib\cmake\WinKernelLite\WinKernelLiteConfig.cmake" (
    echo WinKernelLite is not installed. Installing now...
    cmake --install build --component WinKernelLite
    if ERRORLEVEL 1 (
        echo Failed to install WinKernelLite. Please run:
        echo cmake -B build
        echo cmake --build build
        echo cmake --install build --component WinKernelLite
        goto :eof
    )
)

REM Create build directory for the example
if not exist examples\build mkdir examples\build 2>nul

REM Build the example
cd examples\build
cmake .. -DCMAKE_PREFIX_PATH="C:/Program Files (x86)/WinKernelLite"
if ERRORLEVEL 1 (
    echo Failed to configure the example.
    cd ..\..
    goto :eof
)

cmake --build . --config Debug
if ERRORLEVEL 1 (
    echo Failed to build the example.
    cd ..\..
    goto :eof
)

echo.
echo Running version example:
echo ======================
if exist Debug\version_info.exe (
    Debug\version_info.exe
) else if exist version_info.exe (
    version_info.exe
) else (
    echo Could not find the version_info executable.
)

echo.
pause

REM Return to the original directory
cd ..\..
