## ADDED Requirements

### Requirement: KeQueryPerformanceCounter returns monotonic counter value
The system SHALL provide a `KeQueryPerformanceCounter` function that returns the current performance counter value as a `LARGE_INTEGER`. The counter value MUST be monotonically non-decreasing across successive calls within the same process.

#### Scenario: Basic counter retrieval
- **WHEN** `KeQueryPerformanceCounter` is called with `PerformanceFrequency` set to `NULL`
- **THEN** the return value SHALL be a `LARGE_INTEGER` with `QuadPart > 0`

#### Scenario: Counter monotonicity
- **WHEN** `KeQueryPerformanceCounter` is called twice in succession
- **THEN** the second return value SHALL have `QuadPart >= ` the first return value

### Requirement: KeQueryPerformanceCounter optionally returns frequency
The system SHALL accept an optional `_Out_opt_ PLARGE_INTEGER PerformanceFrequency` parameter. When non-NULL, the system SHALL write the performance counter frequency to the pointed-to `LARGE_INTEGER`.

#### Scenario: Frequency retrieval
- **WHEN** `KeQueryPerformanceCounter` is called with a valid non-NULL `PerformanceFrequency` pointer
- **THEN** the frequency value SHALL have `QuadPart > 0` (typically ~10,000,000 on modern Windows)

#### Scenario: NULL frequency parameter
- **WHEN** `KeQueryPerformanceCounter` is called with `PerformanceFrequency` set to `NULL`
- **THEN** the function SHALL not crash and SHALL return a valid counter value

### Requirement: KeQueryPerformanceCounter matches kernel signature
The function signature SHALL exactly match the kernel-mode declaration from `ntddk.h`:
```c
LARGE_INTEGER KeQueryPerformanceCounter(
    _Out_opt_ PLARGE_INTEGER PerformanceFrequency
);
```
This ensures kernel code using `KeQueryPerformanceCounter` compiles without modification in user mode.

#### Scenario: Kernel code compatibility
- **WHEN** existing kernel code calls `KeQueryPerformanceCounter(&freq)` and `KeQueryPerformanceCounter(NULL)`
- **THEN** both calls SHALL compile and link successfully in user mode via WinKernelLite

### Requirement: No additional dependencies
The implementation SHALL use only `QueryPerformanceCounter` and `QueryPerformanceFrequency` from `kernel32.dll`, which WinKernelLite already links. No new library dependencies SHALL be introduced.

#### Scenario: Build with existing link configuration
- **WHEN** the project is built with existing CMake configuration
- **THEN** the build SHALL succeed without adding new link targets
