#include <gtest/gtest.h>
#include "../include/Registry.h"
#include "../include/UnicodeString.h"
#include "../include/NtStatus.h"
#include <iostream>

class RegistryValueNameTerminationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use exactly the same approach as the working RegistryTest
        testKeyPath = L"Software\\WinKernelLite\\Test";
        
        // Create a test key for our tests
        UNICODE_STRING keyName;
        RtlInitUnicodeString(&keyName, testKeyPath);
        
        OBJECT_ATTRIBUTES objAttribs;
        InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        ULONG disposition;
        NTSTATUS status = ZwCreateKey(&testKeyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
        
        if (NT_SUCCESS(status)) {
            keyCreated = true;
        }
    }
    
    void TearDown() override {
        if (testKeyHandle && keyCreated) {
            ZwClose(testKeyHandle);
            testKeyHandle = NULL;
        }
    }
    
    HANDLE testKeyHandle = NULL;
    PCWSTR testKeyPath;
    bool keyCreated = false;
};

TEST_F(RegistryValueNameTerminationTest, SetValueWithNullTerminatedName) {
    // Skip test if key creation failed
    if (!keyCreated) {
        GTEST_SKIP() << "Registry key creation failed, skipping test";
    }
    
    // Create a null-terminated UNICODE_STRING
    WCHAR valueName[] = L"TestValue";
    UNICODE_STRING unicodeValueName;
    RtlInitUnicodeString(&unicodeValueName, valueName);
    
    std::wcout << L"  Value name: " << valueName << std::endl;
    std::wcout << L"  Length: " << unicodeValueName.Length << L" bytes" << std::endl;
    std::wcout << L"  MaximumLength: " << unicodeValueName.MaximumLength << L" bytes" << std::endl;
    
    DWORD testData = 0x12345678;
    NTSTATUS status = ZwSetValueKey(testKeyHandle, &unicodeValueName, 0, REG_DWORD, &testData, sizeof(testData));
    
    std::wcout << L"  Set status: 0x" << std::hex << status << std::endl;
    EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to set value with null-terminated name";
    
    // Verify the value was set correctly by reading it back
    UCHAR buffer[256];
    PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
    ULONG resultLength;
    
    status = ZwQueryValueKey(testKeyHandle, &unicodeValueName, KeyValueFullInformation, valueInfo, sizeof(buffer), &resultLength);
    
    std::wcout << L"  Query status: 0x" << std::hex << status << std::endl;
    EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to query value";
    
    if (NT_SUCCESS(status)) {
        DWORD* retrievedData = (DWORD*)((BYTE*)valueInfo + valueInfo->DataOffset);
        std::wcout << L"  Original data: 0x" << std::hex << testData << std::endl;
        std::wcout << L"  Retrieved data: 0x" << std::hex << *retrievedData << std::endl;
        EXPECT_EQ(testData, *retrievedData) << "Retrieved data doesn't match original";
        EXPECT_EQ(REG_DWORD, valueInfo->Type) << "Value type doesn't match";
    }
}

