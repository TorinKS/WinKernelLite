#pragma once

#include <Windows.h>
#include <stdio.h>

typedef _Enum_is_bitflag_ enum _POOL_TYPE {
    NonPagedPool,
    NonPagedPoolExecute = NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed = NonPagedPool + 2,
    DontUseThisType,
    NonPagedPoolCacheAligned = NonPagedPool + 4,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS = NonPagedPool + 6,
    MaxPoolType,

    //
    // Define base types for NonPaged (versus Paged) pool, for use in cracking
    // the underlying pool type.
    //

    NonPagedPoolBase = 0,
    NonPagedPoolBaseMustSucceed = NonPagedPoolBase + 2,
    NonPagedPoolBaseCacheAligned = NonPagedPoolBase + 4,
    NonPagedPoolBaseCacheAlignedMustS = NonPagedPoolBase + 6,

    //
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    //

    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,

    NonPagedPoolNx = 512,
    NonPagedPoolNxCacheAligned = NonPagedPoolNx + 4,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32,

} _Enum_is_bitflag_ POOL_TYPE;

#pragma warning(push)
#pragma warning(disable:4471)
typedef _Enum_is_bitflag_ enum _POOL_TYPE POOL_TYPE;
#pragma warning(pop)

// Memory tracking structures and variables
#define MAX_ALLOCATIONS 1000  // Maximum number of allocations to track

typedef struct _MEMORY_TRACKING_ENTRY {
    PVOID Address;           // Memory address
    SIZE_T Size;             // Size of allocation
    const char* FileName;    // Source file name
    int LineNumber;          // Line number in source file
    BOOL IsAllocated;        // Is this entry still allocated?
} MEMORY_TRACKING_ENTRY;

// Global tracking variables
MEMORY_TRACKING_ENTRY g_MemoryAllocations[MAX_ALLOCATIONS];
SIZE_T g_AllocationCount = 0;
SIZE_T g_TotalBytesAllocated = 0;
SIZE_T g_CurrentBytesAllocated = 0;
SIZE_T g_PeakBytesAllocated = 0;

HANDLE g_HeapHandle = NULL;
CRITICAL_SECTION g_MemoryTrackingLock;  // For thread safety

// Initialize heap and tracking
inline void InitHeap()
{
    g_HeapHandle = GetProcessHeap();
    InitializeCriticalSection(&g_MemoryTrackingLock);
    
    // Initialize tracking array
    ZeroMemory(g_MemoryAllocations, sizeof(g_MemoryAllocations));
}

// Clean up tracking resources
inline void CleanupHeap()
{
    DeleteCriticalSection(&g_MemoryTrackingLock);
}

// Internal tracking functions
inline void TrackAllocation(PVOID Address, SIZE_T Size, const char* FileName, int LineNumber)
{
    EnterCriticalSection(&g_MemoryTrackingLock);
    
    // Find an empty slot or reuse one marked as freed
    for (SIZE_T i = 0; i < MAX_ALLOCATIONS; i++)
    {
        if (g_MemoryAllocations[i].Address == NULL || !g_MemoryAllocations[i].IsAllocated)
        {
            g_MemoryAllocations[i].Address = Address;
            g_MemoryAllocations[i].Size = Size;
            g_MemoryAllocations[i].FileName = FileName;
            g_MemoryAllocations[i].LineNumber = LineNumber;
            g_MemoryAllocations[i].IsAllocated = TRUE;
            
            g_AllocationCount++;
            g_TotalBytesAllocated += Size;
            g_CurrentBytesAllocated += Size;
            if (g_CurrentBytesAllocated > g_PeakBytesAllocated)
                g_PeakBytesAllocated = g_CurrentBytesAllocated;
            
            LeaveCriticalSection(&g_MemoryTrackingLock);
            return;
        }
    }
    
    // If we get here, we've run out of tracking slots
    printf("ERROR: Memory tracking table full. Increase MAX_ALLOCATIONS.\n");
    LeaveCriticalSection(&g_MemoryTrackingLock);
}

