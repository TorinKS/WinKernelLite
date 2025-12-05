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
} GLOBAL_STATE;

/* Function declarations */
__forceinline GLOBAL_STATE* GetGlobalState(void);
__forceinline BOOL InitHeap(void);
__forceinline void CleanupHeap(void);
__forceinline void TrackAllocation(PVOID Address, SIZE_T Size, const char* FileName, int LineNumber);
__forceinline BOOL UntrackAllocation(PVOID Address, const char* FreeFileName, int FreeLineNumber);
__forceinline void TrackFreedMemory(PVOID Address, SIZE_T Size, const char* AllocFileName, int AllocLineNumber, const char* FreeFileName, int FreeLineNumber);
__forceinline void TrackFreedMemoryLocked(PVOID Address, SIZE_T Size, const char* AllocFileName, int AllocLineNumber, const char* FreeFileName, int FreeLineNumber, ULONGLONG AllocationId);
__forceinline BOOL CheckForDoubleFree(PVOID Address, const char* FreeFileName, int FreeLineNumber, ULONGLONG AllocationId);
__forceinline void CleanupOldFreedEntries(void);
__forceinline PVOID ExAllocatePoolWithTracking(POOL_TYPE PoolType, SIZE_T NumberOfBytes, const char* FileName, int LineNumber);
__forceinline PVOID ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes);
__forceinline PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);
__forceinline PVOID ExAllocatePoolZeroWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);
__forceinline PVOID _ExAllocatePoolZeroWithTagTracked(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag, const char* FileName, int LineNumber);
inline void _ExFreePoolWithTracking(PVOID pointer, const char* FileName, int LineNumber);
__forceinline void ExFreePool(PVOID pointer);
__forceinline void ExFreePoolWithTag(PVOID pointer, ULONG Tag);
__forceinline void PrintMemoryLeaks(void);
__forceinline void PrintDoubleFreeReport(void);
__forceinline void SetErrorSuppression(BOOL suppress);
__forceinline BOOL GetErrorSuppression(void);
__forceinline void SetFreedMemoryTracking(BOOL enable);
__forceinline BOOL GetFreedMemoryTracking(void);
__forceinline void SetMaxFreedEntries(SIZE_T maxEntries);
__forceinline SIZE_T GetMaxFreedEntries(void);
__forceinline BOOL IsValidHeapPointer(PVOID pointer);

#ifdef __cplusplus
}
#endif

/* Global state variable declaration - C-compatible external linkage */
#ifdef __cplusplus
extern "C" {
#endif

/* External declaration of global state variable - defined in KernelHeap.c */
extern GLOBAL_STATE* g_WinKernelLite_GlobalState;

/* Global state accessor - C-Compatible version */
__forceinline GLOBAL_STATE* GetGlobalState(void) {
    if (g_WinKernelLite_GlobalState == NULL) {
        HEAP_TRACE("GetGlobalState: Creating new global state");
        GLOBAL_STATE* temp = (GLOBAL_STATE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_STATE));
        if (temp != NULL) {
            HEAP_VERBOSE("GetGlobalState: Allocated global state at %p (size: %zu)", temp, sizeof(GLOBAL_STATE));
            /* CriticalSection initialization removed for performance */
            temp->HeapHandle = GetProcessHeap();
            HEAP_VERBOSE("GetGlobalState: Using heap handle %p", temp->HeapHandle);
            /* Initialize the linked lists immediately when creating global state */
            InitializeListHead(&temp->MemoryAllocations);
            InitializeListHead(&temp->FreedMemoryList);
            /* Initialize other fields to safe defaults */
            temp->AllocationCount = 0;
            temp->TotalBytesAllocated = 0;
            temp->CurrentBytesAllocated = 0;
            temp->PeakBytesAllocated = 0;
            temp->DoubleFreeCount = 0;
            temp->FreedEntryCount = 0;
            temp->MaxFreedEntries = 1000; /* Default: keep track of last 1000 freed allocations */
            temp->NextAllocationId = 1; /* Start allocation IDs at 1 */
            temp->SuppressErrors = FALSE;
            temp->TrackFreedMemory = TRUE; /* Enable double-free tracking by default */
            g_WinKernelLite_GlobalState = temp;
            HEAP_INFO("GetGlobalState: Global state initialized successfully");
        } else {
            HEAP_ERROR("GetGlobalState: Failed to allocate global state");
        }
    }
    return g_WinKernelLite_GlobalState;
}

