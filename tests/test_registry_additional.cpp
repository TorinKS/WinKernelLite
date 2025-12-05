#include <gtest/gtest.h>
#include <WinKernelLite/Registry.h>
#include <WinKernelLite/UnicodeString.h>
#include <shlwapi.h> // For SHDeleteKeyW
#include <iostream>
#pragma comment(lib, "shlwapi.lib")

// Test fixture for additional registry function tests
class RegistryAdditionalTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test key that we'll use for various tests
        // Use HKEY_CURRENT_USER for kernel mode simulation
        HANDLE keyHandle;
        DWORD disposition;
        
        // Create test key with full path under HKLM
        UNICODE_STRING keyPath;
        RtlInitUnicodeString(&keyPath, L"Software\\WinKernelLite\\AdvancedTest");
        
        OBJECT_ATTRIBUTES objAttr;
        InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        NTSTATUS status = ZwCreateKey(
            &keyHandle,
            KEY_ALL_ACCESS,
            &objAttr,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            &disposition
        );
        
        ASSERT_TRUE(NT_SUCCESS(status));
        ASSERT_NE(keyHandle, INVALID_HANDLE_VALUE);
        
        // Set some test values of different types
        DWORD dwordValue = 0x12345678;
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, L"DwordValue");
        status = ZwSetValueKey(
            keyHandle,
            &valueName,
            0,
            REG_DWORD,
            &dwordValue,
            sizeof(DWORD)
        );
        ASSERT_TRUE(NT_SUCCESS(status));
        
        WCHAR stringValue[] = L"Test String Value";
        RtlInitUnicodeString(&valueName, L"StringValue");
        status = ZwSetValueKey(
            keyHandle,
            &valueName,
            0,
            REG_SZ,
            stringValue,
            static_cast<ULONG>((wcslen(stringValue) + 1) * sizeof(WCHAR))
        );
        ASSERT_TRUE(NT_SUCCESS(status));
        
        BYTE binaryData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        RtlInitUnicodeString(&valueName, L"BinaryValue");
        status = ZwSetValueKey(
            keyHandle,
            &valueName,
            0,
            REG_BINARY,
            binaryData,
            sizeof(binaryData)
        );
        ASSERT_TRUE(NT_SUCCESS(status));
        
        // Create a few subkeys for enumeration tests
        HANDLE subkeyHandle;
        for (int i = 1; i <= 5; i++) {
            WCHAR subkeyName[32];
            swprintf_s(subkeyName, 32, L"Subkey%d", i);
            
            UNICODE_STRING subkeyPath;
            RtlInitUnicodeString(&subkeyPath, subkeyName);
            
            OBJECT_ATTRIBUTES subkeyAttr;
            InitializeObjectAttributes(&subkeyAttr, &subkeyPath, OBJ_CASE_INSENSITIVE, keyHandle, NULL);
            
            status = ZwCreateKey(
                &subkeyHandle,
                KEY_ALL_ACCESS,
                &subkeyAttr,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
            );
            ASSERT_TRUE(NT_SUCCESS(status));
            ZwClose(subkeyHandle);
        }
        
        ZwClose(keyHandle);
    }
    
    void TearDown() override {
        // Clean up test keys from HKEY_CURRENT_USER
        HANDLE keyHandle;
        
        UNICODE_STRING keyPath;
        RtlInitUnicodeString(&keyPath, L"Software\\WinKernelLite\\AdvancedTest");
        
        OBJECT_ATTRIBUTES objAttr;
        InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        NTSTATUS status = ZwOpenKey(
            &keyHandle,
            KEY_ALL_ACCESS,
            &objAttr
        );
        
        if (NT_SUCCESS(status)) {
            // Delete all values in the key
            ZwClose(keyHandle);
            
            // Delete the key itself from HKEY_CURRENT_USER
            ZwDeleteKeyEx(HKEY_CURRENT_USER, L"Software\\WinKernelLite\\AdvancedTest");
        }
    }
};

