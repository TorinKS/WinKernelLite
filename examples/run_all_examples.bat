@echo off
setlocal

echo Building and running all WinKernelLite examples
echo ==============================================
echo.

:: First make sure the library is installed
if not exist "C:\Program Files (x86)\WinKernelLite" (
    echo Installing WinKernelLite first...
    cd ..
    mkdir build 2>nul
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX="C:/Program Files (x86)/WinKernelLite"
    cmake --build . --config Release
    cmake --install . --component WinKernelLite
    cd ..\examples
    echo.
)

:: Create a build directory for examples
mkdir build 2>nul
cd build

:: Configure the examples project
echo Configuring examples...
cmake .. -DCMAKE_PREFIX_PATH="C:/Program Files (x86)/WinKernelLite"
if %ERRORLEVEL% neq 0 (
    echo Failed to configure examples.
    cd ..
    exit /b 1
)

:: Build all examples
echo Building examples...
cmake --build . --config Debug
if %ERRORLEVEL% neq 0 (
    echo Failed to build examples.
    cd ..
    exit /b 1
)

:: Now run each example

echo.
echo Running Version Info Example:
echo ----------------------------
echo.
if exist bin\examples\Debug\version_info.exe (
    bin\examples\Debug\version_info.exe
) else if exist Debug\version_info.exe (
    Debug\version_info.exe
) else (
    echo Could not find version_info executable.
)

echo.
echo.
echo Running Devices Example:
echo ----------------------
echo.
cd ..\DevicesExample
call build_and_run.bat
cd ..\build

echo.
echo.
echo Running DevicesList Example:
echo -------------------------
echo.
cd ..\DevicesList
if exist build_and_run.bat (
    call build_and_run.bat
) else (
    echo build_and_run.bat not found for DevicesList example.
)
cd ..\build

echo.
echo All examples completed.
cd ..
