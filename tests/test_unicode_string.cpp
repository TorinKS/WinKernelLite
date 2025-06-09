#include <gtest/gtest.h>
#include <string>
#include "../include/UnicodeString.h"
#include "../include/KernelHeapAlloc.h"

class UnicodeStringTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize heap before each test
        ASSERT_TRUE(InitHeap()) << "Failed to initialize heap";
        // Verify heap handle is valid
        GLOBAL_STATE* state = GetGlobalState();
        ASSERT_NE(state->HeapHandle, nullptr) << "Heap handle is null";
    }

    void TearDown() override {
        // Check for memory leaks after each test
        PrintMemoryLeaks();
        // Free any remaining allocations to prevent cascading failures
        GLOBAL_STATE* state = GetGlobalState();
        for (SIZE_T i = 0; i < MAX_ALLOCATIONS; i++) {
            if (state->MemoryAllocations[i].IsAllocated && state->MemoryAllocations[i].Address != NULL) {
                HeapFree(state->HeapHandle, 0, state->MemoryAllocations[i].Address);
                state->MemoryAllocations[i].IsAllocated = FALSE;
            }
        }
        CleanupHeap();
    }
};

TEST_F(UnicodeStringTest, RtlInitUnicodeString_Success) {
    UNICODE_STRING dest = {0};
    const wchar_t* src = L"Hello";
    
    NTSTATUS status = RtlInitUnicodeString(&dest, src);    
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_EQ(dest.Buffer, (PWSTR)src) << "Buffer should point to source string";
    ASSERT_EQ(dest.Length, wcslen(src) * sizeof(WCHAR));
    ASSERT_EQ(dest.MaximumLength, dest.Length + sizeof(UNICODE_NULL));
}

TEST_F(UnicodeStringTest, RtlInitUnicodeString_EmptyString) {
    UNICODE_STRING dest = {0};
    const wchar_t* src = L"";
    
    NTSTATUS status = RtlInitUnicodeString(&dest, src);    
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_EQ(dest.Buffer, (PWSTR)src) << "Buffer should point to source string";
    ASSERT_EQ(dest.Length, 0);
    ASSERT_EQ(dest.MaximumLength, sizeof(UNICODE_NULL));
}

TEST_F(UnicodeStringTest, FreeUnicodeString_NullBuffer) {
    UNICODE_STRING str = {0};
    // Should not crash when freeing a zeroed structure
    FreeUnicodeString(&str);
}

TEST_F(UnicodeStringTest, FreeUnicodeString_ResetFields) {
    UNICODE_STRING dest = {0};
    UNICODE_STRING src = {0};
    
    NTSTATUS status = RtlInitUnicodeString(&src, L"Test");
    ASSERT_EQ(status, STATUS_SUCCESS);
    
    status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &src, &dest);
    ASSERT_EQ(status, STATUS_SUCCESS);
    
    FreeUnicodeString(&dest);
    EXPECT_EQ(dest.Buffer, nullptr);
    EXPECT_EQ(dest.Length, 0);
    EXPECT_EQ(dest.MaximumLength, 0);
}

TEST_F(UnicodeStringTest, HeapAllocation_Tracking) {
    GLOBAL_STATE* state = GetGlobalState();
    const wchar_t* testStr = L"Test String";
    SIZE_T allocSize = sizeof(WCHAR) * (wcslen(testStr) + 1);
    PVOID buffer = ExAllocatePoolTracked(NonPagedPool, allocSize);
    ASSERT_NE(buffer, nullptr) << "Memory allocation failed";

    ASSERT_EQ(state->CurrentBytesAllocated, allocSize);
    ASSERT_EQ(state->CurrentBytesAllocated, state->PeakBytesAllocated);

    // Free the memory
    ExFreePoolTracked(buffer);

    // Verify tracking after free
    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
    ASSERT_EQ(state->AllocationCount, (SIZE_T)1);  // Total count remains the same
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_NullTerminate) {
    const wchar_t* src = L"Test String";
    UNICODE_STRING source = {0};
    source.Buffer = (PWSTR)src;
    source.Length = (USHORT)(wcslen(src) * sizeof(WCHAR));
    source.MaximumLength = source.Length;

    UNICODE_STRING dest = {0};
    NTSTATUS status = RtlDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
        &source,
        &dest);

    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_NE(dest.Buffer, nullptr);
    ASSERT_EQ(dest.Length, source.Length);
    ASSERT_EQ(dest.MaximumLength, source.Length + sizeof(WCHAR));
    ASSERT_EQ(dest.Buffer[dest.Length / sizeof(WCHAR)], UNICODE_NULL);
    
    FreeUnicodeString(&dest);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_NoNullTerminate) {
    const wchar_t* src = L"Test String";
    UNICODE_STRING source = {0};
    source.Buffer = (PWSTR)src;
    source.Length = (USHORT)(wcslen(src) * sizeof(WCHAR));
    source.MaximumLength = source.Length;

    UNICODE_STRING dest = {0};
    NTSTATUS status = RtlDuplicateUnicodeString(0, &source, &dest);

    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_NE(dest.Buffer, nullptr);
    ASSERT_EQ(dest.Length, source.Length);
    ASSERT_EQ(dest.MaximumLength, source.Length);
    
    FreeUnicodeString(&dest);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_EmptyString) {
    UNICODE_STRING source = {0};
    UNICODE_STRING dest = {0};
    
    NTSTATUS status = RtlDuplicateUnicodeString(0, &source, &dest);
    
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_EQ(dest.Buffer, nullptr);
    ASSERT_EQ(dest.Length, 0);
    ASSERT_EQ(dest.MaximumLength, 0);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_AllocateNullString) {
    UNICODE_STRING source = {0};
    UNICODE_STRING dest = {0};
    
    NTSTATUS status = RtlDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE | 
        RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING,
        &source,
        &dest);
    
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_NE(dest.Buffer, nullptr);
    ASSERT_EQ(dest.Length, 0);
    ASSERT_EQ(dest.MaximumLength, sizeof(WCHAR));
    ASSERT_EQ(dest.Buffer[0], UNICODE_NULL);
    
    FreeUnicodeString(&dest);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_InvalidFlags) {
    UNICODE_STRING source = {0};
    UNICODE_STRING dest = {0};
    
    NTSTATUS status = RtlDuplicateUnicodeString(0x4, &source, &dest);
    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_NullOutput) {
    UNICODE_STRING source = {0};
    
    NTSTATUS status = RtlDuplicateUnicodeString(0, &source, NULL);
    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringTest, RtlValidateUnicodeString_Basic) {
    UNICODE_STRING str = {0};
    NTSTATUS status = RtlInitUnicodeString(&str, L"Test");
    ASSERT_EQ(status, STATUS_SUCCESS);
    
    status = RtlValidateUnicodeString(0, &str);
    ASSERT_EQ(status, STATUS_SUCCESS);
}

