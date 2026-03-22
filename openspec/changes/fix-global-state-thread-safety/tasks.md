## 1. KernelHeap thread-safe init

- [x] 1.1 Fix `GetGlobalState()` in `src/KernelHeap.c`: allocate and initialize `GLOBAL_STATE` into a local `temp`, then use `InterlockedCompareExchangePointer` to atomically set `g_WinKernelLite_GlobalState`. If CAS fails (another thread won), free `temp` and return the winner's pointer.
- [x] 1.2 Build and run all tests to verify KernelHeap fix

## 2. Resource thread-safe init

- [x] 2.1 In `src/Resource.c` and `include/Resource.h`: replace `g_WinKernelLite_SystemResourcesInitialized` (BOOLEAN) with a static `INIT_ONCE` variable. Rewrite `EnsureSystemResourcesListInitialized()` to use `InitOnceExecuteOnce` with a callback that calls `InitializeCriticalSection` and `InitializeListHead`.
- [x] 2.2 In `src/Resource.c` and `include/Resource.h`: replace `g_WinKernelLite_KernelApcDisableLockInitialized` (BOOLEAN) with a static `INIT_ONCE` variable. Rewrite `KeEnterCriticalRegion()`, `KeLeaveCriticalRegion()`, and `GetKernelApcDisableCount()` to use `InitOnceExecuteOnce` with a callback that calls `InitializeCriticalSection`. Remove the inline init checks from all three functions.
- [x] 2.3 Build and run all tests to verify Resource fixes
