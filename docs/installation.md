# <img src="icon.png" alt="WinKernelLite Logo" width="64" height="64" style="vertical-align: middle; margin-right: 10px;"> WinKernelLite Installation and Usage Guide

This guide provides detailed instructions for installing WinKernelLite, building and running examples, and integrating it into your projects.

## Overview

WinKernelLite is a static library that provides Windows kernel API compatibility for user-mode testing:
- Requires compilation and linking of the static library
- Provides compiled implementations of kernel-mode APIs
- Installation includes both headers and compiled library
- Can be used as a CMake subdirectory or installed package

## Prerequisites

Before installing WinKernelLite, ensure your system meets the following requirements:

- Windows operating system (supports both 32-bit and 64-bit versions)
- CMake 3.29 or higher
  - **Note**: The project officially requires CMake 3.29. If you have an earlier version:
    - You can [download CMake](https://cmake.org/download/) from the official website
    - Alternatively, you can modify the first line of CMakeLists.txt to use an earlier version (e.g., `cmake_minimum_required(VERSION 3.20)`). Most functionality should work with CMake 3.20+, but this is not officially tested or supported
- Visual Studio 2019 or later with C/C++ development tools
- Git (optional, for cloning the repository)

## Installation Methods

There are two ways to install and use WinKernelLite:

1. Build and install from source
2. Use as a CMake subdirectory

## Method 1: Build and Install from Source

### Step 1: Clone the Repository

```powershell
git clone https://github.com/TorinKS/WinKernelLite.git
cd WinKernelLite
```

### Step 2: Create a Build Directory

```powershell
mkdir build
cd build
```

### Step 3: Configure the Project

```powershell
cmake ..
```

> **Note about CMake Version**: If you encounter errors related to the CMake version, you may need to either:
> - Update to CMake 3.29 or higher
> - Modify the first line of CMakeLists.txt to reduce the required version
>
> Example modification in CMakeLists.txt:
> ```cmake
> # Original line
> # cmake_minimum_required(VERSION 3.29)
> 
> # Modified line for older CMake versions
> cmake_minimum_required(VERSION 3.20)
> ```

Configure with specific options:

```powershell
# Example: Build with examples but don't install them
cmake .. -DBUILD_EXAMPLES=ON -DINSTALL_EXAMPLES=OFF
```

### Step 4: Build the Library

```powershell
cmake --build . --config Release
```

### Step 5: Install (Optional)

This will install the library to the default installation directory (`C:\Program Files\WinKernelLite`).

```powershell
cmake --build . --target install
```

For a custom installation directory:

```powershell
cmake .. -DCMAKE_INSTALL_PREFIX="C:/Custom/Install/Path"
cmake --build . --target install
```

## Method 2: Use as a CMake Subdirectory

You can include WinKernelLite directly in your CMake project without installing it.

### Steps to Use WinKernelLite as a CMake Subdirectory

#### 1. Add the Directory
Place the WinKernelLite source folder inside your project or reference it as a Git submodule:

```powershell
git submodule add https://github.com/TorinKS/WinKernelLite.git external/WinKernelLite
git submodule update --init --recursive
```

Your project structure should look like:

```
your-project/
├── CMakeLists.txt
├── src/
│   └── main.cpp
└── external/
    └── WinKernelLite/
        └── CMakeLists.txt
```

#### 2. Update Your CMakeLists.txt
In your top-level CMakeLists.txt, do the following:

```cmake
cmake_minimum_required(VERSION 3.15)
project(YourProject)

# Your executable/library
add_executable(YourExecutable src/main.cpp)

# Add WinKernelLite subdirectory
add_subdirectory(external/WinKernelLite)

# Link the library
target_link_libraries(YourExecutable PRIVATE WinKernelLite::WinKernelLite)
```

> **Note:** WinKernelLite defines an exported target called WinKernelLite::WinKernelLite which you can link to in your project.

#### 3. Using WinKernelLite Headers

When using WinKernelLite as a submodule, you can include the headers like this:

```c
#include <WinKernelLite/KernelHeap.h>
#include <WinKernelLite/LinkedList.h>
#include <WinKernelLite/UnicodeString.h>
```

### Testing It Works
After configuring with CMake, check that the include directories and symbols from WinKernelLite are found and used properly by your target.

## Building and Running the Example

WinKernelLite includes an example that demonstrates how to use the library:

### DevicesList Example

This example demonstrates linked list operations and device management:

```powershell
cd examples\DevicesList
.\build_and_run.bat
```

### Example Installation Options

By default, the example is built but not installed to Program Files:

- The example is always built in the build directory: `build/bin/examples`
- You can choose whether to install it using the `INSTALL_EXAMPLES` option

#### Building Without Installing the Example (Default)

```powershell
cmake -B build -DINSTALL_EXAMPLES=OFF
cmake --build build --target install
```

#### Building and Installing the Example

```powershell
cmake -B build -DINSTALL_EXAMPLES=ON
cmake --build build --target install
```

## Integration with Different Build Systems

### CMake (Recommended)

The recommended way to use WinKernelLite is with CMake's package discovery:

```cmake
cmake_minimum_required(VERSION 3.15)
project(YourProject)

# Find WinKernelLite package
find_package(WinKernelLite REQUIRED)

# Create your executable  
add_executable(your_target main.cpp)

# Link with WinKernelLite
target_link_libraries(your_target PRIVATE WinKernelLite::WinKernelLite)
```

This automatically handles include paths and library configuration.

### Visual Studio (Manual)

If not using CMake, you can manually configure Visual Studio:

1. Add the include directory:
   - Project Properties -> C/C++ -> General -> Additional Include Directories
   - Add `C:\Program Files\WinKernelLite\include`

## Basic Usage

### Include the Headers

```c
#include <WinKernelLite/KernelHeap.h>
#include <WinKernelLite/LinkedList.h>
#include <WinKernelLite/UnicodeString.h>
```

### Memory Allocation Example

```c
#include <WinKernelLite/KernelHeap.h>
#include <stdio.h>

int main() {
    // Initialize memory tracking
    InitializeMemoryTracking();
    
    // Allocate memory
    PVOID buffer = ExAllocatePool(PagedPool, 1024);
    if (!buffer) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Use the buffer...
    
    // Free the memory
    ExFreePool(buffer);
    
    // Check for memory leaks
    ReportMemoryLeaks();
    
    return 0;
}
```

### Linked List Example

```c
#include <WinKernelLite/KernelHeap.h>
#include <WinKernelLite/LinkedList.h>
#include <stdio.h>

typedef struct _MY_ITEM {
    LIST_ENTRY ListEntry;
    int Data;
} MY_ITEM, *PMY_ITEM;

int main() {
    InitializeMemoryTracking();
    
    LIST_ENTRY listHead;
    InitializeListHead(&listHead);
    
    // Add items to list
    for (int i = 0; i < 5; i++) {
        PMY_ITEM item = ExAllocatePool(PagedPool, sizeof(MY_ITEM));
        if (!item) continue;
        
        item->Data = i;
        InsertTailList(&listHead, &item->ListEntry);
    }
    
    // Display items
    PLIST_ENTRY entry = listHead.Flink;
    while (entry != &listHead) {
        PMY_ITEM item = CONTAINING_RECORD(entry, MY_ITEM, ListEntry);
        printf("Item: %d\n", item->Data);
        entry = entry->Flink;
    }
    
    // Clean up
    while (!IsListEmpty(&listHead)) {
        PLIST_ENTRY entry = RemoveHeadList(&listHead);
        PMY_ITEM item = CONTAINING_RECORD(entry, MY_ITEM, ListEntry);
        ExFreePool(item);
    }
    
    ReportMemoryLeaks();
    return 0;
}
```

## Troubleshooting

### Common Issues

#### CMake Cannot Find WinKernelLite

If CMake cannot find WinKernelLite, ensure you have:

1. Built the library first: `cmake --build build`
2. Set the correct CMake prefix path when building examples:
   ```powershell
   cmake .. -DCMAKE_PREFIX_PATH="../build"
   ```

For installed libraries, you may need to specify the path:

```cmake
set(WinKernelLite_DIR "C:/Program Files/WinKernelLite/lib/cmake/WinKernelLite")
find_package(WinKernelLite REQUIRED)
```

#### Header File Not Found

If header files cannot be found:

1. Check that you're using the correct CMake approach with `find_package()`
2. Verify that you're including files with the correct path:
   ```c
   #include <WinKernelLite/KernelHeap.h>
   ```
3. If using manual setup, check that the include directory is correctly set

#### Example Not Building

The example now uses the modern CMake approach. If the example fails to build:

1. Ensure the main project is built first: `cmake --build build`
2. Navigate to the example directory and run its build script

### Getting Help

If you encounter problems not covered in this guide:

1. Check the [GitHub Issues](https://github.com/TorinKS/WinKernelLite/issues) page for similar problems
2. Create a new issue with details about your problem
3. Include your system information, CMake version, and compiler version