__forceinline BOOL InitHeap(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (!state || !state->HeapHandle) {
        return FALSE;
    }
    
    // Global state is already properly initialized by GetGlobalState()
    // This function is now essentially a no-op but kept for compatibility
    // Reset statistics if needed (this is safe to call multiple times)
    // CriticalSection usage removed for performance - function is now non-thread-safe but faster
    
    // Only reset if we have no active allocations
    if (IsListEmpty(&state->MemoryAllocations)) {
        state->AllocationCount = 0;
        state->TotalBytesAllocated = 0;
        state->CurrentBytesAllocated = 0;
        state->PeakBytesAllocated = 0;
        state->DoubleFreeCount = 0;
        // Don't reset freed memory tracking - it's useful to keep across test runs
        state->SuppressErrors = FALSE;
        state->TrackFreedMemory = TRUE;
        if (state->MaxFreedEntries == 0) {
            state->MaxFreedEntries = 1000;
        }
    }
    
    return TRUE;
}

__forceinline void CleanupHeap(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        // Clean up any remaining tracking entries
        PLIST_ENTRY current, next;
        PMEMORY_TRACKING_ENTRY entry;
        PFREED_MEMORY_ENTRY freedEntry;
        
        // CriticalSection usage removed for performance - function is now non-thread-safe but faster
        
        // Clean up allocation tracking entries
        current = state->MemoryAllocations.Flink;
        while (current != &state->MemoryAllocations) {
            next = current->Flink;  // Save next before we free current
            entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
            
            // Free the tracking entry (the actual memory leak will remain)
            HeapFree(GetProcessHeap(), 0, entry);
            
            current = next;
        }
        
        // Clean up freed memory tracking entries
        current = state->FreedMemoryList.Flink;
        while (current != &state->FreedMemoryList) {
            next = current->Flink;  // Save next before we free current
            freedEntry = CONTAINING_RECORD(current, FREED_MEMORY_ENTRY, ListEntry);
            
            // Free the freed memory tracking entry
            HeapFree(GetProcessHeap(), 0, freedEntry);
            
            current = next;
        }
        
        // CriticalSection cleanup removed for performance
        HeapFree(GetProcessHeap(), 0, state);
        g_WinKernelLite_GlobalState = NULL;
    }
}

__forceinline void TrackAllocation(PVOID Address, SIZE_T Size, const char* FileName, int LineNumber) {
    GLOBAL_STATE* state;
    PMEMORY_TRACKING_ENTRY entry;
    
    state = GetGlobalState();
    if (!state) {
        return;
    }
    
    // Allocate a new tracking entry using the system heap (not our tracked heap)
    entry = (PMEMORY_TRACKING_ENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MEMORY_TRACKING_ENTRY));
    if (!entry) {
        if (!state->SuppressErrors) {
            HEAP_ERROR("Failed to allocate memory tracking entry");
        }
        return;
    }
    
    // Initialize the tracking entry
    entry->Address = Address;
    entry->Size = Size;
    entry->FileName = FileName;
    entry->LineNumber = LineNumber;
    entry->AllocationId = state->NextAllocationId++; // Assign unique ID
    
    // Fast path - no critical section overhead for performance
    // This function is now non-thread-safe but much faster
    
    // Insert at the head of the list for O(1) insertion
    InsertHeadList(&state->MemoryAllocations, &entry->ListEntry);
    
    state->AllocationCount++;
    state->TotalBytesAllocated += Size;
    state->CurrentBytesAllocated += Size;
    if (state->CurrentBytesAllocated > state->PeakBytesAllocated)
        state->PeakBytesAllocated = state->CurrentBytesAllocated;
}

