@echo off
setlocal enabledelayedexpansion

echo WinKernelLite Build and Installation Diagnostic Tool
echo ===================================================
echo.

REM Check for CMake
echo Checking for CMake installation...
cmake --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake is not installed or not in PATH
    exit /b 1
) else (
    for /f "tokens=3" %%i in ('cmake --version ^| find "version"') do (
        echo Found CMake version: %%i
    )
)

echo.
echo Checking repository structure...
if not exist include (
    echo ERROR: 'include' directory not found
    exit /b 1
) else (
    echo Found include directory
    echo Header files:
    dir /b include\*.h
)

if not exist examples (
    echo WARNING: 'examples' directory not found
) else (
    echo Found examples directory
    echo Examples:
    dir /b examples
)

echo.
echo Checking CMake configuration...
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found
    exit /b 1
) else (
    echo Found CMakeLists.txt
    
    echo Checking for INSTALL_EXAMPLES option...
    findstr /C:"option(INSTALL_EXAMPLES" CMakeLists.txt >nul
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: INSTALL_EXAMPLES option not found in CMakeLists.txt
    ) else (
        echo Found INSTALL_EXAMPLES option in CMakeLists.txt
    )
)

REM Test both configurations
echo.
echo Running diagnostic builds...

REM Test INSTALL_EXAMPLES=OFF (default)
echo.
echo Testing with INSTALL_EXAMPLES=OFF (default)...
if exist build_diag_off rmdir /s /q build_diag_off
mkdir build_diag_off
cd build_diag_off

cmake -DCMAKE_INSTALL_PREFIX="%CD%\install" ..
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

echo Installing project...
cmake --install . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Installation failed
    cd ..
    exit /b 1
)

echo Checking installation directory...
if exist "install\bin\examples" (
    echo ERROR: Examples directory exists despite INSTALL_EXAMPLES=OFF (default)
    dir /s "install\bin\examples"
) else (
    echo SUCCESS: Examples were NOT installed (as expected)
)

echo Checking for library files...
if not exist "install\include\WinKernelLite" (
    echo ERROR: WinKernelLite include directory not found in installation
) else (
    echo SUCCESS: WinKernelLite include directory found
    dir /b "install\include\WinKernelLite"
)

cd ..

REM Test INSTALL_EXAMPLES=ON
echo.
echo Testing with INSTALL_EXAMPLES=ON...
if exist build_diag_on rmdir /s /q build_diag_on
mkdir build_diag_on
cd build_diag_on

cmake -DINSTALL_EXAMPLES=ON -DCMAKE_INSTALL_PREFIX="%CD%\install" ..
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

echo Installing project...
cmake --install . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Installation failed
    cd ..
    exit /b 1
)

echo Checking installation directory...
if not exist "install\bin\examples" (
    echo ERROR: Examples were not installed despite INSTALL_EXAMPLES=ON
) else (
    echo SUCCESS: Examples WERE installed (as expected)
    echo Installed examples:
    dir /b "install\bin\examples"
)

cd ..

REM Testing the execution of examples
echo.
echo Testing example execution from build directory...
cd build_diag_on

echo Checking build output directory...
if exist "bin" (
    echo Found bin directory
    dir /b bin
    
    REM Test execution of examples
    if exist "bin\devices_list.exe" (
        echo Running devices_list.exe...
        bin\devices_list.exe >nul 2>&1
        if !ERRORLEVEL! NEQ 0 (
            echo ERROR: devices_list.exe failed to run
        ) else (
            echo SUCCESS: devices_list.exe ran successfully
        )
    )
    
    if exist "bin\version_info_extended.exe" (
        echo Running version_info_extended.exe...
        bin\version_info_extended.exe >nul 2>&1
        if !ERRORLEVEL! NEQ 0 (
            echo ERROR: version_info_extended.exe failed to run
        ) else (
            echo SUCCESS: version_info_extended.exe ran successfully
        )
    )
    
    if exist "bin\devices_example.exe" (
        echo Running devices_example.exe...
        bin\devices_example.exe >nul 2>&1
        if !ERRORLEVEL! NEQ 0 (
            echo ERROR: devices_example.exe failed to run
        ) else (
            echo SUCCESS: devices_example.exe ran successfully
        )
    )
) else (
    echo ERROR: bin directory not found
)

cd ..

echo.
echo Diagnostic complete.
