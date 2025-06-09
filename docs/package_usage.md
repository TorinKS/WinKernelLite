# WinKernelLite Package Configuration Guide

This document summarizes the changes made to fix the CMake package configuration for WinKernelLite and provides guidelines for using the library in your own projects.

## Changes Made

1. **Fixed Include Directory Paths**
   - Adjusted the target include directories to ensure correct paths for both build-time and install-time use
   - Simplified the WinKernelLiteConfig.cmake.in file to rely on the target export
   - Fixed the installation paths for header files

2. **Improved Installation Structure**
   - Ensured headers are installed to `${CMAKE_INSTALL_INCLUDEDIR}/WinKernelLite`
   - Set up proper namespace for exported targets (WinKernelLite::WinKernelLite)
   - Fixed component-based installation

3. **Enhanced Build Scripts**
   - Improved run_version_example.bat for better error handling
   - Added diagnostic tools to help troubleshoot installation issues
   - Created test package to verify the library installation

## Using WinKernelLite in Your Projects

### Basic CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.15)
project(YourProject)

# Find WinKernelLite package
find_package(WinKernelLite REQUIRED)

# Create executable
add_executable(your_app main.c)

# Link with WinKernelLite
target_link_libraries(your_app PRIVATE WinKernelLite::WinKernelLite)
```

### Including Headers

```c
// Include version information
#include <WinKernelLite/Version.h>

// Include other components
#include <WinKernelLite/KernelHeapAlloc.h>
#include <WinKernelLite/LinkedList.h>
#include <WinKernelLite/UnicodeString.h>
#include <WinKernelLite/UnicodeStringUtils.h>
```

### Command Line Options

To build your project with WinKernelLite:

```bash
cmake -B build -DCMAKE_PREFIX_PATH="C:/Program Files (x86)/WinKernelLite"
cmake --build build
```

## Troubleshooting

If you encounter issues with finding the WinKernelLite package:

1. **Verify Installation**: 
   - Check that the library is installed at the expected location
   - Run the diagnostic script: `.\Diagnose-Package.ps1`

2. **Check Include Paths**:
   - Ensure your source files include headers with the correct path (`WinKernelLite/Header.h`)
   - Verify that the include directories are correctly set in your CMake configuration

3. **Debug CMake Configuration**:
   - Add debugging output to check the package paths:
   ```cmake
   find_package(WinKernelLite REQUIRED)
   get_target_property(WKL_INCLUDES WinKernelLite::WinKernelLite INTERFACE_INCLUDE_DIRECTORIES)
   message(STATUS "WinKernelLite includes: ${WKL_INCLUDES}")
   ```

4. **Reinstall if Necessary**:
   - If the installation seems corrupted, reinstall:
   ```
   cmake -B build
   cmake --build build
   cmake --install build --component WinKernelLite
   ```


