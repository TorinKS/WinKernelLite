#pragma once

#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <stdio.h>
#include "WinKernelLite/KernelHeapAlloc.h"
#include "WinKernelLite/LinkedList.h"
#include "WinKernelLite/UnicodeString.h"

#define DebugPrint(x) printf x

extern LIST_ENTRY gDeviceList;

typedef struct _DEVICE_NAME
{
	LIST_ENTRY ListEntry;
	UNICODE_STRING usGuid; // GUID of device. {36fc9e60-c465-11cf-8056-444553540000} for USB devices
	UNICODE_STRING usDeviceType; // Subclass, see DeviceTypes.h
	UNICODE_STRING usDeviceHardwareId;
	UNICODE_STRING usDevicePropertyDriverKeyName;

	// USB specific fields
	UNICODE_STRING Manufacturer;
	UNICODE_STRING Product;
	UNICODE_STRING SerialNumber;

	// Flag to track if device is owned by a list
	BOOLEAN OwnedByList;

} DEVICE_NAME, * PDEVICE_NAME;

// element definition for linked list 
typedef struct
{
	LIST_ENTRY		ListEntry;
	PDEVICE_NAME	pDevName;
} DEVICE_LIST_ENTRY, * PDEVICE_LIST_ENTRY;

// Function declarations - only declarations, no definitions

/**
 * @brief Creates a new device with the specified attributes
 * 
 * @param manufacturer Wide character string for the device manufacturer
 * @param product Wide character string for the product name
 * @param serialNumber Wide character string for the serial number
 * @return PDEVICE_NAME Pointer to the newly created device, or NULL if allocation failed
 */
PDEVICE_NAME CreateDevice(PCWSTR manufacturer, PCWSTR product, PCWSTR serialNumber);

/**
 * @brief Frees all resources associated with a device
 * 
 * @param device Pointer to the device to be freed
 */
VOID FreeDevice(PDEVICE_NAME device);

/**
 * @brief Removes a device from a list and frees all associated resources
 * 
 * @param pListHead Pointer to the head of the list containing the device
 * @param pDevName Pointer to the device to be removed and freed
 */
VOID RemoveAndFreeDevice(PLIST_ENTRY pListHead, PDEVICE_NAME pDevName);

/**
 * @brief Inserts a device into a list
 * 
 * @param pListHead Pointer to the head of the list where the device will be inserted
 * @param pDevName Pointer to the device to be inserted
 * @return NTSTATUS STATUS_SUCCESS if successful, appropriate error code otherwise
 */
NTSTATUS InsertDeviceListEx(__in PLIST_ENTRY pListHead, __in PDEVICE_NAME pDevName);