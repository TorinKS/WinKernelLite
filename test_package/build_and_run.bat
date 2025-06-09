@echo off
echo Building test project to verify WinKernelLite installation...

mkdir build 2>nul
cd build

echo Configuring project...
cmake .. -DCMAKE_PREFIX_PATH="C:/Program Files (x86)/WinKernelLite"
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