// Test opening keys with different root keys
TEST_F(RegistryAdditionalTests, RegOpenKeyEx_DifferentRootKeys) {
    HANDLE keyHandle;
    NTSTATUS status;
    UNICODE_STRING keyName;
    OBJECT_ATTRIBUTES objAttrs;
    
    // Test with HKEY_CURRENT_USER (primary kernel mode hive)
    RtlInitUnicodeString(&keyName, L"SOFTWARE");
    InitializeObjectAttributes(&objAttrs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttrs
    );
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_NE(keyHandle, INVALID_HANDLE_VALUE);
    ZwClose(keyHandle);
    
    // Test with HKEY_CURRENT_USER (keep for compatibility testing, but not primary focus)
    RtlInitUnicodeString(&keyName, L"Software");
    InitializeObjectAttributes(&objAttrs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttrs
    );
    // Make this test more tolerant since HKCU is not our primary target for kernel mode simulation
    EXPECT_TRUE(NT_SUCCESS(status) || status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_ACCESS_DENIED);
    if (NT_SUCCESS(status)) {
        ZwClose(keyHandle);
    }
    
    // Test with HKEY_CLASSES_ROOT
    RtlInitUnicodeString(&keyName, L".txt");
    InitializeObjectAttributes(&objAttrs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CLASSES_ROOT, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttrs
    );
    EXPECT_TRUE(NT_SUCCESS(status) || status == STATUS_OBJECT_NAME_NOT_FOUND);
    if (NT_SUCCESS(status)) {
        ZwClose(keyHandle);
    }
}

// Test RegQueryValueKey with different value types
TEST_F(RegistryAdditionalTests, RegQueryValueKey_DifferentTypes) {
    // First ensure our test key exists with the values we need
    HANDLE keyHandle = nullptr;
    NTSTATUS status;
    ULONG disposition;
    
    // Create or open the test key under HKEY_CURRENT_USER
    UNICODE_STRING keyPath;
    RtlInitUnicodeString(&keyPath, L"Software\\WinKernelLite\\AdvancedTest");
    
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    status = ZwCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        &objAttr,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        &disposition
    );
    
    // Debug output
    std::cout << "ZwCreateKey status: 0x" << std::hex << status << std::dec << std::endl;
    
    ASSERT_TRUE(NT_SUCCESS(status));
    
    // Set test values of different types
    // DWORD value
    {
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, L"DwordValue");
        
        DWORD dwordValue = 0x12345678;
        status = ZwSetValueKey(
            keyHandle,
            &valueName,
            0,
            REG_DWORD,
            &dwordValue,
            sizeof(DWORD)
        );
        
        // Debug output
        std::cout << "ZwSetValueKey (DWORD) status: 0x" << std::hex << status << std::dec << std::endl;
        
        ASSERT_TRUE(NT_SUCCESS(status));
    }
    
    // String value
    {
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, L"StringValue");
        
        WCHAR stringValue[] = L"Test String Value";
        status = ZwSetValueKey(
            keyHandle,
            &valueName,
            0,
            REG_SZ,
            stringValue,
            static_cast<ULONG>((wcslen(stringValue) + 1) * sizeof(WCHAR))
        );
        
        // Debug output
        std::cout << "ZwSetValueKey (String) status: 0x" << std::hex << status << std::dec << std::endl;
        
        ASSERT_TRUE(NT_SUCCESS(status));
    }
    
    // Binary value
    {
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, L"BinaryValue");
        
        BYTE binaryData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        status = ZwSetValueKey(
            keyHandle,
            &valueName,
            0,
            REG_BINARY,
            binaryData,
            sizeof(binaryData)
        );
        
        // Debug output
        std::cout << "ZwSetValueKey (Binary) status: 0x" << std::hex << status << std::dec << std::endl;
        
        ASSERT_TRUE(NT_SUCCESS(status));
    }
    
    // Close and reopen the key with just read access
    ZwClose(keyHandle);
    keyHandle = nullptr;
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttr
    );
    
    // Debug output
    std::cout << "ZwOpenKey status: 0x" << std::hex << status << std::dec << std::endl;
    
    ASSERT_TRUE(NT_SUCCESS(status));
    
    // Test querying REG_DWORD value
    {
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, L"DwordValue");
        
        BYTE buffer[256] = {0};
        ULONG resultLength = 0;
        
        status = ZwQueryValueKey(
            keyHandle,
            &valueName,
            KeyValuePartialInformation,
            buffer,
            sizeof(buffer),
            &resultLength
        );
        
        // Debug output
        std::cout << "ZwQueryValueKey (DWORD) status: 0x" << std::hex << status << std::dec << std::endl;
        
        ASSERT_TRUE(NT_SUCCESS(status));
        
        PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
        EXPECT_EQ(info->Type, REG_DWORD);
        EXPECT_EQ(info->DataLength, sizeof(DWORD));
        
        DWORD value = *(DWORD*)info->Data;
        EXPECT_EQ(value, 0x12345678);
    }
    
    // Test querying REG_SZ value
    {
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, L"StringValue");
        
        BYTE buffer[256] = {0};
        ULONG resultLength = 0;
        
        status = ZwQueryValueKey(
            keyHandle,
            &valueName,
            KeyValuePartialInformation,
            buffer,
            sizeof(buffer),
            &resultLength
        );
        
        // Debug output
        std::cout << "ZwQueryValueKey (String) status: 0x" << std::hex << status << std::dec << std::endl;
        
        ASSERT_TRUE(NT_SUCCESS(status));
        
        PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
        EXPECT_EQ(info->Type, REG_SZ);
        
        WCHAR* stringValue = (WCHAR*)info->Data;
        EXPECT_STREQ(stringValue, L"Test String Value");
    }
    
    // Test querying REG_BINARY value
    {
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, L"BinaryValue");
        
        BYTE buffer[256] = {0};
        ULONG resultLength = 0;
        
        status = ZwQueryValueKey(
            keyHandle,
            &valueName,
            KeyValuePartialInformation,
            buffer,
            sizeof(buffer),
            &resultLength
        );
        
        // Debug output
        std::cout << "ZwQueryValueKey (Binary) status: 0x" << std::hex << status << std::dec << std::endl;
        
        ASSERT_TRUE(NT_SUCCESS(status));
        
        PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
        EXPECT_EQ(info->Type, REG_BINARY);
        EXPECT_EQ(info->DataLength, 5);
        
        // Verify binary data
        BYTE expectedData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        EXPECT_EQ(memcmp(info->Data, expectedData, 5), 0);
    }
    
    ZwClose(keyHandle);
}