__forceinline void TrackFreedMemoryLocked(PVOID Address, SIZE_T Size, const char* AllocFileName, int AllocLineNumber, const char* FreeFileName, int FreeLineNumber, ULONGLONG AllocationId) {
    GLOBAL_STATE* state = GetGlobalState();
    if (!state || !state->TrackFreedMemory) {
        return;
    }
    
    // Fast path - no critical section overhead for performance
    // This function is now non-thread-safe but much faster
    
    // Allocate a new freed memory tracking entry
    PFREED_MEMORY_ENTRY entry = (PFREED_MEMORY_ENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FREED_MEMORY_ENTRY));
    
    if (!entry) {
        if (!state->SuppressErrors) {
            HEAP_ERROR("Failed to allocate freed memory tracking entry");
        }
        return;
    }
    
    // Initialize the freed memory tracking entry
    entry->Address = Address;
    entry->Size = Size;
    entry->AllocFileName = AllocFileName;
    entry->AllocLineNumber = AllocLineNumber;
    entry->FreeFileName = FreeFileName;
    entry->FreeLineNumber = FreeLineNumber;
    entry->ThreadId = GetCurrentThreadId();
    entry->AllocationId = AllocationId; // Store the allocation ID
    GetSystemTimeAsFileTime(&entry->FreeTime);
    
    // Insert at the head of the freed memory list (non-thread-safe)
    InsertHeadList(&state->FreedMemoryList, &entry->ListEntry);
    state->FreedEntryCount++;
    
    // Clean up old entries if we've exceeded the maximum (non-thread-safe)
    if (state->FreedEntryCount > state->MaxFreedEntries) {
        CleanupOldFreedEntries();
    }
}

__forceinline BOOL CheckForDoubleFree(PVOID Address, const char* FreeFileName, int FreeLineNumber, ULONGLONG AllocationId) {
    GLOBAL_STATE* state;
    PLIST_ENTRY current;
    PFREED_MEMORY_ENTRY entry;
    BOOL found = FALSE;
    
    if (!Address) return FALSE; // Address is required
    
    state = GetGlobalState();
    if (!state || !state->TrackFreedMemory) return FALSE;
    
    // Fast path - no critical section overhead for performance
    // This function is now non-thread-safe but much faster
    
    // Walk the freed memory list looking for the exact address AND allocation ID match
    current = state->FreedMemoryList.Flink;
    while (current != &state->FreedMemoryList) {
        entry = CONTAINING_RECORD(current, FREED_MEMORY_ENTRY, ListEntry);
        
        // Check both address AND allocation ID for exact match
        if (entry->Address == Address && entry->AllocationId == AllocationId) {
            found = TRUE;
            state->DoubleFreeCount++;
            
            if (!state->SuppressErrors) {
                HEAP_ERROR("=== DOUBLE-FREE DETECTED ===");
                HEAP_ERROR("Address: %p (Size: %zu bytes, Allocation ID: %llu)", Address, entry->Size, AllocationId);
                HEAP_ERROR("Originally allocated at: %s:%d", entry->AllocFileName, entry->AllocLineNumber);
                HEAP_ERROR("First freed at: %s:%d (Thread: %lu)", 
                       entry->FreeFileName, entry->FreeLineNumber, entry->ThreadId);
                HEAP_ERROR("Attempted second free at: %s:%d (Thread: %lu)", 
                       FreeFileName, FreeLineNumber, GetCurrentThreadId());
                HEAP_ERROR("============================");
            }
            break;
        }
        
        current = current->Flink;
    }
    
    return found;
}

