## Why

WinKernelLite implements complex kernel API wrappers (50-100+ lines each) as `__forceinline` functions in header files. This prevents adding thread synchronization to global state, blocks debugger breakpoints on key functions, duplicates compiled code across every translation unit, and contradicts the real kernel's architecture where these functions live in `ntoskrnl.exe`/`ntdll.dll`, not in WDK headers. A code review identified thread safety as the top-priority fix, and that fix is blocked until implementations move out of headers.

## What Changes

- Move complex function implementations from 4 header files into corresponding `.c` source files
- Headers retain: type definitions, struct declarations, function prototypes, and simple inline wrappers (1-5 lines) that match real WDK inline patterns
- Source files receive: implementations >5 lines, functions touching global state, functions needing synchronization
- Create new source files: `src/Registry.c`, `src/File.c`
- Extend existing source files: `src/Resource.c`, `src/KernelHeap.c`
- No API signature changes - all existing function prototypes remain identical
- No behavioral changes - this is a pure structural refactor

### Functions to move (by file):

**Registry.h -> src/Registry.c (new):**
- `ZwOpenKey`, `ZwQueryValueKey`, `ZwSetValueKey`, `ZwDeleteKey`, `ZwEnumerateKey`, `ZwEnumerateValueKey`

**File.h -> src/File.c (new):**
- `ZwCreateFile`

**Resource.h -> src/Resource.c (existing):**
- `ExInitializeResourceLite`, `ExAcquireResourceExclusiveLite`, `ExAcquireResourceSharedLite`, `ExReleaseResourceLite`, `ExDeleteResourceLite`, `KeEnterCriticalRegion`, `KeLeaveCriticalRegion`

**KernelHeap.h -> src/KernelHeap.c (existing):**
- `GetGlobalState`, `TrackAllocation`, `UntrackAllocation`, `CheckForDoubleFree`, `_ExFreePoolWithTracking`, `PrintMemoryLeaks`
- Keep `ExAllocatePoolWithTag`/`ExFreePoolWithTag` as thin inline wrappers calling the moved implementations

### Functions to keep inline (match real WDK):
- `LinkedList.h` - all functions (WDK inlines these)
- `UnicodeString.h` - `RtlInitUnicodeString` and simple accessors (WDK inlines these)
- `KernelPerf.h` - `KeQueryPerformanceCounter` (simple delegation)

## Capabilities

### New Capabilities
- `header-source-separation`: Rules governing which functions belong in headers vs source files, based on real WDK/kernel architecture

### Modified Capabilities
<!-- No spec-level behavior changes. This is a structural refactor only. -->

## Impact

- **Headers**: Registry.h, File.h, Resource.h, KernelHeap.h - reduced to declarations + simple inlines
- **Source files**: 2 new (Registry.c, File.c), 2 extended (Resource.c, KernelHeap.c)
- **Build system**: CMakeLists.txt auto-discovers .c files via glob, so new source files are picked up automatically
- **Tests**: No changes required - all function signatures and behavior remain identical
- **Consumers**: No changes required - `#include` still provides the same API surface

## Version Impact

**PATCH** - Pure internal refactor. No public API changes, no behavioral changes, no new functions, no removed functions. All existing consumer code compiles and works identically.
