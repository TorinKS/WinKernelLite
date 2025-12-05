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

#ifndef WINKERNEL_RESOURCE_H_
#define WINKERNEL_RESOURCE_H_

#include <Windows.h>
#include <WinKernelLite/LinkedList.h>
#include <WinKernelLite/NtStatus.h>
#include <WinKernelLite/Wdm.h> /* Include our own Wdm.h for DISPATCHER_HEADER and KEVENT */

// Status codes
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER ((NTSTATUS)0xC000000DL)
#endif

typedef ULONG_PTR ERESOURCE_THREAD;
typedef ERESOURCE_THREAD* PERESOURCE_THREAD;

typedef struct _KSEMAPHORE {
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE, * PKSEMAPHORE, * RESTRICTED_POINTER PRKSEMAPHORE;


typedef struct _OWNER_ENTRY {
    ERESOURCE_THREAD OwnerThread;
    union {
        LONG OwnerCount;
        ULONG TableSize;
    };

} OWNER_ENTRY, * POWNER_ENTRY;

typedef struct _ERESOURCE {
    LIST_ENTRY SystemResourcesList;
    POWNER_ENTRY OwnerTable;
    SHORT ActiveCount;
    USHORT Flag;
    PKSEMAPHORE SharedWaiters;
    PKEVENT ExclusiveWaiters;
    OWNER_ENTRY OwnerThreads[2];
    ULONG ContentionCount;
    USHORT NumberOfSharedWaiters;
    USHORT NumberOfExclusiveWaiters;
    union {
        PVOID Address;
        ULONG_PTR CreatorBackTraceIndex;
    };    KSPIN_LOCK SpinLock;
    
    // User-mode implementation additions
    CRITICAL_SECTION CriticalSection;
} ERESOURCE, * PERESOURCE;

/* Resource flags */
#define ResourceNeverExclusive 0x0001
#define ResourceOwnedExclusive 0x0002

/* External declarations of global variables - C-compatible external linkage */
/* Defined in Resource.c */
#ifdef __cplusplus
extern "C" {
#endif

/* Global critical region counter (protected by its own critical section) */
extern LONG g_WinKernelLite_KernelApcDisableCount;
extern CRITICAL_SECTION g_WinKernelLite_KernelApcDisableLock;
extern BOOLEAN g_WinKernelLite_KernelApcDisableLockInitialized;

/* Global system resources list */
extern LIST_ENTRY g_WinKernelLite_SystemResourcesList;
extern CRITICAL_SECTION g_WinKernelLite_SystemResourcesLock;
extern BOOLEAN g_WinKernelLite_SystemResourcesInitialized;

#ifdef __cplusplus
}
#endif

#define IsOwnedExclusive(R) ((R)->Flag & ResourceOwnedExclusive)

inline void EnsureSystemResourcesListInitialized()
{
    if (!g_WinKernelLite_SystemResourcesInitialized) {
        InitializeCriticalSection(&g_WinKernelLite_SystemResourcesLock);
        InitializeListHead(&g_WinKernelLite_SystemResourcesList);
        g_WinKernelLite_SystemResourcesInitialized = TRUE;
    }
}

inline void CleanupGlobalResources()
{
    /* Cleanup APC disable lock if initialized */
    if (g_WinKernelLite_KernelApcDisableLockInitialized) {
        DeleteCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
        g_WinKernelLite_KernelApcDisableLockInitialized = FALSE;
    }
    
    /* Cleanup system resources lock if initialized */
    if (g_WinKernelLite_SystemResourcesInitialized) {
        DeleteCriticalSection(&g_WinKernelLite_SystemResourcesLock);
        g_WinKernelLite_SystemResourcesInitialized = FALSE;
    }
}

// Initialize a resource
inline NTSTATUS 
ExInitializeResourceLite(
    IN PERESOURCE Resource
)
{
    if (!Resource)
        return STATUS_INVALID_PARAMETER;
        
    ZeroMemory(Resource, sizeof(ERESOURCE));
    
    // Make sure global list is initialized
    EnsureSystemResourcesListInitialized();
    
    // Initialize the critical section for our user-mode implementation
    InitializeCriticalSection(&Resource->CriticalSection);
    
    // Add to the global system resources list
    EnterCriticalSection(&g_WinKernelLite_SystemResourcesLock);
    InsertTailList(&g_WinKernelLite_SystemResourcesList, &Resource->SystemResourcesList);
    LeaveCriticalSection(&g_WinKernelLite_SystemResourcesLock);
    
    return STATUS_SUCCESS;
}

// Delete a resource
inline NTSTATUS 
ExDeleteResourceLite(
    IN PERESOURCE Resource
)
{
    if (!Resource)
        return STATUS_INVALID_PARAMETER;
    
    // Remove from global system resources list
    EnterCriticalSection(&g_WinKernelLite_SystemResourcesLock);
    RemoveEntryList(&Resource->SystemResourcesList);
    LeaveCriticalSection(&g_WinKernelLite_SystemResourcesLock);
        
    // Delete the critical section
    DeleteCriticalSection(&Resource->CriticalSection);
    
    return STATUS_SUCCESS;
}


inline BOOLEAN
ExAcquireResourceExclusiveLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
)
{
    ERESOURCE_THREAD CurrentThread;
    BOOLEAN Result = FALSE;
    
    if (!Resource)
        return FALSE;
    
    // Get the current thread ID as the resource thread
    CurrentThread = (ERESOURCE_THREAD)GetCurrentThreadId();
    
    // Enter the critical section if Wait is TRUE, otherwise try to enter
    if (Wait) {
        EnterCriticalSection(&Resource->CriticalSection);
    } 
    else if (!TryEnterCriticalSection(&Resource->CriticalSection)) {
        return FALSE;
    }
    
    // Check if the resource is already owned
    if (Resource->ActiveCount != 0) {
        // If owned exclusively by the current thread, allow recursive exclusive acquisition
        if (IsOwnedExclusive(Resource) && 
            (Resource->OwnerThreads[0].OwnerThread == CurrentThread)) {
            Resource->OwnerThreads[0].OwnerCount += 1;
            Result = TRUE;
        }
        else {
            // Resource is owned by another thread or shared - cannot acquire exclusive
            if (Wait == FALSE) {
                Result = FALSE;
            }
            else {
                // For simplicity in this user-mode implementation:
                // If we need to wait and we're here, we know we're holding the critical section,
                // but the resource is owned by someone else. We'll release and retry.
                LeaveCriticalSection(&Resource->CriticalSection);
                Sleep(1); // Yield to other threads
                return ExAcquireResourceExclusiveLite(Resource, Wait);
            }
        }
    }
    else {
        // Resource is not owned, so we can take it
        Resource->Flag |= ResourceOwnedExclusive;
        Resource->OwnerThreads[0].OwnerThread = CurrentThread;
        Resource->OwnerThreads[0].OwnerCount = 1;
        Resource->ActiveCount = 1;
        Result = TRUE;
    }
    
    // Always release the critical section
    LeaveCriticalSection(&Resource->CriticalSection);    return Result;
}

