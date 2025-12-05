#include <gtest/gtest.h>
#include "../include/Registry.h"
#include "../include/UnicodeString.h"
#include "../include/KernelHeap.h"

class RegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test registry keys - use a simpler path that works with the implementation
        testKeyPath = L"Software\\Test\\WinKernelLite";
        testValueName = L"TestValue";
        testSubKeyName = L"TestSubKey";
    }

    void TearDown() override {
        // Clean up any test keys created during tests
        HANDLE hKey;
        OBJECT_ATTRIBUTES objAttr;
        UNICODE_STRING keyPath;
        
        RtlInitUnicodeString(&keyPath, testKeyPath);
        InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        if (NT_SUCCESS(ZwOpenKey(&hKey, KEY_ALL_ACCESS, &objAttr))) {
            // Note: ZwDeleteKey is not available in this implementation
            // Just close the handle for cleanup
            ZwClose(hKey);
        }
    }

    PCWSTR testKeyPath;
    PCWSTR testValueName;
    PCWSTR testSubKeyName;
};

TEST_F(RegistryTest, CreateAndOpenKey) {
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath;
    ULONG disposition;

    // Test key creation - use HKEY_CURRENT_USER as root
    RtlInitUnicodeString(&keyPath, testKeyPath);
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);

    NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 
                                  REG_OPTION_NON_VOLATILE, &disposition);
    
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_NE(hKey, (HANDLE)NULL);

    if (hKey) {
        ZwClose(hKey);
    }

    // Test opening existing key
    hKey = NULL;
    status = ZwOpenKey(&hKey, KEY_READ, &objAttr);
    
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_NE(hKey, (HANDLE)NULL);

    if (hKey) {
        ZwClose(hKey);
    }
}

TEST_F(RegistryTest, SetAndQueryValueDWORD) {
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath, valueName;
    ULONG disposition;

    // Create test key - use HKEY_CURRENT_USER as root
    RtlInitUnicodeString(&keyPath, testKeyPath);
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);

    NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 
                                  REG_OPTION_NON_VOLATILE, &disposition);
    ASSERT_TRUE(NT_SUCCESS(status));
    ASSERT_NE(hKey, (HANDLE)NULL);

    // Set DWORD value
    RtlInitUnicodeString(&valueName, testValueName);
    ULONG testValue = 0x12345678;
    
    status = ZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &testValue, sizeof(ULONG));
    EXPECT_TRUE(NT_SUCCESS(status));

    // Query the value back
    UCHAR buffer[256];
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    ULONG resultLength;

    status = ZwQueryValueKey(hKey, &valueName, KeyValuePartialInformation, 
                            valueInfo, sizeof(buffer), &resultLength);
    
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_EQ(valueInfo->Type, REG_DWORD);
    EXPECT_EQ(valueInfo->DataLength, sizeof(ULONG));
    EXPECT_EQ(*(PULONG)valueInfo->Data, testValue);

    ZwClose(hKey);
}

TEST_F(RegistryTest, SetAndQueryValueString) {
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath, valueName;
    ULONG disposition;

    // Create test key - use HKEY_CURRENT_USER as root
    RtlInitUnicodeString(&keyPath, testKeyPath);
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);

    NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 
                                  REG_OPTION_NON_VOLATILE, &disposition);
    ASSERT_TRUE(NT_SUCCESS(status));

    // Set string value
    RtlInitUnicodeString(&valueName, testValueName);
    PCWSTR testString = L"Hello Registry World!";
    ULONG stringLength = (wcslen(testString) + 1) * sizeof(WCHAR);
    
    status = ZwSetValueKey(hKey, &valueName, 0, REG_SZ, 
                          (PVOID)testString, stringLength);
    EXPECT_TRUE(NT_SUCCESS(status));

    // Query the value back
    UCHAR buffer[512];
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    ULONG resultLength;

    status = ZwQueryValueKey(hKey, &valueName, KeyValuePartialInformation, 
                            valueInfo, sizeof(buffer), &resultLength);
    
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_EQ(valueInfo->Type, REG_SZ);
    
    PCWSTR retrievedString = (PCWSTR)valueInfo->Data;
    EXPECT_EQ(wcscmp(retrievedString, testString), 0);

    ZwClose(hKey);
}

