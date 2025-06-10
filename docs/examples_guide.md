# WinKernelLite Examples Guide

This document provides an overview of all the examples included with WinKernelLite, explaining what each demonstrates and how to run them.

## Examples Overview

WinKernelLite includes several examples that demonstrate different aspects of the library:

1. **Version Info** - A simple example showing how to access version information
2. **DevicesList** - A more complex example demonstrating linked lists and device management
3. **DevicesExample** - An enhanced example building on the DevicesList concepts with more features

## Running the Examples

You can run all examples at once using the provided batch file:

```bash
cd examples
./run_all_examples.bat
```

Or you can run each example individually by navigating to its directory and using its build script:

```bash
cd examples/DevicesExample
./build_and_run.bat
```

## Example 1: Version Info

**Location**: `examples/version_info.c`

This is the simplest example, demonstrating how to:
- Include WinKernelLite headers
- Access version information from the Version.h header
- Display version information

**Key Concepts**:
- Basic library usage
- Version information access

## Example 2: DevicesList

**Location**: `examples/DevicesList/`

This example demonstrates:
- Creating and managing device information structures
- Using linked lists to track multiple devices
- Proper memory management with allocation and freeing
- Working with UNICODE_STRING for device properties

**Key Files**:
- `DevicesList.h` - API definitions
- `DevicesList.c` - Implementation of device management functions
- `DeviceListApp.c` - Main application using the API

**Key Concepts**:
- Linked list operations
- Memory management
- String handling
- Resource cleanup

For detailed documentation on this example, see [example_devices_list.md](example_devices_list.md).

## Example 3: DevicesExample

**Location**: `examples/DevicesExample/`

This enhanced example builds on the DevicesList concepts but adds:
- More comprehensive device information tracking
- Connection status management
- Better error handling
- More extensive UNICODE_STRING usage
- Clearer memory ownership semantics

**Key Files**:
- `DevicesExample.c` - Complete example application
- `README.md` - Documentation for the example

**Key Concepts**:
- Advanced linked list usage
- Comprehensive string handling
- Memory leak detection
- Status tracking for devices

## Learning Path

For new users, we recommend following the examples in this order:

1. Start with **Version Info** to get familiar with basic inclusion and usage
2. Move to **DevicesList** to understand core concepts like linked lists and memory management
3. Finally, explore **DevicesExample** for more advanced techniques and best practices

## Common Patterns

Across all examples, you'll notice these common patterns:

1. **Initialization**
   ```c
   InitHeap();  // Initialize memory tracking
   InitializeListHead(&listHead);  // Initialize lists
   ```

2. **Memory Allocation**
   ```c
   PVOID memory = ExAllocatePoolTracked(PagedPool, size);
   ```

3. **Linked List Operations**
   ```c
   InsertTailList(&listHead, &entry->ListEntry);  // Add to list
   RemoveEntryList(&entry->ListEntry);  // Remove from list
   ```

4. **Cleanup**
   ```c
   FreeUnicodeString(&string);  // Free strings
   ExFreePool(memory);  // Free memory
   PrintMemoryLeaks();  // Check for leaks
   CleanupHeap();  // Final cleanup
   ```

## Additional Resources

- [Working with UNICODE_STRING](working_with_unicode_string.md) - Detailed guide on string handling
- [Package Usage Guide](package_usage.md) - How to use WinKernelLite in your projects
- [Versioning](versioning.md) - How the versioning system works
