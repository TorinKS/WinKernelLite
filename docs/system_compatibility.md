# System Compatibility Guide for WinKernelLite

This document explains how WinKernelLite handles installation and configuration on different Windows systems.

## Windows Architecture Support

WinKernelLite is designed to work seamlessly on both 32-bit and 64-bit Windows systems, with automatic detection of the appropriate installation paths.

### Default Installation Directories

- **64-bit Windows**: `C:\Program Files (x86)\WinKernelLite`
- **32-bit Windows**: `C:\Program Files\WinKernelLite`

The library automatically detects the system architecture and uses the appropriate path.

## Custom Installation Paths

If you want to install WinKernelLite to a different location, you have several options:

### 1. Using Environment Variables

Set the `WINKERNELLITE_INSTALL_PATH` environment variable before running any installation or build commands:

```powershell
# PowerShell
$env:WINKERNELLITE_INSTALL_PATH = "D:\Libraries\WinKernelLite"

# Then run your build/install commands
cmake -B build
cmake --build build
cmake --install build
```

```cmd
:: Command Prompt
set WINKERNELLITE_INSTALL_PATH=D:\Libraries\WinKernelLite

:: Then run your build/install commands
cmake -B build
cmake --build build
cmake --install build
```

### 2. Using CMake's Install Prefix

Specify the installation directory directly with CMake's `CMAKE_INSTALL_PREFIX` option:

```powershell
cmake -B build -DCMAKE_INSTALL_PREFIX="D:/Libraries/WinKernelLite"
cmake --build build
cmake --install build
```

## Environment Setup

WinKernelLite includes a helper script `set_environment.bat` that sets up the environment variables for you:

```cmd
call set_environment.bat
```

This script:
1. Detects the system architecture
2. Sets the appropriate installation path based on architecture
3. Allows override through the `WINKERNELLITE_INSTALL_PATH` environment variable
4. Sets up the `CMAKE_PREFIX_PATH` environment variable for CMake to find the library

## Verifying Installation

To verify that the library is installed correctly for your system architecture, run:

```cmd
call check_install.bat
```

This will check the appropriate installation path based on your system architecture.

## Diagnostic Tools

If you encounter any issues with installation paths, you can use these diagnostic tools:

```cmd
call debug_install_paths.bat
```

For a comprehensive diagnostic, run:

```cmd
call diagnostics.bat
```

## Include Path Handling

The library is designed to handle include paths correctly on all systems:

1. Headers are installed to `<INSTALL_PATH>/include/WinKernelLite/`
2. Example code can include them with:
   ```c
   #include <WinKernelLite/KernelHeapAlloc.h>
   ```

3. CMake projects can find the library with:
   ```cmake
   find_package(WinKernelLite REQUIRED)
   target_link_libraries(your_target PRIVATE WinKernelLite::WinKernelLite)
   ```

## Troubleshooting

If you encounter path-related issues:

1. Verify the system-detected installation path:
   ```cmd
   call set_environment.bat
   echo %INSTALL_PATH%
   ```

2. Check if the library is installed correctly:
   ```cmd
   call check_install.bat
   ```

3. Run the diagnostic tools:
   ```cmd
   call diagnostics.bat
   ```

4. Try setting a custom installation path:
   ```cmd
   set WINKERNELLITE_INSTALL_PATH=D:\Libraries\WinKernelLite
   cmake -B build
   cmake --build build
   cmake --install build
   ```
