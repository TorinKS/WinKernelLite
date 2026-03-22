## 1. Header

- [x] 1.1 Create `include/KernelPerf.h` with `WINKERNEL_KERNELPERF_H_` include guard, `extern "C"` block, and `__forceinline` implementation of `KeQueryPerformanceCounter` delegating to `QueryPerformanceCounter` / `QueryPerformanceFrequency`

## 2. Tests

- [x] 2.1 Create `tests/test_kernel_perf.cpp` with Google Test cases: counter returns non-zero, frequency returns non-zero, monotonicity across two calls, NULL frequency parameter does not crash