// Acquire a resource for shared access
inline BOOLEAN
ExAcquireResourceSharedLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
)
{
    ERESOURCE_THREAD CurrentThread;
    BOOLEAN Result = FALSE;
    
    if (!Resource)
        return FALSE;
    
    // Get the current thread ID as the resource thread
    CurrentThread = (ERESOURCE_THREAD)GetCurrentThreadId();
    
    // Enter the critical section if Wait is TRUE, otherwise try to enter
    if (Wait) {
        EnterCriticalSection(&Resource->CriticalSection);
    } 
    else if (!TryEnterCriticalSection(&Resource->CriticalSection)) {
        return FALSE;
    }
    
    // Check if the resource is already owned
    if (Resource->ActiveCount != 0) {
        // If owned exclusively by the current thread, we CANNOT acquire it shared
        // This is a key difference from exclusive acquisition - no shared acquisition 
        // when holding exclusive, even for the same thread
        if (IsOwnedExclusive(Resource)) {
            if (Wait == FALSE) {
                Result = FALSE;
            }
            else {
                // For simplicity in this user-mode implementation:
                // If we need to wait and we're here, we release and retry
                LeaveCriticalSection(&Resource->CriticalSection);
                Sleep(1); // Yield to other threads
                return ExAcquireResourceSharedLite(Resource, Wait);
            }
        }
        // It's owned shared, so we can add ourselves as another shared owner
        else {
            // For simplicity, in this implementation we'll just increment ActiveCount
            Resource->ActiveCount++;
            Result = TRUE;
        }
    }
    else {
        // Resource is not owned, so we can take it shared
        Resource->ActiveCount = 1;
        Result = TRUE;
    }
    
    // Always leave the critical section when acquiring shared
    LeaveCriticalSection(&Resource->CriticalSection);
    
    return Result;
}

