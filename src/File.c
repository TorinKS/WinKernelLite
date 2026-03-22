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

#include "include/File.h"

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
)
{
    DWORD dwCreationDisposition;

    /* Validate parameters */
    if (!FileHandle || !ObjectAttributes || !ObjectAttributes->ObjectName || !IoStatusBlock) {
        return STATUS_INVALID_PARAMETER;
    }

    UNREFERENCED_PARAMETER(AllocationSize);
    UNREFERENCED_PARAMETER(EaBuffer);
    UNREFERENCED_PARAMETER(EaLength);

    /* Convert NT CreateDisposition to Win32 CreateDisposition */
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

    /* Create file with CreateFileW */
    *FileHandle = CreateFileW(
        ObjectAttributes->ObjectName->Buffer,
        DesiredAccess,
        ShareAccess,
        NULL, /* Security attributes not supported */
        dwCreationDisposition,
        FileAttributes | (CreateOptions & 0x00FFFFFF), /* Convert relevant options */
        NULL  /* Template file not supported */
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
    IoStatusBlock->Information = FILE_OPENED; /* Simplified */
    return STATUS_SUCCESS;
}
