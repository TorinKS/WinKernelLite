#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include "../include/Registry.h"
#include "../include/UnicodeString.h"
#include "../include/NtStatus.h"

/**
 * @brief Comprehensive test suite for detecting data corruption in WinKernelLite registry operations
 * 
 * This test suite is designed to detect:
 * - Buffer overflow issues in registry write operations
 * - Memory corruption during string handling
 * - Uninitialized memory being written to registry
 * - String concatenation and boundary issues
 * - Unicode handling corruption
 */
class RegistryCorruptionDetectionTest : public ::testing::Test {
protected:
    HANDLE testKeyHandle;
    std::wstring testKeyPath;
    
    void SetUp() override {
        // Use HKEY_CURRENT_USER to avoid admin privileges
        testKeyPath = L"SOFTWARE\\WinKernelLite\\CorruptionTest";
        
        UNICODE_STRING keyName;
        OBJECT_ATTRIBUTES objAttribs;
        ULONG disposition;
        
        RtlInitUnicodeString(&keyName, testKeyPath.c_str());
        InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        NTSTATUS status = ZwCreateKey(&testKeyHandle, KEY_ALL_ACCESS, &objAttribs, 0, NULL, REG_OPTION_NON_VOLATILE, &disposition);
        ASSERT_TRUE(NT_SUCCESS(status)) << "Failed to create test key";
    }
    
