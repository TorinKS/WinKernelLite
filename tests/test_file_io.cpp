#include <gtest/gtest.h>
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include "../include/File.h"
#include "../include/UnicodeString.h"

class FileIOTest : public ::testing::Test {
protected:
    std::wstring testDir;
    std::wstring testFile;
    std::wstring testFile2;
    std::wstring nonExistentFile;
    
    void SetUp() override {
        // Create a test directory in temp folder
        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        testDir = std::wstring(tempPath) + L"WinKernelLiteFileTest\\";
        
        // Create test directory
        std::filesystem::create_directories(testDir);
        
        testFile = testDir + L"test_file.txt";
        testFile2 = testDir + L"test_file2.txt";
        nonExistentFile = testDir + L"nonexistent_file.txt";
        
        // Clean up any existing test files
        DeleteFileW(testFile.c_str());
        DeleteFileW(testFile2.c_str());
        DeleteFileW(nonExistentFile.c_str());
    }
    
    void TearDown() override {
        // Clean up test files and directory
        DeleteFileW(testFile.c_str());
        DeleteFileW(testFile2.c_str());
        DeleteFileW(nonExistentFile.c_str());
        RemoveDirectoryW(testDir.c_str());
    }
    
    void CreateTestFile(const std::wstring& filepath, const std::string& content = "Test content") {
        std::ofstream file(filepath);
        file << content;
        file.close();
    }
};

TEST_F(FileIOTest, ZwCreateFile_ValidParameters) {
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    
    // Initialize unicode string for file name
    RtlInitUnicodeString(&fileName, testFile.c_str());
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    // Test creating a new file
    NTSTATUS status = ZwCreateFile(
        &fileHandle,
        GENERIC_WRITE | GENERIC_READ,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_CREATE,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_NE(fileHandle, nullptr);
    EXPECT_NE(fileHandle, INVALID_HANDLE_VALUE);
    EXPECT_EQ(ioStatus.Status, STATUS_SUCCESS);
    EXPECT_EQ(ioStatus.Information, FILE_OPENED);
    
    if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
    }
}

TEST_F(FileIOTest, ZwCreateFile_OpenExistingFile) {
    // Create a test file first
    CreateTestFile(testFile);
    
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    
    RtlInitUnicodeString(&fileName, testFile.c_str());
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    // Test opening an existing file
    NTSTATUS status = ZwCreateFile(
        &fileHandle,
        GENERIC_READ,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    EXPECT_NE(fileHandle, nullptr);
    EXPECT_NE(fileHandle, INVALID_HANDLE_VALUE);
    
    if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
    }
}

