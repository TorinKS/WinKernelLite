@echo off
setlocal

echo Building and running all WinKernelLite examples
echo ==============================================
echo.

:: First make sure the library is installed
if not exist "%INSTALL_PATH%" (
    echo Installing WinKernelLite first...
    cd ..
    mkdir build 2>nul
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX="%INSTALL_PATH%"
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
cmake .. -DCMAKE_PREFIX_PATH="%INSTALL_PATH%"
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
if exist bin\Debug\version_info_extended.exe (
    bin\Debug\version_info_extended.exe
) else if exist bin\version_info_extended.exe (
    bin\version_info_extended.exe
) else if exist Debug\version_info_extended.exe (
    Debug\version_info_extended.exe
) else (
    echo Could not find version_info_extended executable.
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
if exist bin\Debug\devices_list.exe (
    bin\Debug\devices_list.exe
) else if exist bin\devices_list.exe (
    bin\devices_list.exe
) else if exist ..\DevicesList\build\Debug\devices_list.exe (
    ..\DevicesList\build\Debug\devices_list.exe
) else (
    cd ..\DevicesList
    if exist build_and_run.bat (
        call build_and_run.bat
    ) else (
        echo build_and_run.bat not found for DevicesList example.
    )
    cd ..\build
)

echo.
echo All examples completed.
cd ..
