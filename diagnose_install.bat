@echo off
echo Diagnosing WinKernelLite Installation Issues
echo ===========================================
echo.

REM Check if PowerShell is available for better diagnostics
where powershell >nul 2>&1
if %ERRORLEVEL% equ 0 (
  echo Using PowerShell for enhanced diagnostics...
  powershell -ExecutionPolicy Bypass -File "%~dp0Diagnose-Build-System.ps1"
  goto :test_project
)

REM Fallback to batch-based diagnostics
REM Check if the installation directory exists
if not exist "C:\Program Files (x86)\WinKernelLite" (
  echo ERROR: Installation directory not found.
  goto :eof
)

echo Installation directory found.

REM Check for include directory
if exist "C:\Program Files (x86)\WinKernelLite\include\WinKernelLite" (
  echo Include directory found: C:\Program Files (x86)\WinKernelLite\include\WinKernelLite
  echo Files:
  dir "C:\Program Files (x86)\WinKernelLite\include\WinKernelLite"
) else (
  echo ERROR: Include directory not found.
)

echo.
echo Checking CMake files...

REM Check for CMake config files
if exist "C:\Program Files (x86)\WinKernelLite\lib\cmake\WinKernelLite" (
  echo CMake directory found: C:\Program Files (x86)\WinKernelLite\lib\cmake\WinKernelLite
  echo Files:
  dir "C:\Program Files (x86)\WinKernelLite\lib\cmake\WinKernelLite"
) else (
  echo ERROR: CMake directory not found.
)

echo.
echo Building test project...

:test_project
echo.
echo Testing installation with a sample project...
cd test_package
call build_and_run.bat

echo.
echo Testing include paths...
cd ..
call test_include_paths.bat

pause