TEST_F(UnicodeStringTest, RtlValidateUnicodeString_NullString) {
    NTSTATUS status = RtlValidateUnicodeString(0, NULL);
    ASSERT_EQ(status, STATUS_SUCCESS);
}

TEST_F(UnicodeStringTest, RtlValidateUnicodeString_InvalidLength) {
    UNICODE_STRING str = {0};

    // Set Length to UNICODE_STRING_MAX_BYTES + 2 (sizeof(WCHAR))
    // Since Length is USHORT, this will cause proper overflow
    str.Length = (USHORT)(0xFFFF);  // Maximum USHORT value
    str.MaximumLength = str.Length;
    str.Buffer = (PWSTR)L"Test";
    
    printf("Test values:\n");
    printf("  UNICODE_STRING_MAX_BYTES: 0x%04x\n", UNICODE_STRING_MAX_BYTES);
    printf("  str.Length: 0x%04x\n", str.Length);
    printf("  sizeof(WCHAR): 0x%04x\n", (USHORT)sizeof(WCHAR));
    
    NTSTATUS status = RtlValidateUnicodeString(0, &str);
    printf("Got status: 0x%08x, Expected STATUS_INVALID_PARAMETER: 0x%08x\n", 
           (ULONG)status, (ULONG)STATUS_INVALID_PARAMETER);
    ASSERT_EQ((ULONG)status, (ULONG)STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringTest, RtlValidateUnicodeString_MaxLengthTooSmall) {
    UNICODE_STRING str = {0};
    str.Length = 10;
    str.MaximumLength = 8;
    str.Buffer = (PWSTR)L"Test";
    
    NTSTATUS status = RtlValidateUnicodeString(0, &str);
    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringTest, RtlInitUnicodeString_NullDestination) {
    NTSTATUS status = RtlInitUnicodeString(NULL, L"Test");
    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringTest, RtlInitUnicodeString_NullSource) {
    UNICODE_STRING dest = {0};
    NTSTATUS status = RtlInitUnicodeString(&dest, NULL);
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_EQ(dest.Buffer, nullptr);
    ASSERT_EQ(dest.Length, 0);
    ASSERT_EQ(dest.MaximumLength, 0);
}

TEST_F(UnicodeStringTest, RtlInitUnicodeString_TooLong) {
    UNICODE_STRING dest = {0};
    // Create a string that's too long (32768 chars)
    std::wstring longStr(UNICODE_STRING_MAX_CHARS + 1, L'A');
    
    NTSTATUS status = RtlInitUnicodeString(&dest, longStr.c_str());
    ASSERT_EQ(status, STATUS_NAME_TOO_LONG);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_InvalidString) {
    UNICODE_STRING source = {0};
    source.Buffer = NULL;
    source.Length = sizeof(WCHAR);  // Invalid: non-zero length with NULL buffer
    source.MaximumLength = sizeof(WCHAR);

    UNICODE_STRING dest = {0};
    NTSTATUS status = RtlDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
        &source,
        &dest);

    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_StringTooLong) {
    const wchar_t* str = L"Test";
    UNICODE_STRING source = {0};
    source.Buffer = (PWSTR)str;
    source.Length = UNICODE_STRING_MAX_BYTES;  // Maximum allowed length
    source.MaximumLength = source.Length;

    UNICODE_STRING dest = {0};
    NTSTATUS status = RtlDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,  // This flag will make it exceed max
        &source,
        &dest);

    ASSERT_EQ(status, STATUS_NAME_TOO_LONG);
}