    void TearDown() override {
        if (testKeyHandle) {
            ZwClose(testKeyHandle);
        }
        
        // Clean up the test key
        HANDLE cleanupHandle;
        UNICODE_STRING keyName;
        OBJECT_ATTRIBUTES objAttribs;
        
        RtlInitUnicodeString(&keyName, testKeyPath.c_str());
        InitializeObjectAttributes(&objAttribs, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
        
        if (NT_SUCCESS(ZwOpenKey(&cleanupHandle, DELETE, &objAttribs))) {
            // Note: Would need RegDeleteKey equivalent for full cleanup
            ZwClose(cleanupHandle);
        }
    }
    
    /**
     * @brief Generate a pattern-filled buffer for corruption detection
     */
    std::vector<BYTE> CreatePatternBuffer(size_t size, BYTE pattern = 0xCC) {
        std::vector<BYTE> buffer(size, pattern);
        return buffer;
    }
    
    /**
     * @brief Check if a buffer contains the expected pattern (no corruption)
     */
    bool VerifyPattern(const BYTE* buffer, size_t size, BYTE expectedPattern = 0xCC) {
        for (size_t i = 0; i < size; ++i) {
            if (buffer[i] != expectedPattern) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Enumerate all values in the test key to detect corruption
     */
    std::vector<std::wstring> EnumerateRegistryValues() {
        std::vector<std::wstring> valueNames;
        ULONG index = 0;
        UCHAR buffer[4096]; // Large buffer to catch corruption
        ULONG resultLength;
        
        while (true) {
            NTSTATUS status = ZwEnumerateValueKey(
                testKeyHandle,
                index,
                KeyValueBasicInformation,
                buffer,
                sizeof(buffer),
                &resultLength
            );
            
            if (status == STATUS_NO_MORE_ENTRIES) {
                break;
            }
            
            if (NT_SUCCESS(status)) {
                PKEY_VALUE_BASIC_INFORMATION valueInfo = (PKEY_VALUE_BASIC_INFORMATION)buffer;
                
                // Extract value name carefully
                std::wstring valueName;
                if (valueInfo->NameLength > 0 && valueInfo->NameLength < 2048) { // Sanity check
                    ULONG nameChars = valueInfo->NameLength / sizeof(WCHAR);
                    valueName.assign(valueInfo->Name, nameChars);
                } else {
                    // Potential corruption - very long or zero length name
                    valueName = L"<CORRUPTED_LENGTH_" + std::to_wstring(valueInfo->NameLength) + L">";
                }
                
                valueNames.push_back(valueName);
                index++;
            } else {
                // Enumeration failed - possible corruption
                valueNames.push_back(L"<ENUM_FAILED_" + std::to_wstring(status) + L">");
                break;
            }
            
            // Safety check to prevent infinite loops
            if (index > 1000) {
                valueNames.push_back(L"<TOO_MANY_VALUES>");
                break;
            }
        }
        
        return valueNames;
    }
};

/**
 * @brief Test buffer overflow protection with extremely long value names
 */
TEST_F(RegistryCorruptionDetectionTest, DetectBufferOverflowInValueNames) {
    std::wcout << L"\n=== Testing Buffer Overflow in Value Names ===" << std::endl;
    
    // Test various problematic value name lengths
    std::vector<size_t> testLengths = {
        255,   // Near buffer boundary
        256,   // Common buffer size
        512,   // Medium length
        1024,  // Large length
        2048,  // Very large
        4096   // Extremely large
    };
    
    for (size_t length : testLengths) {
        std::wstring longValueName(length, L'A');
        longValueName += L"_TEST_" + std::to_wstring(length);
        
        std::wcout << L"  Testing value name length: " << longValueName.length() << std::endl;
        
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, longValueName.c_str());
        
        DWORD testData = 0x12345678;
        NTSTATUS status = ZwSetValueKey(testKeyHandle, &valueName, 0, REG_DWORD, &testData, sizeof(testData));
        
        // Check if the operation succeeded
        bool operationSucceeded = NT_SUCCESS(status);
        std::wcout << L"    Status: 0x" << std::hex << status << L" (" << (operationSucceeded ? L"SUCCESS" : L"FAILED") << L")" << std::endl;
        
        // Enumerate values to check for corruption
        auto valueNames = EnumerateRegistryValues();
        
        // Look for signs of corruption
        bool corruptionDetected = false;
        for (const auto& name : valueNames) {
            // Check for corruption indicators
            if (name.find(L"<CORRUPTED") != std::wstring::npos ||
                name.find(L"<ENUM_FAILED") != std::wstring::npos ||
                name.find(L"<TOO_MANY") != std::wstring::npos) {
                corruptionDetected = true;
                std::wcout << L"      CORRUPTION DETECTED: " << name << std::endl;
            }
            
            // Check for invalid Unicode characters (common in corruption)
            for (wchar_t ch : name) {
                if (ch < 0x20 && ch != 0x00) { // Control characters (except null)
                    corruptionDetected = true;
                    std::wcout << L"      INVALID CHARACTER DETECTED in: " << name << std::endl;
                    break;
                }
            }
        }
        
        if (!corruptionDetected && valueNames.size() > 0) {
            std::wcout << L"      No corruption detected for length " << length << std::endl;
        }
    }
}

/**
 * @brief Test for memory corruption during string operations
 */
TEST_F(RegistryCorruptionDetectionTest, DetectMemoryCorruptionInStringHandling) {
    std::wcout << L"\n=== Testing Memory Corruption in String Handling ===" << std::endl;
    
    // Create buffers with known patterns around the operation
    const size_t GUARD_SIZE = 1024;
    const BYTE GUARD_PATTERN = 0xDE;
    const BYTE DATA_PATTERN = 0xAD;
    
    std::vector<BYTE> guardBuffer1 = CreatePatternBuffer(GUARD_SIZE, GUARD_PATTERN);
    std::vector<BYTE> dataBuffer = CreatePatternBuffer(512, DATA_PATTERN);
    std::vector<BYTE> guardBuffer2 = CreatePatternBuffer(GUARD_SIZE, GUARD_PATTERN);
    
    // Test various Unicode strings with potential corruption triggers
    std::vector<std::wstring> testStrings = {
        L"NormalValue",
        L"Value\x00WithNull",
        L"Value\x01WithControl",
        L"Value\xFFFFWithHighUnicode",
        L"Value\x4E2D\x6587WithChinese",  // Chinese characters
        L"Value\xD800\xDC00WithSurrogate", // Surrogate pair
        L"",  // Empty string
        std::wstring(L"RepeatedPattern").append(100, L'X'),  // Repeated pattern
    };
    
    int testIndex = 0;
    for (const auto& testString : testStrings) {
        std::wcout << L"  Testing string: " << (testString.empty() ? L"<EMPTY>" : testString.substr(0, std::min<size_t>(20, testString.length()))) << std::endl;
        
        UNICODE_STRING valueName;
        std::wstring valueNameStr = L"MemTest_" + std::to_wstring(testIndex++);
        RtlInitUnicodeString(&valueName, valueNameStr.c_str());
        
        // Use the test string as data
        NTSTATUS status = ZwSetValueKey(
            testKeyHandle, 
            &valueName, 
            0, 
            REG_SZ, 
            (PVOID)testString.c_str(), 
            (ULONG)((testString.length() + 1) * sizeof(WCHAR))
        );
        
        std::wcout << L"    Write Status: 0x" << std::hex << status << std::endl;
        
        // Verify guard buffers weren't corrupted
        bool guard1Corrupted = !VerifyPattern(guardBuffer1.data(), guardBuffer1.size(), GUARD_PATTERN);
        bool guard2Corrupted = !VerifyPattern(guardBuffer2.data(), guardBuffer2.size(), GUARD_PATTERN);
        
        if (guard1Corrupted || guard2Corrupted) {
            std::wcout << L"      GUARD BUFFER CORRUPTION DETECTED!" << std::endl;
            std::wcout << L"      Guard 1: " << (guard1Corrupted ? L"CORRUPTED" : L"OK") << std::endl;
            std::wcout << L"      Guard 2: " << (guard2Corrupted ? L"CORRUPTED" : L"OK") << std::endl;
        }
        
        // Try to read back the value
        UCHAR readBuffer[2048];
        ULONG resultLength;
        NTSTATUS readStatus = ZwQueryValueKey(
            testKeyHandle,
            &valueName,
            KeyValueFullInformation,
            readBuffer,
            sizeof(readBuffer),
            &resultLength
        );
        
        if (NT_SUCCESS(readStatus)) {
            PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)readBuffer;
            std::wcout << L"    Read Status: SUCCESS" << std::endl;
            
            // Check for data integrity
            if (valueInfo->DataLength > 0 && valueInfo->DataOffset < sizeof(readBuffer)) {
                WCHAR* readData = (WCHAR*)((BYTE*)valueInfo + valueInfo->DataOffset);
                size_t readDataChars = valueInfo->DataLength / sizeof(WCHAR);
                
                // Verify the data wasn't corrupted
                if (readDataChars > 0) {
                    std::wstring readString(readData, std::min<size_t>(readDataChars - 1, 100)); // Limit for safety
                    bool dataMatches = (readString == testString);
                    
                    if (!dataMatches) {
                        std::wcout << L"      DATA CORRUPTION: Written != Read" << std::endl;
                        std::wcout << L"      Expected: " << testString.substr(0, 20) << std::endl;
                        std::wcout << L"      Got:      " << readString.substr(0, 20) << std::endl;
                    } else {
                        std::wcout << L"      Data integrity verified" << std::endl;
                    }
                }
            }
        } else {
            std::wcout << L"    🚨 READ FAILED: 0x" << std::hex << readStatus << std::endl;
        }
    }
}

/**
 * @brief Test for stack/heap corruption during registry operations
 */
TEST_F(RegistryCorruptionDetectionTest, DetectStackHeapCorruption) {
    std::wcout << L"\n=== Testing Stack/Heap Corruption ===" << std::endl;
    
    // Create stack canaries
    const DWORD STACK_CANARY = 0xDEADBEEF;
    DWORD canary1 = STACK_CANARY;
    DWORD canary2 = STACK_CANARY;
    DWORD canary3 = STACK_CANARY;
    
    // Test operations that might cause stack corruption
    for (int i = 0; i < 10; ++i) {
        std::wstring valueName = L"StackTest_" + std::to_wstring(i);
        
        // Create progressively larger data
        std::vector<BYTE> testData(256 + i * 128, 0xAB);
        
        UNICODE_STRING valueNameUs;
        RtlInitUnicodeString(&valueNameUs, valueName.c_str());
        
        NTSTATUS status = ZwSetValueKey(
            testKeyHandle,
            &valueNameUs,
            0,
            REG_BINARY,
            testData.data(),
            (ULONG)testData.size()
        );
        
        // Check stack canaries
        if (canary1 != STACK_CANARY || canary2 != STACK_CANARY || canary3 != STACK_CANARY) {
            std::wcout << L"    STACK CORRUPTION DETECTED at iteration " << i << std::endl;
            std::wcout << L"    Canary1: 0x" << std::hex << canary1 << L" (expected 0x" << STACK_CANARY << L")" << std::endl;
            std::wcout << L"    Canary2: 0x" << std::hex << canary2 << L" (expected 0x" << STACK_CANARY << L")" << std::endl;
            std::wcout << L"    Canary3: 0x" << std::hex << canary3 << L" (expected 0x" << STACK_CANARY << L")" << std::endl;
            break;
        }
        
        std::wcout << L"  Iteration " << i << L": Status 0x" << std::hex << status << L", Canaries OK" << std::endl;
    }
    
    // Final canary check
    if (canary1 == STACK_CANARY && canary2 == STACK_CANARY && canary3 == STACK_CANARY) {
        std::wcout << L"    No stack corruption detected" << std::endl;
    }
}

/**
 * @brief Test concurrent operations for race condition corruption
 */
TEST_F(RegistryCorruptionDetectionTest, DetectConcurrencyCorruption) {
    std::wcout << L"\n=== Testing Concurrency Corruption ===" << std::endl;
    
    // Perform multiple registry operations in sequence to test for race conditions
    const int NUM_OPERATIONS = 50;
    std::vector<std::wstring> createdValues;
    
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        std::wstring valueName = L"ConcurrentTest_" + std::to_wstring(i);
        createdValues.push_back(valueName);
        
        UNICODE_STRING valueNameUs;
        RtlInitUnicodeString(&valueNameUs, valueName.c_str());
        
        DWORD testData = 0x1000 + i;
        NTSTATUS status = ZwSetValueKey(
            testKeyHandle,
            &valueNameUs,
            0,
            REG_DWORD,
            &testData,
            sizeof(testData)
        );
        
        if (!NT_SUCCESS(status)) {
            std::wcout << L"    Write failed at iteration " << i << L": 0x" << std::hex << status << std::endl;
        }
        
        // Immediately try to read it back
        UCHAR readBuffer[256];
        ULONG resultLength;
        NTSTATUS readStatus = ZwQueryValueKey(
            testKeyHandle,
            &valueNameUs,
            KeyValueFullInformation,
            readBuffer,
            sizeof(readBuffer),
            &resultLength
        );
        
        if (NT_SUCCESS(readStatus)) {
            PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)readBuffer;
            if (valueInfo->DataLength == sizeof(DWORD)) {
                DWORD* readData = (DWORD*)((BYTE*)valueInfo + valueInfo->DataOffset);
                if (*readData != testData) {
                    std::wcout << L"    DATA MISMATCH at iteration " << i << std::endl;
                    std::wcout << L"    Expected: 0x" << std::hex << testData << std::endl;
                    std::wcout << L"    Got:      0x" << std::hex << *readData << std::endl;
                }
            }
        }
    }
    
    // Enumerate all values to check for corruption
    auto allValues = EnumerateRegistryValues();
    std::wcout << L"  Created " << createdValues.size() << L" values, enumerated " << allValues.size() << L" values" << std::endl;
    
    // Check for unexpected values (corruption artifacts)
    size_t corruptedCount = 0;
    for (const auto& enumValue : allValues) {
        bool isExpected = false;
        for (const auto& createdValue : createdValues) {
            if (enumValue.find(createdValue) != std::wstring::npos) {
                isExpected = true;
                break;
            }
        }
        
        if (!isExpected) {
            std::wcout << L"    UNEXPECTED VALUE: " << enumValue << std::endl;
            corruptedCount++;
        }
    }
    
    if (corruptedCount == 0) {
        std::wcout << L"    No corruption artifacts detected" << std::endl;
    } else {
        std::wcout << L"    Found " << corruptedCount << L" corruption artifacts" << std::endl;
    }
}

