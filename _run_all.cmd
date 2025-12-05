@echo off
REM ========================================
REM WinKernelLite main build script
REM ========================================
REM Usage: run_all.cmd [Debug|Release] [--build-only]
REM ========================================

set BUILD_CONFIG=Release
set COMMAND=all
set BUILD_DIR=build

REM Parse arguments
:parse_args
if "%1"=="" goto done_parsing
if /i "%1"=="Debug" (
    set BUILD_CONFIG=Debug
    shift
    goto parse_args
)
if /i "%1"=="--build-only" (
    set COMMAND=build-only
    shift
    goto parse_args
)
shift
goto parse_args

:done_parsing

echo ========================================
echo Configuring CMake...
echo ========================================
cmake -B %BUILD_DIR%
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed!
    exit /b %ERRORLEVEL%
)

echo ========================================
echo Building main library in %BUILD_CONFIG% mode...
echo ========================================
cmake --build %BUILD_DIR% --config %BUILD_CONFIG%
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    exit /b %ERRORLEVEL%
)

if "%COMMAND%"=="build-only" goto success

echo ========================================
echo Running tests...
echo ========================================
if exist .\%BUILD_DIR%\runTests.exe (
    cd /d .\%BUILD_DIR%
    runTests.exe
    cd /d ..
) else if exist .\%BUILD_DIR%\bin\runTests.exe (
    cd /d .\%BUILD_DIR%\bin
    runTests.exe
    cd /d ..\..
) else (
    echo ERROR: Cannot find test executable
    echo Looked in .\%BUILD_DIR%\runTests.exe and .\%BUILD_DIR%\bin\runTests.exe
    exit /b 1
)
if %ERRORLEVEL% neq 0 (
    echo ERROR: Tests failed!
    exit /b %ERRORLEVEL%
)

echo ======================================== 
echo Installing library...
echo ========================================
cmake --build %BUILD_DIR% --config %BUILD_CONFIG% --target install_WinKernelLite
if %ERRORLEVEL% neq 0 (
    echo ERROR: Installation failed!
    exit /b %ERRORLEVEL%
)


echo ========================================
echo Building examples...
echo ========================================
if not exist %BUILD_DIR%\examples mkdir %BUILD_DIR%\examples
cmake -S examples -B %BUILD_DIR%\examples
if %ERRORLEVEL% neq 0 (
    echo ERROR: Examples CMake configuration failed!
    exit /b %ERRORLEVEL%
)

cmake --build %BUILD_DIR%\examples
if %ERRORLEVEL% neq 0 (
    echo ERROR: Examples build failed!
    exit /b %ERRORLEVEL%
)


:success

echo ========================================
echo All operations completed successfully!
echo ========================================
echo Build system: MSBuild
echo Build directory: %BUILD_DIR%
echo Main library: Built in %BUILD_DIR%\%BUILD_CONFIG%\WinKernelLite.lib
echo Library installed to Program Files
echo Examples: Built in %BUILD_DIR%\examples\bin\examples\
echo Tests: Available in %BUILD_DIR%\bin\runTests.exe
echo ========================================

