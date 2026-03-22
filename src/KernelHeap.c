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

#include "include/KernelHeap.h"
#include "include/Debug.h"

/* Global state variable definition - single instance across all translation units */
GLOBAL_STATE* g_WinKernelLite_GlobalState = NULL;

GLOBAL_STATE* GetGlobalState(void) {
    GLOBAL_STATE* state = g_WinKernelLite_GlobalState;
    if (state != NULL) {
        return state; /* Fast path - already initialized */
    }

    HEAP_TRACE("GetGlobalState: Creating new global state");
    GLOBAL_STATE* temp = (GLOBAL_STATE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_STATE));
    if (temp == NULL) {
        HEAP_ERROR("GetGlobalState: Failed to allocate global state");
        return NULL;
    }

    HEAP_VERBOSE("GetGlobalState: Allocated global state at %p (size: %zu)", temp, sizeof(GLOBAL_STATE));
    temp->HeapHandle = GetProcessHeap();
    HEAP_VERBOSE("GetGlobalState: Using heap handle %p", temp->HeapHandle);
    InitializeListHead(&temp->MemoryAllocations);
    InitializeListHead(&temp->FreedMemoryList);
    temp->AllocationCount = 0;
    temp->TotalBytesAllocated = 0;
    temp->CurrentBytesAllocated = 0;
    temp->PeakBytesAllocated = 0;
    temp->DoubleFreeCount = 0;
    temp->FreedEntryCount = 0;
    temp->MaxFreedEntries = 1000;
    temp->NextAllocationId = 1;
    temp->SuppressErrors = FALSE;
    temp->TrackFreedMemory = TRUE;

    /* Atomically publish: if another thread won the race, free our copy */
    if (InterlockedCompareExchangePointer(
            (PVOID*)&g_WinKernelLite_GlobalState, temp, NULL) != NULL) {
        /* Another thread initialized first - free our allocation */
        HEAP_TRACE("GetGlobalState: Lost race, freeing duplicate allocation at %p", temp);
        HeapFree(GetProcessHeap(), 0, temp);
    } else {
        HEAP_INFO("GetGlobalState: Global state initialized successfully");
    }

    return g_WinKernelLite_GlobalState;
}

BOOL InitHeap(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (!state || !state->HeapHandle) {
        return FALSE;
    }

    /* Only reset if we have no active allocations */
    if (IsListEmpty(&state->MemoryAllocations)) {
        state->AllocationCount = 0;
        state->TotalBytesAllocated = 0;
        state->CurrentBytesAllocated = 0;
        state->PeakBytesAllocated = 0;
        state->DoubleFreeCount = 0;
        state->SuppressErrors = FALSE;
        state->TrackFreedMemory = TRUE;
        if (state->MaxFreedEntries == 0) {
            state->MaxFreedEntries = 1000;
        }
    }

    return TRUE;
}

void CleanupHeap(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        PLIST_ENTRY current, next;
        PMEMORY_TRACKING_ENTRY entry;
        PFREED_MEMORY_ENTRY freedEntry;

        /* Clean up allocation tracking entries */
        current = state->MemoryAllocations.Flink;
        while (current != &state->MemoryAllocations) {
            next = current->Flink;
            entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
            HeapFree(GetProcessHeap(), 0, entry);
            current = next;
        }

        /* Clean up freed memory tracking entries */
        current = state->FreedMemoryList.Flink;
        while (current != &state->FreedMemoryList) {
            next = current->Flink;
            freedEntry = CONTAINING_RECORD(current, FREED_MEMORY_ENTRY, ListEntry);
            HeapFree(GetProcessHeap(), 0, freedEntry);
            current = next;
        }

        HeapFree(GetProcessHeap(), 0, state);
        g_WinKernelLite_GlobalState = NULL;
    }
}

void TrackAllocation(PVOID Address, SIZE_T Size, const char* FileName, int LineNumber) {
    GLOBAL_STATE* state;
    PMEMORY_TRACKING_ENTRY entry;

    state = GetGlobalState();
    if (!state) {
        return;
    }

    entry = (PMEMORY_TRACKING_ENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MEMORY_TRACKING_ENTRY));
    if (!entry) {
        if (!state->SuppressErrors) {
            HEAP_ERROR("Failed to allocate memory tracking entry");
        }
        return;
    }

    entry->Address = Address;
    entry->Size = Size;
    entry->FileName = FileName;
    entry->LineNumber = LineNumber;
    entry->AllocationId = state->NextAllocationId++;

    InsertHeadList(&state->MemoryAllocations, &entry->ListEntry);

    state->AllocationCount++;
    state->TotalBytesAllocated += Size;
    state->CurrentBytesAllocated += Size;
    if (state->CurrentBytesAllocated > state->PeakBytesAllocated)
        state->PeakBytesAllocated = state->CurrentBytesAllocated;
}

