#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstring>
#include <random>
#include "../include/Registry.h"
#include "../include/UnicodeString.h"
#include "../include/NtStatus.h"

/**
 * @brief Advanced corruption detection tests targeting specific corruption scenarios
 * 
 * This test suite focuses on reproducing and detecting the exact types of corruption
 * observed in production systems, including:
 * - Registry path concatenation errors
 * - Memory alignment issues
 * - Buffer boundary violations
 * - String termination problems
 */
class RegistryCorruptionTest : public ::testing::Test {
protected:
    HANDLE testKeyHandle;
    std::wstring testKeyPath;
    
    void SetUp() override {
        testKeyPath = L"SOFTWARE\\WinKernelLite\\RegistryCorruptionTest\\WinKernelLiteTest";
        
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
    }
    
    /**
     * @brief Create a detailed corruption report for a value
     */
    void AnalyzeCorruptedValue(const std::wstring& valueName) {
        std::wcout << L"\nCORRUPTION ANALYSIS: " << valueName << std::endl;
        std::wcout << L"  Length: " << valueName.length() << L" characters" << std::endl;
        
        // Character analysis
        size_t asciiCount = 0, unicodeCount = 0, controlCount = 0, invalidCount = 0;
        
        for (size_t i = 0; i < valueName.length() && i < 100; ++i) { // Limit analysis
            wchar_t ch = valueName[i];
            
            if (ch >= 0x20 && ch <= 0x7E) {
                asciiCount++;
            } else if (ch >= 0x80 && ch <= 0xFFFF) {
                if ((ch >= 0xD800 && ch <= 0xDFFF)) {
                    invalidCount++; // Unpaired surrogate
                } else {
                    unicodeCount++;
                }
            } else if (ch < 0x20) {
                controlCount++;
                std::wcout << L"    Control char at pos " << i << L": 0x" << std::hex << (int)ch << std::endl;
            } else {
                invalidCount++;
            }
            
            // Show problematic characters
            if (ch < 0x20 || (ch >= 0xD800 && ch <= 0xDFFF) || ch == 0xFFFF) {
                std::wcout << L"    Suspicious char at pos " << i << L": 0x" << std::hex << (int)ch << std::endl;
            }
        }
        
        std::wcout << L"  Character distribution:" << std::endl;
        std::wcout << L"    ASCII:    " << asciiCount << std::endl;
        std::wcout << L"    Unicode:  " << unicodeCount << std::endl;
        std::wcout << L"    Control:  " << controlCount << std::endl;
        std::wcout << L"    Invalid:  " << invalidCount << std::endl;
        
        // Pattern analysis
        if (valueName.find(L"USB") != std::wstring::npos) {
            std::wcout << L" Contains USB-related text" << std::endl;
        }
        if (valueName.find(L"VCLAS") != std::wstring::npos) {
            std::wcout << L" Contains VCLAS pattern" << std::endl;
        }
        if (valueName.find(L"SOFTWARE") != std::wstring::npos) {
            std::wcout << L" Contains registry path fragment" << std::endl;
        }
        
        // Memory pattern analysis
        bool hasRepeatingPattern = false;
        if (valueName.length() >= 4) {
            for (size_t i = 0; i < valueName.length() - 3; ++i) {
                wchar_t pattern[4] = {valueName[i], valueName[i+1], valueName[i+2], valueName[i+3]};
                size_t count = 1;
                for (size_t j = i + 4; j < valueName.length() - 3; j += 4) {
                    if (wcsncmp(&valueName[j], pattern, 4) == 0) {
                        count++;
                    } else {
                        break;
                    }
                }
                if (count >= 3) {
                    hasRepeatingPattern = true;
                    std::wcout << L"  Repeating 4-char pattern detected (" << count << L" times)" << std::endl;
                    break;
                }
            }
        }
        
        if (!hasRepeatingPattern) {
            std::wcout << L"  No obvious repeating patterns" << std::endl;
        }
    }
};

/**
 * @brief Test reproducing the exact corruption patterns observed in production
 */
