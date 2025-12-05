/*
 * Copyright 2025 WinKernelLite Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WINKERNEL_WDM_H_
#define WINKERNEL_WDM_H_

#include <string.h>
#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>

// Ensure we have the necessary Windows NT definitions
#ifndef _NTDEF_
typedef LONG NTSTATUS;
typedef SHORT CSHORT;
#endif

// Define SIZE_T if it's not already defined
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef ULONG_PTR SIZE_T;
#endif

/**
 * @brief Basic types for WinKernelLite
 */
typedef ULONG_PTR KSPIN_LOCK, *PKSPIN_LOCK;
typedef LONG KPRIORITY;

// Forward declarations for structures referenced in DEVICE_OBJECT
struct _DRIVER_OBJECT;
struct _DEVICE_OBJECT;
struct _IRP;
struct _DEVOBJ_EXTENSION;
struct _IO_TIMER;
typedef struct _IO_TIMER *PIO_TIMER;

/**
 * @brief Device types
 *
 * Specifies the type of device represented by a device object
 */
typedef ULONG DEVICE_TYPE;
typedef DEVICE_TYPE* PDEVICE_TYPE;

// Device type definitions
#define FILE_DEVICE_BEEP                    0x00000001
#define FILE_DEVICE_CD_ROM                  0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM      0x00000003
#define FILE_DEVICE_CONTROLLER              0x00000004
#define FILE_DEVICE_DATALINK                0x00000005
#define FILE_DEVICE_DFS                     0x00000006
#define FILE_DEVICE_DISK                    0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM        0x00000008
#define FILE_DEVICE_FILE_SYSTEM             0x00000009
#define FILE_DEVICE_INPORT_PORT             0x0000000a
#define FILE_DEVICE_KEYBOARD                0x0000000b
#define FILE_DEVICE_MAILSLOT                0x0000000c
#define FILE_DEVICE_MIDI_IN                 0x0000000d
#define FILE_DEVICE_MIDI_OUT                0x0000000e
#define FILE_DEVICE_MOUSE                   0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER      0x00000010
#define FILE_DEVICE_NAMED_PIPE              0x00000011
#define FILE_DEVICE_NETWORK                 0x00000012
#define FILE_DEVICE_NETWORK_BROWSER         0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM     0x00000014
#define FILE_DEVICE_NULL                    0x00000015
#define FILE_DEVICE_PARALLEL_PORT           0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD        0x00000017
#define FILE_DEVICE_PRINTER                 0x00000018
#define FILE_DEVICE_SCANNER                 0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT       0x0000001a
#define FILE_DEVICE_SERIAL_PORT             0x0000001b
#define FILE_DEVICE_SCREEN                  0x0000001c
#define FILE_DEVICE_SOUND                   0x0000001d
#define FILE_DEVICE_STREAMS                 0x0000001e
#define FILE_DEVICE_TAPE                    0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM        0x00000020
#define FILE_DEVICE_TRANSPORT               0x00000021
#define FILE_DEVICE_UNKNOWN                 0x00000022
#define FILE_DEVICE_VIDEO                   0x00000023
#define FILE_DEVICE_VIRTUAL_DISK            0x00000024
#define FILE_DEVICE_WAVE_IN                 0x00000025
#define FILE_DEVICE_WAVE_OUT                0x00000026
#define FILE_DEVICE_8042_PORT               0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR      0x00000028
#define FILE_DEVICE_BATTERY                 0x00000029
#define FILE_DEVICE_BUS_EXTENDER            0x0000002a
#define FILE_DEVICE_MODEM                   0x0000002b
#define FILE_DEVICE_VDM                     0x0000002c
#define FILE_DEVICE_MASS_STORAGE            0x0000002d
#define FILE_DEVICE_SMB                     0x0000002e
#define FILE_DEVICE_KS                      0x0000002f
#define FILE_DEVICE_CHANGER                 0x00000030
#define FILE_DEVICE_SMARTCARD               0x00000031
#define FILE_DEVICE_ACPI                    0x00000032
#define FILE_DEVICE_DVD                     0x00000033
#define FILE_DEVICE_FULLSCREEN_VIDEO        0x00000034
#define FILE_DEVICE_DFS_FILE_SYSTEM         0x00000035
#define FILE_DEVICE_DFS_VOLUME              0x00000036
#define FILE_DEVICE_SERENUM                 0x00000037
#define FILE_DEVICE_TERMSRV                 0x00000038
#define FILE_DEVICE_KSEC                    0x00000039
#define FILE_DEVICE_FIPS                    0x0000003A
#define FILE_DEVICE_INFINIBAND              0x0000003B

// Forward declarations for structures referenced in DEVICE_OBJECT
struct _DRIVER_OBJECT;
struct _DEVICE_OBJECT;
struct _IRP;
struct _DEVOBJ_EXTENSION;

/**
 * @brief Dispatcher Header structure
 */
