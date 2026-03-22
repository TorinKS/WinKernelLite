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

#ifndef WINKERNEL_KERNELHEAP_H_
#define WINKERNEL_KERNELHEAP_H_

#include <Windows.h>
#include <stdio.h>
#include "LinkedList.h"
#include "Debug.h"  /* Add debug logging support */

/* Pool type definitions */

#ifdef __cplusplus
extern "C" {
#endif
typedef enum _POOL_TYPE {
    NonPagedPool,
    NonPagedPoolExecute = NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed = NonPagedPool + 2,
    DontUseThisType,
    NonPagedPoolCacheAligned = NonPagedPool + 4,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS = NonPagedPool + 6,
    MaxPoolType
} POOL_TYPE;

typedef struct _MEMORY_TRACKING_ENTRY {
    LIST_ENTRY ListEntry;    /* Linked list entry - must be first */
    PVOID Address;           /* Memory address */
    SIZE_T Size;            /* Size of allocation */
    const char* FileName;    /* Source file name */
    int LineNumber;         /* Line number in source file */
    ULONGLONG AllocationId;  /* Unique allocation identifier to handle address reuse */
} MEMORY_TRACKING_ENTRY, *PMEMORY_TRACKING_ENTRY;

/* Double-free tracking entry */
typedef struct _FREED_MEMORY_ENTRY {
    LIST_ENTRY ListEntry;    /* Linked list entry - must be first */
    PVOID Address;           /* Memory address that was freed */
    SIZE_T Size;            /* Size of the original allocation */
    const char* AllocFileName;    /* Source file where it was allocated */
    int AllocLineNumber;         /* Line number where it was allocated */
    const char* FreeFileName;     /* Source file where it was freed */
    int FreeLineNumber;          /* Line number where it was freed */
    DWORD ThreadId;             /* Thread ID that freed it */
    FILETIME FreeTime;          /* Time when it was freed */
    ULONGLONG AllocationId;     /* Unique allocation identifier to handle address reuse */
} FREED_MEMORY_ENTRY, *PFREED_MEMORY_ENTRY;

/* Global state structure */
typedef struct _GLOBAL_STATE {
    LIST_ENTRY MemoryAllocations;  /* Head of allocation tracking list */
    LIST_ENTRY FreedMemoryList;    /* Head of freed memory tracking list */
    SIZE_T AllocationCount;
    SIZE_T TotalBytesAllocated;
    SIZE_T CurrentBytesAllocated;
    SIZE_T PeakBytesAllocated;
    SIZE_T DoubleFreeCount;        /* Number of double-free attempts detected */
    SIZE_T FreedEntryCount;        /* Number of entries in freed memory list */
    SIZE_T MaxFreedEntries;        /* Maximum number of freed entries to keep (for memory management) */
    ULONGLONG NextAllocationId;    /* Counter for unique allocation IDs */
    HANDLE HeapHandle;
    BOOL SuppressErrors;      /* Control error message output */
    BOOL TrackFreedMemory;    /* Control whether to track freed memory for double-free detection */
    CRITICAL_SECTION TrackingLock;  /* Protects MemoryAllocations, FreedMemoryList, and counters */
} GLOBAL_STATE;

/* Function declarations - implementations in KernelHeap.c */
GLOBAL_STATE* GetGlobalState(void);
BOOL InitHeap(void);
void CleanupHeap(void);
void TrackAllocation(PVOID Address, SIZE_T Size, const char* FileName, int LineNumber);
BOOL UntrackAllocation(PVOID Address, const char* FreeFileName, int FreeLineNumber);
void TrackFreedMemoryLocked(PVOID Address, SIZE_T Size, const char* AllocFileName, int AllocLineNumber, const char* FreeFileName, int FreeLineNumber, ULONGLONG AllocationId);
BOOL CheckForDoubleFree(PVOID Address, const char* FreeFileName, int FreeLineNumber, ULONGLONG AllocationId);
void CleanupOldFreedEntries(void);
PVOID ExAllocatePoolWithTracking(POOL_TYPE PoolType, SIZE_T NumberOfBytes, const char* FileName, int LineNumber);
void _ExFreePoolWithTracking(PVOID pointer, const char* FileName, int LineNumber);
BOOL IsValidHeapPointer(PVOID pointer);
void PrintMemoryLeaks(void);
void PrintDoubleFreeReport(void);
void SetErrorSuppression(BOOL suppress);
BOOL GetErrorSuppression(void);
void SetFreedMemoryTracking(BOOL enable);
BOOL GetFreedMemoryTracking(void);
void SetMaxFreedEntries(SIZE_T maxEntries);
SIZE_T GetMaxFreedEntries(void);

#ifdef __cplusplus
}
#endif

