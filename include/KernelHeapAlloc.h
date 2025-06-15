#ifndef WINKERNEL_KERNELHEAPALLOC_H_
#define WINKERNEL_KERNELHEAPALLOC_H_

#include <Windows.h>
#include <stdio.h>

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

/*
This defines the maximum number of allocations to track. 
We use a simple implementation with a static-length array
*/
#define MAX_ALLOCATIONS 20000

typedef struct _MEMORY_TRACKING_ENTRY {
    PVOID Address;           /* Memory address */
    SIZE_T Size;            /* Size of allocation */
    const char* FileName;    /* Source file name */
    int LineNumber;         /* Line number in source file */
    BOOL IsAllocated;       /* Is this entry still allocated? */
} MEMORY_TRACKING_ENTRY;

/* Global state structure */
typedef struct _GLOBAL_STATE {
    MEMORY_TRACKING_ENTRY MemoryAllocations[MAX_ALLOCATIONS];
    SIZE_T AllocationCount;
    SIZE_T TotalBytesAllocated;
    SIZE_T CurrentBytesAllocated;
    SIZE_T PeakBytesAllocated;
    HANDLE HeapHandle;
    CRITICAL_SECTION MemoryTrackingLock;
    BOOL SuppressErrors;      /* Control error message output */
    BOOL TrackingTableFull;   /* Indicate if tracking table is full */
} GLOBAL_STATE;

/* Function declarations */
#ifndef KERNEL_HEAP_ALLOC_H_IMPL
extern GLOBAL_STATE* g_State;
#endif

__forceinline GLOBAL_STATE* GetGlobalState(void);
__forceinline BOOL InitHeap(void);
__forceinline void CleanupHeap(void);
__forceinline void TrackAllocation(PVOID Address, SIZE_T Size, const char* FileName, int LineNumber);
__forceinline BOOL UntrackAllocation(PVOID Address);
__forceinline PVOID ExAllocatePoolWithTracking(POOL_TYPE PoolType, SIZE_T NumberOfBytes, const char* FileName, int LineNumber);
__forceinline PVOID ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes);
__forceinline void _ExFreePoolWithTracking(PVOID pointer, const char* FileName, int LineNumber);
__forceinline void ExFreePool(PVOID pointer);
__forceinline void PrintMemoryLeaks(void);
__forceinline void SetErrorSuppression(BOOL suppress);
__forceinline BOOL GetErrorSuppression(void);

#ifdef __cplusplus
}
#endif

/* Global state variable declaration */
#ifdef __cplusplus
extern "C" {
#endif

static GLOBAL_STATE* g_State = NULL;

/* Global state accessor */
__forceinline GLOBAL_STATE* GetGlobalState(void) {
    if (g_State == NULL) {
        GLOBAL_STATE* temp = (GLOBAL_STATE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_STATE));
        if (temp != NULL) {
            InitializeCriticalSection(&temp->MemoryTrackingLock);
            temp->HeapHandle = GetProcessHeap();
            g_State = temp;
        }
    }
    return g_State;
}

__forceinline BOOL InitHeap(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (!state || !state->HeapHandle) {
        return FALSE;
    }
    
    ZeroMemory(state->MemoryAllocations, sizeof(state->MemoryAllocations));
    state->AllocationCount = 0;
    state->TotalBytesAllocated = 0;
    state->CurrentBytesAllocated = 0;
    state->PeakBytesAllocated = 0;
    state->SuppressErrors = FALSE;
    state->TrackingTableFull = FALSE;

    return TRUE;
}

__forceinline void CleanupHeap(void) {
    GLOBAL_STATE* state = GetGlobalState();
    if (state) {
        DeleteCriticalSection(&state->MemoryTrackingLock);
        HeapFree(GetProcessHeap(), 0, state);
        g_State = NULL;
    }
}

__forceinline void TrackAllocation(PVOID Address, SIZE_T Size, const char* FileName, int LineNumber) {
    GLOBAL_STATE* state;
    SIZE_T i;
    
    state = GetGlobalState();
    if (!state) return;
    
    EnterCriticalSection(&state->MemoryTrackingLock);
    
    for (i = 0; i < MAX_ALLOCATIONS; i++) {
        if (state->MemoryAllocations[i].Address == NULL || !state->MemoryAllocations[i].IsAllocated) {
            state->MemoryAllocations[i].Address = Address;
            state->MemoryAllocations[i].Size = Size;
            state->MemoryAllocations[i].FileName = FileName;
            state->MemoryAllocations[i].LineNumber = LineNumber;
            state->MemoryAllocations[i].IsAllocated = TRUE;
            
            state->AllocationCount++;
            state->TotalBytesAllocated += Size;
            state->CurrentBytesAllocated += Size;
            if (state->CurrentBytesAllocated > state->PeakBytesAllocated)
                state->PeakBytesAllocated = state->CurrentBytesAllocated;
            
            LeaveCriticalSection(&state->MemoryTrackingLock);
            return;
        }    }
    
    state->TrackingTableFull = TRUE;
    if (!state->SuppressErrors) {
        printf("ERROR: Memory tracking table full. Increase MAX_ALLOCATIONS.\n");
    }
    LeaveCriticalSection(&state->MemoryTrackingLock);
}

