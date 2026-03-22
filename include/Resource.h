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

#define IsOwnedExclusive(R) ((R)->Flag & ResourceOwnedExclusive)

/* Function declarations - implementations in Resource.c */
void EnsureSystemResourcesListInitialized(void);
void CleanupGlobalResources(void);
NTSTATUS ExInitializeResourceLite(IN PERESOURCE Resource);
NTSTATUS ExDeleteResourceLite(IN PERESOURCE Resource);
BOOLEAN ExAcquireResourceExclusiveLite(IN PERESOURCE Resource, IN BOOLEAN Wait);
BOOLEAN ExAcquireResourceSharedLite(IN PERESOURCE Resource, IN BOOLEAN Wait);
VOID ExReleaseResourceLite(IN PERESOURCE Resource);
VOID KeEnterCriticalRegion(VOID);
VOID KeLeaveCriticalRegion(VOID);
LONG GetKernelApcDisableCount(void);

#ifdef __cplusplus
}
#endif

#endif