__forceinline void CleanupOldFreedEntries(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (!state) return;
    
    // This function should be called while holding the MemoryTrackingLock
    // Remove old entries from the tail of the list until we're under the limit
    while (state->FreedEntryCount > state->MaxFreedEntries && !IsListEmpty(&state->FreedMemoryList)) {
        PLIST_ENTRY lastEntry = state->FreedMemoryList.Blink;
        PFREED_MEMORY_ENTRY entry = CONTAINING_RECORD(lastEntry, FREED_MEMORY_ENTRY, ListEntry);
        
        RemoveEntryList(lastEntry);
        HeapFree(GetProcessHeap(), 0, entry);
        state->FreedEntryCount--;
    }
}

__forceinline BOOL UntrackAllocation(PVOID Address, const char* FreeFileName, int FreeLineNumber) {
    GLOBAL_STATE* state;
    PLIST_ENTRY current;
    PMEMORY_TRACKING_ENTRY entry;
    BOOL found = FALSE;
    const char* allocFileName = "Unknown";
    int allocLineNumber = 0;
    SIZE_T allocSize = 0;
    ULONGLONG allocationId = 0;
    
    if (!Address) {
        return FALSE;
    }
    
    state = GetGlobalState();
    if (!state) {
        return FALSE;
    }
    
    // Fast path - no critical section overhead for performance
    // This function is now non-thread-safe but much faster
    
    // Walk the linked list looking for the address
    current = state->MemoryAllocations.Flink;
    
    while (current != &state->MemoryAllocations) {
        entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
        
        if (entry->Address == Address) {
            // Found the entry - save allocation info for freed memory tracking
            allocFileName = entry->FileName;
            allocLineNumber = entry->LineNumber;
            allocSize = entry->Size;
            allocationId = entry->AllocationId;
            
            // Remove it from the allocation list
            RemoveEntryList(&entry->ListEntry);
            
            // Update statistics
            state->CurrentBytesAllocated -= entry->Size;
            
            // Free the tracking entry itself (using system heap)
            HeapFree(GetProcessHeap(), 0, entry);
            
            found = TRUE;
            break;
        }
        
        current = current->Flink;
    }
    
    // If we found and removed the allocation, track it as freed memory
    if (found && state->TrackFreedMemory) {
        TrackFreedMemoryLocked(Address, allocSize, allocFileName, allocLineNumber, FreeFileName, FreeLineNumber, allocationId);
    }
    
    return found;
}

__forceinline PVOID ExAllocatePoolWithTracking(POOL_TYPE PoolType, SIZE_T NumberOfBytes, const char* FileName, int LineNumber) {
    GLOBAL_STATE* state;
    PVOID ptr;
    
    UNREFERENCED_PARAMETER(PoolType); // Mark as unused parameter to fix C4100 warning
    
    state = GetGlobalState();
    if (!state) {
        return NULL;
    }
    
    // Fast path - no heap validation overhead for performance
    ptr = HeapAlloc(state->HeapHandle, 0, NumberOfBytes);
    
    // Return NULL if memory allocation failed
    if (ptr == NULL) {
        if (!state->SuppressErrors) {
            HEAP_ERROR("Memory allocation failed for %zu bytes", NumberOfBytes);
        }
        return NULL;
    }
    
    // Track the allocation (non-thread-safe but fast)
    TrackAllocation(ptr, NumberOfBytes, FileName, LineNumber);
    
    return ptr;
}

__forceinline PVOID ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes) {
    // PoolType parameter is passed to ExAllocatePoolWithTracking for compatibility, but not used there
    return ExAllocatePoolWithTracking(PoolType, NumberOfBytes, "Unknown", 0);
}

__forceinline PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag) {
    // The Tag parameter is ignored in this implementation since we're in user mode
    // It's only used for kernel-mode debugging and memory tracking
    UNREFERENCED_PARAMETER(Tag); // Mark as unused parameter to fix C4100 warning
    
    return ExAllocatePool(PoolType, NumberOfBytes);
}

