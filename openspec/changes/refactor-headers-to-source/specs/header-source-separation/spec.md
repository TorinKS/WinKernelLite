## ADDED Requirements

### Requirement: Complex implementations SHALL reside in source files
Functions with implementations exceeding 5 lines, or functions that access global state, MUST be defined in `.c` source files. Headers SHALL contain only the function prototype (declaration).

#### Scenario: Function accessing global state
- **WHEN** a function reads or writes any `g_WinKernelLite_*` global variable
- **THEN** its implementation MUST be in a `.c` file, not inline in a header

#### Scenario: Function exceeding 5 lines
- **WHEN** a function body exceeds 5 lines of logic (excluding braces and blank lines)
- **THEN** its implementation MUST be in a `.c` file unless the real WDK declares the equivalent function inline

### Requirement: Simple wrappers matching WDK inlines SHALL remain in headers
Functions that are declared inline or as macros in the real Windows Driver Kit headers (wdm.h, ntddk.h) SHALL remain as `__forceinline` in WinKernelLite headers.

#### Scenario: Linked list operations
- **WHEN** implementing `InitializeListHead`, `InsertTailList`, `InsertHeadList`, `RemoveEntryList`, `IsListEmpty`
- **THEN** these SHALL remain `__forceinline` in `LinkedList.h` because the real WDK declares them inline

#### Scenario: Simple kernel API delegation
- **WHEN** a function is a 1-5 line delegation to a Win32 API (e.g., `KeQueryPerformanceCounter` calling `QueryPerformanceCounter`)
- **THEN** it SHALL remain `__forceinline` in its header

### Requirement: Refactored functions SHALL preserve identical signatures
All functions moved from headers to source files MUST retain their exact original function signature. No parameter types, return types, or calling conventions SHALL change.

#### Scenario: Consumer code compilation
- **WHEN** a downstream project (e.g., WinKernelCommLib) includes WinKernelLite headers after the refactor
- **THEN** the project SHALL compile and link without any source code changes

#### Scenario: Test suite passes unchanged
- **WHEN** the existing test suite is executed after the refactor
- **THEN** all tests SHALL pass without modification to any test file

### Requirement: Header files SHALL declare moved functions with extern linkage
Functions moved to `.c` files SHALL have their prototypes declared in the corresponding header file within an `extern "C"` block for C++ compatibility.

#### Scenario: C++ compilation
- **WHEN** a `.cpp` file includes a refactored header
- **THEN** the function prototypes SHALL be enclosed in `extern "C" { }` guards
- **THEN** linking SHALL succeed without name mangling issues