void TrackFreedMemoryLocked(PVOID Address, SIZE_T Size, const char* AllocFileName, int AllocLineNumber, const char* FreeFileName, int FreeLineNumber, ULONGLONG AllocationId) {
    GLOBAL_STATE* state = GetGlobalState();
    PFREED_MEMORY_ENTRY entry;

    if (!state || !state->TrackFreedMemory) {
        return;
    }

    entry = (PFREED_MEMORY_ENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FREED_MEMORY_ENTRY));

    if (!entry) {
        if (!state->SuppressErrors) {
            HEAP_ERROR("Failed to allocate freed memory tracking entry");
        }
        return;
    }

    entry->Address = Address;
    entry->Size = Size;
    entry->AllocFileName = AllocFileName;
    entry->AllocLineNumber = AllocLineNumber;
    entry->FreeFileName = FreeFileName;
    entry->FreeLineNumber = FreeLineNumber;
    entry->ThreadId = GetCurrentThreadId();
    entry->AllocationId = AllocationId;
    GetSystemTimeAsFileTime(&entry->FreeTime);

    InsertHeadList(&state->FreedMemoryList, &entry->ListEntry);
    state->FreedEntryCount++;

    if (state->FreedEntryCount > state->MaxFreedEntries) {
        CleanupOldFreedEntries();
    }
}

