#include <gtest/gtest.h>
#include <string>
#include <WinKernelLite/UnicodeString.h>
#include <WinKernelLite/KernelHeap.h>
#include "WinKernelLiteTestBase.h"

class UnicodeStringTest : public WinKernelLiteTestBase {
protected:
    void SetUp() override {
        WinKernelLiteTestBase::SetUp();
        LogTestInfo("Unicode string test suite initialized");
    }

    void TearDown() override {
        LogTestInfo("Unicode string test suite completed");
        WinKernelLiteTestBase::TearDown();
    }
};

TEST_F(UnicodeStringTest, RtlInitUnicodeString_Success) {
    LogTestStep("Basic Unicode string initialization test");
    
    UNICODE_STRING dest = {0};
    const wchar_t* src = L"Hello";
    
    LogTestInfo("Initializing Unicode string with: '%ls'", src);
    NTSTATUS status = RtlInitUnicodeString(&dest, src);
    
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_EQ(dest.Buffer, (PWSTR)src) << "Buffer should point to source string";
    ASSERT_EQ(dest.Length, wcslen(src) * sizeof(WCHAR));
    ASSERT_EQ(dest.MaximumLength, dest.Length + sizeof(UNICODE_NULL));
    
    LogTestInfo("Unicode string initialized successfully: Length=%u, MaxLength=%u", 
               dest.Length, dest.MaximumLength);
}

TEST_F(UnicodeStringTest, RtlInitUnicodeString_EmptyString) {
    LogTestStep("Empty Unicode string initialization test");
    
    UNICODE_STRING dest = {0};
    const wchar_t* src = L"";
    
    LogTestInfo("Initializing Unicode string with empty string");
    NTSTATUS status = RtlInitUnicodeString(&dest, src);
    
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_EQ(dest.Buffer, (PWSTR)src) << "Buffer should point to source string";
    ASSERT_EQ(dest.Length, 0);
    ASSERT_EQ(dest.MaximumLength, sizeof(UNICODE_NULL));
    
    LogTestInfo("Empty Unicode string initialized: Length=%u, MaxLength=%u", 
               dest.Length, dest.MaximumLength);
}

TEST_F(UnicodeStringTest, FreeUnicodeString_NullBuffer) {
    LogTestStep("Free Unicode string with null buffer test");
    
    UNICODE_STRING str = {0};
    LogTestInfo("Testing FreeUnicodeString with zeroed structure");
    
    // Should not crash when freeing a zeroed structure
    FreeUnicodeString(&str);
    LogTestInfo("FreeUnicodeString with null buffer completed successfully");
}

TEST_F(UnicodeStringTest, FreeUnicodeString_ResetFields) {
    LogTestStep("Free Unicode string field reset test");
    
    UNICODE_STRING dest = {0};
    UNICODE_STRING src = {0};
    
    LogTestInfo("Creating duplicated Unicode string for free test");
    NTSTATUS status = RtlInitUnicodeString(&src, L"Test");
    ASSERT_EQ(status, STATUS_SUCCESS);
    
    status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &src, &dest);
    ASSERT_EQ(status, STATUS_SUCCESS);
    LogTestInfo("Duplicated string created: Length=%u, Buffer=%p", dest.Length, dest.Buffer);
    
    LogTestStep("Freeing duplicated string and verifying field reset");
    FreeUnicodeString(&dest);
    
    EXPECT_EQ(dest.Buffer, nullptr);
    EXPECT_EQ(dest.Length, 0);
    EXPECT_EQ(dest.MaximumLength, 0);
    LogTestInfo("String fields reset after free: Buffer=%p, Length=%u, MaxLength=%u", 
               dest.Buffer, dest.Length, dest.MaximumLength);
}

