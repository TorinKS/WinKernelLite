@echo off
echo Building and running the Devices List Example...

:: Create a build directory
if not exist build mkdir build
cd build

:: Configure the project
echo Configuring project...

:: Try to find WinKernelLite using find_package
echo Looking for installed WinKernelLite package...
cmake ..
if %ERRORLEVEL% neq 0 (
    echo.
    echo Failed to find installed WinKernelLite package.
    echo Please ensure WinKernelLite is installed by running:
    echo   cmake --build build --target install
    echo from the main project directory.
    echo.
    echo Alternatively, you can build from the main project which includes examples.
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
