# WinKernelLite

WinKernelLite is a lightweight library that enables testing and usage of Windows kernel functionality in user-mode applications. It provides implementations of common Windows kernel data structures and functions that can be used in standard user-mode applications, making kernel-style code more accessible and testable.

## Project Goals

While Microsoft provides the Windows Research Kernel (WRK) for academic purposes, kernel internals remain closed-source, making it difficult to understand the exact implementation details behind kernel functions. Additionally, testing kernel code often requires complex setups and can lead to system crashes during development.

WinKernelLite addresses these challenges by:

1. **Simplified Testing Environment**: Allowing developers to test kernel-style code in user mode without risking system stability
2. **Improved Development Workflow**: Enabling the use of standard debugging tools and testing frameworks not available in kernel mode
3. **Consistent API**: Providing implementations that tries to match kernel functionality while working in user-mode contexts
4. **Reference Implementation**: Serving as a learning resource for understanding Windows kernel programming patterns

Currently, the library implements core functionality related to doubly-linked lists (LIST_ENTRY structure), memory allocation with pool tracking, and UNICODE_STRING manipulation - common elements used throughout Windows kernel development.

## Features

- Memory allocation and tracking with Windows Kernel-like semantics
- Linked list implementation similar to Windows Kernel
- Unicode string handling utilities
- Automatic versioning from Git tags

## Usage with CMake

### Using find_package

```cmake
find_package(WinKernelLite REQUIRED)
target_link_libraries(your_target PRIVATE WinKernelLite::WinKernelLite)
```

### Including in your code

```cpp
#include <WinKernelLite/KernelHeapAlloc.h>
#include <WinKernelLite/Version.h>  // For version information

void example() {
    InitHeap();
    
    // Print version information
    printf("Using WinKernelLite version %s\n", WINKERNELLITE_VERSION);
    
    PVOID memory = ExAllocatePoolTracked(NonPagedPool, 1024);
    // Use memory...
    ExFreePoolTracked(memory);
    
    CleanupHeap();
}
```

## Installation

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Install (this will install all components including GoogleTest)
cmake --install build

# Alternative: Using the custom target
cmake --build build --target install_winkernellite
```

For detailed information on using the WinKernelLite package in your own projects, see [Package Usage Guide](docs/package_usage.md).

## Examples

The project includes several examples demonstrating how to use WinKernelLite in real-world scenarios:

1. **Basic Version Info** - Shows how to access version information from the library
2. **Devices List** - Demonstrates managing device information using linked lists and UNICODE_STRING
   - [DevicesList Documentation](docs/example_devices_list.md)
3. **UNICODE_STRING Handling** - Examples of working with Windows kernel-style Unicode strings
   - [UNICODE_STRING Guide](docs/working_with_unicode_string.md)

To build and run the examples:

```bash
# Configure the project
cmake -B build -S .

# Build all examples
cmake --build build --target build_all_examples

# Run a specific example
cd examples/DevicesExample
./build_and_run.bat
```

## Versioning

WinKernelLite uses Git tags for versioning. The project automatically extracts the version from the most recent Git tag when you build the project.

For detailed information on the versioning system, see [docs/versioning.md](docs/versioning.md).

### Git Tag Format

Tags should follow semantic versioning format:

```
v1.2.3
```

The leading 'v' is optional and will be automatically removed.

### Setting a New Version

To set a new version:

```bash
# Tag the current commit with a new version
git tag -a v1.2.3 -m "Version 1.2.3"

# Push the tag to the remote repository
git push origin v1.2.3
```

Alternatively, use the provided PowerShell script:

```powershell
# Create a new version tag
.\Create-Version.ps1 -Version 1.2.3

# Create and push a new version tag
.\Create-Version.ps1 -Version 1.2.3 -Push
```

### Accessing Version Information in Code

Version information is available in your code through the `Version.h` header:

```cpp
#include <WinKernelLite/Version.h>

// Available macros:
// WINKERNELLITE_VERSION_MAJOR - Major version number (int)
// WINKERNELLITE_VERSION_MINOR - Minor version number (int)
// WINKERNELLITE_VERSION_PATCH - Patch version number (int)
// WINKERNELLITE_VERSION - Full version string (e.g., "1.2.3")
// WINKERNELLITE_GIT_TAG - Original Git tag (e.g., "v1.2.3")

void printVersion() {
    printf("WinKernelLite v%d.%d.%d\n", 
           WINKERNELLITE_VERSION_MAJOR,
           WINKERNELLITE_VERSION_MINOR,
           WINKERNELLITE_VERSION_PATCH);
}
```