TEST_F(UnicodeStringTest, HeapAllocation_Tracking) {
    LogTestStep("Heap allocation tracking test");
    
    GLOBAL_STATE* state = GetGlobalState();
    const wchar_t* testStr = L"Test String";
    SIZE_T allocSize = sizeof(WCHAR) * (wcslen(testStr) + 1);
    
    LogTestInfo("Allocating %zu bytes for string: '%ls'", allocSize, testStr);
    PVOID buffer = ExAllocatePoolTracked(NonPagedPool, allocSize);
    ASSERT_NE(buffer, nullptr) << "Memory allocation failed";
    LogTestInfo("Allocated buffer at: %p", buffer);

    ASSERT_EQ(state->CurrentBytesAllocated, allocSize);
    ASSERT_EQ(state->CurrentBytesAllocated, state->PeakBytesAllocated);
    LogTestInfo("Heap tracking verified: Current=%zu, Peak=%zu", 
               state->CurrentBytesAllocated, state->PeakBytesAllocated);

    LogTestStep("Freeing tracked allocation");
    ExFreePoolTracked(buffer);
    LogTestInfo("Buffer freed: %p", buffer);

    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
    ASSERT_EQ(state->AllocationCount, (SIZE_T)1);  // Total count remains the same
    LogTestInfo("Heap tracking after free: Current=%zu, TotalCount=%zu", 
               state->CurrentBytesAllocated, state->AllocationCount);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_NullTerminate) {
    LogTestStep("Duplicate Unicode string with null termination test");
    
    const wchar_t* src = L"Test String";
    UNICODE_STRING source = {0};
    source.Buffer = (PWSTR)src;
    source.Length = (USHORT)(wcslen(src) * sizeof(WCHAR));
    source.MaximumLength = source.Length;

    LogTestInfo("Source string: '%ls', Length=%u", src, source.Length);

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
    
    LogTestInfo("Duplicated string: Length=%u, MaxLength=%u, Buffer=%p", 
               dest.Length, dest.MaximumLength, dest.Buffer);
    LogTestInfo("Null termination verified at position %u", dest.Length / sizeof(WCHAR));
    
    FreeUnicodeString(&dest);
    LogTestInfo("Duplicated string freed successfully");
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_NoNullTerminate) {
    LogTestStep("Duplicate Unicode string without null termination test");
    
    const wchar_t* src = L"Test String";
    UNICODE_STRING source = {0};
    source.Buffer = (PWSTR)src;
    source.Length = (USHORT)(wcslen(src) * sizeof(WCHAR));
    source.MaximumLength = source.Length;

    LogTestInfo("Source string: '%ls', Length=%u", src, source.Length);

    UNICODE_STRING dest = {0};
    NTSTATUS status = RtlDuplicateUnicodeString(0, &source, &dest);

    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_NE(dest.Buffer, nullptr);
    ASSERT_EQ(dest.Length, source.Length);
    ASSERT_EQ(dest.MaximumLength, source.Length);
    
    LogTestInfo("Duplicated string (no null term): Length=%u, MaxLength=%u", 
               dest.Length, dest.MaximumLength);
    
    FreeUnicodeString(&dest);
    LogTestInfo("Non-null-terminated string freed successfully");
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_EmptyString) {
    LogTestStep("Duplicate empty Unicode string test");
    
    UNICODE_STRING source = {0};
    UNICODE_STRING dest = {0};
    
    LogTestInfo("Duplicating empty Unicode string");
    NTSTATUS status = RtlDuplicateUnicodeString(0, &source, &dest);
    
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_EQ(dest.Buffer, nullptr);
    ASSERT_EQ(dest.Length, 0);
    ASSERT_EQ(dest.MaximumLength, 0);
    
    LogTestInfo("Empty string duplication result: Buffer=%p, Length=%u, MaxLength=%u", 
               dest.Buffer, dest.Length, dest.MaximumLength);
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_AllocateNullString) {
    LogTestStep("Duplicate Unicode string with null string allocation test");
    
    UNICODE_STRING source = {0};
    UNICODE_STRING dest = {0};
    
    LogTestInfo("Duplicating empty string with ALLOCATE_NULL_STRING flag");
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
    
    LogTestInfo("Null string allocated: Buffer=%p, Length=%u, MaxLength=%u", 
               dest.Buffer, dest.Length, dest.MaximumLength);
    
    FreeUnicodeString(&dest);
    LogTestInfo("Allocated null string freed successfully");
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_InvalidFlags) {
    LogTestStep("Duplicate Unicode string with invalid flags test");
    
    UNICODE_STRING source = {0};
    UNICODE_STRING dest = {0};
    
    LogTestInfo("Testing invalid flags (0x4) for string duplication");
    NTSTATUS status = RtlDuplicateUnicodeString(0x4, &source, &dest);
    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
    LogTestInfo("Invalid flags correctly rejected with STATUS_INVALID_PARAMETER");
}