inline BOOL UntrackAllocation(PVOID Address)
{
    BOOL found = FALSE;
    
    if (!Address)
        return FALSE;
        
    EnterCriticalSection(&g_MemoryTrackingLock);
    
    for (SIZE_T i = 0; i < MAX_ALLOCATIONS; i++)
    {
        if (g_MemoryAllocations[i].Address == Address && g_MemoryAllocations[i].IsAllocated)
        {
            g_MemoryAllocations[i].IsAllocated = FALSE;
            g_CurrentBytesAllocated -= g_MemoryAllocations[i].Size;
            found = TRUE;
            break;
        }
    }
    
    LeaveCriticalSection(&g_MemoryTrackingLock);
    return found;
}

// Tracked version of ExAllocatePool
// This is the internal implementation
inline PVOID ExAllocatePoolWithTracking(
    POOL_TYPE PoolType,
    SIZE_T NumberOfBytes,
    const char* FileName,
    int LineNumber
    )
{
    PVOID ptr = HeapAlloc(g_HeapHandle, HEAP_GENERATE_EXCEPTIONS, NumberOfBytes);
    
    if (ptr != NULL)
    {
        TrackAllocation(ptr, NumberOfBytes, FileName, LineNumber);
    }
    
    return ptr;
}

// Untracked version (legacy API, avoid using)
inline PVOID ExAllocatePool(
    POOL_TYPE PoolType,
    SIZE_T NumberOfBytes
    )
{
    return ExAllocatePoolWithTracking(PoolType, NumberOfBytes, "Unknown", 0);
}

// Macro to automatically capture file and line
#define ExAllocatePoolTracked(PoolType, NumberOfBytes) \
    ExAllocatePoolWithTracking(PoolType, NumberOfBytes, __FILE__, __LINE__)

// Free memory and remove from tracking
inline void _ExFreePoolWithTracking(PVOID pointer, const char* FileName, int LineNumber)
{
    if (pointer == NULL)
        return;
    
    BOOL found = UntrackAllocation(pointer);
    
    if (!found)
    {
        printf("WARNING: Attempting to free untracked memory at %p from %s:%d\n", 
               pointer, FileName, LineNumber);
    }
    
    HeapFree(g_HeapHandle, 0, pointer);
}

// Legacy version
inline void ExFreePool(PVOID pointer)
{
    _ExFreePoolWithTracking(pointer, "Unknown", 0);
}

// Macro to automatically capture file and line
#define ExFreePoolTracked(pointer) \
    _ExFreePoolWithTracking(pointer, __FILE__, __LINE__)

#define FREE_POOL_TRACKED(_poolptr)     \
    if (_poolptr != NULL) {             \
        ExFreePoolTracked(_poolptr);    \
        _poolptr = NULL;                \
    }

// Legacy macro for backward compatibility
#define FREE_POOL(_poolptr)     \
    if (_poolptr != NULL) {     \
        ExFreePool(_poolptr);   \
        _poolptr = NULL;        \
    }

// Helper functions for leak detection and reporting
inline void PrintMemoryLeaks()
{
    EnterCriticalSection(&g_MemoryTrackingLock);
    
    BOOL foundLeaks = FALSE;
    SIZE_T leakCount = 0;
    SIZE_T leakBytes = 0;
    
    printf("\n=== MEMORY LEAK REPORT ===\n");
    
    for (SIZE_T i = 0; i < MAX_ALLOCATIONS; i++)
    {
        if (g_MemoryAllocations[i].IsAllocated && g_MemoryAllocations[i].Address != NULL)
        {
            if (!foundLeaks)
            {
                printf("Address       | Size     | Allocation Location\n");
                printf("------------- | -------- | ------------------\n");
                foundLeaks = TRUE;
            }
            
            printf("%p | %8d | %s:%d\n", 
                g_MemoryAllocations[i].Address,
                (int)g_MemoryAllocations[i].Size,
                g_MemoryAllocations[i].FileName,
                g_MemoryAllocations[i].LineNumber);
                
            leakCount++;
            leakBytes += g_MemoryAllocations[i].Size;
        }
    }
    
    if (foundLeaks)
    {
        printf("\nTotal: %d leaks, %d bytes\n", (int)leakCount, (int)leakBytes);
    }
    else
    {
        printf("No memory leaks detected!\n");
    }
    
    printf("Memory usage statistics:\n");
    printf("  Total allocations: %d\n", (int)g_AllocationCount);
    printf("  Total bytes allocated: %d\n", (int)g_TotalBytesAllocated);
    printf("  Peak bytes allocated: %d\n", (int)g_PeakBytesAllocated);
    printf("===========================\n");
    
    LeaveCriticalSection(&g_MemoryTrackingLock);
}

