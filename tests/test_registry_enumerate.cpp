#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "../include/Registry.h"
#include "../include/UnicodeString.h"
#include "../include/NtStatus.h"

class RegistryEnumerateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test key with some subkeys for enumeration testing
        // Use HKEY_CURRENT_USER to avoid requiring administrator privileges
        testKeyPath = L"SOFTWARE\\WinKernelLite\\EnumerateTest";
        testSubKey1 = L"TestSubKey1";
        testSubKey2 = L"TestSubKey2";
        testSubKey3 = L"TestSubKey3";
        
        // Create main test key
        UNICODE_STRING keyName;
        OBJECT_ATTRIBUTES objAttribs;
        ULONG disposition;
        
        RtlInitUnicodeString(&keyName, testKeyPath);
        InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        NTSTATUS status = ZwCreateKey(&mainKeyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
        ASSERT_TRUE(NT_SUCCESS(status));
        ASSERT_NE(mainKeyHandle, nullptr);
        
        // Create subkeys
        CreateSubKey(testSubKey1);
        CreateSubKey(testSubKey2);
        CreateSubKey(testSubKey3);
    }
    
    void TearDown() override {
        // Clean up test keys
        if (mainKeyHandle) {
            ZwClose(mainKeyHandle);
            mainKeyHandle = nullptr;
        }
        
        // Clean up subkey handles if any are still open
        CleanupSubKeys();
    }
    
    void CreateSubKey(const WCHAR* subKeyName) {
        HANDLE subKeyHandle = NULL;
        UNICODE_STRING subKeyPath;
        OBJECT_ATTRIBUTES objAttribs;
        ULONG disposition;
        
        // Build full subkey path
        WCHAR fullPath[2048];
        swprintf_s(fullPath, 2048, L"%s\\%s", testKeyPath, subKeyName);
        
        RtlInitUnicodeString(&subKeyPath, fullPath);
        InitializeObjectAttributes(&objAttribs, &subKeyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        NTSTATUS status = ZwCreateKey(&subKeyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
        ASSERT_TRUE(NT_SUCCESS(status));
        
        if (subKeyHandle) {
            subKeyHandles.push_back(subKeyHandle);
        }
    }
    
    void CleanupSubKeys() {
        for (HANDLE handle : subKeyHandles) {
            if (handle) {
                ZwClose(handle);
            }
        }
        subKeyHandles.clear();
    }
    
    const WCHAR* testKeyPath;
    const WCHAR* testSubKey1;
    const WCHAR* testSubKey2; 
    const WCHAR* testSubKey3;
    HANDLE mainKeyHandle = nullptr;
    std::vector<HANDLE> subKeyHandles;
};

// Test basic key enumeration with KeyBasicInformation
TEST_F(RegistryEnumerateTest, EnumerateKeysBasicInformation) {
    ULONG index = 0;
    UCHAR buffer[2048];
    PKEY_BASIC_INFORMATION keyInfo = (PKEY_BASIC_INFORMATION)buffer;
    ULONG resultLength;
    std::vector<std::wstring> foundKeys;
    
    // Enumerate all subkeys
    while (true) {
        NTSTATUS status = ZwEnumerateKey(
            mainKeyHandle,
            index,
            KeyBasicInformation,
            keyInfo,
            sizeof(buffer),
            &resultLength
        );
        
        if (status == STATUS_NO_MORE_ENTRIES) {
            break; // Normal termination
        }
        
        EXPECT_TRUE(NT_SUCCESS(status));
        if (!NT_SUCCESS(status)) {
            break;
        }
        
        // Extract key name
        WCHAR keyName[1024] = {0};
        ULONG nameChars = keyInfo->NameLength / sizeof(WCHAR);
        wcsncpy_s(keyName, 1024, keyInfo->Name, nameChars);
        foundKeys.push_back(std::wstring(keyName));
        
        index++;
    }
    
    // Verify we found at least the expected number of keys (there might be others from previous tests)
    EXPECT_GE(foundKeys.size(), 3);
    
    // Verify the key names we created are present (order may vary)
    bool foundSubKey1 = false, foundSubKey2 = false, foundSubKey3 = false;
    for (const auto& keyName : foundKeys) {
        if (keyName == testSubKey1) foundSubKey1 = true;
        else if (keyName == testSubKey2) foundSubKey2 = true;
        else if (keyName == testSubKey3) foundSubKey3 = true;
    }
    
    EXPECT_TRUE(foundSubKey1);
    EXPECT_TRUE(foundSubKey2);
    EXPECT_TRUE(foundSubKey3);
}

// Test key enumeration with KeyNodeInformation
TEST_F(RegistryEnumerateTest, EnumerateKeysNodeInformation) {
    ULONG index = 0;
    UCHAR buffer[2048];
    PKEY_NODE_INFORMATION keyInfo = (PKEY_NODE_INFORMATION)buffer;
    ULONG resultLength;
    ULONG keyCount = 0;
    
    // Enumerate all subkeys using KeyNodeInformation
    while (true) {
        NTSTATUS status = ZwEnumerateKey(
            mainKeyHandle,
            index,
            KeyNodeInformation,
            keyInfo,
            sizeof(buffer),
            &resultLength
        );
        
        if (status == STATUS_NO_MORE_ENTRIES) {
            break;
        }
        
        EXPECT_TRUE(NT_SUCCESS(status));
        if (!NT_SUCCESS(status)) {
            break;
        }
        
        // Verify structure fields
        EXPECT_GT(keyInfo->NameLength, 0);
        EXPECT_LE(keyInfo->NameLength, sizeof(buffer) - sizeof(KEY_NODE_INFORMATION));
        
        keyCount++;
        index++;
    }
    
    EXPECT_GE(keyCount, 3);
}

// Test enumeration with buffer too small
TEST_F(RegistryEnumerateTest, EnumerateKeysBufferTooSmall) {
    UCHAR smallBuffer[16]; // Intentionally small buffer
    ULONG resultLength;
    
    NTSTATUS status = ZwEnumerateKey(
        mainKeyHandle,
        0,
        KeyBasicInformation,
        smallBuffer,
        sizeof(smallBuffer),
        &resultLength
    );
    
    // Should fail with buffer too small
    EXPECT_EQ(status, STATUS_BUFFER_TOO_SMALL);
    EXPECT_GT(resultLength, sizeof(smallBuffer));
}

// Test enumeration with invalid index
TEST_F(RegistryEnumerateTest, EnumerateKeysInvalidIndex) {
    UCHAR buffer[2048];
    ULONG resultLength;
    
    // Try to enumerate with an index beyond available keys
    NTSTATUS status = ZwEnumerateKey(
        mainKeyHandle,
        999, // Way beyond our 3 subkeys
        KeyBasicInformation,
        buffer,
        sizeof(buffer),
        &resultLength
    );
    
    EXPECT_EQ(status, STATUS_NO_MORE_ENTRIES);
}

// Test enumeration with invalid handle
TEST_F(RegistryEnumerateTest, EnumerateKeysInvalidHandle) {
    UCHAR buffer[2048];
    ULONG resultLength;
    
    NTSTATUS status = ZwEnumerateKey(
        INVALID_HANDLE_VALUE, // Invalid handle
        0,
        KeyBasicInformation,
        buffer,
        sizeof(buffer),
        &resultLength
    );
    
    EXPECT_FALSE(NT_SUCCESS(status));
}

// Test enumeration on key with no subkeys
TEST_F(RegistryEnumerateTest, EnumerateEmptyKey) {
    // Create a key with no subkeys
    HANDLE emptyKeyHandle = NULL;
    UNICODE_STRING emptyKeyName;
    OBJECT_ATTRIBUTES objAttribs;
    ULONG disposition;
    
    RtlInitUnicodeString(&emptyKeyName, L"SOFTWARE\\WinKernelLite\\EnumerateTest\\WinKernelLiteEmptyTest");
    InitializeObjectAttributes(&objAttribs, &emptyKeyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    NTSTATUS createStatus = ZwCreateKey(&emptyKeyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
    ASSERT_TRUE(NT_SUCCESS(createStatus));
    
    // Try to enumerate - should immediately return no more entries
    UCHAR buffer[2048];
    ULONG resultLength;
    
    NTSTATUS status = ZwEnumerateKey(
        emptyKeyHandle,
        0,
        KeyBasicInformation,
        buffer,
        sizeof(buffer),
        &resultLength
    );
    
    EXPECT_EQ(status, STATUS_NO_MORE_ENTRIES);
    
    // Cleanup
    if (emptyKeyHandle) {
        ZwClose(emptyKeyHandle);
    }
}

// Test enumeration using full path created keys
TEST_F(RegistryEnumerateTest, EnumerateFullPathCreatedKeys) {
    // Create additional keys using full path methods
    HANDLE fullPathKeyHandle = NULL;
    UNICODE_STRING fullPathKeyName;
    OBJECT_ATTRIBUTES objAttribs;
    ULONG disposition;
    
    // Create a subkey using Windows-style full path for HKEY_CURRENT_USER
    RtlInitUnicodeString(&fullPathKeyName, L"HKEY_CURRENT_USER\\SOFTWARE\\WinKernelLite\\EnumerateTest\\FullPathSubKey");
    InitializeObjectAttributes(&objAttribs, &fullPathKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    NTSTATUS status = ZwCreateKey(&fullPathKeyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
    EXPECT_TRUE(NT_SUCCESS(status));
    
    if (fullPathKeyHandle) {
        ZwClose(fullPathKeyHandle);
    }
    
    // Now enumerate and verify we can see the new key
    ULONG index = 0;
    UCHAR buffer[2048];
    PKEY_BASIC_INFORMATION keyInfo = (PKEY_BASIC_INFORMATION)buffer;
    ULONG resultLength;
    std::vector<std::wstring> foundKeys;
    
    while (true) {
        status = ZwEnumerateKey(
            mainKeyHandle,
            index,
            KeyBasicInformation,
            keyInfo,
            sizeof(buffer),
            &resultLength
        );
        
        if (status == STATUS_NO_MORE_ENTRIES) {
            break;
        }
        
        EXPECT_TRUE(NT_SUCCESS(status));
        if (!NT_SUCCESS(status)) {
            break;
        }
        
        WCHAR keyName[1024] = {0};
        ULONG nameChars = keyInfo->NameLength / sizeof(WCHAR);
        wcsncpy_s(keyName, 1024, keyInfo->Name, nameChars);
        foundKeys.push_back(std::wstring(keyName));
        
        index++;
    }
    
    // Should now have at least 4 keys (3 original + 1 full path, possibly more from other tests)
    EXPECT_GE(foundKeys.size(), 4);
    
    // Verify the full path created key is found
    bool foundFullPathKey = false;
    for (const auto& keyName : foundKeys) {
        if (keyName == L"FullPathSubKey") {
            foundFullPathKey = true;
            break;
        }
    }
    EXPECT_TRUE(foundFullPathKey);
}

// Test enumeration with different information classes
TEST_F(RegistryEnumerateTest, EnumerateKeysDifferentInfoClasses) {
    UCHAR buffer[2048];
    ULONG resultLength;
    
    // Test KeyFullInformation (if supported)
    NTSTATUS status = ZwEnumerateKey(
        mainKeyHandle,
        0,
        KeyFullInformation,
        buffer,
        sizeof(buffer),
        &resultLength
    );
    
    // This should either succeed or return invalid parameter (depending on implementation)
    EXPECT_TRUE(NT_SUCCESS(status) || status == STATUS_INVALID_PARAMETER);
}