/* Global state variable declaration - C-compatible external linkage */
#ifdef __cplusplus
extern "C" {
#endif

/* External declaration of global state variable - defined in KernelHeap.c */
extern GLOBAL_STATE* g_WinKernelLite_GlobalState;

/* Thin inline wrappers - these match the real WDK pattern of simple delegation */

__forceinline PVOID ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes) {
    return ExAllocatePoolWithTracking(PoolType, NumberOfBytes, "Unknown", 0);
}

__forceinline PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag) {
    UNREFERENCED_PARAMETER(Tag);
    return ExAllocatePool(PoolType, NumberOfBytes);
}

__forceinline PVOID ExAllocatePoolZeroWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag) {
    UNREFERENCED_PARAMETER(Tag);
    PVOID memory = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    if (memory) {
        ZeroMemory(memory, NumberOfBytes);
    }
    return memory;
}

__forceinline PVOID _ExAllocatePoolZeroWithTagTracked(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag, const char* FileName, int LineNumber) {
    UNREFERENCED_PARAMETER(Tag);
    PVOID memory = ExAllocatePoolWithTracking(PoolType, NumberOfBytes, FileName, LineNumber);
    if (memory) {
        ZeroMemory(memory, NumberOfBytes);
    }
    return memory;
}

__forceinline void ExFreePool(PVOID pointer) {
    _ExFreePoolWithTracking(pointer, "Unknown", 0);
}

__forceinline void ExFreePoolWithTag(PVOID pointer, ULONG Tag) {
    UNREFERENCED_PARAMETER(Tag);
    ExFreePool(pointer);
}

/* Macro definitions for automatic file and line capture */
#define ExAllocatePoolTracked(PoolType, NumberOfBytes) \
    ExAllocatePoolWithTracking(PoolType, NumberOfBytes, __FILE__, __LINE__)

#define ExAllocatePoolWithTagTracked(PoolType, NumberOfBytes, Tag) \
    ExAllocatePoolWithTracking(PoolType, NumberOfBytes, __FILE__, __LINE__)

#define ExAllocatePoolZeroWithTagTracked(PoolType, NumberOfBytes, Tag) \
    _ExAllocatePoolZeroWithTagTracked(PoolType, NumberOfBytes, Tag, __FILE__, __LINE__)

#define ExFreePoolTracked(pointer) \
    _ExFreePoolWithTracking(pointer, __FILE__, __LINE__)

#define ExFreePoolWithTagTracked(pointer, Tag) \
    _ExFreePoolWithTracking(pointer, __FILE__, __LINE__)

#define FREE_POOL_TRACKED(_poolptr) \
    do { \
        if (_poolptr != NULL) { \
            ExFreePoolTracked(_poolptr); \
            _poolptr = NULL; \
        } \
    } while(0)

/* Legacy macro for backward compatibility */
#define FREE_POOL(_poolptr) \
    do { \
        if (_poolptr != NULL) { \
            ExFreePool(_poolptr); \
            _poolptr = NULL; \
        } \
    } while(0)

/* Macro for ExAllocatePoolWithTag with FREE_POOL pattern */
#define FREE_POOL_WITH_TAG(_poolptr, _tag) \
    do { \
        if (_poolptr != NULL) { \
            ExFreePool(_poolptr); \
            _poolptr = NULL; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* WINKERNEL_KERNELHEAP_H_ */