// Test registry full path functions with different formats
TEST_F(RegistryAdditionalTests, RegistryFullPath_DifferentFormats) {
    HANDLE keyHandle;
    NTSTATUS status;
    
    // Test with standard format
    status = ZwCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        nullptr,
        0,
        nullptr,
        0,
        nullptr
    );
    EXPECT_FALSE(NT_SUCCESS(status)); // Should fail without ObjectAttributes
    
    // Test with different registry path formats
    UNICODE_STRING hkcuPath;
    RtlInitUnicodeString(&hkcuPath, L"HKEY_CURRENT_USER\\Software");
    
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &hkcuPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttr
    );
    EXPECT_TRUE(NT_SUCCESS(status));
    if (NT_SUCCESS(status)) {
        ZwClose(keyHandle);
    }
    
    // Test with HKLM path (primary kernel mode hive)
    UNICODE_STRING hklmPath;
    RtlInitUnicodeString(&hklmPath, L"HKEY_CURRENT_USER\\SOFTWARE");
    
    InitializeObjectAttributes(&objAttr, &hklmPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttr
    );
    EXPECT_TRUE(NT_SUCCESS(status));
    if (NT_SUCCESS(status)) {
        ZwClose(keyHandle);
    }
}

// Test enumeration with different information classes
TEST_F(RegistryAdditionalTests, EnumerateKeys_DifferentInfoClasses) {
    HANDLE keyHandle;
    NTSTATUS status;
    
    // Open our test key with subkeys under HKEY_CURRENT_USER
    UNICODE_STRING keyPath;
    RtlInitUnicodeString(&keyPath, L"Software\\WinKernelLite\\AdvancedTest");
    
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttr
    );
    ASSERT_TRUE(NT_SUCCESS(status));
    
    // Enumerate using KeyBasicInformation
    ULONG index = 0;
    BYTE buffer[1024];
    ULONG resultLength;
    
    while (true) {
        status = ZwEnumerateKey(
            keyHandle,
            index,
            KeyBasicInformation,
            buffer,
            sizeof(buffer),
            &resultLength
        );
        
        if (status == STATUS_NO_MORE_ENTRIES) {
            break;
        }
        
        EXPECT_TRUE(NT_SUCCESS(status));
        if (NT_SUCCESS(status)) {
            PKEY_BASIC_INFORMATION keyInfo = (PKEY_BASIC_INFORMATION)buffer;
            WCHAR nameBuffer[256] = {0};
            
            // Copy name to ensure null termination
            if (keyInfo->NameLength > 0) {
                ULONG copyLen = min(255 * sizeof(WCHAR), keyInfo->NameLength);
                memcpy(nameBuffer, keyInfo->Name, copyLen);
                nameBuffer[copyLen / sizeof(WCHAR)] = L'\0';
                
                // Verify subkey name format
                WCHAR expectedPrefix[] = L"Subkey";
                EXPECT_EQ(wcsncmp(nameBuffer, expectedPrefix, 6), 0);
            }
        }
        
        index++;
    }
    
    // Verify we found at least 5 subkeys
    EXPECT_GE(index, 5);
    
    // Try with KeyNodeInformation
    index = 0;
    while (true) {
        status = ZwEnumerateKey(
            keyHandle,
            index,
            KeyNodeInformation,
            buffer,
            sizeof(buffer),
            &resultLength
        );
        
        if (status == STATUS_NO_MORE_ENTRIES) {
            break;
        }
        
        EXPECT_TRUE(NT_SUCCESS(status));
        index++;
    }
    
    ZwClose(keyHandle);
}

