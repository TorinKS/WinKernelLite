## Context

Three separate initialization paths in WinKernelLite use check-then-act patterns without synchronization. All are in `.c` files after the recent refactor, making them straightforward to fix.

**Pattern 1 - Pointer initialization** (`GetGlobalState`):
```c
if (g_WinKernelLite_GlobalState == NULL) {
    temp = HeapAlloc(...);
    // ... initialize temp ...
    g_WinKernelLite_GlobalState = temp;  // RACE: two threads both write
}
```

**Pattern 2 - Boolean-guarded one-time init** (`EnsureSystemResourcesListInitialized`, `KeEnterCriticalRegion`, etc.):
```c
if (!g_Initialized) {
    InitializeCriticalSection(&g_Lock);  // RACE: double init
    g_Initialized = TRUE;
}
```

## Goals / Non-Goals

**Goals:**
- Eliminate all TOCTOU races in global state initialization
- Lock-free on the fast path (after initialization completes)
- Use Windows-native primitives only

**Non-Goals:**
- Making heap tracking operations (TrackAllocation, UntrackAllocation, etc.) thread-safe - that is a separate change
- Adding thread safety to debug state initialization (Debug.h/Debug.c) - separate change

## Decisions

### 1. Use InterlockedCompareExchangePointer for GetGlobalState

**Decision:** Allocate and fully initialize a `GLOBAL_STATE` into a local temp pointer, then atomically CAS it into `g_WinKernelLite_GlobalState`. If CAS fails (another thread won the race), free the temp.

**Rationale:** This is the standard Windows pattern for lazy pointer initialization. It's lock-free, requires no additional global state, and the fast path is a single pointer read. The kernel itself uses this pattern (`ExInterlockedCompareExchangePointer`).

**Alternative considered:** `InitOnceExecuteOnce` - works but adds an `INIT_ONCE` variable and a callback function for what is fundamentally a pointer-swap operation. CAS is simpler here.

### 2. Use InitOnceExecuteOnce for CRITICAL_SECTION initialization

**Decision:** Replace the boolean flags (`g_WinKernelLite_KernelApcDisableLockInitialized`, `g_WinKernelLite_SystemResourcesInitialized`) with `INIT_ONCE` variables. Use `InitOnceExecuteOnce` to guarantee exactly-once initialization.

**Rationale:** `InitializeCriticalSection` cannot be safely called twice on the same `CRITICAL_SECTION`. `INIT_ONCE` is the Windows-blessed mechanism for this exact scenario. It handles all synchronization internally and the fast path (after init) is a single interlocked read.

**Alternative considered:** Manual CAS on the boolean flag - fragile because `InitializeCriticalSection` can throw an exception, leaving the flag in an inconsistent state. `InitOnceExecuteOnce` handles errors correctly.

### 3. Remove boolean flag globals, add INIT_ONCE globals

**Decision:** Replace the 2 boolean globals (`g_WinKernelLite_KernelApcDisableLockInitialized`, `g_WinKernelLite_SystemResourcesInitialized`) with 2 `INIT_ONCE` globals. Initialize them with `INIT_ONCE_STATIC_INIT` (zero-init, no runtime constructor needed).

**Rationale:** Cleaner API surface, removes the race-prone boolean pattern entirely.

## Risks / Trade-offs

- **[Risk] CAS loop on high contention** -> Mitigation: In practice, `GetGlobalState` is called once early in process lifetime. The losing thread just frees its allocation. No spin loop.
- **[Risk] InitOnceExecuteOnce callback complexity** -> Mitigation: Callbacks are trivial 3-line functions. The alternative (manual CAS + exception handling) is more complex and error-prone.
