#include <gtest/gtest.h>
#include "../include/UnicodeString.h"

class UnicodeStringTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize heap before each test
        ASSERT_TRUE(InitHeap()) << "Failed to initialize heap";
        // Verify heap handle is valid
        ASSERT_NE(g_HeapHandle, nullptr) << "Heap handle is null";
    }

    void TearDown() override {
        // Check for memory leaks after each test
        PrintMemoryLeaks();
        // Free any remaining allocations to prevent cascading failures
        for (SIZE_T i = 0; i < MAX_ALLOCATIONS; i++) {
            if (g_MemoryAllocations[i].IsAllocated && g_MemoryAllocations[i].Address != NULL) {
                HeapFree(g_HeapHandle, 0, g_MemoryAllocations[i].Address);
                g_MemoryAllocations[i].IsAllocated = FALSE;
            }
        }
        CleanupHeap();
    }
};

TEST_F(UnicodeStringTest, DuplicateUnicodeString_Success) {
    UNICODE_STRING dest = {0};
    const wchar_t* src = L"Hello";
    
    NTSTATUS status = DuplicateUnicodeString(&dest, src);
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_NE(dest.Buffer, nullptr) << "Buffer should be allocated";
    ASSERT_EQ(dest.Length, wcslen(src) * sizeof(WCHAR));
    ASSERT_EQ(dest.MaximumLength, (wcslen(src) + 1) * sizeof(WCHAR));
    EXPECT_STREQ(dest.Buffer, src);

    FreeUnicodeString(&dest);
}

TEST_F(UnicodeStringTest, DuplicateUnicodeString_EmptyString) {
    UNICODE_STRING dest = {0};
    const wchar_t* src = L"";
    
    NTSTATUS status = DuplicateUnicodeString(&dest, src);
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_NE(dest.Buffer, nullptr) << "Buffer should be allocated even for empty string";
    ASSERT_EQ(dest.Length, 0);
    ASSERT_EQ(dest.MaximumLength, sizeof(WCHAR));  // Space for null terminator
    EXPECT_STREQ(dest.Buffer, src);

    FreeUnicodeString(&dest);
}

TEST_F(UnicodeStringTest, FreeUnicodeString_NullBuffer) {
    UNICODE_STRING str = {0};
    // Should not crash when freeing a zeroed structure
    FreeUnicodeString(&str);
}

TEST_F(UnicodeStringTest, FreeUnicodeString_ResetFields) {
    UNICODE_STRING dest = {0};
    const wchar_t* src = L"Test";
    
    NTSTATUS status = DuplicateUnicodeString(&dest, src);
    ASSERT_EQ(status, STATUS_SUCCESS);
    
    FreeUnicodeString(&dest);
    EXPECT_EQ(dest.Buffer, nullptr);
    EXPECT_EQ(dest.Length, 0);
    EXPECT_EQ(dest.MaximumLength, 0);
}

TEST_F(UnicodeStringTest, HeapAllocation_Tracking) {
    // Verify initial state
    ASSERT_EQ(g_AllocationCount, 0);
    ASSERT_EQ(g_CurrentBytesAllocated, 0);

    // Allocate a test string
    UNICODE_STRING dest = {0};
    const wchar_t* src = L"Test String";
    
    NTSTATUS status = DuplicateUnicodeString(&dest, src);
    ASSERT_EQ(status, STATUS_SUCCESS);
    
    // Verify allocation was tracked
    ASSERT_EQ(g_AllocationCount, 1);
    ASSERT_GT(g_CurrentBytesAllocated, 0);
    ASSERT_EQ(g_CurrentBytesAllocated, g_PeakBytesAllocated);

    // Free the string
    FreeUnicodeString(&dest);

    // Verify tracking after free
    ASSERT_EQ(g_CurrentBytesAllocated, 0);
    ASSERT_EQ(g_AllocationCount, 1);  // Total count remains the same
}
