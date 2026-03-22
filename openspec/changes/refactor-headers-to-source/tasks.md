## 1. KernelHeap refactor

- [x] 1.1 Move `GetGlobalState`, `TrackAllocation`, `UntrackAllocation`, `CheckForDoubleFree`, `_ExFreePoolWithTracking`, `PrintMemoryLeaks` implementations from `KernelHeap.h` to `src/KernelHeap.c`. Replace inline definitions in header with function prototypes inside the existing `extern "C"` block. Keep `ExAllocatePoolWithTag` and `ExFreePoolWithTag` as thin inline wrappers calling the moved functions.
- [x] 1.2 Build and run all tests to verify KernelHeap refactor

## 2. Resource refactor

- [x] 2.1 Move `EnsureSystemResourcesListInitialized`, `ExInitializeResourceLite`, `ExAcquireResourceExclusiveLite`, `ExAcquireResourceSharedLite`, `ExReleaseResourceLite`, `ExDeleteResourceLite`, `GetKernelApcDisableCount`, `KeEnterCriticalRegion`, `KeLeaveCriticalRegion` implementations from `Resource.h` to `src/Resource.c`. Replace inline definitions in header with function prototypes. Keep simple macro `IsOwnedExclusive` inline.
- [x] 2.2 Build and run all tests to verify Resource refactor

## 3. File refactor

- [x] 3.1 Move `ZwCreateFile` implementation from `File.h` to new `src/File.c`. Replace inline definition in header with function prototype.
- [x] 3.2 Build and run all tests to verify File refactor

## 4. Registry refactor

- [x] 4.1 Move `ZwOpenKey`, `ZwQueryValueKey`, `ZwSetValueKey`, `ZwDeleteKey`, `ZwEnumerateKey`, `ZwEnumerateValueKey` implementations from `Registry.h` to new `src/Registry.c`. Replace inline definitions in header with function prototypes.
- [x] 4.2 Build and run all tests to verify Registry refactor
