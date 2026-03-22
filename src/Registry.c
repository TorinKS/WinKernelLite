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

#include "include/Registry.h"

NTSTATUS ZwEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
)
{
    if (!KeyHandle) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!KeyInformation) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ResultLength) {
        return STATUS_INVALID_PARAMETER;
    }

    WCHAR tempKeyName[1024];
    DWORD keyNameSize = sizeof(tempKeyName) / sizeof(WCHAR);
    FILETIME lastWriteTime;

    LSTATUS status = RegEnumKeyExW(
        (HKEY)KeyHandle,
        Index,
        tempKeyName,
        &keyNameSize,
        NULL,           // Reserved
        NULL,           // Class name (not used)
        NULL,           // Class name length
        &lastWriteTime  // Last write time
    );

    // Convert the result to the requested information format
    if (status == ERROR_SUCCESS) {
        ULONG nameLength = (ULONG)(keyNameSize * sizeof(WCHAR));
        ULONG requiredSize = 0;

        switch (KeyInformationClass) {
            case KeyBasicInformation: {
                requiredSize = sizeof(KEY_BASIC_INFORMATION) + nameLength;
                if (Length < requiredSize) {
                    if (ResultLength) *ResultLength = requiredSize;
                    return STATUS_BUFFER_TOO_SMALL;
                }

                PKEY_BASIC_INFORMATION basicInfo = (PKEY_BASIC_INFORMATION)KeyInformation;
                basicInfo->LastWriteTime.QuadPart = ((LARGE_INTEGER*)&lastWriteTime)->QuadPart;
                basicInfo->TitleIndex = 0;
                basicInfo->NameLength = nameLength;
                memcpy_s(basicInfo->Name, nameLength, tempKeyName, nameLength);

                if (ResultLength) *ResultLength = requiredSize;
                break;
            }

            case KeyNodeInformation: {
                requiredSize = sizeof(KEY_NODE_INFORMATION) + nameLength;
                if (Length < requiredSize) {
                    if (ResultLength) *ResultLength = requiredSize;
                    return STATUS_BUFFER_TOO_SMALL;
                }

                PKEY_NODE_INFORMATION nodeInfo = (PKEY_NODE_INFORMATION)KeyInformation;
                nodeInfo->LastWriteTime.QuadPart = ((LARGE_INTEGER*)&lastWriteTime)->QuadPart;
                nodeInfo->TitleIndex = 0;
                nodeInfo->ClassLength = 0;
                nodeInfo->ClassOffset = 0;
                nodeInfo->NameLength = nameLength;
                memcpy_s(nodeInfo->Name, nameLength, tempKeyName, nameLength);

                if (ResultLength) *ResultLength = requiredSize;
                break;
            }

            case KeyFullInformation:
            default:
                // Unsupported information class
                return STATUS_INVALID_PARAMETER;
        }

        return STATUS_SUCCESS;
    } else if (status == ERROR_NO_MORE_ITEMS) {
        return STATUS_NO_MORE_ENTRIES;
    } else if (status == ERROR_MORE_DATA) {
        return STATUS_BUFFER_TOO_SMALL;
    } else {
        return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS ZwClose(
    IN HANDLE Handle
)
{
    // Check for obviously invalid handles upfront
    if (Handle == NULL || Handle == INVALID_HANDLE_VALUE) {
        return STATUS_INVALID_PARAMETER;
    }

    // Check if the handle is valid using GetHandleInformation
    DWORD flags = 0;
    if (!GetHandleInformation(Handle, &flags)) {
        return STATUS_INVALID_HANDLE;
    }

    // Handle is valid, so close it
    if (CloseHandle(Handle)) {
        return STATUS_SUCCESS;
    } else {
        // This should be unreachable if GetHandleInformation succeeded
        return STATUS_INVALID_HANDLE;
    }
}

/**
 * @brief Helper function to parse a full registry path into root key and subkey path
 *
 * @param fullPath The full registry path (e.g., L"\\Registry\\Machine\\Software\\Test")
 * @param rootKey Pointer to receive the root key handle
 * @param subKeyPath Pointer to receive the subkey path
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ParseRegistryPath(
    IN PCWSTR fullPath,
    OUT HKEY* rootKey,
    OUT PCWSTR* subKeyPath
)
{
    if (!fullPath || !rootKey || !subKeyPath) {
        return STATUS_INVALID_PARAMETER;
    }

    // Check for known registry path prefixes
    if (wcsncmp(fullPath, L"\\Registry\\Machine\\", 18) == 0) {
        *rootKey = HKEY_LOCAL_MACHINE;
        *subKeyPath = fullPath + 18; // Skip "\\Registry\\Machine\\"
        return STATUS_SUCCESS;
    } else if (wcsncmp(fullPath, L"\\Registry\\User\\", 15) == 0) {
        *rootKey = HKEY_USERS;
        *subKeyPath = fullPath + 15; // Skip "\\Registry\\User\\"
        return STATUS_SUCCESS;
    } else if (wcsncmp(fullPath, L"HKEY_LOCAL_MACHINE\\", 19) == 0) {
        *rootKey = HKEY_LOCAL_MACHINE;
        *subKeyPath = fullPath + 19; // Skip "HKEY_LOCAL_MACHINE\\"
        return STATUS_SUCCESS;
    } else if (wcsncmp(fullPath, L"HKEY_CURRENT_USER\\", 18) == 0) {
        *rootKey = HKEY_CURRENT_USER;
        *subKeyPath = fullPath + 18; // Skip "HKEY_CURRENT_USER\\"
        return STATUS_SUCCESS;
    } else if (wcsncmp(fullPath, L"HKEY_USERS\\", 11) == 0) {
        *rootKey = HKEY_USERS;
        *subKeyPath = fullPath + 11; // Skip "HKEY_USERS\\"
        return STATUS_SUCCESS;
    } else if (wcsncmp(fullPath, L"HKEY_CLASSES_ROOT\\", 18) == 0) {
        *rootKey = HKEY_CLASSES_ROOT;
        *subKeyPath = fullPath + 18; // Skip "HKEY_CLASSES_ROOT\\"
        return STATUS_SUCCESS;
    }

    // Unknown registry path format
    return STATUS_OBJECT_PATH_SYNTAX_BAD;
}

NTSTATUS ZwCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
)
{
    // Validate parameters
    UNREFERENCED_PARAMETER(TitleIndex); // Mark as unused parameter to fix C4100 warning

    if (!KeyHandle) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ObjectAttributes) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ObjectAttributes->ObjectName) {
        return STATUS_INVALID_PARAMETER;
    }

    HKEY rootKey = NULL;
    PCWSTR keyPath = NULL;
    DWORD dispositionValue = 0;
    LSTATUS status;
    NTSTATUS ntStatus;

    // Get root key and subkey path from ObjectAttributes
    if (ObjectAttributes->RootDirectory) {
        // Root directory provided - use it with relative path
        rootKey = (HKEY)ObjectAttributes->RootDirectory;
        keyPath = ObjectAttributes->ObjectName->Buffer;
    } else {
        // No root directory - parse full path from ObjectName
        ntStatus = ParseRegistryPath(
            ObjectAttributes->ObjectName->Buffer,
            &rootKey,
            &keyPath
        );
        if (!NT_SUCCESS(ntStatus)) {
            return ntStatus;
        }
    }

    status = RegCreateKeyExW(
        rootKey,
        keyPath,
        0,               // Reserved
        Class ? Class->Buffer : NULL, // Class
        CreateOptions,   // Options
        DesiredAccess,   // Access
        NULL,            // Security attributes not supported
        (PHKEY)KeyHandle,
        &dispositionValue
    );

    // Pass back the disposition if requested
    if (Disposition) {
        *Disposition = dispositionValue;
    }

    // Convert Windows error to NTSTATUS
    if (status == ERROR_SUCCESS) {
        return STATUS_SUCCESS;
    } else if (status == ERROR_ACCESS_DENIED) {
        return STATUS_ACCESS_DENIED;
    } else if (status == ERROR_INVALID_PARAMETER) {
        return STATUS_INVALID_PARAMETER;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS ZwSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
)
{
    // Validate parameters
    UNREFERENCED_PARAMETER(TitleIndex); // Mark as unused parameter to fix C4100 warning

    if (!KeyHandle) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ValueName) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ValueName->Buffer) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!Data && DataSize > 0) {
        return STATUS_INVALID_PARAMETER;
    }

	// avoid potential problems with the strings not being null-terminated as UNICODE_STRING is not required to have null terminated buffer
	// and we don't know how exactly this works in the Win kernel
    ULONG nameChars = ValueName->Length / sizeof(WCHAR);
    WCHAR* nullTerminatedName = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (nameChars + 1) * sizeof(WCHAR));
    if (!nullTerminatedName) {
        return STATUS_NO_MEMORY;
    }

    // Copy the name and add null terminator
    memcpy_s(nullTerminatedName, ValueName->Length, ValueName->Buffer, ValueName->Length);
    nullTerminatedName[nameChars] = L'\0';

    LSTATUS status = RegSetValueExW(
        (HKEY)KeyHandle,
        nullTerminatedName,
        0,           // Reserved
        Type,
        (CONST BYTE*)Data,
        DataSize
    );

    HeapFree(GetProcessHeap(), 0, nullTerminatedName);

    if (status == ERROR_SUCCESS) {
        return STATUS_SUCCESS;
    } else if (status == ERROR_ACCESS_DENIED) {
        return STATUS_ACCESS_DENIED;
    } else if (status == ERROR_INVALID_PARAMETER) {
        return STATUS_INVALID_PARAMETER;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS ZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
    // Validate parameters
    if (!KeyHandle) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ObjectAttributes) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ObjectAttributes->ObjectName) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ObjectAttributes->ObjectName->Buffer) {
        return STATUS_INVALID_PARAMETER;
    }

    HKEY rootKey = NULL;
    PCWSTR keyPath = NULL;
    LSTATUS status;
    NTSTATUS ntStatus;

    // Get root key and subkey path from ObjectAttributes
    if (ObjectAttributes->RootDirectory) {
        // Root directory provided - use it with relative path
        rootKey = (HKEY)ObjectAttributes->RootDirectory;
        keyPath = ObjectAttributes->ObjectName->Buffer;
    } else {
        // No root directory - parse full path from ObjectName or assume HKEY_CURRENT_USER
        ntStatus = ParseRegistryPath(
            ObjectAttributes->ObjectName->Buffer,
            &rootKey,
            &keyPath
        );
        if (!NT_SUCCESS(ntStatus)) {
            // If path parsing fails, assume it's a relative path under HKEY_CURRENT_USER
            rootKey = HKEY_CURRENT_USER;
            keyPath = ObjectAttributes->ObjectName->Buffer;
        }
    }

    status = RegOpenKeyExW(
        rootKey,
        keyPath,
        0,               // Reserved
        DesiredAccess,   // Access
        (PHKEY)KeyHandle
    );

    // Convert Windows error to NTSTATUS
    if (status == ERROR_SUCCESS) {
        return STATUS_SUCCESS;
    } else if (status == ERROR_FILE_NOT_FOUND) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    } else if (status == ERROR_PATH_NOT_FOUND) {
        return STATUS_OBJECT_PATH_NOT_FOUND;
    } else if (status == ERROR_ACCESS_DENIED) {
        return STATUS_ACCESS_DENIED;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS ZwQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
)
{
    if (!KeyHandle) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ValueName) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ValueName->Buffer) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!KeyValueInformation) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ResultLength) {
        return STATUS_INVALID_PARAMETER;
    }

    DWORD dwType;
    DWORD dwDataSize = 0;
    LSTATUS status;

    switch (KeyValueInformationClass) {
        case KeyValuePartialInformation:
        {
            PKEY_VALUE_PARTIAL_INFORMATION partialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueInformation;

            // First call to get required buffer size
            status = RegQueryValueExW(
                (HKEY)KeyHandle,
                ValueName->Buffer,
                NULL,
                &dwType,
                NULL,
                &dwDataSize
            );

            if (status != ERROR_SUCCESS && status != ERROR_MORE_DATA) {
                if (status == ERROR_FILE_NOT_FOUND) {
                    return STATUS_OBJECT_NAME_NOT_FOUND;
                } else {
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Calculate required size for the structure
            ULONG requiredSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + dwDataSize - sizeof(BYTE);
            *ResultLength = requiredSize;

            if (Length < requiredSize) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            // Setup the structure fields
            partialInfo->TitleIndex = 0;
            partialInfo->Type = dwType;
            partialInfo->DataLength = dwDataSize;

            status = RegQueryValueExW(
                (HKEY)KeyHandle,
                ValueName->Buffer,
                NULL,
                &dwType,
                (LPBYTE)partialInfo->Data,
                &dwDataSize
            );

            if (status == ERROR_SUCCESS) {
                return STATUS_SUCCESS;
            } else if (status == ERROR_FILE_NOT_FOUND) {
                return STATUS_OBJECT_NAME_NOT_FOUND;
            } else if (status == ERROR_MORE_DATA) {
                return STATUS_BUFFER_TOO_SMALL;
            } else {
                return STATUS_UNSUCCESSFUL;
            }
        }

        case KeyValueFullInformation:
        {
            PKEY_VALUE_FULL_INFORMATION fullInfo = (PKEY_VALUE_FULL_INFORMATION)KeyValueInformation;
            // First call to get required buffer size
            status = RegQueryValueExW(
                (HKEY)KeyHandle,
                ValueName->Buffer,
                NULL,
                &dwType,
                NULL,
                &dwDataSize
            );

            if (status != ERROR_SUCCESS && status != ERROR_MORE_DATA) {
                if (status == ERROR_FILE_NOT_FOUND) {
                    return STATUS_OBJECT_NAME_NOT_FOUND;
                } else if (status == ERROR_MORE_DATA) {
                    return STATUS_BUFFER_TOO_SMALL;
                } else {
                    return STATUS_UNSUCCESSFUL;
                }
            }

            if (Length < sizeof(KEY_VALUE_FULL_INFORMATION)) {
                *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION) + dwDataSize;
                return STATUS_BUFFER_TOO_SMALL;
            }

            // Setup the structure fields
            fullInfo->TitleIndex = 0;
            fullInfo->Type = dwType;
            fullInfo->NameLength = ValueName->Length;
            fullInfo->DataLength = dwDataSize;
            fullInfo->DataOffset = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) +
                                 ValueName->Length + sizeof(WCHAR);

            // Make sure we have enough space for name and data
            if (Length < fullInfo->DataOffset + dwDataSize) {
                *ResultLength = fullInfo->DataOffset + dwDataSize;
                return STATUS_BUFFER_TOO_SMALL;
            }

            // Copy the name
            memcpy_s(fullInfo->Name, ValueName->Length + sizeof(WCHAR), ValueName->Buffer, ValueName->Length);
            ((PWSTR)((PBYTE)fullInfo->Name + ValueName->Length))[0] = L'\0';


            status = RegQueryValueExW(
                (HKEY)KeyHandle,
                ValueName->Buffer,
                NULL,
                &dwType,
                (LPBYTE)((PBYTE)KeyValueInformation + fullInfo->DataOffset),
                &dwDataSize
            );

            *ResultLength = fullInfo->DataOffset + dwDataSize;

            if (status == ERROR_SUCCESS) {
                return STATUS_SUCCESS;
            } else if (status == ERROR_FILE_NOT_FOUND) {
                return STATUS_OBJECT_NAME_NOT_FOUND;
            } else if (status == ERROR_MORE_DATA) {
                return STATUS_BUFFER_TOO_SMALL;
            } else {
                return STATUS_UNSUCCESSFUL;
            }
        }

        case KeyValueBasicInformation:
        {
            PKEY_VALUE_BASIC_INFORMATION basicInfo = (PKEY_VALUE_BASIC_INFORMATION)KeyValueInformation;

            // First call to get required buffer size and type
            status = RegQueryValueExW(
                (HKEY)KeyHandle,
                ValueName->Buffer,
                NULL,
                &dwType,
                NULL,
                &dwDataSize
            );

            if (status != ERROR_SUCCESS && status != ERROR_MORE_DATA) {
                if (status == ERROR_FILE_NOT_FOUND) {
                    return STATUS_OBJECT_NAME_NOT_FOUND;
                } else {
                    return STATUS_UNSUCCESSFUL;
                }
            }

            // Calculate required size for the structure
            ULONG requiredSize = sizeof(KEY_VALUE_BASIC_INFORMATION) + ValueName->Length;
            *ResultLength = requiredSize;

            if (Length < requiredSize) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            // Setup the structure fields
            basicInfo->TitleIndex = 0;
            basicInfo->Type = dwType;
            basicInfo->NameLength = ValueName->Length;

            // Copy the name
            memcpy_s(basicInfo->Name, ValueName->Length, ValueName->Buffer, ValueName->Length);

            return STATUS_SUCCESS;
        }

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS ZwEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
)
{
    if (!KeyHandle) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!KeyValueInformation) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!ResultLength) {
        return STATUS_INVALID_PARAMETER;
    }

    // Status variable is not used in this function but kept for consistency with original API
    // NTSTATUS status = STATUS_SUCCESS;
    HKEY hKey = (HKEY)KeyHandle;
    DWORD maxValueNameLen = 0, maxValueDataLen = 0;
    WCHAR valueName[MAX_PATH] = { 0 };
    DWORD valueNameLen = MAX_PATH;
    DWORD type = 0;
    BYTE* data = NULL;
    DWORD dataSize = 0;

    // Get value name and data size
    LSTATUS winStatus = RegQueryInfoKeyW(
        hKey,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &maxValueNameLen,
        &maxValueDataLen,
        NULL,
        NULL
    );
    if (winStatus != ERROR_SUCCESS) {
        return winStatus == ERROR_ACCESS_DENIED ? STATUS_ACCESS_DENIED : STATUS_UNSUCCESSFUL;
    }

    // Get value name, type and data
    valueNameLen = MAX_PATH; // Reset length before calling RegEnumValueW
    winStatus = RegEnumValueW(
        hKey,
        Index,
        valueName,
        &valueNameLen,
        NULL,
        &type,
        NULL,
        &dataSize
    );
    if (winStatus != ERROR_SUCCESS) {
        return winStatus == ERROR_NO_MORE_ITEMS ? STATUS_NO_MORE_ENTRIES :
               winStatus == ERROR_MORE_DATA ? STATUS_BUFFER_TOO_SMALL :
               winStatus == ERROR_ACCESS_DENIED ? STATUS_ACCESS_DENIED : STATUS_UNSUCCESSFUL;
    }

    // Fill in the information based on the requested class
    switch (KeyValueInformationClass) {
        case KeyValueBasicInformation: {
            PKEY_VALUE_BASIC_INFORMATION pBasic = (PKEY_VALUE_BASIC_INFORMATION)KeyValueInformation;
            size_t requiredSize = sizeof(KEY_VALUE_BASIC_INFORMATION) + (valueNameLen * sizeof(WCHAR));
            *ResultLength = (ULONG)requiredSize;

            if (Length < requiredSize) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            pBasic->TitleIndex = 0;
            pBasic->Type = type;
            pBasic->NameLength = valueNameLen * sizeof(WCHAR);
            memcpy_s(pBasic->Name, pBasic->NameLength, valueName, pBasic->NameLength);
            break;
        }
        case KeyValueFullInformation: {
            PKEY_VALUE_FULL_INFORMATION pFull = (PKEY_VALUE_FULL_INFORMATION)KeyValueInformation;
            size_t requiredSize = sizeof(KEY_VALUE_FULL_INFORMATION) + (valueNameLen * sizeof(WCHAR)) + dataSize;
            *ResultLength = (ULONG)requiredSize;

            if (Length < requiredSize) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            data = (BYTE*)HeapAlloc(GetProcessHeap(), 0, dataSize);
            if (!data) {
                return STATUS_NO_MEMORY;
            }

            DWORD actualDataSize = dataSize;
            valueNameLen = MAX_PATH; // Reset length before calling RegEnumValueW
            LSTATUS valueStatus = RegEnumValueW(
                hKey,
                Index,
                valueName,
                &valueNameLen,
                NULL,
                &type,
                data,
                &actualDataSize
            );
            if (valueStatus != ERROR_SUCCESS) {
                HeapFree(GetProcessHeap(), 0, data);
                return valueStatus == ERROR_NO_MORE_ITEMS ? STATUS_NO_MORE_ENTRIES :
                       valueStatus == ERROR_MORE_DATA ? STATUS_BUFFER_TOO_SMALL :
                       valueStatus == ERROR_ACCESS_DENIED ? STATUS_ACCESS_DENIED : STATUS_UNSUCCESSFUL;
            }

            pFull->TitleIndex = 0;
            pFull->Type = type;
            pFull->DataOffset = sizeof(KEY_VALUE_FULL_INFORMATION) + (valueNameLen * sizeof(WCHAR));
            pFull->DataLength = dataSize;
            pFull->NameLength = valueNameLen * sizeof(WCHAR);
            memcpy_s(pFull->Name, pFull->NameLength, valueName, pFull->NameLength);
            memcpy_s((BYTE*)pFull + pFull->DataOffset, dataSize, data, dataSize);
            HeapFree(GetProcessHeap(), 0, data);
            break;
        }
        case KeyValuePartialInformation: {
            PKEY_VALUE_PARTIAL_INFORMATION pPartial = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueInformation;
            size_t requiredSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + dataSize;
            *ResultLength = (ULONG)requiredSize;

            if (Length < requiredSize) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            data = (BYTE*)HeapAlloc(GetProcessHeap(), 0, dataSize);
            if (!data) {
                return STATUS_NO_MEMORY;
            }

            DWORD actualDataSize = dataSize;
            valueNameLen = MAX_PATH; // Reset length before calling RegEnumValueW
            LSTATUS valueStatus = RegEnumValueW(
                hKey,
                Index,
                valueName,
                &valueNameLen,
                NULL,
                &type,
                data,
                &actualDataSize
            );
            if (valueStatus != ERROR_SUCCESS) {
                HeapFree(GetProcessHeap(), 0, data);
                return valueStatus == ERROR_NO_MORE_ITEMS ? STATUS_NO_MORE_ENTRIES :
                       valueStatus == ERROR_MORE_DATA ? STATUS_BUFFER_TOO_SMALL :
                       valueStatus == ERROR_ACCESS_DENIED ? STATUS_ACCESS_DENIED : STATUS_UNSUCCESSFUL;
            }

            pPartial->TitleIndex = 0;
            pPartial->Type = type;
            pPartial->DataLength = dataSize;
            memcpy_s(pPartial->Data, dataSize, data, dataSize);
            HeapFree(GetProcessHeap(), 0, data);
            break;
        }

        default:
            return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

// ZwDeleteKey: Deletes a registry key using RegDeleteKeyW. Only works for empty keys (not recursive).
NTSTATUS ZwDeleteKey(HANDLE KeyHandle) {
    // This deletes the key that KeyHandle refers to
    // We need to close the handle after the key is deleted
    LSTATUS status = RegDeleteKeyW((HKEY)KeyHandle, L"");

    if (status != ERROR_SUCCESS) {
        // Try to close the handle to avoid leaks even if delete failed
        RegCloseKey((HKEY)KeyHandle);
        return status;
    }

    // Key was deleted, now close the handle (safe because RegDeleteKeyW doesn't invalidate it)
    RegCloseKey((HKEY)KeyHandle);
    return STATUS_SUCCESS;
}

NTSTATUS ZwDeleteKeyEx(HKEY KeyHandle, LPCWSTR SubKey) {
    // RegDeleteKeyW returns ERROR_SUCCESS (0) on success, map to STATUS_SUCCESS (0)
    LSTATUS status = RegDeleteKeyW(KeyHandle, SubKey);
    return (status == ERROR_SUCCESS) ? STATUS_SUCCESS : status;
}
