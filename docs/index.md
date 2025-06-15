# WinKernelLite Documentation

Welcome to the WinKernelLite documentation. This index provides links to all available documentation for the library.

## Getting Started

- [README](../README.md) - Overview and basic usage
- [Package Usage Guide](package_usage.md) - How to use WinKernelLite in your projects
- [Building Examples](building_examples.md) - Guide to building the included examples
- [Examples Installation](examples_installation.md) - Detailed guide on examples installation options
- [Include Path Resolution](include_path_resolution.md) - How include path issues were resolved
- [System Compatibility](system_compatibility.md) - Guide to x86/x64 system compatibility

## Core Concepts

- [Versioning](versioning.md) - How the versioning system works

## Working with WinKernelLite

- [UNICODE_STRING Guide](working_with_unicode_string.md) - Detailed guide on working with UNICODE_STRING

## Examples

- [Examples Guide](examples_guide.md) - Overview of all examples
- [Device List Example](example_devices_list.md) - Managing device information with linked lists
- [UNICODE_STRING Guide](working_with_unicode_string.md) - Detailed guide on working with UNICODE_STRING

## API Reference

WinKernelLite provides the following key components:

### Memory Management

- `KernelHeapAlloc.h` - Memory allocation functions similar to Windows kernel ExAllocatePool/ExFreePool
  - `InitHeap()` - Initialize the memory tracking system
  - `ExAllocatePoolTracked()` - Allocate memory with tracking
  - `ExFreePoolTracked()` - Free memory with tracking
  - `PrintMemoryLeaks()` - Display memory leaks for debugging
  - `CleanupHeap()` - Clean up the memory tracking system

### Data Structures

- `LinkedList.h` - Implementation of Windows kernel LIST_ENTRY functionality
  - `InitializeListHead()` - Initialize a list head
  - `InsertHeadList()` - Insert an entry at the beginning of a list
  - `InsertTailList()` - Insert an entry at the end of a list
  - `RemoveEntryList()` - Remove an entry from a list
  - `RemoveHeadList()` - Remove and return the first entry in a list
  - `RemoveTailList()` - Remove and return the last entry in a list
  - `IsListEmpty()` - Check if a list is empty
  - `CONTAINING_RECORD` macro - Extract a structure pointer from a list entry

### String Handling

- `UnicodeString.h` - Windows kernel UNICODE_STRING implementation
  - `RtlInitUnicodeString()` - Initialize a UNICODE_STRING
  - `RtlDuplicateUnicodeString()` - Create a copy of a UNICODE_STRING
  - `RtlCopyUnicodeString()` - Copy one UNICODE_STRING to another
  - `RtlCompareUnicodeString()` - Compare two UNICODE_STRING values
  - `AllocateUnicodeString()` - Allocate a buffer for a UNICODE_STRING
  - `FreeUnicodeString()` - Free a UNICODE_STRING buffer

- `UnicodeStringUtils.h` - Additional utilities for UNICODE_STRING
  - `DumpUnicodeString()` - Print a UNICODE_STRING for debugging
  - `FindUnicodeSubstring()` - Search for a substring in a UNICODE_STRING
  - `RtlAppendUnicodeToString()` - Append a wide character string to a UNICODE_STRING
  - `RtlAppendUnicodeStringToString()` - Append one UNICODE_STRING to another

### Version Information

- `Version.h` - Provides version information about the library
  - `WINKERNELLITE_VERSION_MAJOR` - Major version number
  - `WINKERNELLITE_VERSION_MINOR` - Minor version number
  - `WINKERNELLITE_VERSION_PATCH` - Patch version number
  - `WINKERNELLITE_VERSION` - Full version string
  - `WINKERNELLITE_GIT_TAG` - Original Git tag

## Diagnostic Tools

WinKernelLite includes several diagnostic scripts to help with troubleshooting:

- `diagnostics.bat` - Comprehensive diagnostic tool
- `test_example_installation.bat` - Tests installation options
- `test_example_execution.bat` - Tests example functionality
- `Diagnose-Package.ps1` - PowerShell script to diagnose package installation issues
- `diagnose_install.bat` - Batch script to check installation paths
- `debug_install_paths.bat` - Script to debug installation path issues
- `check_install.bat` - Simple installation verification script