TEST_F(RegistryCorruptionTest, ReproduceObservedCorruption) {
    std::wcout << L"\n=== Reproducing Observed Corruption Patterns ===" << std::endl;
    
    // These are based on the actual corruption examples 
    std::vector<std::wstring> observedCorruptionPatterns = {
        L"USBVCLAS",
        L"USB-bfc1",
        L"潣牲灵楴", // Looks like corrupted Unicode
        L"វ榽띃였", // Mixed Unicode from different languages
        L"猳Ȳ",      // More mixed Unicode
    };
    
    // Try to reproduce similar corruption by testing edge cases
    for (const auto& pattern : observedCorruptionPatterns) {
        std::wcout << L"\n  Testing pattern: " << pattern << std::endl;
        
        // Test 1: Use the pattern as a value name
        UNICODE_STRING valueName1;
        std::wstring testName1 = L"Direct_" + pattern;
        RtlInitUnicodeString(&valueName1, testName1.c_str());
        
        DWORD testData = 0xDEADBEEF;
        NTSTATUS status1 = ZwSetValueKey(testKeyHandle, &valueName1, 0, REG_DWORD, &testData, sizeof(testData));
        std::wcout << L"    Direct use as value name: 0x" << std::hex << status1 << std::endl;
        
        // Test 2: Use the pattern as data
        UNICODE_STRING valueName2;
        std::wstring testName2 = L"AsData_" + std::to_wstring(rand());
        RtlInitUnicodeString(&valueName2, testName2.c_str());
        
        NTSTATUS status2 = ZwSetValueKey(
            testKeyHandle, 
            &valueName2, 
            0, 
            REG_SZ, 
            (PVOID)pattern.c_str(), 
            (ULONG)((pattern.length() + 1) * sizeof(WCHAR))
        );
        std::wcout << L"    Use as string data: 0x" << std::hex << status2 << std::endl;
        
        // Test 3: Concatenate with path-like strings
        std::wstring pathTest = L"SOFTWARE\\WinKernelRules\\DeviceRules\\" + pattern;
        UNICODE_STRING valueName3;
        std::wstring testName3 = L"PathTest_" + std::to_wstring(rand());
        RtlInitUnicodeString(&valueName3, testName3.c_str());
        
        NTSTATUS status3 = ZwSetValueKey(
            testKeyHandle,
            &valueName3,
            0,
            REG_SZ,
            (PVOID)pathTest.c_str(),
            (ULONG)((pathTest.length() + 1) * sizeof(WCHAR))
        );
        std::wcout << L"    Path concatenation: 0x" << std::hex << status3 << std::endl;
    }
}

/**
 * @brief Test buffer boundary conditions that might cause the observed corruption
 */
TEST_F(RegistryCorruptionTest, TestBufferBoundaryConditions) {
    std::wcout << L"\n=== Testing Buffer Boundary Conditions ===" << std::endl;
    
    // Test various buffer sizes that are commonly problematic
    std::vector<size_t> criticalSizes = {
        127, 128, 129,    // Common buffer boundaries
        255, 256, 257,    // Byte boundary
        511, 512, 513,    // Common registry buffer sizes
        1023, 1024, 1025, // KB boundary
        2047, 2048, 2049, // 2KB boundary
        4095, 4096, 4097  // Page boundary
    };
    
    for (size_t size : criticalSizes) {
        std::wcout << L"  Testing size " << size << L"..." << std::endl;
        
        // Create strings of exact size with different patterns
        std::vector<std::wstring> testStrings = {
            std::wstring(size, L'A'),                    // All same character
            std::wstring(size / 2, L'U') + std::wstring(size - size/2, L'S'), // Two-part pattern
        };
        
        // Add a string with the problematic pattern repeated
        if (size >= 8) {
            std::wstring problematicPattern = L"USBVCLAS";
            std::wstring repeatedPattern;
            while (repeatedPattern.length() < size) {
                repeatedPattern += problematicPattern;
            }
            repeatedPattern = repeatedPattern.substr(0, size);
            testStrings.push_back(repeatedPattern);
        }
        
        for (size_t patternIndex = 0; patternIndex < testStrings.size(); ++patternIndex) {
            const auto& testString = testStrings[patternIndex];
            
            UNICODE_STRING valueName;
            std::wstring valueNameStr = L"Boundary_" + std::to_wstring(size) + L"_" + std::to_wstring(patternIndex);
            RtlInitUnicodeString(&valueName, valueNameStr.c_str());
            
            // Test as string data
            NTSTATUS status = ZwSetValueKey(
                testKeyHandle,
                &valueName,
                0,
                REG_SZ,
                (PVOID)testString.c_str(),
                (ULONG)((testString.length() + 1) * sizeof(WCHAR))
            );
            
            if (!NT_SUCCESS(status)) {
                std::wcout << L" FAILED at size " << size << L", pattern " << patternIndex << L": 0x" << std::hex << status << std::endl;
            }
            
            // Try to read it back immediately
            UCHAR readBuffer[8192];
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
                    readDataChars--; // Exclude null terminator
                    
                    if (readDataChars != testString.length()) {
                        std::wcout << L"   LENGTH MISMATCH: Expected " << testString.length() << L", got " << readDataChars << std::endl;
                    } else {
                        // Compare the data
                        bool dataMatches = (wcsncmp(readData, testString.c_str(), readDataChars) == 0);
                        if (!dataMatches) {
                            std::wcout << L"   DATA CORRUPTION at size " << size << L", pattern " << patternIndex << std::endl;
                            
                            // Find first difference
                            for (size_t i = 0; i < std::min<size_t>(readDataChars, testString.length()); ++i) {
                                if (readData[i] != testString[i]) {
                                    std::wcout << L"   First diff at pos " << i << L": 0x" << std::hex << readData[i] << L" != 0x" << testString[i] << std::endl;
                                    break;
                                }
                            }
                        }
                    }
                }
            } else {
                std::wcout << L"   READ FAILED: 0x" << std::hex << readStatus << std::endl;
            }
        }
    }
}

