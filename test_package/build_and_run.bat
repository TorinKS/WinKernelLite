@echo off
echo Building test project to verify WinKernelLite installation...

REM Load environment variables including correct installation path
call %~dp0..\set_environment.bat silent

mkdir build 2>nul
cd build

echo Configuring project...
cmake .. -DCMAKE_PREFIX_PATH="%INSTALL_PATH%"
if ERRORLEVEL 1 (
    echo Failed to configure test project
    cd ..
    exit /b 1
)

echo Building project...
cmake --build . --config Debug
if ERRORLEVEL 1 (
    echo Failed to build test project
    cd ..
    exit /b 1
)

echo.
echo Running test...
if exist Debug\test_version.exe (
    Debug\test_version.exe
) else if exist test_version.exe (
    test_version.exe
) else (
    echo Could not find the test executable.
    cd ..
    exit /b 1
)

cd ..
echo Test completed successfully
