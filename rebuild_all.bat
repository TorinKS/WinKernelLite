@echo off
echo Building WinKernelLite from scratch...

:: Clean up previous builds
echo Cleaning up previous builds...
if exist build rmdir /s /q build
if exist build-test rmdir /s /q build-test

:: Create build directory
echo Creating build directory...
mkdir build
cd build

:: Configure
echo Configuring project...
cmake ..
if %ERRORLEVEL% neq 0 (
    echo Failed to configure the project.
    cd ..
    exit /b 1
)

:: Build
echo Building project...
cmake --build . --config Debug
if %ERRORLEVEL% neq 0 (
    echo Failed to build the project.
    cd ..
    exit /b 1
)

:: Install
echo Installing project...
cmake --install . --prefix "C:/Program Files (x86)/WinKernelLite"
if %ERRORLEVEL% neq 0 (
    echo Failed to install the project.
    cd ..
    exit /b 1
)

:: Set up environment for examples
echo Setting up environment for examples...
cd ..
call setup_examples_env.bat

:: Build and run examples
echo Building and running examples...
cd examples\DevicesExample
call build_and_run.bat
cd ..\..

cd examples\VersionInfo
call build_and_run.bat
cd ..\..

cd examples\DevicesList
call build_and_run.bat
cd ..\..

cd examples\VersionInfo
call build_and_run.bat
cd ..\..

echo Build complete!