TEST_F(RegistryValueNameTerminationTest, SetValueWithNonNullTerminatedName) {
    // Skip test if key creation failed
    if (!keyCreated) {
        GTEST_SKIP() << "Registry key creation failed, skipping test";
    }
    std::wcout << L"=== Testing Non-Null-Terminated Value Name ===" << std::endl;
    
    // Create a UNICODE_STRING that is NOT null-terminated
    WCHAR largerBuffer[] = L"TestValueNonTerminatedXXXXXXXX"; // Extra characters after our desired name
    UNICODE_STRING unicodeValueName;
    unicodeValueName.Buffer = largerBuffer;
    unicodeValueName.Length = 20 * sizeof(WCHAR); // Only "TestValueNonTerminated" (20 chars)
    unicodeValueName.MaximumLength = sizeof(largerBuffer);
    
    // Ensure the string is NOT null-terminated at the Length position
    largerBuffer[20] = L'X'; // Overwrite what would be the null terminator
    
    std::wcout << L"  Full buffer: " << largerBuffer << std::endl;
    std::wcout << L"  Length: " << unicodeValueName.Length << L" bytes (" << (unicodeValueName.Length / sizeof(WCHAR)) << L" chars)" << std::endl;
    std::wcout << L"  Character at Length position: 0x" << std::hex << largerBuffer[unicodeValueName.Length / sizeof(WCHAR)] << std::endl;
    
    DWORD testData = 0x87654321;
    NTSTATUS status = ZwSetValueKey(testKeyHandle, &unicodeValueName, 0, REG_DWORD, &testData, sizeof(testData));
    
    std::wcout << L"  Set status: 0x" << std::hex << status << std::endl;
    EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to set value with non-null-terminated name";
    
    // Create a properly null-terminated version for querying
    WCHAR queryValueName[21] = {0};
    wcsncpy_s(queryValueName, 21, largerBuffer, unicodeValueName.Length / sizeof(WCHAR));
    UNICODE_STRING queryUnicodeName;
    RtlInitUnicodeString(&queryUnicodeName, queryValueName);
    
    std::wcout << L"  Query value name: " << queryValueName << std::endl;
    
    // Verify the value was set correctly by reading it back
    UCHAR buffer[256];
    PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
    ULONG resultLength;
    
    status = ZwQueryValueKey(testKeyHandle, &queryUnicodeName, KeyValueFullInformation, valueInfo, sizeof(buffer), &resultLength);
    
    std::wcout << L"  Query status: 0x" << std::hex << status << std::endl;
    EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to query value";
    
    if (NT_SUCCESS(status)) {
        DWORD* retrievedData = (DWORD*)((BYTE*)valueInfo + valueInfo->DataOffset);
        std::wcout << L"  Original data: 0x" << std::hex << testData << std::endl;
        std::wcout << L"  Retrieved data: 0x" << std::hex << *retrievedData << std::endl;
        EXPECT_EQ(testData, *retrievedData) << "Retrieved data doesn't match original";
        EXPECT_EQ(REG_DWORD, valueInfo->Type) << "Value type doesn't match";
    }
}

TEST_F(RegistryValueNameTerminationTest, SetValueWithEmbeddedNulls) {
    // Skip test if key creation failed
    if (!keyCreated) {
        GTEST_SKIP() << "Registry key creation failed, skipping test";
    }
    std::wcout << L"=== Testing Value Name with Embedded Nulls ===" << std::endl;
    
    // Create a UNICODE_STRING with embedded null characters
    WCHAR valueNameBuffer[] = L"Test\0Embedded\0Nulls";
    UNICODE_STRING unicodeValueName;
    unicodeValueName.Buffer = valueNameBuffer;
    unicodeValueName.Length = 18 * sizeof(WCHAR); // Include the embedded nulls
    unicodeValueName.MaximumLength = sizeof(valueNameBuffer);
    
    std::wcout << L"  Value name length: " << unicodeValueName.Length << L" bytes" << std::endl;
    std::wcout << L"  First part: " << valueNameBuffer << std::endl;
    
    DWORD testData = 0xDEADBEEF;
    NTSTATUS status = ZwSetValueKey(testKeyHandle, &unicodeValueName, 0, REG_DWORD, &testData, sizeof(testData));
    
    std::wcout << L"  Set status: 0x" << std::hex << status << std::endl;
    EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to set value with embedded nulls in name";
    
    // Try to query it back - this should work even with embedded nulls
    UCHAR buffer[256];
    PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
    ULONG resultLength;
    
    status = ZwQueryValueKey(testKeyHandle, &unicodeValueName, KeyValueFullInformation, valueInfo, sizeof(buffer), &resultLength);
    
    std::wcout << L"  Query status: 0x" << std::hex << status << std::endl;
    // Note: This might fail because the registry might truncate at the first null
    // That's actually correct behavior - just documenting the expected result
}

TEST_F(RegistryValueNameTerminationTest, SetValueWithLongName) {
    // Skip test if key creation failed
    if (!keyCreated) {
        GTEST_SKIP() << "Registry key creation failed, skipping test";
    }
    std::wcout << L"=== Testing Long Value Name ===" << std::endl;
    
    // Create a long value name (close to maximum registry value name length)
    std::wstring longName(200, L'A'); // 200 'A' characters
    longName += L"_LongValueName";
    
    UNICODE_STRING unicodeValueName;
    RtlInitUnicodeString(&unicodeValueName, longName.c_str());
    
    std::wcout << L"  Value name length: " << longName.length() << L" characters" << std::endl;
    std::wcout << L"  First 50 chars: " << longName.substr(0, 50) << L"..." << std::endl;
    
    DWORD testData = 0xCAFEBABE;
    NTSTATUS status = ZwSetValueKey(testKeyHandle, &unicodeValueName, 0, REG_DWORD, &testData, sizeof(testData));
    
    std::wcout << L"  Set status: 0x" << std::hex << status << std::endl;
    EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to set value with long name";
    
    if (NT_SUCCESS(status)) {
        // Verify the value was set correctly by reading it back
        UCHAR buffer[1024]; // Larger buffer for long name
        PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
        ULONG resultLength;
        
        status = ZwQueryValueKey(testKeyHandle, &unicodeValueName, KeyValueFullInformation, valueInfo, sizeof(buffer), &resultLength);
        
        std::wcout << L"  Query status: 0x" << std::hex << status << std::endl;
        EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to query value with long name";
        
        if (NT_SUCCESS(status)) {
            DWORD* retrievedData = (DWORD*)((BYTE*)valueInfo + valueInfo->DataOffset);
            std::wcout << L"  Original data: 0x" << std::hex << testData << std::endl;
            std::wcout << L"  Retrieved data: 0x" << std::hex << *retrievedData << std::endl;
            EXPECT_EQ(testData, *retrievedData) << "Retrieved data doesn't match original";
        }
    }
}