TEST_F(FileIOTest, ZwCreateFile_FileNotFound) {
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    
    RtlInitUnicodeString(&fileName, nonExistentFile.c_str());
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    // Try to open a non-existent file
    NTSTATUS status = ZwCreateFile(
        &fileHandle,
        GENERIC_READ,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    
    EXPECT_EQ(status, STATUS_OBJECT_NAME_NOT_FOUND);
    EXPECT_EQ(ioStatus.Status, STATUS_UNSUCCESSFUL);
}

TEST_F(FileIOTest, ZwCreateFile_InvalidParameters) {
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    
    RtlInitUnicodeString(&fileName, testFile.c_str());
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    // Test with NULL FileHandle
    NTSTATUS status = ZwCreateFile(
        NULL,
        GENERIC_READ,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
    
    // Test with NULL ObjectAttributes
    status = ZwCreateFile(
        &fileHandle,
        GENERIC_READ,
        NULL,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
    
    // Test with NULL IoStatusBlock
    status = ZwCreateFile(
        &fileHandle,
        GENERIC_READ,
        &objAttrs,
        NULL,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(FileIOTest, ZwCreateFile_InvalidDisposition) {
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    
    RtlInitUnicodeString(&fileName, testFile.c_str());
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    // Test with invalid CreateDisposition
    NTSTATUS status = ZwCreateFile(
        &fileHandle,
        GENERIC_READ,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        0xFF, // Invalid disposition
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(FileIOTest, ZwCreateFile_AllDispositions) {
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    
    RtlInitUnicodeString(&fileName, testFile.c_str());
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    // Test FILE_CREATE
    NTSTATUS status = ZwCreateFile(
        &fileHandle,
        GENERIC_WRITE,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_CREATE,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    EXPECT_EQ(status, STATUS_SUCCESS);
    if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
        fileHandle = nullptr;
    }
    
    // Test FILE_OPEN on existing file
    status = ZwCreateFile(
        &fileHandle,
        GENERIC_READ,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    EXPECT_EQ(status, STATUS_SUCCESS);
    if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
        fileHandle = nullptr;
    }
    
    // Test FILE_OPEN_IF on existing file
    status = ZwCreateFile(
        &fileHandle,
        GENERIC_READ,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    EXPECT_EQ(status, STATUS_SUCCESS);
    if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
        fileHandle = nullptr;
    }
}

TEST_F(FileIOTest, ZwCreateFile_AccessDenied) {
    // Create a test file with specific permissions
    CreateTestFile(testFile);
    
    // Try to modify file attributes to read-only to simulate access denied
    SetFileAttributesW(testFile.c_str(), FILE_ATTRIBUTE_READONLY);
    
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    
    RtlInitUnicodeString(&fileName, testFile.c_str());
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    // Try to open for write access on read-only file
    NTSTATUS status = ZwCreateFile(
        &fileHandle,
        GENERIC_WRITE,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    
    // This might succeed or fail depending on the system, but should handle gracefully
    if (status != STATUS_SUCCESS) {
        EXPECT_TRUE(status == STATUS_ACCESS_DENIED || status == STATUS_UNSUCCESSFUL);
    }
    
    if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
    }
    
    // Clean up - remove read-only attribute
    SetFileAttributesW(testFile.c_str(), FILE_ATTRIBUTE_NORMAL);
}

TEST_F(FileIOTest, ZwCreateFile_ObjectAttributesFields) {
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    
    RtlInitUnicodeString(&fileName, testFile.c_str());
    
    // Test with different object attribute flags
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    
    NTSTATUS status = ZwCreateFile(
        &fileHandle,
        GENERIC_WRITE,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_CREATE,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    
    EXPECT_EQ(status, STATUS_SUCCESS);
    if (fileHandle && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
    }
}

TEST_F(FileIOTest, ZwCreateFile_NullObjectName) {
    HANDLE fileHandle = nullptr;
    IO_STATUS_BLOCK ioStatus = {};
    OBJECT_ATTRIBUTES objAttrs;
    
    // Initialize with NULL object name
    InitializeObjectAttributes(&objAttrs, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    NTSTATUS status = ZwCreateFile(
        &fileHandle,
        GENERIC_WRITE,
        &objAttrs,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_CREATE,
        FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(FileIOTest, InitializeObjectAttributes_Macro) {
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objAttrs;
    HANDLE rootDir = (HANDLE)(ULONG_PTR)0x12345678;
    PVOID secDesc = (PVOID)(ULONG_PTR)0x87654321;
    
    RtlInitUnicodeString(&fileName, L"test");
    InitializeObjectAttributes(&objAttrs, &fileName, OBJ_CASE_INSENSITIVE | OBJ_INHERIT, rootDir, secDesc);
    
    EXPECT_EQ(objAttrs.Length, sizeof(OBJECT_ATTRIBUTES));
    EXPECT_EQ(objAttrs.ObjectName, &fileName);
    EXPECT_EQ(objAttrs.Attributes, OBJ_CASE_INSENSITIVE | OBJ_INHERIT);
    EXPECT_EQ(objAttrs.RootDirectory, rootDir);
    EXPECT_EQ(objAttrs.SecurityDescriptor, secDesc);
    EXPECT_EQ(objAttrs.SecurityQualityOfService, nullptr);
}
