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

#include "include/Resource.h"

/* Global Resource state variables - single instance across all translation units */

/* Critical region counter (protected by its own critical section) */
LONG g_WinKernelLite_KernelApcDisableCount = 0;
CRITICAL_SECTION g_WinKernelLite_KernelApcDisableLock;
BOOLEAN g_WinKernelLite_KernelApcDisableLockInitialized = FALSE;

/* Global system resources list */
LIST_ENTRY g_WinKernelLite_SystemResourcesList;
CRITICAL_SECTION g_WinKernelLite_SystemResourcesLock;
BOOLEAN g_WinKernelLite_SystemResourcesInitialized = FALSE;

void EnsureSystemResourcesListInitialized(void)
{
    if (!g_WinKernelLite_SystemResourcesInitialized) {
        InitializeCriticalSection(&g_WinKernelLite_SystemResourcesLock);
        InitializeListHead(&g_WinKernelLite_SystemResourcesList);
        g_WinKernelLite_SystemResourcesInitialized = TRUE;
    }
}

void CleanupGlobalResources(void)
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

NTSTATUS
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

NTSTATUS
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

BOOLEAN
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
    LeaveCriticalSection(&Resource->CriticalSection);
    return Result;
}

BOOLEAN
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

VOID
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

VOID
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

VOID
KeLeaveCriticalRegion(
    VOID
)
{
    /* Initialize the critical section if needed (safety check) */
    if (!g_WinKernelLite_KernelApcDisableLockInitialized) {
        InitializeCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
        g_WinKernelLite_KernelApcDisableLockInitialized = TRUE;
    }
    /* Re-enables normal kernel APCs */
    /* In our user-mode implementation, we'll use a critical section for thread safety */
    EnterCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    InterlockedDecrement(&g_WinKernelLite_KernelApcDisableCount);
    LeaveCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
}

LONG GetKernelApcDisableCount(void)
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
