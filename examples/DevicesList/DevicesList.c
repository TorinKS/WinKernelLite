// Include DevicesList.h which already contains all the necessary WinKernelLite headers
#include "DevicesList.h"

// Define the global device list - initialized in the .c file to avoid multiple definitions
LIST_ENTRY gDeviceList = {0};  // Zero-initialize for safety

// Helper function to clean up a device during initialization failure
static VOID CleanupFailedDevice(
    PDEVICE_NAME device,
    BOOLEAN cleanManufacturer,
    BOOLEAN cleanProduct,
    BOOLEAN cleanSerial)
{
    if (cleanManufacturer && device->Manufacturer.Buffer) {
        FreeUnicodeString(&device->Manufacturer);
    }
    
    if (cleanProduct && device->Product.Buffer) {
        FreeUnicodeString(&device->Product);
    }
    
    if (cleanSerial && device->SerialNumber.Buffer) {
        FreeUnicodeString(&device->SerialNumber);
    }
    ExFreePool(device);
}

// Implementation of CreateDevice
PDEVICE_NAME CreateDevice(PCWSTR manufacturer, PCWSTR product, PCWSTR serialNumber)
{
    // Use tracked allocation
    PDEVICE_NAME device = ExAllocatePoolTracked(PagedPool, sizeof(DEVICE_NAME));
    if (!device) {
        return NULL;
    }

    RtlZeroMemory(device, sizeof(DEVICE_NAME));
    
    // Initialize ownership flag - device not yet owned by any list
    device->OwnedByList = FALSE;
    
    BOOLEAN hasManufacturer = FALSE;
    BOOLEAN hasProduct = FALSE;
    BOOLEAN hasSerial = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    
    // Initialize the required fields
    if (manufacturer) {
        UNICODE_STRING tempString;
        status = RtlInitUnicodeString(&tempString, manufacturer);
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        status = RtlDuplicateUnicodeString(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
            &tempString,
            &device->Manufacturer
        );
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        hasManufacturer = TRUE;
    }
    
    if (product) {
        UNICODE_STRING tempString;
        status = RtlInitUnicodeString(&tempString, product);
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        status = RtlDuplicateUnicodeString(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
            &tempString,
            &device->Product
        );
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        hasProduct = TRUE;
    }
    
    if (serialNumber) {
        UNICODE_STRING tempString;
        status = RtlInitUnicodeString(&tempString, serialNumber);
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        status = RtlDuplicateUnicodeString(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
            &tempString,
            &device->SerialNumber
        );
        if (!NT_SUCCESS(status)) {
            goto Cleanup;
        }
        hasSerial = TRUE;
    }
    
    return device;
    
Cleanup:
    CleanupFailedDevice(device, hasManufacturer, hasProduct, hasSerial);
    return NULL;
}

VOID FreeDevice(PDEVICE_NAME device)
{
    if (!device) {
        return;
    }
    
    // Free the strings
    FreeUnicodeString(&device->usGuid);
    FreeUnicodeString(&device->usDeviceType);
    FreeUnicodeString(&device->usDeviceHardwareId);
    FreeUnicodeString(&device->usDevicePropertyDriverKeyName);
    FreeUnicodeString(&device->Manufacturer);
    FreeUnicodeString(&device->Product);
    FreeUnicodeString(&device->SerialNumber);
    
    // Free the device
    ExFreePool(device);
}

VOID RemoveAndFreeDevice(PLIST_ENTRY pListHead, PDEVICE_NAME pDevName)
{
    if (!pListHead || !pDevName) {
        return;
    }
    
    // Find the entry in the list
    PLIST_ENTRY pEntry = pListHead->Flink;
    while (pEntry != pListHead) {
        PDEVICE_LIST_ENTRY pListEntry = CONTAINING_RECORD(pEntry, DEVICE_LIST_ENTRY, ListEntry);
        if (pListEntry->pDevName == pDevName) {
            // Remove the entry from the list
            RemoveEntryList(pEntry);
            
            // Free the device
            FreeDevice(pDevName);
            
            // Free the list entry
            ExFreePool(pListEntry);
            
            return;
        }
        
        pEntry = pEntry->Flink;
    }
}

NTSTATUS InsertDeviceListEx(__in PLIST_ENTRY pListHead, __in PDEVICE_NAME pDevName)
{
    if (!pListHead || !pDevName) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Create a new list entry
    PDEVICE_LIST_ENTRY pListEntry = ExAllocatePoolTracked(PagedPool, sizeof(DEVICE_LIST_ENTRY));
    if (!pListEntry) {
        return STATUS_NO_MEMORY;
    }
    
    // Initialize the list entry
    pListEntry->pDevName = pDevName;
    
    // Insert the entry into the list
    InsertTailList(pListHead, &pListEntry->ListEntry);
    
    // Mark the device as owned by a list
    pDevName->OwnedByList = TRUE;
    
    return STATUS_SUCCESS;
}
