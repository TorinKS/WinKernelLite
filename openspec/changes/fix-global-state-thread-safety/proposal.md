## Why

Code review identified TOCTOU (Time-of-Check-Time-of-Use) race conditions in all global state initialization paths. Multiple threads calling `GetGlobalState()`, `EnsureSystemResourcesListInitialized()`, or `KeEnterCriticalRegion()` simultaneously can double-initialize, leaking memory or corrupting state. Now that these implementations are in `.c` files (after the refactor-headers-to-source change), we can apply proper Windows synchronization primitives.

## What Changes

- Fix `GetGlobalState()` in `src/KernelHeap.c`: use `InterlockedCompareExchangePointer` to atomically publish the global state pointer. Allocate to a temp variable, initialize fully, then CAS into the global. If CAS fails (another thread won), free the temp.
- Fix `EnsureSystemResourcesListInitialized()` in `src/Resource.c`: replace the `BOOLEAN` flag check with `InitOnceExecuteOnce` using a static `INIT_ONCE` variable.
- Fix `KeEnterCriticalRegion()`, `KeLeaveCriticalRegion()`, and `GetKernelApcDisableCount()` in `src/Resource.c`: replace the `BOOLEAN` flag check with `InitOnceExecuteOnce` using a static `INIT_ONCE` variable.
- Remove the now-unnecessary `g_WinKernelLite_KernelApcDisableLockInitialized` and `g_WinKernelLite_SystemResourcesInitialized` boolean flags from `Resource.h` and `Resource.c`.

## Capabilities

### New Capabilities
- `thread-safe-init`: Requirements for thread-safe one-time initialization of global state

### Modified Capabilities
<!-- No existing spec-level behavior changes. -->

## Impact

- **src/KernelHeap.c**: `GetGlobalState()` modified to use CAS pattern
- **src/Resource.c**: 4 functions modified to use `InitOnceExecuteOnce`; 2 boolean globals removed
- **include/Resource.h**: Remove `extern` declarations for 2 boolean flags; add `INIT_ONCE` externs
- **Tests**: No changes - behavior is identical, just thread-safe

## Version Impact

**PATCH** - Bug fix. No API changes. Existing single-threaded behavior is identical; multi-threaded behavior is corrected from broken to correct.