__forceinline PVOID ExAllocatePoolZeroWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag) {
    // Allocate memory and initialize it to zeros
    // PoolType and Tag are passed along but aren't actually used in our implementation
    UNREFERENCED_PARAMETER(Tag); // Mark as unused parameter to fix C4100 warning
    
    PVOID memory = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    if (memory) {
        ZeroMemory(memory, NumberOfBytes);
    }
    return memory;
}

__forceinline PVOID _ExAllocatePoolZeroWithTagTracked(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag, const char* FileName, int LineNumber) {
    // Allocate memory with tracking and initialize it to zeros
    // PoolType and Tag parameters are not used in this implementation
    UNREFERENCED_PARAMETER(Tag); // Mark as unused parameter to fix C4100 warning
    
    PVOID memory = ExAllocatePoolWithTracking(PoolType, NumberOfBytes, FileName, LineNumber);
    if (memory) {
        ZeroMemory(memory, NumberOfBytes);
    }
    return memory;
}

__forceinline BOOL IsValidHeapPointer(PVOID pointer) {
    GLOBAL_STATE* state = GetGlobalState();
    if (!state || !pointer) return FALSE;
    
    // Fast path - no critical section overhead for performance
    // This function is now non-thread-safe but much faster
    
    // Walk the linked list looking for the address
    PLIST_ENTRY current = state->MemoryAllocations.Flink;
    while (current != &state->MemoryAllocations) {
        PMEMORY_TRACKING_ENTRY entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
        
        if (entry->Address == pointer) {
            return TRUE;
        }
        
        current = current->Flink;
    }
    
    return FALSE;
}

