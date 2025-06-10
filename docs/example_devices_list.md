# Using WinKernelLite for Device Management

This guide demonstrates how to use WinKernelLite to manage device information in a linked list structure, similar to how a Windows kernel driver might track connected devices.

## Overview

The DevicesList example shows how to:

1. Create and manage device information structures
2. Use kernel-style linked lists for tracking multiple devices
3. Properly handle memory allocation and cleanup
4. Implement ownership semantics for memory management
5. Work with UNICODE_STRING for storing device information

## Key Concepts

### Device Structure

The example defines a `DEVICE_NAME` structure that represents device information:

```c
typedef struct _DEVICE_NAME
{
    LIST_ENTRY ListEntry;
    UNICODE_STRING usGuid;                      // GUID of device
    UNICODE_STRING usDeviceType;                // Device type
    UNICODE_STRING usDeviceHardwareId;          // Hardware ID
    UNICODE_STRING usDevicePropertyDriverKeyName; // Driver key name
    
    // USB specific fields
    UNICODE_STRING Manufacturer;
    UNICODE_STRING Product;
    UNICODE_STRING SerialNumber;
    
    // Ownership tracking
    BOOLEAN OwnedByList;
} DEVICE_NAME, *PDEVICE_NAME;
```

This structure contains:
- A `LIST_ENTRY` for linking into a device list
- Various `UNICODE_STRING` fields for device properties
- An ownership flag to track if the device is owned by a list

### List Entry Structure

To track devices in a linked list, a list entry structure is used:

```c
typedef struct
{
    LIST_ENTRY     ListEntry;
    PDEVICE_NAME   pDevName;
} DEVICE_LIST_ENTRY, *PDEVICE_LIST_ENTRY;
```

This structure provides a level of indirection, allowing a device to be referenced by multiple lists if needed.

## Memory Management

### Allocation and Initialization

The example demonstrates proper memory allocation and initialization:

```c
PDEVICE_NAME device = CreateDevice(L"Sony", L"Walkman", L"SN12345");
if (!device) {
    // Handle allocation failure
    return -1;
}
```

The `CreateDevice` function:
- Allocates memory using `ExAllocatePoolTracked`
- Initializes UNICODE_STRING fields
- Sets the ownership flag
- Provides proper cleanup on failure

### Cleanup and Resource Management

The example shows three approaches to cleanup:

1. **Direct cleanup** - For devices not owned by a list:
   ```c
   FreeDevice(device);
   ```

2. **List-based cleanup** - For removing a specific device from a list:
   ```c
   RemoveAndFreeDevice(&gDeviceList, deviceToRemove);
   ```

3. **Complete list cleanup** - For cleaning up an entire list:
   ```c
   while (!IsListEmpty(&gDeviceList)) {
       PLIST_ENTRY entry = gDeviceList.Flink;
       PDEVICE_LIST_ENTRY listEntry = CONTAINING_RECORD(entry, DEVICE_LIST_ENTRY, ListEntry);
       RemoveAndFreeDevice(&gDeviceList, listEntry->pDevName);
   }
   ```

### Memory Leak Detection

The example also demonstrates using WinKernelLite's memory tracking features:

```c
// Print memory leak report
PrintMemoryLeaks();

// Clean up tracking resources
CleanupHeap();
```

## List Operations

### Initializing a List

Before using a list, it must be initialized:

```c
LIST_ENTRY gDeviceList;  // Global list

// In your initialization function:
InitializeListHead(&gDeviceList);
```

### Adding Devices to a List

Devices are added to the list using the `InsertDeviceListEx` function:

```c
NTSTATUS status = InsertDeviceListEx(&gDeviceList, device);
if (!NT_SUCCESS(status)) {
    // Handle error
    FreeDevice(device);
    return -1;
}
```

### Traversing a List

To iterate through all devices in a list:

```c
PLIST_ENTRY currentEntry = gDeviceList.Flink;
while (currentEntry != &gDeviceList) {
    PDEVICE_LIST_ENTRY listEntry = CONTAINING_RECORD(currentEntry, DEVICE_LIST_ENTRY, ListEntry);
    PDEVICE_NAME device = listEntry->pDevName;
    
    // Work with the device
    DumpUnicodeString(&device->Manufacturer, "Manufacturer");
    DumpUnicodeString(&device->Product, "Product");
    
    // Move to next entry
    currentEntry = currentEntry->Flink;
}
```

## Working with UNICODE_STRING

### Creating and Initializing Strings

The example shows how to create and initialize UNICODE_STRING objects:

```c
UNICODE_STRING tempString;
RtlInitUnicodeString(&tempString, L"String content");

// To make a copy
RtlDuplicateUnicodeString(
    RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
    &tempString,
    &destination
);
```

### Displaying String Content

To display UNICODE_STRING content, use the DumpUnicodeString utility:

```c
DumpUnicodeString(&device->Manufacturer, "Manufacturer");
```

## Complete Example

For a complete, working example, refer to the DevicesList example in the examples directory:

- `DevicesList.h` - API definitions
- `DevicesList.c` - Implementation of device management functions
- `DeviceListApp.c` - Application demonstrating the API usage

## Key Takeaways

1. Use the `ExAllocatePoolTracked` and `ExFreePool` functions for memory management
2. Initialize lists with `InitializeListHead`
3. Track ownership of allocated resources to prevent memory leaks
4. Clean up resources in the reverse order of allocation
5. Use the `CONTAINING_RECORD` macro to extract structure pointers from list entries
6. Leverage WinKernelLite's memory tracking to detect leaks

By following these patterns, you can create kernel-style code that is testable in user mode while maintaining compatibility with actual kernel development practices.
