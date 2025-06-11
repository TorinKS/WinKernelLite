@echo off
echo Testing WinKernelLite include paths...

:: Test include path scenarios
echo Testing direct includes...
echo #include ^<KernelHeapAlloc.h^> > test_include.c
echo int main() { return 0; } >> test_include.c

:: Check if compilation succeeds with direct include
echo Compiling test_include.c with direct include...
call "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" > NUL
cl.exe /I"C:\Program Files (x86)\WinKernelLite\include\WinKernelLite" /nologo test_include.c >NUL 2>&1
if %ERRORLEVEL% equ 0 (
    echo PASS: Direct include works
) else (
    echo FAIL: Direct include fails
)

:: Test with proper prefix
echo.
echo Testing with WinKernelLite prefix...
echo #include ^<WinKernelLite/KernelHeapAlloc.h^> > test_prefix.c
echo int main() { return 0; } >> test_prefix.c

:: Check if compilation succeeds with prefixed include
echo Compiling test_prefix.c with WinKernelLite prefix...
cl.exe /I"C:\Program Files (x86)\WinKernelLite\include" /nologo test_prefix.c >NUL 2>&1
if %ERRORLEVEL% equ 0 (
    echo PASS: Prefixed include works
) else (
    echo FAIL: Prefixed include fails
)

:: Clean up test files
del test_include.c test_prefix.c test_include.obj test_prefix.obj 2>NUL

echo.
echo Testing include paths complete.