inline void _ExFreePoolWithTracking(PVOID pointer, const char* FileName, int LineNumber) {
    GLOBAL_STATE* state;
    BOOL found;
    BOOL isDoubleFree = FALSE;
    ULONGLONG currentAllocationId = 0;
    
    HEAP_TRACE("_ExFreePoolWithTracking: Freeing %p from %s:%d", pointer, 
               FileName ? FileName : "Unknown", LineNumber);
    
    if (!pointer) {
        HEAP_TRACE("_ExFreePoolWithTracking: NULL pointer provided, returning");
        return;
    }
    
    state = GetGlobalState();
    if (!state) {
        HEAP_ERROR("_ExFreePoolWithTracking: Failed to get global state");
        return;
    }
    
    HEAP_VERBOSE("_ExFreePoolWithTracking: State info - heap handle: %p, TrackFreedMemory: %d", 
                 state->HeapHandle, state->TrackFreedMemory);
    
    // First, get the allocation ID for this pointer from the active allocations list
    if (state->TrackFreedMemory) {
        PLIST_ENTRY current = state->MemoryAllocations.Flink;
        while (current != &state->MemoryAllocations) {
            PMEMORY_TRACKING_ENTRY entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
            if (entry->Address == pointer) {
                currentAllocationId = entry->AllocationId;
                break;
            }
            current = current->Flink;
        }
        
        // Only check for double-free if we found a current allocation ID
        if (currentAllocationId != 0) {
            HEAP_TRACE("_ExFreePoolWithTracking: Checking for double-free of %p (ID: %llu)", pointer, currentAllocationId);
            isDoubleFree = CheckForDoubleFree(pointer, FileName, LineNumber, currentAllocationId);
            if (isDoubleFree) {
                HEAP_WARN("_ExFreePoolWithTracking: Double-free detected for %p, returning without freeing", pointer);
                // Don't actually free the memory again - just return immediately
                return;
            }
            HEAP_TRACE("_ExFreePoolWithTracking: No double-free detected for %p", pointer);
        }
    }
    
    // Try to untrack the allocation - this will return TRUE if it was found and removed
    HEAP_TRACE("_ExFreePoolWithTracking: Calling UntrackAllocation for %p", pointer);
    found = UntrackAllocation(pointer, FileName, LineNumber);
    
    HEAP_VERBOSE("_ExFreePoolWithTracking: UntrackAllocation returned %d for %p", found, pointer);
    
    if (!found && !state->SuppressErrors) {
        HEAP_WARN("_ExFreePoolWithTracking: Address %p not found in tracking, validating heap pointer", pointer);
        // Check if it's a valid heap pointer that we just don't track
        BOOL isValidHeapPointer = FALSE;
        __try {
            // Safer check: try to validate with Windows heap functions
            isValidHeapPointer = HeapValidate(state->HeapHandle, 0, pointer);
            HEAP_VERBOSE("_ExFreePoolWithTracking: HeapValidate returned %d for %p", isValidHeapPointer, pointer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            HEAP_ERROR("_ExFreePoolWithTracking: Exception during HeapValidate for %p: 0x%08X", 
                       pointer, GetExceptionCode());
            isValidHeapPointer = FALSE;
        }
        
        if (isValidHeapPointer) {
            HEAP_WARN("Attempting to free untracked but valid heap memory at %p from %s:%d", 
                   pointer, FileName, LineNumber);
        } else {
            HEAP_WARN("Attempting to free invalid memory pointer at %p from %s:%d", 
                   pointer, FileName, LineNumber);
        }
    }
    
    // Free the memory only if:
    // 1. It's not a double-free AND
    // 2. Either we found it in our tracking (meaning it's valid) OR it's an untracked but valid heap pointer
    if (!isDoubleFree) {
        HEAP_TRACE("_ExFreePoolWithTracking: Calling HeapFree for %p", pointer);
        __try {
            HeapFree(state->HeapHandle, 0, pointer);
            HEAP_VERBOSE("_ExFreePoolWithTracking: Successfully freed %p", pointer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            HEAP_ERROR("_ExFreePoolWithTracking: Exception occurred while freeing memory at %p from %s:%d (Exception: 0x%08X)", 
                       pointer, FileName, LineNumber, GetExceptionCode());
            if (!state->SuppressErrors) {
                HEAP_ERROR("Exception occurred while freeing memory at %p from %s:%d (Exception: 0x%08X)", 
                       pointer, FileName, LineNumber, GetExceptionCode());
            }
        }
    }
}

__forceinline void ExFreePool(PVOID pointer) {
    _ExFreePoolWithTracking(pointer, "Unknown", 0);
}

__forceinline void ExFreePoolWithTag(PVOID pointer, ULONG Tag) {
    // The Tag parameter is ignored in this implementation since we're in user mode
    // It's only used for kernel-mode debugging and memory tracking
    UNREFERENCED_PARAMETER(Tag); // Mark as unused parameter to fix C4100 warning
    ExFreePool(pointer);
}

__forceinline void PrintMemoryLeaks(void) {
    GLOBAL_STATE* state;
    BOOL foundLeaks;
    SIZE_T leakCount;
    SIZE_T leakBytes;
    PLIST_ENTRY current;
    PMEMORY_TRACKING_ENTRY entry;
    
    state = GetGlobalState();
    if (!state) return;
    
    foundLeaks = FALSE;
    leakCount = 0;
    leakBytes = 0;
    
    // CriticalSection usage removed for performance - function is now non-thread-safe but faster
    
    HEAP_INFO("=== MEMORY LEAK REPORT ===");
    
    // Walk the linked list of allocations
    current = state->MemoryAllocations.Flink;
    while (current != &state->MemoryAllocations) {
        entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
        
        if (!foundLeaks) {
            HEAP_INFO("Address       | Size     | Allocation Location");
            HEAP_INFO("------------- | -------- | ------------------");
            foundLeaks = TRUE;
        }
        
        HEAP_INFO("%p | %8d | %s:%d", 
            entry->Address,
            (int)entry->Size,
            entry->FileName,
            entry->LineNumber);
            
        leakCount++;
        leakBytes += entry->Size;
        
        current = current->Flink;
    }
    
    if (foundLeaks) {
        HEAP_INFO("Total: %d leaks, %d bytes", (int)leakCount, (int)leakBytes);
    } else {
        HEAP_INFO("No memory leaks detected!");
    }
    
    HEAP_INFO("Memory usage statistics:");
    HEAP_INFO("  Total allocations: %d", (int)state->AllocationCount);
    HEAP_INFO("  Total bytes allocated: %d", (int)state->TotalBytesAllocated);
    HEAP_INFO("  Peak bytes allocated: %d", (int)state->PeakBytesAllocated);
    HEAP_INFO("  Double-free attempts: %d", (int)state->DoubleFreeCount);
    HEAP_INFO("  Freed entries tracked: %d", (int)state->FreedEntryCount);
    HEAP_INFO("===========================");
}

__forceinline void PrintDoubleFreeReport(void) {
    GLOBAL_STATE* state;
    PLIST_ENTRY current;
    PFREED_MEMORY_ENTRY entry;
    SIZE_T entryCount = 0;
    
    state = GetGlobalState();
    if (!state) return;
    
    // CriticalSection usage removed for performance - function is now non-thread-safe but faster
    
    HEAP_INFO("=== FREED MEMORY REPORT ===");
    HEAP_INFO("Total double-free attempts detected: %d", (int)state->DoubleFreeCount);
    HEAP_INFO("Currently tracking %d freed allocations", (int)state->FreedEntryCount);
    HEAP_INFO("Maximum freed entries to track: %d", (int)state->MaxFreedEntries);
    
    if (state->FreedEntryCount > 0) {
        HEAP_INFO("Recent freed allocations:");
        HEAP_INFO("Address       | Size     | Alloc ID | Alloc Location  | Free Location   | Thread");
        HEAP_INFO("------------- | -------- | -------- | --------------- | --------------- | ------");
        
        // Walk the freed memory list (showing most recent first)
        current = state->FreedMemoryList.Flink;
        while (current != &state->FreedMemoryList && entryCount < 20) // Limit output to 20 entries
        {
            entry = CONTAINING_RECORD(current, FREED_MEMORY_ENTRY, ListEntry);
            
            HEAP_INFO("%p | %8d | %8llu | %15s:%-4d | %15s:%-4d | %6lu", 
                entry->Address,
                (int)entry->Size,
                entry->AllocationId,
                entry->AllocFileName, entry->AllocLineNumber,
                entry->FreeFileName, entry->FreeLineNumber,
                entry->ThreadId);
                
            entryCount++;
            current = current->Flink;
        }
        
        if (state->FreedEntryCount > 20) {
            HEAP_INFO("... and %d more entries", (int)(state->FreedEntryCount - 20));
        }
    }
    
    HEAP_INFO("==============================");
}

__forceinline void SetErrorSuppression(BOOL suppress) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        state->SuppressErrors = suppress;
    }
}

__forceinline BOOL GetErrorSuppression(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        return state->SuppressErrors;
    }
    return FALSE;
}

__forceinline void SetFreedMemoryTracking(BOOL enable) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        state->TrackFreedMemory = enable;
    }
}

__forceinline BOOL GetFreedMemoryTracking(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        return state->TrackFreedMemory;
    }
    return FALSE;
}

__forceinline void SetMaxFreedEntries(SIZE_T maxEntries) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        // CriticalSection usage removed for performance - function is now non-thread-safe but faster
        state->MaxFreedEntries = maxEntries;
        
        // Clean up excess entries if needed
        if (state->FreedEntryCount > maxEntries) {
            CleanupOldFreedEntries();
        }
    }
}

__forceinline SIZE_T GetMaxFreedEntries(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        return state->MaxFreedEntries;
    }
    return 0;
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

