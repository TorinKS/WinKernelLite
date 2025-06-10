// DevicesExample.c - Example program demonstrating the DevicesList API
#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <stdio.h>

// Include WinKernelLite headers
#include <WinKernelLite/KernelHeapAlloc.h>
#include <WinKernelLite/LinkedList.h>
#include <WinKernelLite/UnicodeString.h>
#include <WinKernelLite/UnicodeStringUtils.h>
#include <WinKernelLite/Version.h>

// Define our device structure
typedef struct _DEVICE_INFO {
    LIST_ENTRY ListEntry;             // For linking in the list
    UNICODE_STRING DeviceName;        // Name of the device
    UNICODE_STRING DeviceId;          // Unique ID
    UNICODE_STRING Manufacturer;      // Manufacturer name
    ULONG DeviceType;                 // Type of device
    BOOLEAN IsConnected;              // Whether device is currently connected
} DEVICE_INFO, *PDEVICE_INFO;

// Global list of devices
LIST_ENTRY g_DeviceList;

// Function to create a new device
PDEVICE_INFO CreateDeviceInfo(
    PCWSTR name,
    PCWSTR id,
    PCWSTR manufacturer,
    ULONG deviceType,
    BOOLEAN isConnected)
{
    // Allocate device structure
    PDEVICE_INFO device = (PDEVICE_INFO)ExAllocatePoolTracked(PagedPool, sizeof(DEVICE_INFO));
    if (!device) {
        printf("Failed to allocate device info structure\n");
        return NULL;
    }

    // Initialize list entry
    InitializeListHead(&device->ListEntry);
    
    // Initialize to zeroes first
    RtlZeroMemory(device, sizeof(DEVICE_INFO));
    
    // Set simple properties
    device->DeviceType = deviceType;
    device->IsConnected = isConnected;
    
    // Initialize strings
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN nameInitialized = FALSE;
    BOOLEAN idInitialized = FALSE;
    BOOLEAN manufacturerInitialized = FALSE;
    
    if (name) {
        UNICODE_STRING tempName;
        RtlInitUnicodeString(&tempName, name);
        status = RtlDuplicateUnicodeString(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
            &tempName,
            &device->DeviceName
        );
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        nameInitialized = TRUE;
    }
    
    if (id) {
        UNICODE_STRING tempId;
        RtlInitUnicodeString(&tempId, id);
        status = RtlDuplicateUnicodeString(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
            &tempId,
            &device->DeviceId
        );
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        idInitialized = TRUE;
    }
    
    if (manufacturer) {
        UNICODE_STRING tempManufacturer;
        RtlInitUnicodeString(&tempManufacturer, manufacturer);
        status = RtlDuplicateUnicodeString(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
            &tempManufacturer,
            &device->Manufacturer
        );
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        manufacturerInitialized = TRUE;
    }
    
    return device;
    
Cleanup:
    // Clean up on failure
    if (nameInitialized) {
        FreeUnicodeString(&device->DeviceName);
    }
    
    if (idInitialized) {
        FreeUnicodeString(&device->DeviceId);
    }
    
    if (manufacturerInitialized) {
        FreeUnicodeString(&device->Manufacturer);
    }
    
    ExFreePool(device);
    return NULL;
}

// Function to free a device
VOID FreeDeviceInfo(PDEVICE_INFO device)
{
    if (!device) {
        return;
    }
    
    // Free strings
    FreeUnicodeString(&device->DeviceName);
    FreeUnicodeString(&device->DeviceId);
    FreeUnicodeString(&device->Manufacturer);
    
    // Free the structure
    ExFreePool(device);
}

// Function to add a device to the list
NTSTATUS AddDeviceToList(PDEVICE_INFO device)
{
    if (!device) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Add to the end of the list
    InsertTailList(&g_DeviceList, &device->ListEntry);
    
    return STATUS_SUCCESS;
}

// Function to remove a device from the list
VOID RemoveDeviceFromList(PDEVICE_INFO device)
{
    if (!device) {
        return;
    }
    
    // Remove from the list
    RemoveEntryList(&device->ListEntry);
}

// Function to print device info
VOID PrintDeviceInfo(PDEVICE_INFO device)
{
    if (!device) {
        return;
    }
    
    printf("Device Information:\n");
    printf("-------------------\n");
    
    printf("Name: ");
    DumpUnicodeString(&device->DeviceName, "Name");
    
    printf("ID: ");
    DumpUnicodeString(&device->DeviceId, "ID");
    
    printf("Manufacturer: ");
    DumpUnicodeString(&device->Manufacturer, "Manufacturer");
    
    printf("Type: %lu\n", device->DeviceType);
    printf("Status: %s\n\n", device->IsConnected ? "Connected" : "Disconnected");
}

// Function to print all devices
VOID PrintAllDevices()
{
    if (IsListEmpty(&g_DeviceList)) {
        printf("No devices in the list.\n");
        return;
    }
    
    printf("\nList of all devices:\n");
    printf("====================\n\n");
    
    PLIST_ENTRY entry = g_DeviceList.Flink;
    int count = 0;
    
    while (entry != &g_DeviceList) {
        PDEVICE_INFO device = CONTAINING_RECORD(entry, DEVICE_INFO, ListEntry);
        printf("Device #%d:\n", ++count);
        PrintDeviceInfo(device);
        entry = entry->Flink;
    }
}

// Main function
int main()
{
    // Print library version
    printf("Using WinKernelLite version %s\n\n", WINKERNELLITE_VERSION);
    
    // Initialize heap
    InitHeap();
    
    // Initialize device list
    InitializeListHead(&g_DeviceList);
    
    // Create some sample devices
    PDEVICE_INFO keyboard = CreateDeviceInfo(
        L"Keyboard",
        L"USB\\VID_045E&PID_00DB",
        L"Microsoft",
        1, // Input device
        TRUE // Connected
    );
    
    PDEVICE_INFO mouse = CreateDeviceInfo(
        L"Mouse",
        L"USB\\VID_046D&PID_C52B",
        L"Logitech",
        1, // Input device
        TRUE // Connected
    );
    
    PDEVICE_INFO printer = CreateDeviceInfo(
        L"Laser Printer",
        L"USB\\VID_03F0&PID_3D17",
        L"HP",
        2, // Output device
        FALSE // Not connected
    );
    
    // Add devices to the list
    AddDeviceToList(keyboard);
    AddDeviceToList(mouse);
    AddDeviceToList(printer);
    
    // Print all devices
    PrintAllDevices();
    
    // Disconnect a device
    printf("Disconnecting the mouse...\n");
    mouse->IsConnected = FALSE;
    
    // Print updated list
    PrintAllDevices();
    
    // Remove a device
    printf("Removing the printer...\n");
    RemoveDeviceFromList(printer);
    FreeDeviceInfo(printer);
    
    // Print updated list
    PrintAllDevices();
    
    // Clean up all remaining devices
    printf("Cleaning up all devices...\n");
    while (!IsListEmpty(&g_DeviceList)) {
        PLIST_ENTRY entry = RemoveHeadList(&g_DeviceList);
        PDEVICE_INFO device = CONTAINING_RECORD(entry, DEVICE_INFO, ListEntry);
        FreeDeviceInfo(device);
    }
    
    // Check for memory leaks
    printf("Checking for memory leaks...\n");
    PrintMemoryLeaks();
    
    // Clean up heap
    CleanupHeap();
    
    return 0;
}
