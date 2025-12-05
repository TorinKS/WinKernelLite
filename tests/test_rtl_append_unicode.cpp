#include <gtest/gtest.h>
#include "../include/UnicodeString.h"
#include "../include/Wdm.h"

class RtlAppendUnicodeStringTest : public ::testing::Test {
protected:
    WCHAR buffer[1024];
    UNICODE_STRING destination;

    void SetUp() override {
        // Initialize buffer with zeros
        RtlZeroMemory(buffer, sizeof(buffer));
        
        // Set up a UNICODE_STRING with initial content "Hello"
        wcscpy_s(buffer, 1024, L"Hello");
        destination.Buffer = buffer;
        destination.Length = 10; // 5 characters * 2 bytes
        destination.MaximumLength = 512; // 256 characters * 2 bytes
    }
};

// Test appending a normal string
TEST_F(RtlAppendUnicodeStringTest, AppendNormalString) {
    NTSTATUS status = RtlAppendUnicodeToString(&destination, L" World");
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 22); // 11 characters * 2 bytes
    EXPECT_STREQ(destination.Buffer, L"Hello World");
}

// Test appending an empty string
TEST_F(RtlAppendUnicodeStringTest, AppendEmptyString) {
    NTSTATUS status = RtlAppendUnicodeToString(&destination, L"");
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 10); // Should remain unchanged
    EXPECT_STREQ(destination.Buffer, L"Hello");
}

// Test appending NULL
TEST_F(RtlAppendUnicodeStringTest, AppendNull) {
    NTSTATUS status = RtlAppendUnicodeToString(&destination, NULL);
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 10); // Should remain unchanged
    EXPECT_STREQ(destination.Buffer, L"Hello");
}

// Test buffer too small
TEST_F(RtlAppendUnicodeStringTest, BufferTooSmall) {
    // Set smaller max length to test overflow
    destination.MaximumLength = 12; // Only room for 6 characters
    
    NTSTATUS status = RtlAppendUnicodeToString(&destination, L" World");
    
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
    EXPECT_EQ(destination.Length, 10); // Should remain unchanged
    EXPECT_STREQ(destination.Buffer, L"Hello");
}

// Test multiple append operations
TEST_F(RtlAppendUnicodeStringTest, MultipleAppends) {
    NTSTATUS status;
    
    status = RtlAppendUnicodeToString(&destination, L", ");
    EXPECT_EQ(status, STATUS_SUCCESS);
    
    status = RtlAppendUnicodeToString(&destination, L"Windows ");
    EXPECT_EQ(status, STATUS_SUCCESS);
      status = RtlAppendUnicodeToString(&destination, L"Kernel!");
    EXPECT_EQ(status, STATUS_SUCCESS);
    
    EXPECT_EQ(destination.Length, 44); // 22 characters * 2 bytes
    EXPECT_STREQ(destination.Buffer, L"Hello, Windows Kernel!");
}