TEST_F(UnicodeStringTest, RtlDuplicateUnicodeString_NullOutput) {
    LogTestStep("Duplicate Unicode string with null output test");
    
    UNICODE_STRING source = {0};
    
    LogTestInfo("Testing null output parameter for string duplication");
    NTSTATUS status = RtlDuplicateUnicodeString(0, &source, NULL);
    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
    LogTestInfo("Null output parameter correctly rejected");
}

TEST_F(UnicodeStringTest, RtlValidateUnicodeString_Basic) {
    LogTestStep("Basic Unicode string validation test");
    
    UNICODE_STRING str = {0};
    NTSTATUS status = RtlInitUnicodeString(&str, L"Test");
    ASSERT_EQ(status, STATUS_SUCCESS);
    LogTestInfo("Initialized test string: '%ls'", L"Test");
    
    status = RtlValidateUnicodeString(0, &str);
    ASSERT_EQ(status, STATUS_SUCCESS);
    LogTestInfo("String validation successful for basic string");
}

TEST_F(UnicodeStringTest, RtlValidateUnicodeString_NullString) {
    LogTestStep("Validate null Unicode string test");
    
    LogTestInfo("Testing validation of null string pointer");
    NTSTATUS status = RtlValidateUnicodeString(0, NULL);
    ASSERT_EQ(status, STATUS_SUCCESS);
    LogTestInfo("Null string validation returned STATUS_SUCCESS (expected)");
}

