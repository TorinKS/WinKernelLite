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
INIT_ONCE g_WinKernelLite_KernelApcDisableInitOnce = INIT_ONCE_STATIC_INIT;

/* Global system resources list */
LIST_ENTRY g_WinKernelLite_SystemResourcesList;
CRITICAL_SECTION g_WinKernelLite_SystemResourcesLock;
INIT_ONCE g_WinKernelLite_SystemResourcesInitOnce = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK InitSystemResourcesCallback(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    UNREFERENCED_PARAMETER(InitOnce);
    UNREFERENCED_PARAMETER(Parameter);
    UNREFERENCED_PARAMETER(Context);
    InitializeCriticalSection(&g_WinKernelLite_SystemResourcesLock);
    InitializeListHead(&g_WinKernelLite_SystemResourcesList);
    return TRUE;
}

static BOOL CALLBACK InitKernelApcDisableLockCallback(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    UNREFERENCED_PARAMETER(InitOnce);
    UNREFERENCED_PARAMETER(Parameter);
    UNREFERENCED_PARAMETER(Context);
    InitializeCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    return TRUE;
}

void EnsureSystemResourcesListInitialized(void)
{
    InitOnceExecuteOnce(&g_WinKernelLite_SystemResourcesInitOnce,
                        InitSystemResourcesCallback, NULL, NULL);
}

void CleanupGlobalResources(void)
{
    BOOL pending;

    /* Only delete if InitOnce completed (CS was actually initialized) */
    if (InitOnceBeginInitialize(&g_WinKernelLite_KernelApcDisableInitOnce,
                                INIT_ONCE_CHECK_ONLY, &pending, NULL)) {
        DeleteCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    }
    g_WinKernelLite_KernelApcDisableInitOnce = (INIT_ONCE)INIT_ONCE_STATIC_INIT;

    if (InitOnceBeginInitialize(&g_WinKernelLite_SystemResourcesInitOnce,
                                INIT_ONCE_CHECK_ONLY, &pending, NULL)) {
        DeleteCriticalSection(&g_WinKernelLite_SystemResourcesLock);
    }
    g_WinKernelLite_SystemResourcesInitOnce = (INIT_ONCE)INIT_ONCE_STATIC_INIT;
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

    if (!Resource)
        return FALSE;

    CurrentThread = (ERESOURCE_THREAD)GetCurrentThreadId();

    for (;;) {
        if (Wait) {
            EnterCriticalSection(&Resource->CriticalSection);
        }
        else if (!TryEnterCriticalSection(&Resource->CriticalSection)) {
            return FALSE;
        }

        if (Resource->ActiveCount != 0) {
            if (IsOwnedExclusive(Resource) &&
                (Resource->OwnerThreads[0].OwnerThread == CurrentThread)) {
                Resource->OwnerThreads[0].OwnerCount += 1;
                LeaveCriticalSection(&Resource->CriticalSection);
                return TRUE;
            }
            else if (Wait == FALSE) {
                LeaveCriticalSection(&Resource->CriticalSection);
                return FALSE;
            }
            else {
                /* Owned by another thread - release CS and retry */
                LeaveCriticalSection(&Resource->CriticalSection);
                Sleep(1);
                continue;
            }
        }
        else {
            /* Resource is free - take it exclusively */
            Resource->Flag |= ResourceOwnedExclusive;
            Resource->OwnerThreads[0].OwnerThread = CurrentThread;
            Resource->OwnerThreads[0].OwnerCount = 1;
            Resource->ActiveCount = 1;
            LeaveCriticalSection(&Resource->CriticalSection);
            return TRUE;
        }
    }
}

BOOLEAN
ExAcquireResourceSharedLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
)
{
    if (!Resource)
        return FALSE;

    for (;;) {
        if (Wait) {
            EnterCriticalSection(&Resource->CriticalSection);
        }
        else if (!TryEnterCriticalSection(&Resource->CriticalSection)) {
            return FALSE;
        }

        if (Resource->ActiveCount != 0) {
            if (IsOwnedExclusive(Resource)) {
                if (Wait == FALSE) {
                    LeaveCriticalSection(&Resource->CriticalSection);
                    return FALSE;
                }
                else {
                    /* Owned exclusively by another thread - release CS and retry */
                    LeaveCriticalSection(&Resource->CriticalSection);
                    Sleep(1);
                    continue;
                }
            }
            /* Owned shared - add ourselves */
            Resource->ActiveCount++;
            LeaveCriticalSection(&Resource->CriticalSection);
            return TRUE;
        }
        else {
            /* Resource is free - take it shared */
            Resource->ActiveCount = 1;
            LeaveCriticalSection(&Resource->CriticalSection);
            return TRUE;
        }
    }
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

static void EnsureKernelApcDisableLockInitialized(void)
{
    InitOnceExecuteOnce(&g_WinKernelLite_KernelApcDisableInitOnce,
                        InitKernelApcDisableLockCallback, NULL, NULL);
}

VOID
KeEnterCriticalRegion(
    VOID
)
{
    EnsureKernelApcDisableLockInitialized();
    EnterCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    InterlockedIncrement(&g_WinKernelLite_KernelApcDisableCount);
    LeaveCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
}

VOID
KeLeaveCriticalRegion(
    VOID
)
{
    EnsureKernelApcDisableLockInitialized();
    EnterCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    InterlockedDecrement(&g_WinKernelLite_KernelApcDisableCount);
    LeaveCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
}

LONG GetKernelApcDisableCount(void)
{
    LONG currentValue;

    EnsureKernelApcDisableLockInitialized();
    EnterCriticalSection(&g_WinKernelLite_KernelApcDisableLock);
    currentValue = g_WinKernelLite_KernelApcDisableCount;
    LeaveCriticalSection(&g_WinKernelLite_KernelApcDisableLock);

    return currentValue;
}
