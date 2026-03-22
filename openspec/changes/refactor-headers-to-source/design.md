## Context

WinKernelLite headers contain both declarations and complex implementations inline. This architecture emerged organically but now blocks thread safety work identified in the code review. The project uses CMake with automatic source file discovery via glob patterns, so adding new `.c` files requires no build system changes.

Current state by file:
- `KernelHeap.h`: ~600 lines, contains `GetGlobalState`, `TrackAllocation`, `UntrackAllocation`, `CheckForDoubleFree`, `_ExFreePoolWithTracking`, `PrintMemoryLeaks` - all touching `g_WinKernelLite_GlobalState`
- `Resource.h`: ~400 lines, contains full locking implementations with `CRITICAL_SECTION` usage and `Sleep(1)` spin loops
- `Registry.h`: ~850 lines, contains 6 Zw* functions with Win32 API mapping and buffer management
- `File.h`: ~230 lines, contains `ZwCreateFile` with disposition mapping and error translation
- Existing `.c` files: `KernelHeap.c` (1 line - global var), `Resource.c` (~30 lines - global vars), `Debug.c` (1 line - global var)

## Goals / Non-Goals

**Goals:**
- Move complex implementations (>5 lines) from headers to `.c` files
- Enable future thread safety fixes by having implementations in compilable source
- Match real WDK/kernel architecture: inline only what the WDK inlines
- All 36 existing tests pass without modification

**Non-Goals:**
- Fixing any bugs (thread safety, API fidelity) - those are separate changes
- Changing any function signatures or behavior
- Refactoring UnicodeString.h - `RtlInitUnicodeString` and accessors are inline in the real WDK
- Refactoring LinkedList.h - all functions are inline macros in the real WDK
- Optimizing compilation or adding precompiled headers

## Decisions

### 1. Move implementations per-file, one header at a time

**Decision:** Refactor each header independently in sequence: KernelHeap, Resource, File, Registry.

**Rationale:** Each header is self-contained enough to move independently. Sequential approach lets us build and test after each move, catching issues early. KernelHeap first because it has the most global state and is the highest-priority target for thread safety.

**Alternative considered:** Move all at once - rejected because a single broken move would be hard to bisect.

### 2. Keep thin inline wrappers in headers for ExAllocatePoolWithTag / ExFreePoolWithTag

**Decision:** These two functions stay inline in `KernelHeap.h` but call non-inline helpers in `KernelHeap.c`.

**Rationale:** These are the primary API surface that consumers call directly. Keeping them inline preserves zero-overhead for the common case (the allocation itself is just `HeapAlloc`). The tracking/bookkeeping logic that touches global state moves to `.c`. This mirrors how the kernel separates the fast path (inline in wdm.h) from the pool manager internals (in ntoskrnl.exe).

### 3. Function prototypes use same signatures, no new types

**Decision:** The moved functions keep identical signatures. Headers gain `extern` function declarations where `__forceinline` definitions were.

**Rationale:** Zero consumer impact. Tests and downstream projects like WinKernelCommLib compile without changes.

### 4. Internal helpers become non-static in .c files

**Decision:** Functions like `GetGlobalState`, `TrackAllocation`, `CheckForDoubleFree` become regular (non-static, non-inline) functions declared in the header and defined in the `.c` file. They retain their existing linkage (C linkage via `extern "C"` blocks).

**Rationale:** These functions are called from inline wrappers in the header, so they cannot be `static`. They need external linkage.

## Risks / Trade-offs

- **[Risk] Subtle behavior change from losing inlining** -> Mitigation: These are wrapper functions calling Win32 APIs; the Win32 call dominates execution time. Inline vs non-inline overhead is negligible. All tests verify behavior is unchanged.
- **[Risk] Include order dependencies** -> Mitigation: Headers already include their dependencies (`<Windows.h>`, other project headers). Moving implementations doesn't change include requirements.
- **[Risk] Linker errors from duplicate definitions** -> Mitigation: Remove `__forceinline`/`inline` from moved functions. Verify single-definition rule with build test.
