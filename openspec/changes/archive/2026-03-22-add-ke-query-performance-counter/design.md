## Context

WinKernelLite provides user-mode implementations of kernel APIs for testing. Most functions are declared `__forceinline` in headers under `include/`, with minimal `.c` files for global state only. The project uses `WINKERNEL_*_H_` include guards, `extern "C"` blocks for C++ compatibility, and Google Test for testing via `WinKernelLiteTestBase`.

No timer or performance counter API exists yet. `KeQueryPerformanceCounter` is the first timing-related function to be added.

## Goals / Non-Goals

**Goals:**
- Provide a user-mode `KeQueryPerformanceCounter` that matches the kernel signature from `ntddk.h`
- Follow existing WinKernelLite patterns for header layout, inline implementation, and testing
- Enable WinKernelCommLib timing code to compile and link in user mode

**Non-Goals:**
- Implementing other timer APIs (KeInitializeTimer, KeSetTimer, etc.) - separate future change
- Adding a full KernelTimer subsystem with DPC support
- Matching kernel-mode performance characteristics exactly

## Decisions

### 1. Inline implementation in a new header

**Decision:** Implement `KeQueryPerformanceCounter` as `__forceinline` in a new `KernelPerf.h` header, with no corresponding `.c` file.

**Rationale:** This matches the pattern used by `KernelHeap.h`, `Resource.h`, `LinkedList.h`, and others - the majority of WinKernelLite functions are inline in headers. A `.c` file is only needed for global state, and this function is stateless.

**Alternative considered:** Adding to `wdm.h` - rejected because `wdm.h` contains structure definitions and forward declarations, not function implementations. A dedicated header keeps the API surface organized by category.

### 2. Header naming: KernelPerf.h

**Decision:** Use `KernelPerf.h` rather than `KernelTimer.h`.

**Rationale:** "Perf" scopes the header to performance counter APIs (`KeQueryPerformanceCounter`, potentially `KeQueryTickCount` in the future). "Timer" would imply DPC-based timer APIs (`KeInitializeTimer`, `KeSetTimer`) which are a different subsystem and not in scope.

### 3. Direct delegation to Win32 APIs

**Decision:** Call `QueryPerformanceCounter` and `QueryPerformanceFrequency` directly with no caching or abstraction layer.

**Rationale:** These Win32 APIs are thin wrappers around the same hardware counter the kernel uses. No global state, lazy initialization, or caching is needed. The frequency is constant per boot but caching it would add complexity for negligible gain since `QueryPerformanceFrequency` is already fast.

## Risks / Trade-offs

- **[Minimal risk] QueryPerformanceCounter failure** - `QueryPerformanceCounter` can technically fail on pre-Vista systems, but WinKernelLite targets modern Windows where it always succeeds. No error handling needed. -> Mitigation: Document the Windows version assumption.
- **[Trade-off] No frequency caching** - Callers that query frequency in a tight loop pay a syscall per call. -> Mitigation: Acceptable for testing scenarios; callers can cache the frequency themselves, matching how kernel code typically uses it.
