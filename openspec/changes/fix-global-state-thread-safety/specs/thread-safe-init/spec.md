## ADDED Requirements

### Requirement: GetGlobalState SHALL be thread-safe
`GetGlobalState()` SHALL use `InterlockedCompareExchangePointer` to atomically publish the global state pointer. If two threads race, exactly one SHALL succeed and the other SHALL free its local allocation without leaking memory.

#### Scenario: Concurrent first call from two threads
- **WHEN** two threads call `GetGlobalState()` simultaneously while `g_WinKernelLite_GlobalState` is NULL
- **THEN** exactly one `GLOBAL_STATE` allocation SHALL survive, the other SHALL be freed, and both threads SHALL receive the same valid pointer

#### Scenario: Fast path after initialization
- **WHEN** `GetGlobalState()` is called after the global state is already initialized
- **THEN** the function SHALL return the existing pointer without allocating or performing any synchronization beyond a pointer read

### Requirement: CRITICAL_SECTION initialization SHALL use InitOnceExecuteOnce
All `CRITICAL_SECTION` globals that are lazily initialized SHALL use `InitOnceExecuteOnce` with a static `INIT_ONCE` variable to guarantee exactly-once initialization.

#### Scenario: Concurrent EnsureSystemResourcesListInitialized
- **WHEN** two threads call `EnsureSystemResourcesListInitialized()` simultaneously before the system resources list is initialized
- **THEN** `InitializeCriticalSection` SHALL be called exactly once, and both threads SHALL proceed safely after initialization completes

#### Scenario: Concurrent KeEnterCriticalRegion
- **WHEN** two threads call `KeEnterCriticalRegion()` simultaneously before the APC disable lock is initialized
- **THEN** `InitializeCriticalSection` for the APC disable lock SHALL be called exactly once

### Requirement: Boolean initialization flags SHALL be removed
The `g_WinKernelLite_KernelApcDisableLockInitialized` and `g_WinKernelLite_SystemResourcesInitialized` boolean globals SHALL be replaced by `INIT_ONCE` variables. No race-prone boolean check-then-act patterns SHALL remain.

#### Scenario: No boolean flags in Resource.h or Resource.c
- **WHEN** the codebase is searched for `KernelApcDisableLockInitialized` or `SystemResourcesInitialized`
- **THEN** no boolean declarations or checks SHALL be found; only `INIT_ONCE` variables SHALL control initialization state
