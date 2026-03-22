#include <gtest/gtest.h>
#include "../include/KernelPerf.h"

TEST(KernelPerfTest, CounterReturnsNonZero) {
    LARGE_INTEGER counter = KeQueryPerformanceCounter(NULL);
    EXPECT_GT(counter.QuadPart, 0);
}

TEST(KernelPerfTest, FrequencyReturnsNonZero) {
    LARGE_INTEGER freq;
    KeQueryPerformanceCounter(&freq);
    EXPECT_GT(freq.QuadPart, 0);
}

TEST(KernelPerfTest, CounterIsMonotonic) {
    LARGE_INTEGER start = KeQueryPerformanceCounter(NULL);
    LARGE_INTEGER end = KeQueryPerformanceCounter(NULL);
    EXPECT_GE(end.QuadPart, start.QuadPart);
}

TEST(KernelPerfTest, NullFrequencyDoesNotCrash) {
    LARGE_INTEGER counter = KeQueryPerformanceCounter(NULL);
    EXPECT_GT(counter.QuadPart, 0);
}
