## Why

WinKernelCommLib needs high-resolution timing for performance metrics. The kernel API `KeQueryPerformanceCounter` has no WinKernelLite equivalent, so any kernel code that measures timing fails to link in user mode. Adding a user-mode wrapper closes this gap and unblocks timing-dependent test scenarios.

## What Changes

- Add `KeQueryPerformanceCounter` function that wraps `QueryPerformanceCounter` / `QueryPerformanceFrequency` from `kernel32.dll`
- Add header declaration following existing WinKernelLite patterns (extern "C" block, include guards)
- Add corresponding implementation in a `.c` file
- Add basic test validating counter monotonicity and frequency

## Capabilities

### New Capabilities
- `performance-counter`: User-mode implementation of `KeQueryPerformanceCounter` providing high-resolution monotonic timing and frequency queries

### Modified Capabilities
<!-- No existing capabilities are affected. This is a pure addition. -->

## Impact

- **API surface**: One new public function (`KeQueryPerformanceCounter`) added
- **Dependencies**: None new - `QueryPerformanceCounter` and `QueryPerformanceFrequency` are in `kernel32.dll`, already linked by WinKernelLite
- **Consumers**: WinKernelCommLib (and any other client using `Ke*` timing APIs) can now compile and test timing code in user mode

## Version Impact

**MINOR** - This change adds a new public function without modifying or removing any existing API. All existing consumer code remains unaffected.
