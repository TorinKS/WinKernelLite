# Building WinKernelLite Examples

This guide explains how to build and run the examples included with the WinKernelLite library.

## Prerequisites

- CMake 3.15 or higher
- Visual Studio 2019 or later with C/C++ development tools
- WinKernelLite library installed (or built from source)

## Build vs. Install

By default, examples are built but not installed to Program Files:

- Examples are always built in the build directory: `build/bin/examples`
- You can choose whether to install them using the `INSTALL_EXAMPLES` option

## Setup Environment

Before building the examples, it's recommended to run the environment setup script:

```bash
.\setup_examples_env.bat
```

This script:
1. Creates the correct directory structure for include files
2. Copies header files to the build directory
3. Sets up environment variables for CMake

## Building Examples Individually

Each example has its own directory with a `build_and_run.bat` script:

### DevicesExample

```bash
cd examples\DevicesExample
.\build_and_run.bat
```

### DevicesList

```bash
cd examples\DevicesList
.\build_and_run.bat
```

### VersionInfo

```bash
cd examples\VersionInfo
.\build_and_run.bat
```

## Building All Examples at Once

You can build all examples at once using:

```bash
cd examples
.\run_all_examples.bat
```

## Installation Options

### Building Without Installing Examples (Default)

This option builds the examples but doesn't install them to Program Files:

```bash
cmake -B build -DINSTALL_EXAMPLES=OFF
cmake --build build --target install
```

The examples will still be available in the `build/bin` directory.

### Building and Installing Examples

If you want to install the examples along with the library:

```bash
cmake -B build -DINSTALL_EXAMPLES=ON
cmake --build build --target install
```

This will install the examples to `C:/Program Files (x86)/WinKernelLite/bin/examples`.

## Common Issues and Solutions

### Header Files Not Found

If you get errors about header files not being found:

1. Make sure the library is installed correctly:
   ```bash
   .\diagnose_install.bat
   ```

2. Test if the compiler can find the header files:
   ```bash
   .\test_include_paths.bat
   ```

3. Run the environment setup script:
   ```bash
   .\setup_examples_env.bat
   ```

### Link Errors

If you get link errors:

1. Check if the library is in the CMAKE_PREFIX_PATH:
   ```bash
   echo %CMAKE_PREFIX_PATH%
   ```

2. Make sure you're linking against WinKernelLite::WinKernelLite in your CMakeLists.txt:
   ```cmake
   target_link_libraries(your_target PRIVATE WinKernelLite::WinKernelLite)
   ```

## Include Directory Structure

The WinKernelLite library installs its headers in:
- `C:\Program Files (x86)\WinKernelLite\include\WinKernelLite\`

When including headers in your source code, use:
```c
#include <WinKernelLite/KernelHeapAlloc.h>
#include <WinKernelLite/LinkedList.h>
// etc.
```

Make sure your CMake configuration has the correct include directory:
```cmake
find_package(WinKernelLite REQUIRED)
# This automatically sets up the include directories
```

## Manual Include Configuration

If find_package doesn't work, you can manually set up the includes:

```cmake
include_directories("C:/Program Files (x86)/WinKernelLite/include")
```