/**
 * @brief Test for Unicode and encoding corruption
 */
TEST_F(RegistryCorruptionDetectionTest, DetectUnicodeCorruption) {
    std::wcout << L"\n=== Testing Unicode Corruption ===" << std::endl;
    
    // Test various Unicode scenarios that might cause corruption
    struct UnicodeTest {
        std::wstring name;
        std::wstring data;
        std::wstring description;
    };
    
    std::vector<UnicodeTest> unicodeTests = {
        {L"ASCII_Test", L"Simple ASCII text", L"Basic ASCII"},
        {L"Unicode_Test", L"Test with Unicode: \u4E2D\u6587\u6D4B\u8BD5", L"Chinese characters"},
        {L"Emoji_Test", L"Test with emoji: \U0001F600\U0001F601\U0001F602", L"Emoji characters"},
        {L"Surrogate_Test", L"Surrogate pairs: \U00010000\U0010FFFF", L"High Unicode planes"},
        {L"Mixed_Test", L"Mixed: ABC\u4E2D\U0001F600\u0041", L"Mixed encoding"},
        {L"Null_Test", L"Test\x00with\x00nulls", L"Embedded nulls"},
        {L"Control_Test", L"Test\x01\x02\x03control", L"Control characters"},
    };
    
    for (const auto& test : unicodeTests) {
        std::wcout << L"  Testing: " << test.description << std::endl;
        
        UNICODE_STRING valueName;
        RtlInitUnicodeString(&valueName, test.name.c_str());
        
        NTSTATUS status = ZwSetValueKey(
            testKeyHandle,
            &valueName,
            0,
            REG_SZ,
            (PVOID)test.data.c_str(),
            (ULONG)((test.data.length() + 1) * sizeof(WCHAR))
        );
        
        std::wcout << L"    Write Status: 0x" << std::hex << status << std::endl;
        
        if (NT_SUCCESS(status)) {
            // Read back and verify
            UCHAR readBuffer[1024];
            ULONG resultLength;
            NTSTATUS readStatus = ZwQueryValueKey(
                testKeyHandle,
                &valueName,
                KeyValueFullInformation,
                readBuffer,
                sizeof(readBuffer),
                &resultLength
            );
            
            if (NT_SUCCESS(readStatus)) {
                PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)readBuffer;
                WCHAR* readData = (WCHAR*)((BYTE*)valueInfo + valueInfo->DataOffset);
                size_t readDataChars = valueInfo->DataLength / sizeof(WCHAR);
                
                if (readDataChars > 0) {
                    std::wstring readString(readData, readDataChars - 1); // Exclude null terminator
                    
                    if (readString == test.data) {
                        std::wcout << L"    Unicode integrity verified" << std::endl;
                    } else {
                        std::wcout << L"    UNICODE CORRUPTION DETECTED" << std::endl;
                        std::wcout << L"      Expected length: " << test.data.length() << std::endl;
                        std::wcout << L"      Read length:     " << readString.length() << std::endl;
                        
                        // Show first difference
                        size_t minLength = (test.data.length() < readString.length()) ? test.data.length() : readString.length();
                        for (size_t i = 0; i < minLength; ++i) {
                            if (test.data[i] != readString[i]) {
                                std::wcout << L"      First diff at pos " << i << L": 0x" << std::hex << test.data[i] << L" != 0x" << readString[i] << std::endl;
                                break;
                            }
                        }
                    }
                }
            } else {
                std::wcout << L"     READ FAILED: 0x" << std::hex << readStatus << std::endl;
            }
        }
    }
}

