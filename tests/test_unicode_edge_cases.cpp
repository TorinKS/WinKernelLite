#include <gtest/gtest.h>
#include <Windows.h>
#include <string>
#include <vector>
#include "../include/UnicodeString.h"

class UnicodeEdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
    }
    
    void TearDown() override {
        // Clean up any allocated memory
    }
    
    void VerifyUnicodeString(const UNICODE_STRING& ustr, const std::wstring& expected) {
        EXPECT_EQ(ustr.Length, expected.length() * sizeof(WCHAR));
        EXPECT_EQ(ustr.MaximumLength, ustr.Length + sizeof(WCHAR));
        EXPECT_NE(ustr.Buffer, nullptr);
        
        if (ustr.Buffer) {
            std::wstring actual(ustr.Buffer, ustr.Length / sizeof(WCHAR));
            EXPECT_EQ(actual, expected);
        }
    }
};

TEST_F(UnicodeEdgeCasesTest, RtlInitUnicodeString_EmptyString) {
    UNICODE_STRING ustr;
    
    // Test with empty string
    RtlInitUnicodeString(&ustr, L"");
    
    EXPECT_EQ(ustr.Length, 0);
    EXPECT_EQ(ustr.MaximumLength, 2);  // Space for null terminator
    EXPECT_NE(ustr.Buffer, nullptr);   // Buffer points to empty string, not nullptr
    EXPECT_EQ(wcslen(ustr.Buffer), 0); // Buffer contains empty string
}

TEST_F(UnicodeEdgeCasesTest, RtlInitUnicodeString_NullPointer) {
    UNICODE_STRING ustr;
    
    // Test with NULL string
    RtlInitUnicodeString(&ustr, nullptr);
    
    EXPECT_EQ(ustr.Length, 0);
    EXPECT_EQ(ustr.MaximumLength, 0);
    EXPECT_EQ(ustr.Buffer, nullptr);
}

TEST_F(UnicodeEdgeCasesTest, RtlInitUnicodeString_SingleCharacter) {
    UNICODE_STRING ustr;
    
    // Test with single character
    RtlInitUnicodeString(&ustr, L"A");
    
    EXPECT_EQ(ustr.Length, sizeof(WCHAR));
    EXPECT_GT(ustr.MaximumLength, 0);
    EXPECT_NE(ustr.Buffer, nullptr);
    
    if (ustr.Buffer) {
        EXPECT_EQ(ustr.Buffer[0], L'A');
    }
}

TEST_F(UnicodeEdgeCasesTest, RtlInitUnicodeString_UnicodeCharacters) {
    UNICODE_STRING ustr;
    
    // Test with Unicode characters (non-ASCII)
    const wchar_t* testString = L"Hello世界🌍";
    RtlInitUnicodeString(&ustr, testString);
    
    size_t expectedLength = wcslen(testString) * sizeof(WCHAR);
    EXPECT_EQ(ustr.Length, expectedLength);
    EXPECT_GT(ustr.MaximumLength, 0);
    EXPECT_NE(ustr.Buffer, nullptr);
    
    if (ustr.Buffer) {
        EXPECT_EQ(wcscmp(ustr.Buffer, testString), 0);
    }
}

