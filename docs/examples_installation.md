# WinKernelLite Examples Installation Guide

This document explains the options available for building and installing examples in the WinKernelLite library.

## Overview

WinKernelLite provides two separate CMake options for examples:

1. `BUILD_EXAMPLES` - Controls whether examples are built (default: ON)
2. `INSTALL_EXAMPLES` - Controls whether examples are installed to Program Files (default: OFF)

## Configuration Options

### Default Configuration (Recommended)

By default, WinKernelLite is configured to:
- Build examples (`BUILD_EXAMPLES=ON`)
- Not install examples (`INSTALL_EXAMPLES=OFF`)

This means:
- Examples will be built and available in your build directory
- Examples will not be installed to Program Files

This configuration keeps your system directories clean while still providing access to examples.

### Build But Don't Install (Default)

```bash
cmake -B build
# OR explicitly:
cmake -B build -DBUILD_EXAMPLES=ON -DINSTALL_EXAMPLES=OFF
```

With this configuration:
- Examples will be built and placed in `build/bin`
- Examples will NOT be installed to Program Files
- You can still run examples directly from the build directory

### Build And Install

```bash
cmake -B build -DINSTALL_EXAMPLES=ON
```

With this configuration:
- Examples will be built in `build/bin`
- Examples will also be installed to `C:/Program Files (x86)/WinKernelLite/bin/examples`
- Header files used by examples will be installed to `C:/Program Files (x86)/WinKernelLite/include/examples`

### Don't Build Examples

```bash
cmake -B build -DBUILD_EXAMPLES=OFF
```

With this configuration:
- Examples will not be built at all
- No example executables will be created
- Build time will be slightly reduced

## Where To Find Built Examples

Depending on your configuration, you can find the example executables in:

1. **Build directory** (if `BUILD_EXAMPLES=ON`):
   - `build/bin/devices_list.exe`
   - `build/bin/version_info_extended.exe`
   - `build/bin/devices_example.exe`

2. **Install directory** (if `INSTALL_EXAMPLES=ON`):
   - `C:/Program Files (x86)/WinKernelLite/bin/examples/devices_list.exe`
   - `C:/Program Files (x86)/WinKernelLite/bin/examples/version_info_extended.exe`
   - `C:/Program Files (x86)/WinKernelLite/bin/examples/devices_example.exe`

## Running Examples

### From Build Directory

```bash
cd build/bin
.\devices_list.exe
.\version_info_extended.exe
.\devices_example.exe
```

### From Install Directory (if installed)

```bash
cd "C:/Program Files (x86)/WinKernelLite/bin/examples"
.\devices_list.exe
.\version_info_extended.exe
.\devices_example.exe
```

## Testing Installation Options

To validate that the installation options are working correctly, you can run:

```bash
.\test_example_installation.bat
```

This script will:
1. Test building with `INSTALL_EXAMPLES=OFF` (default)
2. Verify examples are NOT installed to the install directory
3. Test building with `INSTALL_EXAMPLES=ON`
4. Verify examples ARE installed to the install directory

## Troubleshooting

If you encounter any issues with building or installing examples:

1. Run the diagnostic tool:
   ```bash
   .\diagnostics.bat
   ```

2. Verify examples build and run correctly:
   ```bash
   .\test_example_execution.bat
   ```

3. Check your CMake configuration:
   ```bash
   cd build
   cmake -L .
   ```
   Look for the `BUILD_EXAMPLES` and `INSTALL_EXAMPLES` options in the output.
