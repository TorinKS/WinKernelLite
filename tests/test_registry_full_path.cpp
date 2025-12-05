#include <gtest/gtest.h>
#include "../include/Registry.h"
#include "../include/UnicodeString.h"
#include "../include/NtStatus.h"

class RegistryFullPathTest : public ::testing::Test {
protected:
    // Common test paths used across multiple tests
    UNICODE_STRING testKeyPath;
    UNICODE_STRING testKeyPathHKCU;
    UNICODE_STRING testKeyPathNT;
    UNICODE_STRING testValueName;
    
    void SetUp() override {
        // Initialize common key paths once for all tests
        RtlInitUnicodeString(&testKeyPath, L"SOFTWARE\\WinKernelLite\\RegistryFullPathTest");
        RtlInitUnicodeString(&testKeyPathHKCU, L"HKEY_CURRENT_USER\\SOFTWARE\\WinKernelLite\\RegistryFullPathTest");
        RtlInitUnicodeString(&testKeyPathNT, L"\\Registry\\Machine\\SOFTWARE\\WinKernelLite\\RegistryFullPathTest");
        RtlInitUnicodeString(&testValueName, L"TestValue");
    }

    void TearDown() override {
        // Clean up any test keys that might have been created
        // Use kernel APIs for cleanup like the working registry test
        HANDLE hKey;
        OBJECT_ATTRIBUTES objAttr;
        
        InitializeObjectAttributes(&objAttr, &testKeyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        if (NT_SUCCESS(ZwOpenKey(&hKey, KEY_ALL_ACCESS, &objAttr))) {
            // Just close the handle for cleanup
            ZwClose(hKey);
        }
    }
    
    // Helper method to create object attributes with root handle
    void InitializeObjectAttributesWithRoot(POBJECT_ATTRIBUTES objAttr, PUNICODE_STRING keyName, HANDLE rootHandle) {
        InitializeObjectAttributes(objAttr, keyName, OBJ_CASE_INSENSITIVE, rootHandle, NULL);
    }
    
    // Helper method to create object attributes with full path (no root)
    void InitializeObjectAttributesFullPath(POBJECT_ATTRIBUTES objAttr, PUNICODE_STRING keyName) {
        InitializeObjectAttributes(objAttr, keyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    }
};

// Test creating a key using the traditional root handle + relative path method
TEST_F(RegistryFullPathTest, CreateKeyWithRootHandle) {
    HANDLE keyHandle = NULL;
    OBJECT_ATTRIBUTES objAttribs;
    ULONG disposition;
    
    // Use the pre-initialized key path with root handle
    InitializeObjectAttributesWithRoot(&objAttribs, &testKeyPath, HKEY_CURRENT_USER);
    
    // Create the key
    NTSTATUS status = ZwCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        &objAttribs,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        &disposition
    );
    
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_NE(keyHandle, nullptr);
    
    if (keyHandle) {
        ZwClose(keyHandle);
    }
}

// Test creating a key using full path with NULL root directory
TEST_F(RegistryFullPathTest, CreateKeyWithFullPath) {
    HANDLE keyHandle = NULL;
    OBJECT_ATTRIBUTES objAttribs;
    ULONG disposition;
    
    // Use the pre-initialized HKCU full path
    InitializeObjectAttributesFullPath(&objAttribs, &testKeyPathHKCU);
    
    // Create the key
    NTSTATUS status = ZwCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        &objAttribs,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        &disposition
    );
    
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_NE(keyHandle, nullptr);
    
    if (keyHandle) {
        ZwClose(keyHandle);
    }
}

// Test creating a key using NT-style registry path
TEST_F(RegistryFullPathTest, CreateKeyWithNTPath) {
    HANDLE keyHandle = NULL;
    OBJECT_ATTRIBUTES objAttribs;
    ULONG disposition;
    
    // Use the pre-initialized NT-style path
    InitializeObjectAttributesFullPath(&objAttribs, &testKeyPathNT);
    
    // Create the key
    NTSTATUS status = ZwCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        &objAttribs,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        &disposition
    );
    
    // NT path parsing should work regardless of permissions.
    // For non-admin users: expect STATUS_ACCESS_DENIED
    // For admin users: expect STATUS_SUCCESS
    // Both cases prove that NT path parsing is working correctly.
    if (NT_SUCCESS(status)) {
        // Running as administrator - creation succeeded
        EXPECT_NE(keyHandle, nullptr);
        ZwClose(keyHandle);
    } else {
        // Running as regular user - expect access denied
        EXPECT_EQ(status, STATUS_ACCESS_DENIED);
        EXPECT_EQ(keyHandle, nullptr);
    }
}

