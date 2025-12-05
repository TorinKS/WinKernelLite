@echo off
echo Building and running the Registry Example...

:: Create a build directory
if not exist build mkdir build
cd build

:: Clean any cached files to avoid configuration conflicts
if exist CMakeCache.txt del CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles

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
echo Running the registry example:
echo ============================
echo.
if exist Debug\registry_example.exe (
    echo Running registry_example...
    Debug\registry_example.exe
    echo.
    echo Running registry_simple_test...
    Debug\registry_simple_test.exe
) else if exist bin\Debug\registry_example.exe (
    echo Running registry_example...
    bin\Debug\registry_example.exe
    echo.
    echo Running registry_simple_test...
    bin\Debug\registry_simple_test.exe
) else (
    echo Could not find the example executable.
    cd ..
    exit /b 1
)

:: Return to the original directory
cd ..