TEST_F(UnicodeEdgeCasesTest, RtlInitUnicodeString_MaxLength) {
    UNICODE_STRING ustr;
    
    // Test with very long string (close to maximum)
    std::wstring longString(1000, L'X');
    RtlInitUnicodeString(&ustr, longString.c_str());
    
    EXPECT_EQ(ustr.Length, longString.length() * sizeof(WCHAR));
    EXPECT_GT(ustr.MaximumLength, 0);
    EXPECT_NE(ustr.Buffer, nullptr);
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeToString_EmptyStrings) {
    UNICODE_STRING destination;
    WCHAR buffer[256];
    
    destination.Buffer = buffer;
    destination.Length = 0;
    destination.MaximumLength = sizeof(buffer);
    buffer[0] = L'\0';
    
    // Append empty string to empty string
    NTSTATUS status = RtlAppendUnicodeToString(&destination, L"");
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 0);
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeToString_NullSource) {
    UNICODE_STRING destination;
    WCHAR buffer[256];
    
    destination.Buffer = buffer;
    destination.Length = 0;
    destination.MaximumLength = sizeof(buffer);
    buffer[0] = L'\0';
    
    // Append NULL string
    NTSTATUS status = RtlAppendUnicodeToString(&destination, nullptr);
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 0);
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeToString_BufferOverflow) {
    UNICODE_STRING destination;
    WCHAR buffer[10]; // Small buffer
    
    destination.Buffer = buffer;
    destination.Length = 0;
    destination.MaximumLength = sizeof(buffer);
    buffer[0] = L'\0';
    
    // Try to append a string that would overflow
    std::wstring longString(20, L'A');
    NTSTATUS status = RtlAppendUnicodeToString(&destination, longString.c_str());
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeToString_ExactFit) {
    UNICODE_STRING destination;
    WCHAR buffer[11]; // Exactly fits "Hello" + null terminator
    
    destination.Buffer = buffer;
    destination.Length = 0;
    destination.MaximumLength = sizeof(buffer);
    buffer[0] = L'\0';
    
    // Append string that exactly fits
    NTSTATUS status = RtlAppendUnicodeToString(&destination, L"Hello");
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 5 * sizeof(WCHAR));
    
    std::wstring result(destination.Buffer, destination.Length / sizeof(WCHAR));
    EXPECT_EQ(result, L"Hello");
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeStringToString_NullDestination) {
    UNICODE_STRING source;
    RtlInitUnicodeString(&source, L"Test");
    
    // Test with NULL destination
    NTSTATUS status = RtlAppendUnicodeStringToString(nullptr, &source);
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeStringToString_NullSource) {
    UNICODE_STRING destination;
    WCHAR buffer[256];
    
    destination.Buffer = buffer;
    destination.Length = 0;
    destination.MaximumLength = sizeof(buffer);
    buffer[0] = L'\0';
    
    // Test with NULL source
    NTSTATUS status = RtlAppendUnicodeStringToString(&destination, nullptr);
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeStringToString_EmptySource) {
    UNICODE_STRING destination;
    UNICODE_STRING source;
    WCHAR buffer[256];
    
    destination.Buffer = buffer;
    destination.Length = 0;
    destination.MaximumLength = sizeof(buffer);
    buffer[0] = L'\0';
    
    // Initialize source as empty
    source.Buffer = nullptr;
    source.Length = 0;
    source.MaximumLength = 0;
    
    NTSTATUS status = RtlAppendUnicodeStringToString(&destination, &source);
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 0);
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeStringToString_ChainedAppends) {
    UNICODE_STRING destination;
    UNICODE_STRING source1, source2, source3;
    WCHAR buffer[256];
    
    destination.Buffer = buffer;
    destination.Length = 0;
    destination.MaximumLength = sizeof(buffer);
    buffer[0] = L'\0';
    
    RtlInitUnicodeString(&source1, L"Hello");
    RtlInitUnicodeString(&source2, L" ");
    RtlInitUnicodeString(&source3, L"World");
    
    // Chain multiple appends
    NTSTATUS status1 = RtlAppendUnicodeStringToString(&destination, &source1);
    NTSTATUS status2 = RtlAppendUnicodeStringToString(&destination, &source2);
    NTSTATUS status3 = RtlAppendUnicodeStringToString(&destination, &source3);
    
    EXPECT_EQ(status1, STATUS_SUCCESS);
    EXPECT_EQ(status2, STATUS_SUCCESS);
    EXPECT_EQ(status3, STATUS_SUCCESS);
    
    std::wstring result(destination.Buffer, destination.Length / sizeof(WCHAR));
    EXPECT_EQ(result, L"Hello World");
}

TEST_F(UnicodeEdgeCasesTest, RtlAppendUnicodeStringToString_BufferOverflow) {
    UNICODE_STRING destination;
    UNICODE_STRING source;
    WCHAR buffer[10]; // Small buffer
    WCHAR sourceBuffer[20];
    
    destination.Buffer = buffer;
    destination.Length = 4 * sizeof(WCHAR); // Pre-fill with "Test"
    destination.MaximumLength = sizeof(buffer);
    wcscpy_s(buffer, 10, L"Test");
    
    // Create source that would cause overflow
    wcscpy_s(sourceBuffer, _countof(sourceBuffer), L"LongString");  // Shorter but still overflows
    source.Buffer = sourceBuffer;
    source.Length = static_cast<USHORT>(wcslen(sourceBuffer) * sizeof(WCHAR));
    source.MaximumLength = sizeof(sourceBuffer);
    
    NTSTATUS status = RtlAppendUnicodeStringToString(&destination, &source);
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
    
    // Original content should be preserved
    std::wstring result(destination.Buffer, destination.Length / sizeof(WCHAR));
    EXPECT_EQ(result, L"Test");
}