TEST_F(UnicodeStringTest, RtlValidateUnicodeString_InvalidLength) {
    LogTestStep("Validate Unicode string with invalid length test");
    
    UNICODE_STRING str = {0};

    // Set Length to maximum USHORT value to test overflow conditions
    str.Length = (USHORT)(0xFFFF);  // Maximum USHORT value
    str.MaximumLength = str.Length;
    str.Buffer = (PWSTR)L"Test";
    
    LogTestInfo("Testing validation with invalid length:");
    LogTestInfo("  UNICODE_STRING_MAX_BYTES: 0x%04x", UNICODE_STRING_MAX_BYTES);
    LogTestInfo("  str.Length: 0x%04x", str.Length);
    LogTestInfo("  sizeof(WCHAR): 0x%04x", (USHORT)sizeof(WCHAR));
    
    NTSTATUS status = RtlValidateUnicodeString(0, &str);
    LogTestInfo("Validation result: 0x%08x, Expected STATUS_INVALID_PARAMETER: 0x%08x", 
               (ULONG)status, (ULONG)STATUS_INVALID_PARAMETER);
    ASSERT_EQ((ULONG)status, (ULONG)STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringTest, RtlValidateUnicodeString_MaxLengthTooSmall) {
    LogTestStep("Validate Unicode string with MaxLength smaller than Length test");
    
    UNICODE_STRING str = {0};
    str.Length = 10;
    str.MaximumLength = 8;
    str.Buffer = (PWSTR)L"Test";
    
    LogTestInfo("Testing validation with MaxLength (%u) < Length (%u)", 
               str.MaximumLength, str.Length);
    
    NTSTATUS status = RtlValidateUnicodeString(0, &str);
    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
    LogTestInfo("Invalid MaxLength/Length relationship correctly detected");
}

TEST_F(UnicodeStringTest, RtlInitUnicodeString_NullDestination) {
    LogTestStep("Initialize Unicode string with null destination test");
    
    LogTestInfo("Testing RtlInitUnicodeString with null destination");
    NTSTATUS status = RtlInitUnicodeString(NULL, L"Test");
    ASSERT_EQ(status, STATUS_INVALID_PARAMETER);
    LogTestInfo("Null destination correctly rejected");
}

TEST_F(UnicodeStringTest, RtlInitUnicodeString_NullSource) {
    LogTestStep("Initialize Unicode string with null source test");
    
    UNICODE_STRING dest = {0};
    LogTestInfo("Testing RtlInitUnicodeString with null source");
    NTSTATUS status = RtlInitUnicodeString(&dest, NULL);
    
    ASSERT_EQ(status, STATUS_SUCCESS);
    ASSERT_EQ(dest.Buffer, nullptr);
    ASSERT_EQ(dest.Length, 0);
    ASSERT_EQ(dest.MaximumLength, 0);
    LogTestInfo("Null source handled correctly: Buffer=%p, Length=%u, MaxLength=%u", 
               dest.Buffer, dest.Length, dest.MaximumLength);
}

TEST_F(UnicodeStringTest, RtlInitUnicodeString_TooLong) {
    LogTestStep("Initialize Unicode string with too long source test");
    
    UNICODE_STRING dest = {0};
    // Create a string that's too long (32768 chars + 1)
    std::wstring longStr(UNICODE_STRING_MAX_CHARS + 1, L'A');
    
    LogTestInfo("Testing with string length: %zu chars (max allowed: %u)", 
               longStr.length(), UNICODE_STRING_MAX_CHARS);
    
    NTSTATUS status = RtlInitUnicodeString(&dest, longStr.c_str());
    ASSERT_EQ(status, STATUS_NAME_TOO_LONG);
    LogTestInfo("Too long string correctly rejected with STATUS_NAME_TOO_LONG");
}

TEST_F(UnicodeStringTest, RtlCompareUnicodeString_Basic) {
    LogTestStep("Basic Unicode string comparison test");
    
    UNICODE_STRING string1;
    UNICODE_STRING string2;
    
    // Initialize two equal strings
    RtlInitUnicodeString(&string1, L"Hello");
    RtlInitUnicodeString(&string2, L"Hello");
    LogTestInfo("Comparing equal strings: 'Hello' vs 'Hello'");
    
    // Test equal strings
    EXPECT_EQ(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    LogTestInfo("Equal strings comparison result: 0 (expected)");
    
    // Test case sensitivity
    RtlInitUnicodeString(&string2, L"hello");
    LogTestInfo("Testing case sensitivity: 'Hello' vs 'hello'");
    EXPECT_NE(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    EXPECT_EQ(RtlCompareUnicodeString(&string1, &string2, TRUE), 0);
    LogTestInfo("Case sensitive: ≠0, Case insensitive: =0 (expected)");
    
    // Test string1 > string2
    RtlInitUnicodeString(&string1, L"Hello");
    RtlInitUnicodeString(&string2, L"Hella");
    LogTestInfo("Testing string comparison: 'Hello' vs 'Hella'");
    EXPECT_GT(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    LogTestInfo("'Hello' > 'Hella': >0 (expected)");
    
    // Test string1 < string2
    RtlInitUnicodeString(&string1, L"Hella");
    RtlInitUnicodeString(&string2, L"Hello");
    LogTestInfo("Testing string comparison: 'Hella' vs 'Hello'");
    EXPECT_LT(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    LogTestInfo("'Hella' < 'Hello': <0 (expected)");
    
    // Test different lengths (prefix)
    RtlInitUnicodeString(&string1, L"Hello");
    RtlInitUnicodeString(&string2, L"HelloWorld");
    LogTestInfo("Testing prefix comparison: 'Hello' vs 'HelloWorld'");
    EXPECT_LT(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    LogTestInfo("Shorter string < longer string: <0 (expected)");
    
    // Test empty strings
    RtlInitUnicodeString(&string1, L"");
    RtlInitUnicodeString(&string2, L"");
    EXPECT_EQ(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    LogTestInfo("Empty string comparison result: 0 (expected)");
    
    // Test empty vs non-empty
    RtlInitUnicodeString(&string1, L"");
    RtlInitUnicodeString(&string2, L"Hello");
    EXPECT_LT(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    LogTestInfo("Empty string < non-empty string: <0 (expected)");
}

TEST_F(UnicodeStringTest, RtlCompareUnicodeString_CaseInsensitive) {
    LogTestStep("Case insensitive Unicode string comparison test");
    
    UNICODE_STRING string1;
    UNICODE_STRING string2;
    
    // Test mixed case
    RtlInitUnicodeString(&string1, L"Hello");
    RtlInitUnicodeString(&string2, L"hElLo");
    LogTestInfo("Testing mixed case: 'Hello' vs 'hElLo'");
    EXPECT_NE(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    EXPECT_EQ(RtlCompareUnicodeString(&string1, &string2, TRUE), 0);
    LogTestInfo("Case sensitive: ≠0, Case insensitive: =0 (expected)");
    
    // Test uppercase vs lowercase
    RtlInitUnicodeString(&string1, L"WINDOWS");
    RtlInitUnicodeString(&string2, L"windows");
    LogTestInfo("Testing case comparison: 'WINDOWS' vs 'windows'");
    EXPECT_NE(RtlCompareUnicodeString(&string1, &string2, FALSE), 0);
    EXPECT_EQ(RtlCompareUnicodeString(&string1, &string2, TRUE), 0);
    LogTestInfo("Case insensitive comparison successful");
    
    // Test comparison with mixed case and different strings
    RtlInitUnicodeString(&string1, L"Wind");  // 'W', 'i', 'n', 'd'
    RtlInitUnicodeString(&string2, L"Wint");  // 'W', 'i', 'n', 't'
    LogTestInfo("Testing case insensitive ordering: 'Wind' vs 'Wint'");
    
    // In a case-insensitive comparison, 'd' comes before 't' in ASCII
    LONG result = RtlCompareUnicodeString(&string1, &string2, TRUE);
    EXPECT_LT(result, 0);
    LogTestInfo("'Wind' < 'Wint' (case insensitive): %ld (expected <0)", result);
}

TEST_F(UnicodeStringTest, RtlCompareUnicodeString_EdgeCases) {
    LogTestStep("Unicode string comparison edge cases test");
    
    UNICODE_STRING string1;
    UNICODE_STRING string2;
    UNICODE_STRING nullString;
    
    // Initialize strings
    RtlInitUnicodeString(&string1, L"Test");
    RtlInitUnicodeString(&string2, L"Test");
    
    // Initialize a null string (Buffer is NULL)
    nullString.Length = 0;
    nullString.MaximumLength = 0;
    nullString.Buffer = NULL;
    LogTestInfo("Testing edge cases with null buffers");
    
    // Test with null buffer
    EXPECT_EQ(RtlCompareUnicodeString(&nullString, &nullString, FALSE), 0);
    EXPECT_LT(RtlCompareUnicodeString(&nullString, &string1, FALSE), 0);
    EXPECT_GT(RtlCompareUnicodeString(&string1, &nullString, FALSE), 0);
    LogTestInfo("Null buffer comparisons: null==null, null<string, string>null (expected)");
}

TEST_F(UnicodeStringTest, RtlUpcaseUnicodeString_Basic) {
    LogTestStep("Basic Unicode string uppercase conversion test");
    
    UNICODE_STRING source;
    UNICODE_STRING dest;
    
    // Initialize source string with mixed case
    RtlInitUnicodeString(&source, L"Hello World 123");
    LogTestInfo("Source string for uppercase: '%ls'", L"Hello World 123");
    
    // Test with allocation
    NTSTATUS status = RtlUpcaseUnicodeString(&dest, &source, TRUE);
    ASSERT_EQ(status, STATUS_SUCCESS);
    
    // Verify the result
    ASSERT_EQ(dest.Length, source.Length);
    ASSERT_NE(dest.Buffer, nullptr);
    
    // Check the content is properly converted to uppercase
    ASSERT_EQ(dest.Buffer[0], L'H');
    ASSERT_EQ(dest.Buffer[1], L'E');
    ASSERT_EQ(dest.Buffer[2], L'L');
    ASSERT_EQ(dest.Buffer[3], L'L');
    ASSERT_EQ(dest.Buffer[4], L'O');
    
    LogTestInfo("Uppercase conversion (allocated): Length=%u, Buffer=%p", 
               dest.Length, dest.Buffer);
    LogTestInfo("First 5 chars: %lc%lc%lc%lc%lc", 
               dest.Buffer[0], dest.Buffer[1], dest.Buffer[2], dest.Buffer[3], dest.Buffer[4]);
    
    // Free the allocated string
    FreeUnicodeString(&dest);
    LogTestInfo("Allocated uppercase string freed");
}

TEST_F(UnicodeStringTest, RtlUnicodeStringToInteger_Basic) {
    LogTestStep("Basic Unicode string to integer conversion test");
    
    UNICODE_STRING str;
    ULONG value;
    
    // Test basic decimal conversion
    RtlInitUnicodeString(&str, L"123");
    LogTestInfo("Converting '123' to integer (base 10)");
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 10, &value), STATUS_SUCCESS);
    ASSERT_EQ(value, 123UL);
    LogTestInfo("Decimal conversion result: %lu", value);
    
    // Test hexadecimal conversion with explicit base
    RtlInitUnicodeString(&str, L"FF");
    LogTestInfo("Converting 'FF' to integer (base 16)");
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 16, &value), STATUS_SUCCESS);
    ASSERT_EQ(value, 255UL);
    LogTestInfo("Hexadecimal conversion result: %lu", value);
}

TEST_F(UnicodeStringTest, RtlCopyUnicodeString_Basic) {
    LogTestStep("Basic Unicode string copy test");
    
    UNICODE_STRING source;
    UNICODE_STRING dest;
    WCHAR buffer[50] = {0};
    
    // Initialize source string
    RtlInitUnicodeString(&source, L"Hello World");
    LogTestInfo("Source string for copy: '%ls'", L"Hello World");
    
    // Setup destination with buffer
    dest.Buffer = buffer;
    dest.Length = 0;
    dest.MaximumLength = sizeof(buffer);
    
    LogTestInfo("Destination buffer: %zu bytes available", sizeof(buffer));
    
    // Copy string
    RtlCopyUnicodeString(&dest, &source);
    
    // Verify result
    ASSERT_EQ(dest.Length, source.Length);
    ASSERT_EQ(dest.MaximumLength, sizeof(buffer));
    
    // Check content
    for (USHORT i = 0; i < source.Length / sizeof(WCHAR); i++) {
        ASSERT_EQ(dest.Buffer[i], source.Buffer[i]);
    }
    
    // Check null termination
    ASSERT_EQ(dest.Buffer[source.Length / sizeof(WCHAR)], UNICODE_NULL);
    
    LogTestInfo("String copy successful: Length=%u, null-terminated at position %u", 
               dest.Length, source.Length / sizeof(WCHAR));
}