__forceinline BOOL UntrackAllocation(PVOID Address) {
    GLOBAL_STATE* state;
    BOOL found = FALSE;
    SIZE_T i;
    
    if (!Address) return FALSE;
    
    state = GetGlobalState();
    if (!state) return FALSE;
        
    EnterCriticalSection(&state->MemoryTrackingLock);
    
    for (i = 0; i < MAX_ALLOCATIONS; i++) {
        if (state->MemoryAllocations[i].Address == Address && state->MemoryAllocations[i].IsAllocated) {
            state->MemoryAllocations[i].IsAllocated = FALSE;
            state->CurrentBytesAllocated -= state->MemoryAllocations[i].Size;
            found = TRUE;
            break;
        }
    }
    
    LeaveCriticalSection(&state->MemoryTrackingLock);
    return found;
}

__forceinline PVOID ExAllocatePoolWithTracking(POOL_TYPE PoolType, SIZE_T NumberOfBytes, const char* FileName, int LineNumber) {
    GLOBAL_STATE* state;
    BOOL hasTrackingSlot = FALSE;
    SIZE_T i;
    PVOID ptr;
    
    state = GetGlobalState();
    if (!state) return NULL;
    
    // Only check for tracking slots if the table isn't already full
    if (!state->TrackingTableFull) {
        EnterCriticalSection(&state->MemoryTrackingLock);
        
        for (i = 0; i < MAX_ALLOCATIONS; i++) {
            if (state->MemoryAllocations[i].Address == NULL || !state->MemoryAllocations[i].IsAllocated) {
                hasTrackingSlot = TRUE;
                break;
            }
        }
        LeaveCriticalSection(&state->MemoryTrackingLock);
        
        if (!hasTrackingSlot) {
            state->TrackingTableFull = TRUE;
            if (!state->SuppressErrors) {
                printf("ERROR: Memory tracking table full. Increase MAX_ALLOCATIONS.\n");
            }
            // Continue with allocation even if tracking table is full
        }
    }
    
    // Use 0 instead of HEAP_GENERATE_EXCEPTIONS to get NULL return on failure
    ptr = HeapAlloc(state->HeapHandle, 0, NumberOfBytes);
    
    // Return NULL if memory allocation failed
    if (ptr == NULL) {
        if (!state->SuppressErrors) {
            printf("ERROR: Memory allocation failed for %zu bytes\n", NumberOfBytes);
        }
        return NULL;
    }
    
    if (hasTrackingSlot) {
        // Only track if we have a slot available
        TrackAllocation(ptr, NumberOfBytes, FileName, LineNumber);
    }
    
    return ptr;
}

__forceinline PVOID ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes) {
    return ExAllocatePoolWithTracking(PoolType, NumberOfBytes, "Unknown", 0);
}

__forceinline void _ExFreePoolWithTracking(PVOID pointer, const char* FileName, int LineNumber) {
    GLOBAL_STATE* state;
    BOOL found;
    
    if (!pointer) return;
    
    state = GetGlobalState();
    if (!state) return;
    
    found = UntrackAllocation(pointer);
    
    if (!found && !state->SuppressErrors) {
        printf("WARNING: Attempting to free untracked memory at %p from %s:%d\n", 
               pointer, FileName, LineNumber);
    }
    
    HeapFree(state->HeapHandle, 0, pointer);
}

__forceinline void ExFreePool(PVOID pointer) {
    _ExFreePoolWithTracking(pointer, "Unknown", 0);
}

__forceinline void PrintMemoryLeaks(void) {
    GLOBAL_STATE* state;
    BOOL foundLeaks;
    SIZE_T leakCount;
    SIZE_T leakBytes;
    SIZE_T i;
    
    state = GetGlobalState();
    if (!state) return;
    
    foundLeaks = FALSE;
    leakCount = 0;
    leakBytes = 0;
    
    EnterCriticalSection(&state->MemoryTrackingLock);
    
    printf("\n=== MEMORY LEAK REPORT ===\n");
    
    for (i = 0; i < MAX_ALLOCATIONS; i++) {
        if (state->MemoryAllocations[i].IsAllocated && state->MemoryAllocations[i].Address != NULL) {
            if (!foundLeaks) {
                printf("Address       | Size     | Allocation Location\n");
                printf("------------- | -------- | ------------------\n");
                foundLeaks = TRUE;
            }
            
            printf("%p | %8d | %s:%d\n", 
                state->MemoryAllocations[i].Address,
                (int)state->MemoryAllocations[i].Size,
                state->MemoryAllocations[i].FileName,
                state->MemoryAllocations[i].LineNumber);
                
            leakCount++;
            leakBytes += state->MemoryAllocations[i].Size;
        }
    }
    
    if (foundLeaks) {
        printf("\nTotal: %d leaks, %d bytes\n", (int)leakCount, (int)leakBytes);
    } else {
        printf("No memory leaks detected!\n");
    }
    
    printf("Memory usage statistics:\n");
    printf("  Total allocations: %d\n", (int)state->AllocationCount);
    printf("  Total bytes allocated: %d\n", (int)state->TotalBytesAllocated);
    printf("  Peak bytes allocated: %d\n", (int)state->PeakBytesAllocated);
    printf("===========================\n");
    
    LeaveCriticalSection(&state->MemoryTrackingLock);
}

/* Error suppression control functions */
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

/* Macro definitions for automatic file and line capture */
#define ExAllocatePoolTracked(PoolType, NumberOfBytes) \
    ExAllocatePoolWithTracking(PoolType, NumberOfBytes, __FILE__, __LINE__)

#define ExFreePoolTracked(pointer) \
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

#ifdef __cplusplus
}
#endif

#endif  /* WINKERNEL_KERNELHEAPALLOC_H_ */

