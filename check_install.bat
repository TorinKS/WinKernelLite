@echo off
echo WinKernelLite Installation Check
echo ==============================
echo.
echo Checking include directory...
if exist "C:\Program Files (x86)\WinKernelLite\include\gtest" (
    echo GTest headers found - NOT CORRECT!
) else (
    echo No GTest headers found - CORRECT!
)

echo.
echo Checking WinKernelLite headers...
if exist "C:\Program Files (x86)\WinKernelLite\include\WinKernelLite" (
    echo WinKernelLite headers found - CORRECT!
    dir "C:\Program Files (x86)\WinKernelLite\include\WinKernelLite"
) else (
    echo WinKernelLite headers NOT found - INCORRECT!
)

echo.
echo Checking CMake configuration...
if exist "C:\Program Files (x86)\WinKernelLite\lib\cmake\WinKernelLite" (
    echo WinKernelLite CMake config found - CORRECT!
    dir "C:\Program Files (x86)\WinKernelLite\lib\cmake\WinKernelLite"
) else (
    echo WinKernelLite CMake config NOT found - INCORRECT!
)


pause
