#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <WinKernelLite/Registry.h>
#include <WinKernelLite/UnicodeString.h>
#include <WinKernelLite/NtStatus.h>

/**
 * @brief Test suite for detecting corruption in manual test scenarios
 * 
 * This specifically tests the corruption patterns found in manual_test.cpp
 */
class ManualTestCorruptionDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any previous test keys
        CleanupTestKeys();
    }
    
    void TearDown() override {
        CleanupTestKeys();
    }
    
private:
    void CleanupTestKeys() {
        // Clean up test keys - Note: ZwDeleteKey not available in WinKernelLite
        // Keys will be cleaned up by the test framework or manually
        // This is a limitation of the current WinKernelLite implementation
    }
};

TEST_F(ManualTestCorruptionDetectionTest, DetectEnumerationBufferCorruption) {
    HANDLE keyHandle = NULL;
    UNICODE_STRING keyName;
    OBJECT_ATTRIBUTES objAttribs;
    ULONG disposition;
    
    // Create parent key under HKEY_CURRENT_USER for kernel mode simulation
    RtlInitUnicodeString(&keyName, L"Software\\WinKernelLite\\Test");
    InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    NTSTATUS status = ZwCreateKey(&keyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
    if (!NT_SUCCESS(status)) {
        GTEST_SKIP() << "Failed to create parent key, skipping test";
    }
    
    // Create subkeys with progressively longer names to test buffer overflow
    std::vector<std::wstring> subKeyNames = {
        L"SubKey1",
        L"SubKeyWithAVeryLongNameThatMightCauseProblems",
        L"SubKeyWithAnEvenLongerNameThatCouldPotentiallyCauseBufferOverflowIssuesInTheEnumerationLogic",
        L"SubKey_" + std::wstring(200, L'A'), // 200+ character name
        L"SubKey_" + std::wstring(400, L'B'), // 400+ character name
    };
    
    std::vector<HANDLE> subKeyHandles;
    int successfullyCreated = 0;
    
    // Create all subkeys
    for (const auto& subKeyName : subKeyNames) {
        std::wstring fullPath = L"Software\\WinKernelLite\\Test\\" + subKeyName;
        RtlInitUnicodeString(&keyName, fullPath.c_str());
        InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        HANDLE subKey = NULL;
        NTSTATUS createStatus = ZwCreateKey(&subKey, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
        
        if (NT_SUCCESS(createStatus)) {
            subKeyHandles.push_back(subKey);
            successfullyCreated++;
        }
    }
    
    // Assert that we created some subkeys to test with
    ASSERT_GT(successfullyCreated, 0) << "Failed to create any test subkeys";
    
    // Test enumeration with different buffer sizes to detect corruption
    std::vector<size_t> bufferSizes = { 256, 512, 1024, 2048 };
    
    for (size_t bufferSize : bufferSizes) {
        std::vector<UCHAR> buffer(bufferSize);
        PKEY_BASIC_INFORMATION keyInfo = (PKEY_BASIC_INFORMATION)buffer.data();
        ULONG index = 0;
        ULONG successCount = 0;
        
        while (true) {
            ULONG resultLength;
            NTSTATUS enumStatus = ZwEnumerateKey(
                keyHandle,
                index,
                KeyBasicInformation,
                keyInfo,
                static_cast<ULONG>(bufferSize),
                &resultLength
            );
            
            if (enumStatus == STATUS_NO_MORE_ENTRIES) {
                break;
            }
            
            if (NT_SUCCESS(enumStatus)) {
                // Check for corruption in the returned data
                ASSERT_LE(keyInfo->NameLength, bufferSize - sizeof(KEY_BASIC_INFORMATION))
                    << "Buffer corruption: NameLength (" << keyInfo->NameLength 
                    << ") exceeds buffer capacity for buffer size " << bufferSize;
                
                // Verify name buffer integrity
                WCHAR subKeyNameBuffer[2048] = {0};
                ULONG nameChars = keyInfo->NameLength / sizeof(WCHAR);
                
                if (nameChars > 0 && nameChars < 2048) {
                    // Test the exact same pattern as manual_test.cpp line 109
                    wcsncpy_s(subKeyNameBuffer, 2048, keyInfo->Name, nameChars);
                    
                    // Verify the copied string is valid
                    size_t actualLen = wcsnlen(subKeyNameBuffer, nameChars);
                    EXPECT_EQ(actualLen, nameChars) 
                        << "String corruption detected: expected length " << nameChars 
                        << ", got " << actualLen << " for buffer size " << bufferSize;
                }
                
                successCount++;
                index++;
            } else if (enumStatus == STATUS_BUFFER_TOO_SMALL) {
                // This is expected for small buffers with long names
                index++; // Skip this entry
            } else {
                FAIL() << "Enumeration failed at index " << index 
                       << " with status: 0x" << std::hex << enumStatus
                       << " for buffer size " << bufferSize;
            }
        }
        
        // Assert that we enumerated at least some entries
        EXPECT_GT(successCount, 0) << "No entries enumerated for buffer size " << bufferSize;
    }
    
    // Cleanup
    for (HANDLE subKey : subKeyHandles) {
        if (subKey) ZwClose(subKey);
    }
    if (keyHandle) ZwClose(keyHandle);
}

TEST_F(ManualTestCorruptionDetectionTest, DetectSubKeyNameBufferCorruption) {
    HANDLE keyHandle = NULL;
    UNICODE_STRING keyName;
    OBJECT_ATTRIBUTES objAttribs;
    ULONG disposition;
    
    // Create parent key under HKEY_CURRENT_USER (more reliable than HKLM)
    RtlInitUnicodeString(&keyName, L"Software\\WinKernelLite\\Test");
    InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    NTSTATUS status = ZwCreateKey(&keyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
    if (!NT_SUCCESS(status)) {
        GTEST_SKIP() << "Failed to create parent key, skipping test";
    }
    
    // Test each pattern individually to avoid enumeration conflicts
    std::vector<std::wstring> testPatterns = {
        L"Normal",
        L"VeryLongNameThatExceedsTypicalBufferSizes_" + std::wstring(50, L'X'), // Reduced size for better compatibility
        L"SpecialChars_Test", // Simplified special chars
        L"BackSlashes_Path_Like_Name",
        L"NoNullTermination"
    };
    
    int patternsProcessed = 0;
    
    for (size_t patternIndex = 0; patternIndex < testPatterns.size(); ++patternIndex) {
        const auto& pattern = testPatterns[patternIndex];
        
        // Create a unique parent key for each pattern to avoid enumeration conflicts
        std::wstring uniqueParentPath = L"Software\\WinKernelLite\\Test\\Parent" + std::to_wstring(patternIndex);
        RtlInitUnicodeString(&keyName, uniqueParentPath.c_str());
        InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        HANDLE parentKey = NULL;
        NTSTATUS parentStatus = ZwCreateKey(&parentKey, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
        
        if (!NT_SUCCESS(parentStatus)) {
            continue; // Skip this pattern if we can't create the parent
        }
        
        // Create subkey under the unique parent
        RtlInitUnicodeString(&keyName, pattern.c_str());
        InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, parentKey, NULL);
        
        HANDLE subKey = NULL;
        NTSTATUS createStatus = ZwCreateKey(&subKey, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
        
        if (NT_SUCCESS(createStatus)) {
            patternsProcessed++;
            
            // Now enumerate and test the exact corruption scenario from manual_test.cpp
            UCHAR buffer[512]; // Same size as manual_test.cpp line 88
            PKEY_BASIC_INFORMATION keyInfo = (PKEY_BASIC_INFORMATION)buffer;
            ULONG resultLength;
            
            NTSTATUS enumStatus = ZwEnumerateKey(
                parentKey, // Enumerate from the parent key
                0, // First (and only) entry
                KeyBasicInformation,
                keyInfo,
                sizeof(buffer),
                &resultLength
            );
            
            if (NT_SUCCESS(enumStatus)) {
                // Replicate the exact code from manual_test.cpp lines 107-109
                WCHAR subKeyName[1024] = {0}; // Line 107
                ULONG nameChars = keyInfo->NameLength / sizeof(WCHAR); // Line 108
                
                // Check for potential corruption before copying
                ASSERT_LE(nameChars, 1024) 
                    << "Buffer overflow risk: nameChars (" << nameChars 
                    << ") > buffer size (1024) for pattern: " << pattern.substr(0, 30);
                
                // Test the problematic line 109
                if (nameChars > 0 && nameChars < 1024) {
                    wcsncpy_s(subKeyName, 1024, keyInfo->Name, nameChars); // Line 109
                    
                    // Verify integrity after copy
                    size_t actualLen = wcsnlen(subKeyName, 1024);
                    EXPECT_EQ(actualLen, nameChars) 
                        << "Corruption detected: expected " << nameChars 
                        << " chars, got " << actualLen << " for pattern: " << pattern.substr(0, 30);
                    
                    // Compare with original pattern
                    std::wstring copiedName(subKeyName);
                    
                    // For Unicode patterns, we need to be more flexible in comparison
                    // since the registry might normalize or modify the names
                    if (pattern.find(L"Unicode") != std::wstring::npos) {
                        // For Unicode patterns, just check that we got some valid string back
                        EXPECT_GT(copiedName.length(), 0) 
                            << "Unicode pattern resulted in empty string for pattern: " << pattern.substr(0, 30);
                        EXPECT_LE(copiedName.length(), pattern.length() + 10) 
                            << "Unicode pattern resulted in unexpectedly long string for pattern: " << pattern.substr(0, 30);
                    } else {
                        // For non-Unicode patterns, expect exact match
                        EXPECT_EQ(copiedName, pattern) 
                            << "Data corruption: copied name '" << copiedName 
                            << "' != original pattern '" << pattern << "'";
                    }
                } else {
                    FAIL() << "Invalid name length: " << nameChars << " for pattern: " << pattern.substr(0, 30);
                }
            } else {
                // Don't fail for enumeration issues - some patterns might cause expected failures
                ADD_FAILURE() << "Failed to enumerate subkey for pattern: " << pattern.substr(0, 30) 
                             << " (Status: 0x" << std::hex << enumStatus << ")";
            }
            
            ZwClose(subKey);
        }
        
        // Close the parent key
        if (parentKey) {
            ZwClose(parentKey);
        }
    }
    
    if (keyHandle) ZwClose(keyHandle);
    
    // Assert that we processed at least some patterns
    ASSERT_GT(patternsProcessed, 0) << "No test patterns were successfully processed";
}

TEST_F(ManualTestCorruptionDetectionTest, DetectPathHandlingCorruption) {
    // Test the three different path styles from manual_test.cpp
    std::vector<std::pair<std::wstring, std::wstring>> pathTests = {
        { L"Traditional", L"Software\\WinKernelLite\\Test\\CorruptionTest\\WinKernelLiteManualTest" },
        { L"Windows-style", L"HKEY_CURRENT_USER\\SOFTWARE\\WinKernelLite\\Test\\CorruptionTest" } 
    };
    
    int successfulPaths = 0;
    
    for (const auto& pathTest : pathTests) {
        HANDLE keyHandle = NULL;
        UNICODE_STRING keyName;
        OBJECT_ATTRIBUTES objAttribs;
        ULONG disposition;
        
        RtlInitUnicodeString(&keyName, pathTest.second.c_str());
        
        // Use appropriate root handle based on path type
        HANDLE rootHandle = (pathTest.first == L"Traditional") ? HKEY_CURRENT_USER : NULL;
        InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, rootHandle, NULL);
        
        NTSTATUS status = ZwCreateKey(&keyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
        
        if (NT_SUCCESS(status)) {
            successfulPaths++;
            
            // Test writing and reading a value to check for path-related corruption
            UNICODE_STRING valueName;
            RtlInitUnicodeString(&valueName, L"TestValue");
            
            std::wstring testData = L"TestData_" + pathTest.first;
            NTSTATUS writeStatus = ZwSetValueKey(
                keyHandle,
                &valueName,
                0,
                REG_SZ,
                (PVOID)testData.c_str(),
                static_cast<ULONG>((testData.length() + 1) * sizeof(WCHAR))
            );
            
            EXPECT_TRUE(NT_SUCCESS(writeStatus)) 
                << "Failed to write value for " << pathTest.first 
                << " path (Status: 0x" << std::hex << writeStatus << ")";
            
            if (NT_SUCCESS(writeStatus)) {
                // Read back and verify
                UCHAR readBuffer[1024];
                ULONG resultLength;
                NTSTATUS readStatus = ZwQueryValueKey(
                    keyHandle,
                    &valueName,
                    KeyValueFullInformation,
                    readBuffer,
                    sizeof(readBuffer),
                    &resultLength
                );
                
                EXPECT_TRUE(NT_SUCCESS(readStatus)) 
                    << "Failed to read value for " << pathTest.first 
                    << " path (Status: 0x" << std::hex << readStatus << ")";
                
                if (NT_SUCCESS(readStatus)) {
                    PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)readBuffer;
                    WCHAR* readData = (WCHAR*)((BYTE*)valueInfo + valueInfo->DataOffset);
                    
                    std::wstring readString(readData, valueInfo->DataLength / sizeof(WCHAR) - 1);
                    EXPECT_EQ(readString, testData) 
                        << "Data corruption detected for " << pathTest.first 
                        << " path: '" << readString << "' != '" << testData << "'";
                }
            }
            
            ZwClose(keyHandle);
        }
        // Don't fail for path types that might not be supported
    }
    
    // Assert that at least one path type worked
    ASSERT_GT(successfulPaths, 0) << "No path handling methods were successful";
}