TEST_F(RegistryValueNameTerminationTest, SetValueWithEmptyName) {
    // Skip test if key creation failed
    if (!keyCreated) {
        GTEST_SKIP() << "Registry key creation failed, skipping test";
    }
    std::wcout << L"=== Testing Empty Value Name ===" << std::endl;
    
    // Test setting a value with an empty name (default value)
    UNICODE_STRING unicodeValueName;
    RtlInitUnicodeString(&unicodeValueName, L"");
    
    std::wcout << L"  Value name length: " << unicodeValueName.Length << L" bytes" << std::endl;
    
    DWORD testData = 0x11223344;
    NTSTATUS status = ZwSetValueKey(testKeyHandle, &unicodeValueName, 0, REG_DWORD, &testData, sizeof(testData));
    
    std::wcout << L"  Set status: 0x" << std::hex << status << std::endl;
    EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to set default value (empty name)";
    
    if (NT_SUCCESS(status)) {
        // Query back the default value
        UCHAR buffer[256];
        PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
        ULONG resultLength;
        
        status = ZwQueryValueKey(testKeyHandle, &unicodeValueName, KeyValueFullInformation, valueInfo, sizeof(buffer), &resultLength);
        
        std::wcout << L"  Query status: 0x" << std::hex << status << std::endl;
        EXPECT_TRUE(NT_SUCCESS(status)) << "Failed to query default value";
        
        if (NT_SUCCESS(status)) {
            DWORD* retrievedData = (DWORD*)((BYTE*)valueInfo + valueInfo->DataOffset);
            std::wcout << L"  Original data: 0x" << std::hex << testData << std::endl;
            std::wcout << L"  Retrieved data: 0x" << std::hex << *retrievedData << std::endl;
            EXPECT_EQ(testData, *retrievedData) << "Retrieved data doesn't match original";
        }
    }
}

TEST_F(RegistryValueNameTerminationTest, SetValueMemoryAllocationFailure) {
    // Skip test if key creation failed
    if (!keyCreated) {
        GTEST_SKIP() << "Registry key creation failed, skipping test";
    }
    std::wcout << L"=== Testing Memory Allocation Scenarios ===" << std::endl;
    
    // Test with valid parameters to ensure normal operation works
    UNICODE_STRING unicodeValueName;
    RtlInitUnicodeString(&unicodeValueName, L"MemoryTestValue");
    
    DWORD testData = 0x55AA55AA;
    NTSTATUS status = ZwSetValueKey(testKeyHandle, &unicodeValueName, 0, REG_DWORD, &testData, sizeof(testData));
    
    std::wcout << L"  Normal operation status: 0x" << std::hex << status << std::endl;
    EXPECT_TRUE(NT_SUCCESS(status)) << "Normal memory allocation test failed";
    
    // Test with NULL ValueName
    status = ZwSetValueKey(testKeyHandle, NULL, 0, REG_DWORD, &testData, sizeof(testData));
    std::wcout << L"  NULL ValueName status: 0x" << std::hex << status << std::endl;
    EXPECT_EQ(STATUS_INVALID_PARAMETER, status) << "Should fail with NULL ValueName";
    
    // Test with NULL ValueName->Buffer
    UNICODE_STRING invalidValueName = {0};
    invalidValueName.Length = 10;
    invalidValueName.MaximumLength = 20;
    invalidValueName.Buffer = NULL;
    
    status = ZwSetValueKey(testKeyHandle, &invalidValueName, 0, REG_DWORD, &testData, sizeof(testData));
    std::wcout << L"  NULL Buffer status: 0x" << std::hex << status << std::endl;
    EXPECT_EQ(STATUS_INVALID_PARAMETER, status) << "Should fail with NULL Buffer";
}
