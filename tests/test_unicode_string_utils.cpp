#include <gtest/gtest.h>
#include "../include/UnicodeStringUtils.h"

TEST(UnicodeStringUtilsTest, DumpUnicodeString_Basic) {
    UNICODE_STRING str;
    const wchar_t* text = L"Hello World";
    
    ASSERT_EQ(STATUS_SUCCESS, RtlInitUnicodeString(&str, text));
    
    testing::internal::CaptureStdout();
    DumpUnicodeString(&str, "Test String");
    std::string output = testing::internal::GetCapturedStdout();
    
    // Verify output contains key information
    EXPECT_NE(output.find("Length: 22"), std::string::npos);  // 11 chars * 2 bytes
    EXPECT_NE(output.find("MaximumLength: 24"), std::string::npos);  // includes null terminator
    EXPECT_NE(output.find("Hello World"), std::string::npos);
}

TEST(UnicodeStringUtilsTest, DumpUnicodeString_Empty) {
    UNICODE_STRING str;
    ASSERT_EQ(STATUS_SUCCESS, RtlInitUnicodeString(&str, L""));
      testing::internal::CaptureStdout();
    DumpUnicodeString(&str, NULL);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_NE(output.find("Length: 0"), std::string::npos);
    EXPECT_NE(output.find("MaximumLength: 2"), std::string::npos);  // space for null terminator
}

TEST(UnicodeStringUtilsTest, DumpUnicodeString_Null) {    testing::internal::CaptureStdout();
    DumpUnicodeString(nullptr, NULL);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_NE(output.find("NULL UNICODE_STRING pointer"), std::string::npos);
}
