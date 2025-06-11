@echo off
echo =====================================
echo WinKernelLite Installation Guide
echo =====================================
echo.
echo By default, WinKernelLite is configured to build examples but not install them.
echo This ensures that the Program Files directory only contains the library files.
echo.
echo Build and install options:
echo.
echo 1. Build without installing examples (default):
echo    cmake -B build -DINSTALL_EXAMPLES=OFF
echo    cmake --build build --target install
echo.
echo 2. Build and install examples:
echo    cmake -B build -DINSTALL_EXAMPLES=ON
echo    cmake --build build --target install
echo.
echo 3. Build examples but don't install anything:
echo    cmake -B build
echo    cmake --build build
echo.
echo 4. Clean rebuild without examples:
echo    rm -r -Force build
echo    cmake -B build -DBUILD_EXAMPLES=OFF
echo    cmake --build build --target install
echo.
echo NOTE: The examples are always built in the build directory and can be
echo       found at "build/bin" regardless of installation.
echo.
echo Diagnostic and testing tools:
echo.
echo 1. Run the diagnostic tool:
echo    .\diagnostics.bat
echo.
echo 2. Test example installation:
echo    .\test_example_installation.bat
echo.
echo 3. Test example execution:
echo    .\test_example_execution.bat
echo.
echo For detailed documentation, see:
echo  - docs\examples_installation.md
echo  - docs\building_examples.md
echo.
pause