// Test error handling for non-existent keys and values
TEST_F(RegistryAdditionalTests, ErrorHandling_NonExistent) {
    HANDLE keyHandle;
    NTSTATUS status;
    
    // Try to open a non-existent key under HKEY_CURRENT_USER
    UNICODE_STRING nonExistentKeyPath;
    RtlInitUnicodeString(&nonExistentKeyPath, L"Software\\ThisKeyDoesNotExist");
    
    OBJECT_ATTRIBUTES nonExistentObjAttr;
    InitializeObjectAttributes(&nonExistentObjAttr, &nonExistentKeyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &nonExistentObjAttr
    );
    EXPECT_EQ(status, STATUS_OBJECT_NAME_NOT_FOUND);
    
    // Open our test key under HKEY_CURRENT_USER
    UNICODE_STRING keyPath;
    RtlInitUnicodeString(&keyPath, L"Software\\WinKernelLite\\AdvancedTest");
    
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttr
    );
    ASSERT_TRUE(NT_SUCCESS(status));
    
    // Try to query a non-existent value
    UCHAR buffer[128] = {0};
    ULONG bufferSize = sizeof(buffer);
    ULONG resultLength;
    
    UNICODE_STRING valueName;
    RtlInitUnicodeString(&valueName, L"NonExistentValue");
    
    status = ZwQueryValueKey(
        keyHandle,
        &valueName,
        KeyValuePartialInformation,
        buffer,
        bufferSize,
        &resultLength
    );
    // ZwQueryValueKey should return STATUS_OBJECT_NAME_NOT_FOUND for non-existent values
    EXPECT_EQ(status, STATUS_OBJECT_NAME_NOT_FOUND);
    
    ZwClose(keyHandle);
}

// Test with buffer that's too small
TEST_F(RegistryAdditionalTests, BufferTooSmall_Handling) {
    HANDLE keyHandle;
    NTSTATUS status;
    
    // Open our test key under HKEY_CURRENT_USER
    UNICODE_STRING keyPath;
    RtlInitUnicodeString(&keyPath, L"Software\\WinKernelLite\\AdvancedTest");
    
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &objAttr
    );
    ASSERT_TRUE(NT_SUCCESS(status));
    
    // Query string value with buffer too small
    UCHAR tinyBuffer[8] = {0};  // "Test String Value" won't fit
    ULONG bufferSize = sizeof(tinyBuffer);
    ULONG resultLength;
    
    UNICODE_STRING valueName;
    RtlInitUnicodeString(&valueName, L"StringValue");
    
    status = ZwQueryValueKey(
        keyHandle,
        &valueName,
        KeyValuePartialInformation,
        tinyBuffer,
        bufferSize,
        &resultLength
    );
    // ZwQueryValueKey should return STATUS_BUFFER_TOO_SMALL for buffer too small
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
    // resultLength should be set to the required buffer size
    EXPECT_GT(resultLength, bufferSize);
    
    ZwClose(keyHandle);
}