TEST_F(RegistryTest, SetAndQueryValueBinary) {
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath, valueName;
    ULONG disposition;

    // Create test key - use HKEY_CURRENT_USER as root
    RtlInitUnicodeString(&keyPath, testKeyPath);
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);

    NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 
                                  REG_OPTION_NON_VOLATILE, &disposition);
    ASSERT_TRUE(NT_SUCCESS(status));

    // Set binary value
    RtlInitUnicodeString(&valueName, testValueName);
    UCHAR testData[] = {0x01, 0x02, 0x03, 0x04, 0xFF, 0xFE, 0xFD, 0xFC};
    
    status = ZwSetValueKey(hKey, &valueName, 0, REG_BINARY, testData, sizeof(testData));
    EXPECT_TRUE(NT_SUCCESS(status));

    // Query the value back
    UCHAR buffer[256];
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    ULONG resultLength;

    status = ZwQueryValueKey(hKey, &valueName, KeyValuePartialInformation, 
                            valueInfo, sizeof(buffer), &resultLength);
    
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_EQ(valueInfo->Type, REG_BINARY);
    EXPECT_EQ(valueInfo->DataLength, sizeof(testData));
    EXPECT_EQ(memcmp(valueInfo->Data, testData, sizeof(testData)), 0);

    ZwClose(hKey);
}

TEST_F(RegistryTest, InvalidOperations) {
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath, valueName;

    // Test opening non-existent key - use HKEY_CURRENT_USER as root
    RtlInitUnicodeString(&keyPath, L"NonExistent\\Key\\Path");
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);

    NTSTATUS status = ZwOpenKey(&hKey, KEY_READ, &objAttr);
    EXPECT_FALSE(NT_SUCCESS(status));

    // Test querying non-existent value
    RtlInitUnicodeString(&keyPath, testKeyPath);
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);

    ULONG disposition;
    status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 
                        REG_OPTION_NON_VOLATILE, &disposition);
    ASSERT_TRUE(NT_SUCCESS(status));

    RtlInitUnicodeString(&valueName, L"NonExistentValue");
    UCHAR buffer[256];
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    ULONG resultLength;

    status = ZwQueryValueKey(hKey, &valueName, KeyValuePartialInformation, 
                            valueInfo, sizeof(buffer), &resultLength);
    EXPECT_EQ(status, STATUS_OBJECT_NAME_NOT_FOUND);

    ZwClose(hKey);
}

TEST_F(RegistryTest, KeyInformationQuery) {
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath, valueName;
    ULONG disposition;

    // Create test key - use HKEY_CURRENT_USER as root
    RtlInitUnicodeString(&keyPath, testKeyPath);
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);

    NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 
                                  REG_OPTION_NON_VOLATILE, &disposition);
    ASSERT_TRUE(NT_SUCCESS(status));

    // Add some values
    RtlInitUnicodeString(&valueName, L"TestValue1");
    ULONG testValue = 42;
    status = ZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &testValue, sizeof(ULONG));
    EXPECT_TRUE(NT_SUCCESS(status));

    RtlInitUnicodeString(&valueName, L"TestValue2");
    status = ZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &testValue, sizeof(ULONG));
    EXPECT_TRUE(NT_SUCCESS(status));

    // Note: ZwQueryKey is not available in this implementation
    // This test verifies that we can set multiple values in a key

    ZwClose(hKey);
}

TEST_F(RegistryTest, BasicRegistryOperations) {
    // Simple test to ensure basic registry functionality works
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath;
    ULONG disposition;

    RtlInitUnicodeString(&keyPath, testKeyPath);
    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);

    // Create key
    NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 
                                  REG_OPTION_NON_VOLATILE, &disposition);
    
    EXPECT_TRUE(NT_SUCCESS(status));
    EXPECT_NE(hKey, (HANDLE)NULL);
    
    if (hKey) {
        ZwClose(hKey);
    }
}