/**
 * @brief Test memory alignment issues that might cause corruption
 */
TEST_F(RegistryCorruptionTest, TestMemoryAlignmentIssues) {
    std::wcout << L"\n=== Testing Memory Alignment Issues ===" << std::endl;
    
    // Test odd-sized buffers and unaligned access patterns
    for (int offset = 0; offset < 8; ++offset) {
        std::wcout << L"  Testing alignment offset " << offset << L"..." << std::endl;
        
        // Create a buffer with specific alignment
        size_t bufferSize = 1024 + offset;
        std::vector<BYTE> alignedBuffer(bufferSize + 16); // Extra space for alignment
        BYTE* alignedPtr = alignedBuffer.data() + offset;
        
        // Fill with a pattern
        for (size_t i = 0; i < bufferSize; ++i) {
            alignedPtr[i] = (BYTE)(0xAA + (i % 3));
        }
        
        UNICODE_STRING valueName;
        std::wstring valueNameStr = L"Alignment_" + std::to_wstring(offset);
        RtlInitUnicodeString(&valueName, valueNameStr.c_str());
        
        NTSTATUS status = ZwSetValueKey(
            testKeyHandle,
            &valueName,
            0,
            REG_BINARY,
            alignedPtr,
            (ULONG)bufferSize
        );
        
        std::wcout << L"    Write status: 0x" << std::hex << status << std::endl;
        
        if (NT_SUCCESS(status)) {
            // Read back and verify
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
                BYTE* readData = (BYTE*)valueInfo + valueInfo->DataOffset;
                
                if (valueInfo->DataLength == bufferSize) {
                    bool dataMatches = (memcmp(readData, alignedPtr, bufferSize) == 0);
                    if (!dataMatches) {
                        std::wcout << L"    ALIGNMENT CORRUPTION detected!" << std::endl;
                        
                        // Find first difference
                        for (size_t i = 0; i < bufferSize; ++i) {
                            if (readData[i] != alignedPtr[i]) {
                                std::wcout << L"    First diff at offset " << i << L": 0x" << std::hex << (int)readData[i] << L" != 0x" << (int)alignedPtr[i] << std::endl;
                                break;
                            }
                        }
                    } else {
                        std::wcout << L"   Alignment test passed" << std::endl;
                    }
                } else {
                    std::wcout << L"   SIZE MISMATCH: Expected " << bufferSize << L", got " << valueInfo->DataLength << std::endl;
                }
            }
        }
    }
}

/**
 * @brief Test for string concatenation issues that might cause path corruption
 */
TEST_F(RegistryCorruptionTest, TestStringConcatenationCorruption) {
    std::wcout << L"\n=== Testing String Concatenation Corruption ===" << std::endl;
    
    // Test scenarios that might cause the "SOFTWARE\\_TestFlt" type corruption
    std::vector<std::pair<std::wstring, std::wstring>> pathTests = {
        {L"SOFTWARE", L"WinKernelRules"},
        {L"SOFTWARE\\WinKernelRules", L"DeviceRules"},
        {L"SOFTWARE\\WinKernelRules\\DeviceRules", L"USB_DEVICE"},
        {L"HKEY_CURRENT_USER\\SOFTWARE", L"TestApp"},
        {L"\\Registry\\Machine\\SOFTWARE", L"TestKey"},
    };
    
    for (const auto& pathTest : pathTests) {
        std::wcout << L"  Testing path: '" << pathTest.first << L"' + '" << pathTest.second << L"'" << std::endl;
        
        // Method 1: Use concatenated path as value name
        std::wstring concatenated1 = pathTest.first + L"\\" + pathTest.second;
        UNICODE_STRING valueName1;
        RtlInitUnicodeString(&valueName1, concatenated1.c_str());
        
        DWORD testData = 0x12345678;
        NTSTATUS status1 = ZwSetValueKey(testKeyHandle, &valueName1, 0, REG_DWORD, &testData, sizeof(testData));
        std::wcout << L"    Concatenated as name: 0x" << std::hex << status1 << std::endl;
        
        // Method 2: Use each part as separate operations
        UNICODE_STRING valueName2;
        RtlInitUnicodeString(&valueName2, pathTest.first.c_str());
        
        NTSTATUS status2 = ZwSetValueKey(
            testKeyHandle,
            &valueName2,
            0,
            REG_SZ,
            (PVOID)pathTest.second.c_str(),
            (ULONG)((pathTest.second.length() + 1) * sizeof(WCHAR))
        );
        std::wcout << L"    Part1 as name, Part2 as data: 0x" << std::hex << status2 << std::endl;
        
        // Method 3: Test with buffer that might overflow
        std::wstring longPath = pathTest.first;
        for (int i = 0; i < 10; ++i) {
            longPath += L"\\" + pathTest.second + std::to_wstring(i);
        }
        
        if (longPath.length() < 512) { // Reasonable limit
            UNICODE_STRING valueName3;
            std::wstring testName3 = L"LongPath_" + std::to_wstring(rand() % 1000);
            RtlInitUnicodeString(&valueName3, testName3.c_str());
            
            NTSTATUS status3 = ZwSetValueKey(
                testKeyHandle,
                &valueName3,
                0,
                REG_SZ,
                (PVOID)longPath.c_str(),
                (ULONG)((longPath.length() + 1) * sizeof(WCHAR))
            );
            std::wcout << L"    Long path as data: 0x" << std::hex << status3 << std::endl;
        }
    }
}

