/*
 * Copyright 2025 WinKernelLite Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WINKERNEL_FILE_H
#define WINKERNEL_FILE_H

#include <Windows.h>
#include <WinKernelLite/UnicodeString.h>
#include <WinKernelLite/NtStatus.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IO_STATUS_BLOCK structure
 */
typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

/**
 * @brief Object attributes structure
 */
typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

/**
 * @brief Initialize OBJECT_ATTRIBUTES structure
 */
#define InitializeObjectAttributes(p, n, a, r, s) \
    { \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
        (p)->RootDirectory = (r); \
        (p)->Attributes = (a); \
        (p)->ObjectName = (n); \
        (p)->SecurityDescriptor = (s); \
        (p)->SecurityQualityOfService = NULL; \
    }

#define OBJ_INHERIT                         0x00000002L
#define OBJ_PERMANENT                       0x00000010L
#define OBJ_EXCLUSIVE                       0x00000020L
#define OBJ_CASE_INSENSITIVE                0x00000040L
#define OBJ_OPENIF                          0x00000080L
#define OBJ_OPENLINK                        0x00000100L
#define OBJ_KERNEL_HANDLE                   0x00000200L
#define OBJ_FORCE_ACCESS_CHECK              0x00000400L
#define OBJ_VALID_ATTRIBUTES                0x000007F2L

#define FILE_SUPERSEDE                      0x00000000
#define FILE_OPEN                           0x00000001
#define FILE_CREATE                         0x00000002
#define FILE_OPEN_IF                        0x00000003
#define FILE_OVERWRITE                      0x00000004
#define FILE_OVERWRITE_IF                   0x00000005
#define FILE_MAXIMUM_DISPOSITION            0x00000005

#define FILE_DIRECTORY_FILE                 0x00000001
#define FILE_WRITE_THROUGH                  0x00000002
#define FILE_SEQUENTIAL_ONLY                0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING      0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT           0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT        0x00000020
#define FILE_NON_DIRECTORY_FILE             0x00000040
#define FILE_CREATE_TREE_CONNECTION         0x00000080
#define FILE_COMPLETE_IF_OPLOCKED           0x00000100
#define FILE_NO_EA_KNOWLEDGE                0x00000200
#define FILE_OPEN_FOR_RECOVERY              0x00000400
#define FILE_RANDOM_ACCESS                  0x00000800
#define FILE_DELETE_ON_CLOSE                0x00001000
#define FILE_OPEN_BY_FILE_ID                0x00000002
#define FILE_OPEN_FOR_BACKUP_INTENT         0x00004000
#define FILE_NO_COMPRESSION                 0x00008000
#define FILE_RESERVE_OPFILTER               0x00100000
#define FILE_OPEN_REPARSE_POINT             0x00200000
#define FILE_OPEN_NO_RECALL                 0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY      0x00800000

// Information values for IoStatusBlock
#define FILE_SUPERSEDED                     0x00000000
#define FILE_OPENED                         0x00000001
#define FILE_CREATED                        0x00000002
#define FILE_OVERWRITTEN                    0x00000003
#define FILE_EXISTS                         0x00000004
#define FILE_DOES_NOT_EXIST                 0x00000005

/**
 * @brief User mode implementation of ZwCreateFile
 * Creates or opens a file or device
 * 
 * @param FileHandle Pointer to receive the handle to the file
 * @param DesiredAccess The access mask for the file
 * @param ObjectAttributes Attributes for the file
 * @param IoStatusBlock Receives the completion status and information
 * @param AllocationSize Initial allocation size (optional)
 * @param FileAttributes File attributes (FILE_ATTRIBUTE_*)
 * @param ShareAccess File sharing options
 * @param CreateDisposition How to create or open the file
 * @param CreateOptions Options for creating or opening the file
 * @param EaBuffer Extended attributes buffer (not supported in this implementation)
 * @param EaLength Length of the extended attributes buffer
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ZwCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
);



#ifdef __cplusplus
}
#endif

// Implementation of user mode file functions

inline NTSTATUS ZwCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
)
{
    // Validate parameters
    if (!FileHandle || !ObjectAttributes || !ObjectAttributes->ObjectName || !IoStatusBlock) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // These parameters are not used in this implementation
    UNREFERENCED_PARAMETER(AllocationSize);
    UNREFERENCED_PARAMETER(EaBuffer);
    UNREFERENCED_PARAMETER(EaLength);
    
    // Convert NT CreateDisposition to Win32 CreateDisposition
    DWORD dwCreationDisposition;
    switch (CreateDisposition) {
        case FILE_SUPERSEDE:
        case FILE_OVERWRITE_IF:
            dwCreationDisposition = CREATE_ALWAYS;
            break;
        case FILE_CREATE:
            dwCreationDisposition = CREATE_NEW;
            break;
        case FILE_OPEN:
            dwCreationDisposition = OPEN_EXISTING;
            break;
        case FILE_OPEN_IF:
            dwCreationDisposition = OPEN_ALWAYS;
            break;
        case FILE_OVERWRITE:
            dwCreationDisposition = TRUNCATE_EXISTING;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
    }
    
    // Create file with CreateFileW
    *FileHandle = CreateFileW(
        ObjectAttributes->ObjectName->Buffer,
        DesiredAccess,
        ShareAccess,
        NULL, // Security attributes not supported
        dwCreationDisposition,
        FileAttributes | (CreateOptions & 0x00FFFFFF), // Convert relevant options
        NULL  // Template file not supported
    );
    
    if (*FileHandle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        IoStatusBlock->Status = STATUS_UNSUCCESSFUL;
        
        if (error == ERROR_FILE_NOT_FOUND) {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        } else if (error == ERROR_ACCESS_DENIED) {
            return STATUS_ACCESS_DENIED;
        } else {
            return STATUS_UNSUCCESSFUL;
        }
    }
    
    IoStatusBlock->Status = STATUS_SUCCESS;
    IoStatusBlock->Information = FILE_OPENED; // Simplified - real implementation would have more accurate information
      return STATUS_SUCCESS;
}



#endif // WINKERNEL_FILE_H