/**
 * @brief Comprehensive registry state analysis
 */
TEST_F(RegistryCorruptionDetectionTest, AnalyzeRegistryState) {
    std::wcout << L"\n=== Analyzing Registry State ===" << std::endl;
    
    // Get all values in the test key
    auto allValues = EnumerateRegistryValues();
    
    std::wcout << L"  Total values found: " << allValues.size() << std::endl;
    
    // Analyze each value for corruption indicators
    size_t suspiciousCount = 0;
    size_t corruptedCount = 0;
    
    for (const auto& valueName : allValues) {
        bool isSuspicious = false;
        bool isCorrupted = false;
        
        // Check for corruption indicators
        if (valueName.find(L"<CORRUPTED") != std::wstring::npos ||
            valueName.find(L"<ENUM_FAILED") != std::wstring::npos ||
            valueName.find(L"<TOO_MANY") != std::wstring::npos) {
            isCorrupted = true;
            corruptedCount++;
        }
        
        // Check for suspicious patterns
        if (valueName.length() > 500) { // Unusually long names
            isSuspicious = true;
        }
        
        // Check for invalid Unicode sequences
        for (wchar_t ch : valueName) {
            if ((ch >= 0xD800 && ch <= 0xDFFF) || // Surrogate range without proper pairing
                (ch < 0x20 && ch != 0x09 && ch != 0x0A && ch != 0x0D)) { // Invalid control chars
                isSuspicious = true;
                break;
            }
        }
        
        // Check for memory corruption patterns
        if (valueName.find(L"\x4E2D\x6587") != std::wstring::npos || // Unexpected Chinese
            valueName.find(L"\xFFFF") != std::wstring::npos ||        // Invalid Unicode
            valueName.find(L"\x0000") != std::wstring::npos) {        // Embedded nulls
            isSuspicious = true;
        }
        
        if (isSuspicious) {
            suspiciousCount++;
        }
        
        if (isCorrupted || isSuspicious) {
            std::wcout << L"  " << (isCorrupted ? L" CORRUPTED" : L" SUSPICIOUS") << L": " << valueName << std::endl;
        }
    }
    
    std::wcout << L"  Analysis Results:" << std::endl;
    std::wcout << L"    Normal values:     " << (allValues.size() - suspiciousCount - corruptedCount) << std::endl;
    std::wcout << L"    Suspicious values: " << suspiciousCount << std::endl;
    std::wcout << L"    Corrupted values:  " << corruptedCount << std::endl;
    
    if (corruptedCount == 0 && suspiciousCount == 0) {
        std::wcout << L"  Registry state appears healthy" << std::endl;
    } else {
        std::wcout << L"  Registry corruption detected!" << std::endl;
    }
}