/**
 * @brief Test for null termination and string handling issues
 */
TEST_F(RegistryCorruptionTest, TestStringTerminationIssues) {
    std::wcout << L"\n=== Testing String Termination Issues ===" << std::endl;
    
    // Test various string termination scenarios
    struct StringTest {
        std::wstring description;
        std::vector<WCHAR> data;
        ULONG dataSize;
    };
    
    std::vector<StringTest> stringTests;
    
    // Test 1: Missing null terminator
    {
        StringTest test;
        test.description = L"Missing null terminator";
        test.data = {L'T', L'E', L'S', L'T'};
        test.dataSize = static_cast<ULONG>(test.data.size() * sizeof(WCHAR));
        stringTests.push_back(test);
    }
    
    // Test 2: Multiple null terminators
    {
        StringTest test;
        test.description = L"Multiple null terminators";
        test.data = {L'T', L'E', L'S', L'T', L'\0', L'\0', L'\0'};
        test.dataSize = static_cast<ULONG>(test.data.size() * sizeof(WCHAR));
        stringTests.push_back(test);
    }
    
    // Test 3: Embedded nulls
    {
        StringTest test;
        test.description = L"Embedded nulls";
        test.data = {L'T', L'E', L'\0', L'S', L'T', L'\0'};
        test.dataSize = static_cast<ULONG>(test.data.size() * sizeof(WCHAR));
        stringTests.push_back(test);
    }
    
    // Test 4: Size mismatch (size doesn't match actual string)
    {
        StringTest test;
        test.description = L"Size larger than data";
        test.data = {L'T', L'E', L'S', L'T', L'\0'};
        test.dataSize = 1024; // Much larger than actual data
        stringTests.push_back(test);
    }
    
    for (size_t i = 0; i < stringTests.size(); ++i) {
        const auto& test = stringTests[i];
        std::wcout << L"  Testing: " << test.description << std::endl;
        
        UNICODE_STRING valueName;
        std::wstring valueNameStr = L"StringTest_" + std::to_wstring(i);
        RtlInitUnicodeString(&valueName, valueNameStr.c_str());
        
        NTSTATUS status = ZwSetValueKey(
            testKeyHandle,
            &valueName,
            0,
            REG_SZ,
            (PVOID)test.data.data(),
            test.dataSize
        );
        
        std::wcout << L"    Write status: 0x" << std::hex << status << std::endl;
        
        if (NT_SUCCESS(status)) {
            // Try to read it back
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
                WCHAR* readData = (WCHAR*)((BYTE*)valueInfo + valueInfo->DataOffset);
                
                std::wcout << L"    Read data length: " << valueInfo->DataLength << L" bytes" << std::endl;
                
                // Check for potential corruption
                if (valueInfo->DataLength > test.dataSize + 100) { // Allow some tolerance
                    std::wcout << L"    EXCESSIVE SIZE: Read " << valueInfo->DataLength << L", expected ~" << test.dataSize << std::endl;
                }
                
                // Check for reasonable string length
                size_t readStringLen = wcsnlen(readData, valueInfo->DataLength / sizeof(WCHAR));
                std::wcout << L"    String length: " << readStringLen << L" characters" << std::endl;
                
                if (readStringLen > 1000) {
                    std::wcout << L"    SUSPICIOUSLY LONG STRING" << std::endl;
                }
            } else {
                std::wcout << L"    Read failed: 0x" << std::hex << readStatus << std::endl;
            }
        }
    }
}
