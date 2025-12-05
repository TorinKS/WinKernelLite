#include <gtest/gtest.h>
#include "../include/UnicodeString.h"
#include "../include/Wdm.h"

class RtlAppendUnicodeStringStringTest : public ::testing::Test {
protected:
    WCHAR destBuffer[1024];
    WCHAR sourceBuffer[1024];
    UNICODE_STRING destination;
    UNICODE_STRING source;

    void SetUp() override {
        // Initialize buffers with zeros
        RtlZeroMemory(destBuffer, sizeof(destBuffer));
        RtlZeroMemory(sourceBuffer, sizeof(sourceBuffer));
        
        // Set up a destination UNICODE_STRING with initial content "Hello"
        wcscpy_s(destBuffer, 1024, L"Hello");
        destination.Buffer = destBuffer;
        destination.Length = 10; // 5 characters * 2 bytes
        destination.MaximumLength = 512; // 256 characters * 2 bytes
        
        // Set up a source UNICODE_STRING with content " World"
        wcscpy_s(sourceBuffer, 256, L" World");
        source.Buffer = sourceBuffer;
        source.Length = 12; // 6 characters * 2 bytes
        source.MaximumLength = 512; // 256 characters * 2 bytes
    }
};

// Test appending a normal string
TEST_F(RtlAppendUnicodeStringStringTest, AppendNormalString) {
    NTSTATUS status = RtlAppendUnicodeStringToString(&destination, &source);
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 22); // 11 characters * 2 bytes
    EXPECT_STREQ(destination.Buffer, L"Hello World");
}

// Test appending an empty string
TEST_F(RtlAppendUnicodeStringStringTest, AppendEmptyString) {
    // Create an empty source string
    UNICODE_STRING emptySource;
    emptySource.Buffer = sourceBuffer;
    emptySource.Length = 0;
    emptySource.MaximumLength = 512;
    
    NTSTATUS status = RtlAppendUnicodeStringToString(&destination, &emptySource);
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_EQ(destination.Length, 10); // Should remain unchanged
    EXPECT_STREQ(destination.Buffer, L"Hello");
}

// Test buffer too small
TEST_F(RtlAppendUnicodeStringStringTest, BufferTooSmall) {
    // Set smaller max length to test overflow
    destination.MaximumLength = 12; // Only room for 6 characters
    
    NTSTATUS status = RtlAppendUnicodeStringToString(&destination, &source);
    
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
    EXPECT_EQ(destination.Length, 10); // Should remain unchanged
    EXPECT_STREQ(destination.Buffer, L"Hello");
}

// Test multiple append operations
TEST_F(RtlAppendUnicodeStringStringTest, MultipleAppends) {
    NTSTATUS status;
    
    // Create a comma + space string
    WCHAR commaBuffer[4] = L", ";
    UNICODE_STRING comma;
    comma.Buffer = commaBuffer;
    comma.Length = 4; // 2 characters * 2 bytes
    comma.MaximumLength = 8;
    
    // Create a "Windows" string
    WCHAR windowsBuffer[16] = L"Windows ";
    UNICODE_STRING windows;
    windows.Buffer = windowsBuffer;
    windows.Length = 16; // 8 characters * 2 bytes
    windows.MaximumLength = 32;
    
    // Create a "Kernel!" string
    WCHAR kernelBuffer[16] = L"Kernel!";
    UNICODE_STRING kernel;
    kernel.Buffer = kernelBuffer;
    kernel.Length = 14; // 7 characters * 2 bytes
    kernel.MaximumLength = 32;
    
    // Append the strings one by one
    status = RtlAppendUnicodeStringToString(&destination, &comma);
    EXPECT_EQ(status, STATUS_SUCCESS);
    
    status = RtlAppendUnicodeStringToString(&destination, &windows);
    EXPECT_EQ(status, STATUS_SUCCESS);
    
    status = RtlAppendUnicodeStringToString(&destination, &kernel);
    EXPECT_EQ(status, STATUS_SUCCESS);
    
    EXPECT_EQ(destination.Length, 44); // 22 characters * 2 bytes
    EXPECT_STREQ(destination.Buffer, L"Hello, Windows Kernel!");
}

// Test NULL parameters
TEST_F(RtlAppendUnicodeStringStringTest, NullParameters) {
    NTSTATUS status;
    
    // Test NULL destination
    status = RtlAppendUnicodeStringToString(NULL, &source);
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
    
    // Test NULL source
    status = RtlAppendUnicodeStringToString(&destination, NULL);
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}