TEST_F(UnicodeEdgeCasesTest, SpecialCharacters_Handling) {
    // Test various special characters
    std::vector<std::wstring> testStrings = {
        L"", // Empty
        L" ", // Space
        L"\t", // Tab
        L"\n", // Newline
        L"\r", // Carriage return
        L"\r\n", // CRLF
        L"\\", // Backslash
        L"/", // Forward slash
        L"\"", // Quote
        L"'", // Apostrophe
        L"null\0embedded", // Embedded null (will be truncated)
        L"Unicode: àáâãäåæçèéêë",
        L"Symbols: !@#$%^&*()_+-=[]{}|;:,.<>?",
        L"Numbers: 1234567890",
        L"Mixed: Hello123!@#世界"
    };
    
    for (const auto& testStr : testStrings) {
        UNICODE_STRING ustr;
        RtlInitUnicodeString(&ustr, testStr.c_str());
        
        // Basic validation
        if (testStr.empty()) {
            EXPECT_EQ(ustr.Length, 0);
        } else {
            EXPECT_GT(ustr.Length, 0);
            EXPECT_NE(ustr.Buffer, nullptr);
        }
    }
}

TEST_F(UnicodeEdgeCasesTest, LengthCalculations_EdgeCases) {
    // Test various string lengths
    std::vector<size_t> testLengths = {0, 1, 2, 3, 4, 8, 16, 32, 63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 512, 513, 1023, 1024};
    
    for (size_t len : testLengths) {
        std::wstring testStr(len, L'X');
        UNICODE_STRING ustr;
        
        RtlInitUnicodeString(&ustr, testStr.c_str());
        
        EXPECT_EQ(ustr.Length, static_cast<USHORT>(len * sizeof(WCHAR)));
        if (len > 0) {
            EXPECT_NE(ustr.Buffer, nullptr);
            EXPECT_GT(ustr.MaximumLength, 0);
        }
    }
}

TEST_F(UnicodeEdgeCasesTest, BufferBoundary_Tests) {
    // Test buffer boundary conditions
    UNICODE_STRING destination;
    WCHAR buffer[8]; // Small buffer for boundary testing (8 WCHARs = 16 bytes)
    
    destination.Buffer = buffer;
    destination.Length = 0;
    destination.MaximumLength = sizeof(buffer); // 16 bytes
    buffer[0] = L'\0';
    
    // Fill almost to capacity
    std::wstring almostFull(7, L'A'); // 7 chars = 14 bytes + 2 bytes for terminator = 16 bytes
    NTSTATUS status = RtlAppendUnicodeToString(&destination, almostFull.c_str());
    EXPECT_EQ(status, STATUS_SUCCESS);
    
    // Try to add one more character (should fail due to no room for null terminator)
    status = RtlAppendUnicodeToString(&destination, L"X");
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
    
    // Verify original content is preserved
    std::wstring result(destination.Buffer, destination.Length / sizeof(WCHAR));
    EXPECT_EQ(result, almostFull);
}

TEST_F(UnicodeEdgeCasesTest, ConsecutiveOperations_StateValidation) {
    UNICODE_STRING ustr1, ustr2, ustr3;
    
    // Initialize multiple strings
    RtlInitUnicodeString(&ustr1, L"First");
    RtlInitUnicodeString(&ustr2, L"Second");
    RtlInitUnicodeString(&ustr3, L"Third");
    
    // Verify each maintains its state
    VerifyUnicodeString(ustr1, L"First");
    VerifyUnicodeString(ustr2, L"Second");
    VerifyUnicodeString(ustr3, L"Third");
    
    // Modify one and verify others are unaffected
    UNICODE_STRING dest;
    WCHAR buffer[256];
    dest.Buffer = buffer;
    dest.Length = 0;
    dest.MaximumLength = sizeof(buffer);
    buffer[0] = L'\0';
    
    RtlAppendUnicodeStringToString(&dest, &ustr1);
    RtlAppendUnicodeToString(&dest, L" ");
    RtlAppendUnicodeStringToString(&dest, &ustr2);
    
    // Original strings should be unchanged
    VerifyUnicodeString(ustr1, L"First");
    VerifyUnicodeString(ustr2, L"Second");
    VerifyUnicodeString(ustr3, L"Third");
    
    // Destination should contain concatenation
    std::wstring result(dest.Buffer, dest.Length / sizeof(WCHAR));
    EXPECT_EQ(result, L"First Second");
}
