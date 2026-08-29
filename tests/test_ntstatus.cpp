/**
 * @file test_ntstatus.cpp
 * @brief Verifies WinKernelLite publishes NTSTATUS values used by shared code.
 */

#include <gtest/gtest.h>

#include <Windows.h>

#include <WinKernelLite/NtStatus.h>

/** Verifies STATUS_NOT_FOUND retains the exact public WDK bit pattern. */
TEST(NtStatusContractTest, StatusNotFoundMatchesTheWdkContract) {
    EXPECT_EQ(
        static_cast<ULONG>(STATUS_NOT_FOUND),
        static_cast<ULONG>(0xC0000225UL));
}

/** Verifies resource-exhaustion failures retain the exact public WDK bits. */
TEST(NtStatusContractTest, StatusInsufficientResourcesMatchesTheWdkContract) {
    EXPECT_EQ(
        static_cast<ULONG>(STATUS_INSUFFICIENT_RESOURCES),
        static_cast<ULONG>(0xC000009AUL));
}