// Test opening a key using full path with NULL root directory
TEST_F(RegistryFullPathTest, OpenKeyWithFullPath) {
    // First create a key using the traditional method
    HANDLE createHandle = NULL;
    OBJECT_ATTRIBUTES createObjAttribs;
    
    InitializeObjectAttributesWithRoot(&createObjAttribs, &testKeyPath, HKEY_CURRENT_USER);
    
    NTSTATUS createStatus = ZwCreateKey(
        &createHandle,
        KEY_ALL_ACCESS,
        &createObjAttribs,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        NULL
    );
    
    ASSERT_TRUE(NT_SUCCESS(createStatus));
    ASSERT_NE(createHandle, nullptr);
    ZwClose(createHandle);
    
    // Now try to open it using full path
    HANDLE openHandle = NULL;
    OBJECT_ATTRIBUTES openObjAttribs;
    
    InitializeObjectAttributesFullPath(&openObjAttribs, &testKeyPathHKCU);
    
    NTSTATUS openStatus = ZwOpenKey(
        &openHandle,
        KEY_READ,
        &openObjAttribs
    );
    
    EXPECT_TRUE(NT_SUCCESS(openStatus));
    EXPECT_NE(openHandle, nullptr);
    
    if (openHandle) {
        ZwClose(openHandle);
    }
}

// Test setting and querying values using both methods
TEST_F(RegistryFullPathTest, SetAndQueryValueBothMethods) {
    // Create key using full path method
    HANDLE keyHandle = NULL;
    OBJECT_ATTRIBUTES objAttribs;
    
    InitializeObjectAttributesFullPath(&objAttribs, &testKeyPathHKCU);
    
    NTSTATUS status = ZwCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        &objAttribs,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        NULL
    );
    
    ASSERT_TRUE(NT_SUCCESS(status));
    ASSERT_NE(keyHandle, nullptr);
    
    // Set a test value using the pre-initialized value name
    DWORD testData = 12345;
    status = ZwSetValueKey(
        keyHandle,
        &testValueName,
        0,
        REG_DWORD,
        &testData,
        sizeof(testData)
    );
    
    EXPECT_TRUE(NT_SUCCESS(status));
    
    // Close the key
    ZwClose(keyHandle);
    
    // Now reopen using traditional method and query the value
    OBJECT_ATTRIBUTES openObjAttribs;
    
    InitializeObjectAttributesWithRoot(&openObjAttribs, &testKeyPath, HKEY_CURRENT_USER);
    
    status = ZwOpenKey(
        &keyHandle,
        KEY_READ,
        &openObjAttribs
    );
    
    ASSERT_TRUE(NT_SUCCESS(status));
    ASSERT_NE(keyHandle, nullptr);
    
    // Query the value using the pre-initialized value name
    BYTE buffer[1024];
    ULONG resultLength;
    
    status = ZwQueryValueKey(
        keyHandle,
        &testValueName,
        KeyValueFullInformation,
        buffer,
        sizeof(buffer),
        &resultLength
    );
    
    EXPECT_TRUE(NT_SUCCESS(status));
    
    // Verify the value
    PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
    EXPECT_EQ(valueInfo->Type, REG_DWORD);
    EXPECT_EQ(valueInfo->DataLength, sizeof(DWORD));
    
    DWORD* retrievedData = (DWORD*)((BYTE*)valueInfo + valueInfo->DataOffset);
    EXPECT_EQ(*retrievedData, testData);
    
    ZwClose(keyHandle);
}

// Test error handling for invalid paths
TEST_F(RegistryFullPathTest, InvalidPathHandling) {
    HANDLE keyHandle = NULL;
    UNICODE_STRING invalidKeyName;
    OBJECT_ATTRIBUTES objAttribs;
    
    // Test with invalid path format
    RtlInitUnicodeString(&invalidKeyName, L"INVALID_ROOT\\SOFTWARE\\Test");
    InitializeObjectAttributesFullPath(&objAttribs, &invalidKeyName);
    
    NTSTATUS status = ZwCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        &objAttribs,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        NULL
    );
    
    EXPECT_FALSE(NT_SUCCESS(status));
    EXPECT_EQ(status, STATUS_OBJECT_PATH_SYNTAX_BAD);
}
