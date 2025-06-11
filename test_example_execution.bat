@echo off
echo Testing that examples build and run correctly from build directory

REM Create a clean build directory
if exist build_run_test rmdir /s /q build_run_test
mkdir build_run_test
cd build_run_test

REM Configure the project (with BUILD_EXAMPLES=ON but INSTALL_EXAMPLES=OFF)
echo Configuring CMake...
cmake -DBUILD_EXAMPLES=ON -DINSTALL_EXAMPLES=OFF ..

REM Build the project
echo Building the project...
cmake --build . --config Release

REM Check if examples were built
echo Checking if examples were built...
if not exist "bin\examples_list.exe" (
    if exist "bin\devices_list.exe" (
        echo SUCCESS: DevicesList example was built correctly as 'devices_list.exe'
    ) else (
        echo ERROR: DevicesList example was not built
        exit /b 1
    )
)

if not exist "bin\version_info.exe" (
    if exist "bin\version_info_extended.exe" (
        echo SUCCESS: VersionInfo example was built correctly as 'version_info_extended.exe'
    ) else (
        echo ERROR: VersionInfo example was not built
        exit /b 1
    )
)

if not exist "bin\devices_example.exe" (
    echo ERROR: DevicesExample example was not built
    exit /b 1
) else (
    echo SUCCESS: DevicesExample example was built correctly
)

REM Test that examples run correctly
echo Running examples from build directory...

echo Running DevicesList example...
bin\devices_list.exe
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: DevicesList example failed to run
    exit /b 1
) else (
    echo SUCCESS: DevicesList example ran successfully
)

echo Running VersionInfo example...
bin\version_info_extended.exe
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: VersionInfo example failed to run
    exit /b 1
) else (
    echo SUCCESS: VersionInfo example ran successfully
)

echo Running DevicesExample example...
bin\devices_example.exe
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: DevicesExample example failed to run
    exit /b 1
) else (
    echo SUCCESS: DevicesExample example ran successfully
)

REM Navigate back to the root directory
cd ..

echo.
echo All examples build and run correctly from the build directory!