BOOL CheckForDoubleFree(PVOID Address, const char* FreeFileName, int FreeLineNumber, ULONGLONG AllocationId) {
    GLOBAL_STATE* state;
    PLIST_ENTRY current;
    PFREED_MEMORY_ENTRY entry;
    BOOL found = FALSE;

    if (!Address) return FALSE;

    state = GetGlobalState();
    if (!state || !state->TrackFreedMemory) return FALSE;

    current = state->FreedMemoryList.Flink;
    while (current != &state->FreedMemoryList) {
        entry = CONTAINING_RECORD(current, FREED_MEMORY_ENTRY, ListEntry);

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

void CleanupOldFreedEntries(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (!state) return;

    while (state->FreedEntryCount > state->MaxFreedEntries && !IsListEmpty(&state->FreedMemoryList)) {
        PLIST_ENTRY lastEntry = state->FreedMemoryList.Blink;
        PFREED_MEMORY_ENTRY entry = CONTAINING_RECORD(lastEntry, FREED_MEMORY_ENTRY, ListEntry);

        RemoveEntryList(lastEntry);
        HeapFree(GetProcessHeap(), 0, entry);
        state->FreedEntryCount--;
    }
}

BOOL UntrackAllocation(PVOID Address, const char* FreeFileName, int FreeLineNumber) {
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

    current = state->MemoryAllocations.Flink;

    while (current != &state->MemoryAllocations) {
        entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);

        if (entry->Address == Address) {
            allocFileName = entry->FileName;
            allocLineNumber = entry->LineNumber;
            allocSize = entry->Size;
            allocationId = entry->AllocationId;

            RemoveEntryList(&entry->ListEntry);
            state->CurrentBytesAllocated -= entry->Size;
            HeapFree(GetProcessHeap(), 0, entry);

            found = TRUE;
            break;
        }

        current = current->Flink;
    }

    if (found && state->TrackFreedMemory) {
        TrackFreedMemoryLocked(Address, allocSize, allocFileName, allocLineNumber, FreeFileName, FreeLineNumber, allocationId);
    }

    return found;
}

PVOID ExAllocatePoolWithTracking(POOL_TYPE PoolType, SIZE_T NumberOfBytes, const char* FileName, int LineNumber) {
    GLOBAL_STATE* state;
    PVOID ptr;

    UNREFERENCED_PARAMETER(PoolType);

    state = GetGlobalState();
    if (!state) {
        return NULL;
    }

    ptr = HeapAlloc(state->HeapHandle, 0, NumberOfBytes);

    if (ptr == NULL) {
        if (!state->SuppressErrors) {
            HEAP_ERROR("Memory allocation failed for %zu bytes", NumberOfBytes);
        }
        return NULL;
    }

    TrackAllocation(ptr, NumberOfBytes, FileName, LineNumber);

    return ptr;
}

void _ExFreePoolWithTracking(PVOID pointer, const char* FileName, int LineNumber) {
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

        if (currentAllocationId != 0) {
            HEAP_TRACE("_ExFreePoolWithTracking: Checking for double-free of %p (ID: %llu)", pointer, currentAllocationId);
            isDoubleFree = CheckForDoubleFree(pointer, FileName, LineNumber, currentAllocationId);
            if (isDoubleFree) {
                HEAP_WARN("_ExFreePoolWithTracking: Double-free detected for %p, returning without freeing", pointer);
                return;
            }
            HEAP_TRACE("_ExFreePoolWithTracking: No double-free detected for %p", pointer);
        }
    }

    HEAP_TRACE("_ExFreePoolWithTracking: Calling UntrackAllocation for %p", pointer);
    found = UntrackAllocation(pointer, FileName, LineNumber);

    HEAP_VERBOSE("_ExFreePoolWithTracking: UntrackAllocation returned %d for %p", found, pointer);

    if (!found && !state->SuppressErrors) {
        HEAP_WARN("_ExFreePoolWithTracking: Address %p not found in tracking, validating heap pointer", pointer);
        BOOL isValidHeapPtr = FALSE;
        __try {
            isValidHeapPtr = HeapValidate(state->HeapHandle, 0, pointer);
            HEAP_VERBOSE("_ExFreePoolWithTracking: HeapValidate returned %d for %p", isValidHeapPtr, pointer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            HEAP_ERROR("_ExFreePoolWithTracking: Exception during HeapValidate for %p: 0x%08X",
                       pointer, GetExceptionCode());
            isValidHeapPtr = FALSE;
        }

        if (isValidHeapPtr) {
            HEAP_WARN("Attempting to free untracked but valid heap memory at %p from %s:%d",
                   pointer, FileName, LineNumber);
        } else {
            HEAP_WARN("Attempting to free invalid memory pointer at %p from %s:%d",
                   pointer, FileName, LineNumber);
        }
    }

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

BOOL IsValidHeapPointer(PVOID pointer) {
    GLOBAL_STATE* state = GetGlobalState();
    PLIST_ENTRY current;

    if (!state || !pointer) return FALSE;

    current = state->MemoryAllocations.Flink;
    while (current != &state->MemoryAllocations) {
        PMEMORY_TRACKING_ENTRY entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);

        if (entry->Address == pointer) {
            return TRUE;
        }

        current = current->Flink;
    }

    return FALSE;
}

void PrintMemoryLeaks(void) {
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

    HEAP_INFO("=== MEMORY LEAK REPORT ===");

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

void PrintDoubleFreeReport(void) {
    GLOBAL_STATE* state;
    PLIST_ENTRY current;
    PFREED_MEMORY_ENTRY entry;
    SIZE_T entryCount = 0;

    state = GetGlobalState();
    if (!state) return;

    HEAP_INFO("=== FREED MEMORY REPORT ===");
    HEAP_INFO("Total double-free attempts detected: %d", (int)state->DoubleFreeCount);
    HEAP_INFO("Currently tracking %d freed allocations", (int)state->FreedEntryCount);
    HEAP_INFO("Maximum freed entries to track: %d", (int)state->MaxFreedEntries);

    if (state->FreedEntryCount > 0) {
        HEAP_INFO("Recent freed allocations:");
        HEAP_INFO("Address       | Size     | Alloc ID | Alloc Location  | Free Location   | Thread");
        HEAP_INFO("------------- | -------- | -------- | --------------- | --------------- | ------");

        current = state->FreedMemoryList.Flink;
        while (current != &state->FreedMemoryList && entryCount < 20)
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

void SetErrorSuppression(BOOL suppress) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        state->SuppressErrors = suppress;
    }
}

BOOL GetErrorSuppression(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        return state->SuppressErrors;
    }
    return FALSE;
}

void SetFreedMemoryTracking(BOOL enable) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        state->TrackFreedMemory = enable;
    }
}

BOOL GetFreedMemoryTracking(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        return state->TrackFreedMemory;
    }
    return FALSE;
}

void SetMaxFreedEntries(SIZE_T maxEntries) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        state->MaxFreedEntries = maxEntries;

        if (state->FreedEntryCount > maxEntries) {
            CleanupOldFreedEntries();
        }
    }
}

SIZE_T GetMaxFreedEntries(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        return state->MaxFreedEntries;
    }
    return 0;
}
