#include <gtest/gtest.h>
#include "../include/UnicodeString.h"

// Simple test for RtlUnicodeStringToInteger without dependency on heap allocation
TEST(SimpleUnicodeTest, RtlUnicodeStringToInteger_Basic) {
    UNICODE_STRING str;
    ULONG value;
    
    // Test basic decimal conversion
    RtlInitUnicodeString(&str, L"123");
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 10, &value), STATUS_SUCCESS);
    ASSERT_EQ(value, 123UL);
    
    // Test hexadecimal conversion with explicit base
    RtlInitUnicodeString(&str, L"FF");
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 16, &value), STATUS_SUCCESS);
    ASSERT_EQ(value, 255UL);
    
    // Test binary conversion
    RtlInitUnicodeString(&str, L"1010");
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 2, &value), STATUS_SUCCESS);
    ASSERT_EQ(value, 10UL);
}

TEST(SimpleUnicodeTest, RtlUnicodeStringToInteger_AutoDetect) {
    UNICODE_STRING str;
    ULONG value;
    
    // Test auto-detect hex with 0x prefix
    RtlInitUnicodeString(&str, L"0xFF");
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 0, &value), STATUS_SUCCESS);
    ASSERT_EQ(value, 255UL);
    
    // Test auto-detect binary with 0b prefix
    RtlInitUnicodeString(&str, L"0b1010");
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 0, &value), STATUS_SUCCESS);
    ASSERT_EQ(value, 10UL);
}

TEST(SimpleUnicodeTest, RtlUnicodeStringToInteger_Errors) {
    UNICODE_STRING str;
    ULONG value;
    
    // Test null parameters
    ASSERT_EQ(RtlUnicodeStringToInteger(nullptr, 10, &value), STATUS_INVALID_PARAMETER);
    RtlInitUnicodeString(&str, L"123");
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 10, nullptr), STATUS_INVALID_PARAMETER);
    
    // Test invalid base
    ASSERT_EQ(RtlUnicodeStringToInteger(&str, 3, &value), STATUS_INVALID_PARAMETER);
}
