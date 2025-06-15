#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <stdio.h>

#include "DevicesList.h"
#include "WinKernelLite/UnicodeStringUtils.h"

#pragma comment(lib, "ntdll.lib")

extern void PrintMemoryLeaks(void);
extern void CleanupHeap(void);

void CreateMemoryLeak() {
    PVOID leakedMemory = ExAllocatePoolTracked(PagedPool, 1024);
    printf("Created a deliberate memory leak of 1024 bytes\n");
}

int main(void)
{
	InitHeap();
	InitializeListHead(&gDeviceList);
	
	// Use our new CreateDevice helper function instead of manual allocation
	PDEVICE_NAME device = CreateDevice(L"Sony", L"Walkman", L"SN12345");
	PDEVICE_NAME device1 = CreateDevice(L"Apple", L"iPhone", L"AP98765");
	PDEVICE_NAME device2 = CreateDevice(L"Microsoft", L"Surface", L"MS55555");

	// Check for allocation failures
	if (!device || !device1 || !device2) {
		// Free any successfully allocated devices that aren't owned by a list
		if (device) FreeDevice(device);
		if (device1) FreeDevice(device1);
		if (device2) FreeDevice(device2);
		printf("Memory allocation failed\n");
		return -1;
	}
		// Insert devices into list
	InsertDeviceListEx(&gDeviceList, device);
	InsertDeviceListEx(&gDeviceList, device1);
	InsertDeviceListEx(&gDeviceList, device2);
	
	// At this point, the list owns these devices, so don't call FreeDevice()
	// directly on these pointers anymore
	
    /* Debugging Tool: Uncomment to test memory leak detection
     * This intentionally creates a memory leak to verify that
     * the memory tracking system is working properly
     */
    // CreateMemoryLeak();

	// dump linked list entries
	PLIST_ENTRY currentPointer = gDeviceList.Flink;
	while (currentPointer != &gDeviceList)
	{
		PDEVICE_LIST_ENTRY listEntry = CONTAINING_RECORD(currentPointer, DEVICE_LIST_ENTRY, ListEntry);
		PDEVICE_NAME pdeviceName = listEntry->pDevName;

		// Print all available information for each device using the library's DumpUnicodeString
		printf("Device Information:\n");
		printf("Manufacturer: ");
		DumpUnicodeString(&pdeviceName->Manufacturer, "Manufacturer");
		printf("Product: ");
		DumpUnicodeString(&pdeviceName->Product, "Product");
		printf("Serial Number: ");
		DumpUnicodeString(&pdeviceName->SerialNumber, "SerialNumber");
		printf("\n");

		currentPointer = currentPointer->Flink;
	}

	// Clean up the whole list - using our new ownership-aware mechanism
	while (!IsListEmpty(&gDeviceList)) {
		PLIST_ENTRY entry = gDeviceList.Flink;
		PDEVICE_LIST_ENTRY listEntry = CONTAINING_RECORD(entry, DEVICE_LIST_ENTRY, ListEntry);
		
		// Remove and free the device in one safe operation
		RemoveAndFreeDevice(&gDeviceList, listEntry->pDevName);
	}

    // Print memory leak report at the end
    printf("\nChecking for memory leaks...\n");
    PrintMemoryLeaks();
    
    // Clean up our tracking resources
    CleanupHeap();


	return 0;
}
