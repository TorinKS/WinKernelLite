@echo off
echo Building and running the Devices Example...

:: Create a build directory
if not exist build mkdir build
cd build

:: Configure the project
echo Configuring project...
cmake .. -DCMAKE_PREFIX_PATH="C:/Program Files (x86)/WinKernelLite"
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
if exist Debug\devices_example.exe (
    Debug\devices_example.exe
) else if exist bin\Debug\devices_example.exe (
    bin\Debug\devices_example.exe
) else (
    echo Could not find the example executable.
    cd ..
    exit /b 1
)

:: Return to the original directory
cd ..
