@echo off
echo Building and running the DevicesList Example...

REM Load environment variables including correct installation path
call %~dp0..\..\set_environment.bat silent

:: Create a build directory
if not exist build mkdir build
cd build

:: Configure the project
echo Configuring project...

:: Set include paths for both install dir and source build 
set CMAKE_PREFIX_PATH=%INSTALL_PATH%;../../build
set CMAKE_MODULE_PATH=../../examples

:: Configure with both paths
cmake .. -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" -DCMAKE_MODULE_PATH="%CMAKE_MODULE_PATH%"
if %ERRORLEVEL% neq 0 (
    echo Failed to configure the project.
    cd ..
    exit /b 1
)

:: Build the project
echo Building project...
cmake --build . --config Debug
if %ERRORLEVEL% neq 0 (
    echo Failed to build the project.
    cd ..
    exit /b 1
)

:: Run the example
echo.
echo Running the example:
echo ===================
echo.
if exist Debug\devices_list.exe (
    Debug\devices_list.exe
) else if exist bin\Debug\devices_list.exe (
    bin\Debug\devices_list.exe
) else (
    echo Could not find the example executable.
    cd ..
    exit /b 1
)

:: Return to the original directory
cd ..
