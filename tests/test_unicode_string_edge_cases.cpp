#include <gtest/gtest.h>
#include <Windows.h>
#include <string>
#include <vector>
#include "../include/UnicodeString.h"

class UnicodeStringEdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup any common test data
    }
    
    void TearDown() override {
        // Cleanup any test data
    }
};

TEST_F(UnicodeStringEdgeCasesTest, RtlInitUnicodeString_NullPointer) {
    UNICODE_STRING ustr;
    
    // Test with NULL source string
    RtlInitUnicodeString(&ustr, nullptr);
    
    EXPECT_EQ(ustr.Length, 0);
    EXPECT_EQ(ustr.MaximumLength, 0);
    EXPECT_EQ(ustr.Buffer, nullptr);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlInitUnicodeString_EmptyString) {
    UNICODE_STRING ustr;
    
    // Test with empty string
    RtlInitUnicodeString(&ustr, L"");
    
    EXPECT_EQ(ustr.Length, 0);
    EXPECT_EQ(ustr.MaximumLength, sizeof(WCHAR)); // Should account for null terminator
    EXPECT_NE(ustr.Buffer, nullptr);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlInitUnicodeString_VeryLongString) {
    // Create a very long string (close to USHRT_MAX characters)
    const size_t longStringLength = 32000; // Close to USHORT limit
    std::wstring longString(longStringLength, L'A');
    
    UNICODE_STRING ustr;
    RtlInitUnicodeString(&ustr, longString.c_str());
    
    EXPECT_EQ(ustr.Length, longStringLength * sizeof(WCHAR));
    EXPECT_EQ(ustr.MaximumLength, (longStringLength + 1) * sizeof(WCHAR));
    EXPECT_NE(ustr.Buffer, nullptr);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlInitUnicodeString_MaxLengthOverflow) {
    // Test potential overflow scenarios
    // Create a string that might cause overflow when calculating MaximumLength
    const size_t maxPossibleChars = USHRT_MAX / sizeof(WCHAR);
    
    if constexpr (maxPossibleChars > 1000) { // Only test if reasonable
        std::wstring maxString(maxPossibleChars - 1, L'B');
        
        UNICODE_STRING ustr;
        RtlInitUnicodeString(&ustr, maxString.c_str());
        
        // Should handle the maximum case gracefully
        EXPECT_LE(ustr.Length, USHRT_MAX);
        EXPECT_LE(ustr.MaximumLength, USHRT_MAX);
    }
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeToString_NullDestination) {
    UNICODE_STRING dest;
    RtlInitUnicodeString(&dest, nullptr);
    
    // Try to append to NULL destination
    NTSTATUS status = RtlAppendUnicodeToString(&dest, L"Test");
    
    // Should handle gracefully (either fail or succeed based on implementation)
    EXPECT_TRUE(NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeToString_NullSource) {
    WCHAR buffer[100];
    UNICODE_STRING dest;
    dest.Buffer = buffer;
    dest.Length = 0;
    dest.MaximumLength = sizeof(buffer);
    
    // Try to append NULL source
    NTSTATUS status = RtlAppendUnicodeToString(&dest, nullptr);
    
    // Should succeed (appending nothing)
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(dest.Length, 0);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeToString_BufferTooSmall) {
    WCHAR smallBuffer[5];
    UNICODE_STRING dest;
    dest.Buffer = smallBuffer;
    dest.Length = 0;
    dest.MaximumLength = sizeof(smallBuffer);
    
    // Try to append a string that won't fit
    NTSTATUS status = RtlAppendUnicodeToString(&dest, L"This string is definitely too long for the buffer");
    
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
    // Buffer should remain unchanged
    EXPECT_EQ(dest.Length, 0);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeToString_ExactFit) {
    WCHAR buffer[6]; // 5 characters + null terminator
    UNICODE_STRING dest;
    dest.Buffer = buffer;
    dest.Length = 0;
    dest.MaximumLength = sizeof(buffer);
    
    // Append exactly 5 characters
    NTSTATUS status = RtlAppendUnicodeToString(&dest, L"Hello");
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(dest.Length, 5 * sizeof(WCHAR));
    EXPECT_EQ(wcsncmp(dest.Buffer, L"Hello", 5), 0);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeToString_AppendToExisting) {
    WCHAR buffer[20];
    UNICODE_STRING dest;
    dest.Buffer = buffer;
    dest.MaximumLength = sizeof(buffer);
    
    // Initialize with some content
    wcscpy_s(buffer, 20, L"Hello");
    dest.Length = 5 * sizeof(WCHAR);
    
    // Append more content
    NTSTATUS status = RtlAppendUnicodeToString(&dest, L" World");
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(dest.Length, 11 * sizeof(WCHAR));
    EXPECT_EQ(wcsncmp(dest.Buffer, L"Hello World", 11), 0);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeStringToString_BothNull) {
    UNICODE_STRING dest;
    UNICODE_STRING src;
    
    RtlInitUnicodeString(&dest, nullptr);
    RtlInitUnicodeString(&src, nullptr);
    
    NTSTATUS status = RtlAppendUnicodeStringToString(&dest, &src);
    
    // Should handle gracefully
    EXPECT_TRUE(NT_SUCCESS(status) || status == STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeStringToString_NullSource) {
    WCHAR buffer[100];
    UNICODE_STRING dest;
    dest.Buffer = buffer;
    dest.Length = 0;
    dest.MaximumLength = sizeof(buffer);
    
    // Try to append NULL source
    NTSTATUS status = RtlAppendUnicodeStringToString(&dest, nullptr);
    
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeStringToString_EmptyStrings) {
    WCHAR buffer1[100];
    WCHAR buffer2[100];
    UNICODE_STRING dest, src;
    
    dest.Buffer = buffer1;
    dest.Length = 0;
    dest.MaximumLength = sizeof(buffer1);
    
    src.Buffer = buffer2;
    src.Length = 0;
    src.MaximumLength = sizeof(buffer2);
    
    NTSTATUS status = RtlAppendUnicodeStringToString(&dest, &src);
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(dest.Length, 0);
}

TEST_F(UnicodeStringEdgeCasesTest, RtlAppendUnicodeStringToString_Overflow) {
    WCHAR smallBuffer[5];
    WCHAR sourceBuffer[100];
    UNICODE_STRING dest, src;
    
    dest.Buffer = smallBuffer;
    dest.Length = 0;
    dest.MaximumLength = sizeof(smallBuffer);
    
    wcscpy_s(sourceBuffer, 100, L"This is a very long string that will not fit");
    src.Buffer = sourceBuffer;
    src.Length = (USHORT)(wcslen(sourceBuffer) * sizeof(WCHAR));
    src.MaximumLength = sizeof(sourceBuffer);
    
    NTSTATUS status = RtlAppendUnicodeStringToString(&dest, &src);
    
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
    EXPECT_EQ(dest.Length, 0); // Should remain unchanged
}

TEST_F(UnicodeStringEdgeCasesTest, UnicodeString_SpecialCharacters) {
    // Test with various special Unicode characters
    const wchar_t* specialStrings[] = {
        L"\u0000",     // Null character
        L"\u0001\u0002\u0003", // Control characters
        L"\u00A9\u00AE",       // Copyright, registered trademark
        L"\u20AC",             // Euro symbol
        L"\U0001F600",         // Emoji (if supported)
        L"\uFFFD",             // Replacement character
        L"\u3042\u3044\u3046", // Japanese Hiragana
        L"\u4E2D\u6587",       // Chinese characters
    };
    
    for (const wchar_t* testStr : specialStrings) {
        UNICODE_STRING ustr;
        RtlInitUnicodeString(&ustr, testStr);
        
        EXPECT_NE(ustr.Buffer, nullptr);
        EXPECT_EQ(ustr.Length, wcslen(testStr) * sizeof(WCHAR));
        
        // Test appending special characters
        WCHAR buffer[200];
        UNICODE_STRING dest;
        dest.Buffer = buffer;
        dest.Length = 0;
        dest.MaximumLength = sizeof(buffer);
        
        NTSTATUS status = RtlAppendUnicodeToString(&dest, testStr);
        EXPECT_EQ(status, STATUS_SUCCESS);
    }
}

TEST_F(UnicodeStringEdgeCasesTest, UnicodeString_LengthCalculations) {
    // Test edge cases in length calculations
    struct TestCase {
        const wchar_t* str;
        USHORT expectedLength;
        USHORT expectedMaxLength;
    };
    
    TestCase testCases[] = {
        {L"", 0, sizeof(WCHAR)},
        {L"A", sizeof(WCHAR), 2 * sizeof(WCHAR)},
        {L"AB", 2 * sizeof(WCHAR), 3 * sizeof(WCHAR)},
        {L"ABC", 3 * sizeof(WCHAR), 4 * sizeof(WCHAR)},
    };
    
    for (const TestCase& testCase : testCases) {
        UNICODE_STRING ustr;
        RtlInitUnicodeString(&ustr, testCase.str);
        
        EXPECT_EQ(ustr.Length, testCase.expectedLength) 
            << "Length mismatch for string: " << (testCase.str ? "valid" : "null");
        EXPECT_EQ(ustr.MaximumLength, testCase.expectedMaxLength)
            << "MaximumLength mismatch for string: " << (testCase.str ? "valid" : "null");
    }
}

TEST_F(UnicodeStringEdgeCasesTest, UnicodeString_BufferAlignment) {
    // Test potential alignment issues
    WCHAR buffer[100];
    UNICODE_STRING ustr;
    
    // Test with various buffer alignments
    for (int offset = 0; offset < 8; offset += 2) {
        WCHAR* alignedBuffer = (WCHAR*)((char*)buffer + offset);
        
        ustr.Buffer = alignedBuffer;
        ustr.Length = 10 * sizeof(WCHAR);
        ustr.MaximumLength = static_cast<USHORT>((100 - offset/2) * sizeof(WCHAR));
        
        // Try to use the aligned buffer
        wcscpy_s(alignedBuffer, (100 - offset/2), L"Test");
        
        // Verify the string works correctly
        EXPECT_EQ(wcslen(alignedBuffer), 4);
    }
}

TEST_F(UnicodeStringEdgeCasesTest, RtlCompareUnicodeString_EdgeCases) {
    UNICODE_STRING str1, str2;
    
    // Test comparing with null strings
    RtlInitUnicodeString(&str1, nullptr);
    RtlInitUnicodeString(&str2, nullptr);
    
    LONG result = RtlCompareUnicodeString(&str1, &str2, FALSE);
    EXPECT_EQ(result, 0); // Both null should be equal
    
    // Test comparing null with non-null
    RtlInitUnicodeString(&str2, L"Test");
    result = RtlCompareUnicodeString(&str1, &str2, FALSE);
    EXPECT_LT(result, 0); // Null should be less than non-null
    
    // Test comparing empty strings
    RtlInitUnicodeString(&str1, L"");
    RtlInitUnicodeString(&str2, L"");
    result = RtlCompareUnicodeString(&str1, &str2, FALSE);
    EXPECT_EQ(result, 0);
    
    // Test case sensitivity
    RtlInitUnicodeString(&str1, L"Test");
    RtlInitUnicodeString(&str2, L"test");
    
    result = RtlCompareUnicodeString(&str1, &str2, FALSE); // Case sensitive
    EXPECT_NE(result, 0);
    
    result = RtlCompareUnicodeString(&str1, &str2, TRUE); // Case insensitive
    EXPECT_EQ(result, 0);
}

TEST_F(UnicodeStringEdgeCasesTest, UnicodeString_MemoryCorruption) {
    // Test detection/handling of corrupted UNICODE_STRING structures
    UNICODE_STRING ustr;
    
    // Test with mismatched Length and MaximumLength
    WCHAR buffer[10];
    ustr.Buffer = buffer;
    ustr.Length = 20 * sizeof(WCHAR); // Longer than buffer
    ustr.MaximumLength = 5 * sizeof(WCHAR); // Shorter than Length
    
    // Operations should handle this gracefully
    wcscpy_s(buffer, 10, L"Test");
    
    // Test with NULL buffer but non-zero length
    ustr.Buffer = NULL;
    ustr.Length = 10 * sizeof(WCHAR);
    ustr.MaximumLength = 20 * sizeof(WCHAR);
    
    // This should be handled gracefully by functions
    NTSTATUS status = RtlAppendUnicodeToString(&ustr, L"Test");
    EXPECT_NE(status, STATUS_SUCCESS); // Should fail gracefully
}
