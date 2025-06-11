@echo off
echo Testing the installation of WinKernelLite examples...

:: Create a test directory
set TEST_DIR=%TEMP%\winkernellite_test
if exist %TEST_DIR% rmdir /s /q %TEST_DIR%
mkdir %TEST_DIR%
cd %TEST_DIR%

echo Testing installation at: %TEST_DIR%

:: Copy all examples to the test directory
xcopy /E /I /Y "C:\projects\WinKernelLite\examples" examples
cd examples

:: Try to build each example
for /d %%D in (*) do (
    if exist "%%D\CMakeLists.txt" (
        echo.
        echo === Testing example: %%D ===
        cd %%D
        
        :: Create build dir
        if not exist build mkdir build
        cd build
        
        :: Configure and build
        cmake .. -DCMAKE_PREFIX_PATH="C:/Program Files (x86)/WinKernelLite;C:/projects/WinKernelLite/build" -DCMAKE_MODULE_PATH="../.."
        if %ERRORLEVEL% neq 0 (
            echo ERROR: Failed to configure example %%D
        ) else (
            cmake --build . --config Debug
            if %ERRORLEVEL% neq 0 (
                echo ERROR: Failed to build example %%D
            ) else (
                echo SUCCESS: Built example %%D
            )
        )
        
        cd ../..
    )
)

:: Return to original directory
cd \
echo.
echo Test complete. Results are in %TEST_DIR%
