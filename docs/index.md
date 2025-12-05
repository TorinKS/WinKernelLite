# <img src="icon.png" alt="WinKernelLite Logo" width="64" height="64" style="vertical-align: middle; margin-right: 10px;"> WinKernelLite Documentation

Welcome to the WinKernelLite documentation! This is a comprehensive guide to using and contributing to the lightweight Windows kernel compatibility simulation layer.

## Getting Started

- **[Installation Guide](installation.md)** - Complete instructions for installing and setting up WinKernelLite
- **[Quick Start](../README.md#usage)** - Basic usage examples to get you started quickly

## Development & Contributing

- **[Contributing Guide](contributing_guide.md)** - Guidelines for contributors

## Project Information

**WinKernelLite** is a Windows kernel compatibility simulation layer for testing and debugging driver/kernel-related code in user-mode. It allows developers to write code using Windows driver development functions and run and test them in user-mode instead of debugging code in kernel mode.

### Key Features

- **Kernel Memory Allocation**: Implementation of `ExAllocatePool`, `ExFreePool`, and related functions
- **Linked List Management**: Windows kernel-style linked list manipulation routines
- **Unicode String Handling**: Support for `UNICODE_STRING` structure and associated functions
- **Memory Tracking**: Debug tools for tracking memory allocations and detecting leaks

### Example

The library comes with an example demonstrating its use:

1. **[DevicesList](../examples/DevicesList/)** - Demonstrates linked lists with LIST_ENTRY and device management

## Repository

- **[GitHub Repository](https://github.com/TorinKS/WinKernelLite)** - Main source code repository
- **[Issues](https://github.com/TorinKS/WinKernelLite/issues)** - Bug reports and feature requests
- **[Releases](https://github.com/TorinKS/WinKernelLite/releases)** - Version history and downloads

---

*WinKernelLite is designed to make Windows kernel development more accessible and efficient by enabling user-mode testing of kernel-style code.* 