/**
 * @file test_exallocate_pool2.cpp
 * @brief Characterizes the WinKernelLite simulation of the modern WDK pool API.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>

#include <WinKernelLite/KernelHeap.h>

/** Owns an isolated tracked heap for each modern pool-allocation scenario. */
class ExAllocatePool2Test : public ::testing::Test {
protected:
    /** Initializes the simulated kernel heap and proves its state is available. */
    void SetUp() override {
        ASSERT_TRUE(InitHeap());
        ASSERT_NE(GetGlobalState(), nullptr);
    }

    /** Releases all simulator bookkeeping so one scenario cannot affect another. */
    void TearDown() override {
        CleanupHeap();
    }
};

/** Verifies WinKernelLite publishes the POOL_FLAGS values used by current WDK code. */
TEST_F(ExAllocatePool2Test, PoolFlagValuesMatchTheWdkContract) {
    EXPECT_EQ(POOL_FLAG_UNINITIALIZED, static_cast<POOL_FLAGS>(0x2));
    EXPECT_EQ(POOL_FLAG_NON_PAGED, static_cast<POOL_FLAGS>(0x40));
    EXPECT_EQ(POOL_FLAG_NON_PAGED_EXECUTE, static_cast<POOL_FLAGS>(0x80));
    EXPECT_EQ(POOL_FLAG_PAGED, static_cast<POOL_FLAGS>(0x100));
}

/** Verifies a nonpaged ExAllocatePool2 request is zeroed and heap-tracked. */
TEST_F(ExAllocatePool2Test, NonPagedAllocationIsZeroInitializedAndTracked) {
    constexpr SIZE_T allocationSize = 128;
    constexpr ULONG allocationTag = '2tET';

    auto* allocation = static_cast<std::uint8_t*>(
        ExAllocatePool2(POOL_FLAG_NON_PAGED, allocationSize, allocationTag));

    ASSERT_NE(allocation, nullptr);
    EXPECT_TRUE(std::all_of(
        allocation,
        allocation + allocationSize,
        [](std::uint8_t value) { return value == 0; }));
    EXPECT_EQ(GetGlobalState()->CurrentBytesAllocated, allocationSize);

    ExFreePoolWithTag(allocation, allocationTag);
    EXPECT_EQ(GetGlobalState()->CurrentBytesAllocated, static_cast<SIZE_T>(0));
}

/** Verifies paged and executable-nonpaged selectors map to supported host pools. */
TEST_F(ExAllocatePool2Test, SupportedPoolSelectorsAllocateSuccessfully) {
    constexpr ULONG allocationTag = '2tET';

    PVOID paged = ExAllocatePool2(POOL_FLAG_PAGED, 32, allocationTag);
    PVOID executable = ExAllocatePool2(
        POOL_FLAG_NON_PAGED_EXECUTE,
        32,
        allocationTag);

    ASSERT_NE(paged, nullptr);
    ASSERT_NE(executable, nullptr);
    ExFreePoolWithTag(paged, allocationTag);
    ExFreePoolWithTag(executable, allocationTag);
}

/** Verifies UNINITIALIZED remains an accepted opt-out without changing tracking. */
TEST_F(ExAllocatePool2Test, UninitializedModifierPreservesAllocationTracking) {
    constexpr SIZE_T allocationSize = 64;
    constexpr ULONG allocationTag = '2tET';

    PVOID allocation = ExAllocatePool2(
        POOL_FLAG_NON_PAGED | POOL_FLAG_UNINITIALIZED,
        allocationSize,
        allocationTag);

    ASSERT_NE(allocation, nullptr);
    EXPECT_EQ(GetGlobalState()->CurrentBytesAllocated, allocationSize);
    ExFreePoolWithTag(allocation, allocationTag);
}

/** Verifies invalid selector combinations and the WDK-reserved zero tag fail closed. */
TEST_F(ExAllocatePool2Test, InvalidFlagsAndZeroTagReturnNullWithoutTracking) {
    constexpr ULONG allocationTag = '2tET';

    EXPECT_EQ(ExAllocatePool2(0, 32, allocationTag), nullptr);
    EXPECT_EQ(
        ExAllocatePool2(
            POOL_FLAG_NON_PAGED | POOL_FLAG_PAGED,
            32,
            allocationTag),
        nullptr);
    EXPECT_EQ(ExAllocatePool2(POOL_FLAG_NON_PAGED, 32, 0), nullptr);
    EXPECT_EQ(GetGlobalState()->CurrentBytesAllocated, static_cast<SIZE_T>(0));
}

/** Verifies required behavior the simulator cannot honor is rejected explicitly. */
TEST_F(ExAllocatePool2Test, UnsupportedRequiredModifierReturnsNull) {
    constexpr ULONG allocationTag = '2tET';

    EXPECT_EQ(
        ExAllocatePool2(
            POOL_FLAG_NON_PAGED | POOL_FLAG_USE_QUOTA,
            32,
            allocationTag),
        nullptr);
    EXPECT_EQ(GetGlobalState()->CurrentBytesAllocated, static_cast<SIZE_T>(0));
}

/** Verifies unknown optional flags do not prevent an otherwise valid allocation. */
TEST_F(ExAllocatePool2Test, UnknownOptionalModifierIsIgnored) {
    constexpr ULONG allocationTag = '2tET';
    constexpr POOL_FLAGS unknownOptionalFlag = 0x0000000200000000ULL;

    PVOID allocation = ExAllocatePool2(
        POOL_FLAG_NON_PAGED | unknownOptionalFlag,
        32,
        allocationTag);

    ASSERT_NE(allocation, nullptr);
    ExFreePoolWithTag(allocation, allocationTag);
}