typedef struct _DISPATCHER_HEADER {
    UCHAR Type;
    UCHAR Absolute;
    UCHAR Size;
    UCHAR Inserted;
    LONG SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER;

/**
 * @brief Kernel Event structure
 */
typedef struct _KEVENT {
    DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT;

/**
 * @brief Device Queue Entry structure
 */
typedef struct _KDEVICE_QUEUE_ENTRY {
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;

/**
 * @brief Device Queue structure
 */
typedef struct _KDEVICE_QUEUE {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY DeviceListHead;
    KSPIN_LOCK Lock;
    BOOLEAN Busy;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE;

/**
 * @brief Deferred Procedure Call structure
 */
typedef struct _KDPC {
    CSHORT Type;
    UCHAR Number;
    UCHAR Importance;
    LIST_ENTRY DpcListEntry;
    PVOID DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    PVOID DpcData;
} KDPC, *PKDPC;

/**
 * @brief Volume Parameter Block structure
 * 
 * Represents mounted file system volume
 */
typedef struct _VPB {
    CSHORT Type;
    CSHORT Size;
    USHORT Flags;
    USHORT VolumeLabelLength;
    struct _DEVICE_OBJECT *DeviceObject;
    struct _DEVICE_OBJECT *RealDevice;
    ULONG SerialNumber;
    ULONG ReferenceCount;
    WCHAR VolumeLabel[32];
} VPB, *PVPB;

/**
 * @brief Wait Context Block structure
 */
typedef struct _WAIT_CONTEXT_BLOCK {
    KDEVICE_QUEUE_ENTRY WaitQueueEntry;
    PVOID DeviceRoutine;
    PVOID DeviceContext;
    ULONG NumberOfMapRegisters;
    PVOID DeviceObject;
    PVOID CurrentIrp;
    PKDPC BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;

/**
 * @brief Security descriptor structure
 */
typedef PVOID PSECURITY_DESCRIPTOR;

/**
 * @brief Device Object structure
 */
typedef struct _DEVICE_OBJECT {
    CSHORT Type;
    USHORT Size;                     // Size of the device object
    LONG ReferenceCount;             // Reference count for open handles
    struct _DRIVER_OBJECT *DriverObject; // Pointer to driver object
    struct _DEVICE_OBJECT *NextDevice;   // Next device created by the same driver
    struct _DEVICE_OBJECT *AttachedDevice; // Attached device (filter drivers)
    struct _IRP *CurrentIrp;         // Current IRP being processed
    PIO_TIMER Timer;                 // Device timer
    ULONG Flags;                     // Device flags (DO_xxx)
    ULONG Characteristics;           // Device characteristics
    PVPB Vpb;                        // Volume parameter block
    PVOID DeviceExtension;           // Driver-defined device extension
    DEVICE_TYPE DeviceType;          // Type of device
    CCHAR StackSize;                 // Minimum IRP stack size
    union {
        LIST_ENTRY ListEntry;        // Entry for list of devices
        WAIT_CONTEXT_BLOCK Wcb;      // Wait context block
    } Queue;
    ULONG AlignmentRequirement;      // Alignment requirement for data transfers
    KDEVICE_QUEUE DeviceQueue;       // Queue for pending IRPs
    KDPC Dpc;                        // DPC object for the device
    ULONG ActiveThreadCount;         // Number of active threads
    PSECURITY_DESCRIPTOR SecurityDescriptor; // Security descriptor
    KEVENT DeviceLock;               // Synchronization event
    USHORT SectorSize;               // Sector size (for volumes)
    USHORT Spare1;                   // Reserved
    struct _DEVOBJ_EXTENSION *DeviceObjectExtension; // Device object extension
    PVOID Reserved;                  // Reserved for system use
} DEVICE_OBJECT, *PDEVICE_OBJECT;

/**
 * @brief Device flags definitions
 */
#define DO_VERIFY_VOLUME              0x00000002
#define DO_BUFFERED_IO                0x00000004
#define DO_EXCLUSIVE                  0x00000008
#define DO_DIRECT_IO                  0x00000010
#define DO_MAP_IO_BUFFER              0x00000020
#define DO_DEVICE_INITIALIZING        0x00000080
#define DO_SHUTDOWN_REGISTERED        0x00000800
#define DO_BUS_ENUMERATED_DEVICE      0x00001000
#define DO_POWER_PAGABLE              0x00002000
#define DO_POWER_INRUSH               0x00004000
#define DO_DEVICE_TO_BE_RESET         0x04000000
#define DO_DAX_VOLUME                 0x10000000

/**
 * @brief Device characteristics definitions
 */
#define FILE_REMOVABLE_MEDIA          0x00000001
#define FILE_READ_ONLY_DEVICE         0x00000002
#define FILE_FLOPPY_DISKETTE          0x00000004
#define FILE_WRITE_ONCE_MEDIA         0x00000008
#define FILE_REMOTE_DEVICE            0x00000010
#define FILE_DEVICE_IS_MOUNTED        0x00000020
#define FILE_VIRTUAL_VOLUME           0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME 0x00000080
#define FILE_DEVICE_SECURE_OPEN       0x00000100
#define FILE_CHARACTERISTIC_PNP_DEVICE 0x00000800
#define FILE_CHARACTERISTIC_TS_DEVICE 0x00001000
#define FILE_CHARACTERISTIC_WEBDAV_DEVICE 0x00002000
#define FILE_CHARACTERISTIC_CSV       0x00010000
#define FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL 0x00020000
#define FILE_PORTABLE_DEVICE          0x00040000

/**
 * @brief Trace level definitions for DoTraceEx function
 */
#define TRACE_LEVEL_NONE        0
#define TRACE_LEVEL_CRITICAL    1
#define TRACE_LEVEL_FATAL       1
#define TRACE_LEVEL_ERROR       2
#define TRACE_LEVEL_WARNING     3
#define TRACE_LEVEL_INFORMATION 4
#define TRACE_LEVEL_VERBOSE     5

/**
 * @brief Debug component definitions for DoTraceEx function
 */
#define APP_GENERAL        0x00000001
#define APP_WL_FILTER      0x00000002
#define APP_DEVICE_FILTER  0x00000004

/**
 * @brief Outputs a formatted trace message with specified level and component
 *
 * This function provides a debug tracing mechanism similar to WPP tracing in
 * Windows drivers. In WinKernelLite's user-mode simulation, this outputs to
 * standard output.
 *
 * @param Level Trace severity level (one of TRACE_LEVEL_* constants)
 * @param Component Debug component identifier (one of DC_* constants)
 * @param Format Printf-style format string
 * @param ... Variable arguments used in the format string
 */
__inline void DoTraceEx(
    ULONG Level,
    ULONG Component,
    const char* Format,
    ...
) {
    // Only trace if we're in debug mode
#ifdef _DEBUG
    va_list Args;
    char Buffer[512];
    const char* LevelStr = "UNKNOWN";
    
    switch (Level) {
        case TRACE_LEVEL_NONE:        LevelStr = "NONE"; break;
        case TRACE_LEVEL_CRITICAL:    LevelStr = "CRITICAL"; break;
        case TRACE_LEVEL_ERROR:       LevelStr = "ERROR"; break;
        case TRACE_LEVEL_WARNING:     LevelStr = "WARNING"; break;
        case TRACE_LEVEL_INFORMATION: LevelStr = "INFO"; break;
        case TRACE_LEVEL_VERBOSE:     LevelStr = "VERBOSE"; break;
    }
    
    // Format the component as hex
    char ComponentStr[16];
    sprintf_s(ComponentStr, sizeof(ComponentStr), "0x%08X", Component);
    
    // Format the prefix
    char Prefix[64];
    sprintf_s(Prefix, sizeof(Prefix), "[%s][%s] ", LevelStr, ComponentStr);
    
    // Format the message
    va_start(Args, Format);
    vsprintf_s(Buffer, sizeof(Buffer), Format, Args);
    va_end(Args);
    
    // Output to debug console
    OutputDebugStringA(Prefix);
    OutputDebugStringA(Buffer);
    OutputDebugStringA("\n");
    
    // Also output to standard output
    printf("%s%s\n", Prefix, Buffer);
#endif // _DEBUG
}

// Memory operation functions

/**
 * @brief Memory utility functions
 * 
 * These macros provide common memory operations used in kernel-mode code.
 * They are implemented using standard C library functions for WinKernelLite's
 * user-mode simulation.
 */
// Only define these macros if they're not already defined by Windows SDK
#ifndef RtlZeroMemory
#define RtlZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#endif

#ifndef RtlFillMemory
#define RtlFillMemory(Destination, Length, Fill) memset((Destination), (Fill), (Length))
#endif

#ifndef RtlCopyMemory
#define RtlCopyMemory(Destination, Source, Length) memcpy_s((Destination), (Length), (Source), (Length))
#endif

#ifndef RtlMoveMemory
#define RtlMoveMemory(Destination, Source, Length) memmove((Destination), (Source), (Length))
#endif

#ifndef RtlCompareMemory
#define RtlCompareMemory(Source1, Source2, Length) memcmp((Source1), (Source2), (Length))
#endif

// Define memory operation macros as aliases to functions
#ifndef RtlZeroBytes
#define RtlZeroBytes RtlZeroMemory
#endif

#ifndef RtlFillBytes
#define RtlFillBytes RtlFillMemory
#endif

#ifndef RtlCopyBytes
#define RtlCopyBytes RtlCopyMemory
#endif
#define RtlMoveBytes RtlMoveMemory
#define RtlCompareBytes RtlCompareMemory

#endif // WINKERNEL_WDM_H_
