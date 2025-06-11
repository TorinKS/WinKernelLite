@echo off
echo Testing WinKernelLite installation with INSTALL_EXAMPLES=OFF

REM Create a clean build directory
if exist build_test rmdir /s /q build_test
mkdir build_test
cd build_test

REM Configure the project with INSTALL_EXAMPLES=OFF
echo Configuring CMake with INSTALL_EXAMPLES=OFF...
cmake -DINSTALL_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX="%CD%\install" ..

REM Build the project
echo Building the project...
cmake --build . --config Release

REM Install the project
echo Installing the project...
cmake --install . --config Release

REM Check if examples were installed
echo Checking installation directory...
if exist "install\bin\examples" (
    echo ERROR: Examples directory exists despite INSTALL_EXAMPLES=OFF
) else (
    echo SUCCESS: Examples were NOT installed (as expected)
)

REM Check if examples were built
echo Checking build directory...
if exist "bin\examples" (
    echo SUCCESS: Examples were built (as expected)
)

REM Navigate back to the root directory
cd ..

REM Now test with INSTALL_EXAMPLES=ON
echo.
echo Testing WinKernelLite installation with INSTALL_EXAMPLES=ON

REM Create a clean build directory
if exist build_test_on rmdir /s /q build_test_on
mkdir build_test_on
cd build_test_on

REM Configure the project with INSTALL_EXAMPLES=ON
echo Configuring CMake with INSTALL_EXAMPLES=ON...
cmake -DINSTALL_EXAMPLES=ON -DCMAKE_INSTALL_PREFIX="%CD%\install" ..

REM Build the project
echo Building the project...
cmake --build . --config Release

REM Install the project
echo Installing the project...
cmake --install . --config Release

REM Check if examples were installed
echo Checking installation directory...
if exist "install\bin\examples" (
    echo SUCCESS: Examples WERE installed (as expected)
) else (
    echo ERROR: Examples were not installed despite INSTALL_EXAMPLES=ON
)

REM Navigate back to the root directory
cd ..

echo.
echo Testing completed.