// Release a resource
inline VOID
ExReleaseResourceLite(
    IN PERESOURCE Resource
)
{
    ERESOURCE_THREAD CurrentThread;
    
    if (!Resource)
        return;
    
    // Get the current thread ID as the resource thread
    CurrentThread = (ERESOURCE_THREAD)GetCurrentThreadId();
    
    // Enter the critical section to safely modify the resource
    EnterCriticalSection(&Resource->CriticalSection);
    
    if (IsOwnedExclusive(Resource)) {
        // If owned exclusively, verify it's our thread
        if (Resource->OwnerThreads[0].OwnerThread == CurrentThread) {
            // Decrement the count, and if it reaches 0, release ownership
            Resource->OwnerThreads[0].OwnerCount -= 1;
            if (Resource->OwnerThreads[0].OwnerCount == 0) {
                Resource->Flag &= ~ResourceOwnedExclusive;
                Resource->OwnerThreads[0].OwnerThread = 0;
                Resource->ActiveCount = 0;
            }
        }
    }
    else {
        // For a shared resource, just decrement the active count
        if (Resource->ActiveCount > 0) {
            Resource->ActiveCount -= 1;
        }
    }
    
    // Release the critical section
    LeaveCriticalSection(&Resource->CriticalSection);
}

// Enter a critical region (disable APCs)
inline VOID
KeEnterCriticalRegion(
    VOID
)
{
    /* Initialize the critical section if needed */
    if (!g_WinKernelLite_KernelApcDisableLockInitialized) {
        InitializeCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
        g_WinKernelLite_KernelApcDisableLockInitialized = TRUE;
    }

    /* In kernel mode, this disables normal kernel APCs */
    /* In our user-mode implementation, we'll use a critical section for thread safety */
    EnterCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    InterlockedIncrement(&g_WinKernelLite_KernelApcDisableCount);
    LeaveCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
}

// Leave a critical region (enable APCs)
inline VOID
KeLeaveCriticalRegion(
    VOID
)
{
    /* Initialize the critical section if needed (safety check) */
    if (!g_WinKernelLite_KernelApcDisableLockInitialized) {
        InitializeCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
        g_WinKernelLite_KernelApcDisableLockInitialized = TRUE;
    }    /* Re-enables normal kernel APCs */
    /* In our user-mode implementation, we'll use a critical section for thread safety */
    EnterCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    InterlockedDecrement(&g_WinKernelLite_KernelApcDisableCount);
    LeaveCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
}

// Get the current APC disable count (thread-safe)
inline LONG GetKernelApcDisableCount()
{
    LONG currentValue;
    
    /* Initialize the critical section if needed */
    if (!g_WinKernelLite_KernelApcDisableLockInitialized) {
        InitializeCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
        g_WinKernelLite_KernelApcDisableLockInitialized = TRUE;
    }
    
    EnterCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    currentValue = g_WinKernelLite_KernelApcDisableCount;
    LeaveCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    
    return currentValue;
}

#endif
