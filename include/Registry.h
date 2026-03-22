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

#ifndef WINKERNEL_REGISTRY_H
#define WINKERNEL_REGISTRY_H

#include <Windows.h>
#include <WinKernelLite/UnicodeString.h>
#include <WinKernelLite/NtStatus.h>
#include <WinKernelLite/File.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registry information class used with ZwEnumerateKey
 */
typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation,
    KeyVirtualizationInformation,
    KeyHandleTagsInformation,
    KeyTrustInformation,
    KeyLayerInformation,
    MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

/**
 * @brief Basic registry key information
 */
typedef struct _KEY_BASIC_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

/**
 * @brief Full registry key information
 */
typedef struct _KEY_FULL_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG ClassLength;
    ULONG ClassOffset;
    ULONG SubKeys;
    ULONG MaxNameLen;
    ULONG MaxClassLen;
    ULONG Values;
    ULONG MaxValueNameLen;
    ULONG MaxValueDataLen;
    WCHAR Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

/**
 * @brief Registry key node information
 */
typedef struct _KEY_NODE_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG ClassLength;
    ULONG ClassOffset;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

/**
 * @brief Value key information class for ZwSetValueKey
 */
typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    KeyValueLayerInformation,
    MaxKeyValueInfoClass
} KEY_VALUE_INFORMATION_CLASS;

/**
 * @brief Registry value full information structure
 */
typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    WCHAR Name[1];
    // Data follows the name in memory
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

/**
 * @brief Registry value basic information structure
 */
typedef struct _KEY_VALUE_BASIC_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

/**
 * @brief Registry value partial information structure
 */
typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    BYTE Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

/**
 * @brief User mode implementation of ZwEnumerateKey
 * Enumerates registry key information
 *
 * @param KeyHandle Handle to the registry key
 * @param Index Index of the subkey to enumerate
 * @param KeyInformationClass Type of information to retrieve
 * @param KeyInformation Buffer to receive the information
 * @param Length Size of the KeyInformation buffer in bytes
 * @param ResultLength Pointer to receive the size of data written
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ZwEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

/**
 * @brief User mode implementation of ZwClose
 * Closes an object handle
 *
 * @param Handle Handle to close
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ZwClose(
    IN HANDLE Handle
);

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
);

/**
 * @brief User mode implementation of ZwCreateKey
 * Creates or opens a registry key
 *
 * @param KeyHandle Pointer to receive the handle to the key
 * @param DesiredAccess The access mask for the key
 * @param ObjectAttributes Attributes for the key
 * @param TitleIndex Reserved, must be zero
 * @param Class Class of the key (can be NULL)
 * @param CreateOptions Options for creating the key
 * @param Disposition Optional pointer to receive creation disposition
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ZwCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
);

/**
 * @brief User mode implementation of ZwSetValueKey
 * Sets a registry value
 *
 * @param KeyHandle Handle to the registry key
 * @param ValueName Name of the value to set
 * @param TitleIndex Reserved, must be zero
 * @param Type Type of the value data
 * @param Data The value data to set
 * @param DataSize Size of the value data in bytes
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ZwSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
);

/**
 * @brief User mode implementation of ZwOpenKey
 * Opens an existing registry key
 *
 * @param KeyHandle Pointer to receive the handle to the key
 * @param DesiredAccess The access mask for the key
 * @param ObjectAttributes Attributes for the key
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

/**
 * @brief User mode implementation of ZwQueryValueKey
 * Queries registry key value information
 *
 * @param KeyHandle Handle to the registry key
 * @param ValueName Name of the value to query
 * @param KeyValueInformationClass Type of information to retrieve
 * @param KeyValueInformation Buffer to receive the information
 * @param Length Size of the KeyValueInformation buffer in bytes
 * @param ResultLength Pointer to receive the size of data written or needed
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ZwQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

/**
 * @brief User mode implementation of ZwEnumerateValueKey
 * Enumerates value entries for a registry key
 *
 * @param KeyHandle Handle to the registry key
 * @param Index Index of the value entry to enumerate
 * @param KeyValueInformationClass Type of information to retrieve
 * @param KeyValueInformation Buffer to receive the information
 * @param Length Size of the KeyValueInformation buffer in bytes
 * @param ResultLength Pointer to receive the size of data written
 * @return NTSTATUS STATUS_SUCCESS on success or appropriate error code
 */
NTSTATUS ZwEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

// Registry key create options - only define if not already defined by Windows SDK
#ifndef REG_OPTION_VOLATILE
#define REG_OPTION_VOLATILE                 0x00000001L
#endif

#ifndef REG_OPTION_CREATE_LINK
#define REG_OPTION_CREATE_LINK              0x00000002L
#endif

#ifndef REG_OPTION_BACKUP_RESTORE
#define REG_OPTION_BACKUP_RESTORE           0x00000004L
#endif

#ifndef REG_OPTION_OPEN_LINK
#define REG_OPTION_OPEN_LINK                0x00000008L
#endif

#ifndef REG_OPTION_DONT_VIRTUALIZE
#define REG_OPTION_DONT_VIRTUALIZE          0x00000010L
#endif

// Registry key disposition values - only define if not already defined
#ifndef REG_CREATED_NEW_KEY
#define REG_CREATED_NEW_KEY                 0x00000001L
#endif

#ifndef REG_OPENED_EXISTING_KEY
#define REG_OPENED_EXISTING_KEY             0x00000002L
#endif

/**
 * @brief Deletes a registry key (empty keys only).
 */
NTSTATUS ZwDeleteKey(HANDLE KeyHandle);

/**
 * @brief Extended function to delete a subkey of a parent key.
 */
NTSTATUS ZwDeleteKeyEx(HKEY KeyHandle, LPCWSTR SubKey);

#ifdef __cplusplus
}
#endif

/* Simplified API wrappers - kept inline because names conflict with Win32 macros */

inline NTSTATUS RegCreateKey(
    PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, HANDLE RootKey,
    LPCWSTR SubKey, ULONG CreateOptions, PULONG Disposition OPTIONAL)
{
    UNICODE_STRING unicodeSubKey;
    OBJECT_ATTRIBUTES objectAttributes;
    RtlInitUnicodeString(&unicodeSubKey, SubKey);
    InitializeObjectAttributes(&objectAttributes, &unicodeSubKey, OBJ_CASE_INSENSITIVE, RootKey, NULL);
    return ZwCreateKey(KeyHandle, DesiredAccess, &objectAttributes, 0, NULL, CreateOptions, Disposition);
}

inline NTSTATUS RegOpenKey(
    PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, HANDLE RootKey, LPCWSTR SubKey)
{
    UNICODE_STRING unicodeSubKey;
    OBJECT_ATTRIBUTES objectAttributes;
    RtlInitUnicodeString(&unicodeSubKey, SubKey);
    InitializeObjectAttributes(&objectAttributes, &unicodeSubKey, OBJ_CASE_INSENSITIVE, RootKey, NULL);
    return ZwOpenKey(KeyHandle, DesiredAccess, &objectAttributes);
}

inline NTSTATUS RegSetValueKey(
    HANDLE KeyHandle, LPCWSTR ValueName, ULONG Type, PVOID Data, ULONG DataSize)
{
    UNICODE_STRING unicodeValueName;
    RtlInitUnicodeString(&unicodeValueName, ValueName);
    return ZwSetValueKey(KeyHandle, &unicodeValueName, 0, Type, Data, DataSize);
}

inline NTSTATUS RegEnumerateKey(
    HANDLE KeyHandle, ULONG Index, PWSTR Name, ULONG NameSize)
{
    BYTE buffer[2048];
    ULONG resultLength;
    NTSTATUS status = ZwEnumerateKey(KeyHandle, Index, KeyBasicInformation, buffer, sizeof(buffer), &resultLength);
    if (NT_SUCCESS(status)) {
        PKEY_BASIC_INFORMATION keyInfo = (PKEY_BASIC_INFORMATION)buffer;
        ULONG copyLen = min(NameSize - 1, keyInfo->NameLength / sizeof(WCHAR));
        wcsncpy_s(Name, NameSize / sizeof(WCHAR), keyInfo->Name, copyLen);
        Name[copyLen] = L'\0';
    }
    return status;
}

inline NTSTATUS RegQueryValueKey(
    HANDLE KeyHandle, LPCWSTR ValueName, PULONG Type, PVOID Data, PULONG DataSize)
{
    UNICODE_STRING unicodeValueName;
    ULONG bufferSize, resultLength;
    PKEY_VALUE_FULL_INFORMATION fullInfo;
    NTSTATUS status;

    RtlInitUnicodeString(&unicodeValueName, ValueName);
    bufferSize = sizeof(KEY_VALUE_FULL_INFORMATION) + *DataSize;
    fullInfo = (PKEY_VALUE_FULL_INFORMATION)HeapAlloc(GetProcessHeap(), 0, bufferSize);
    if (!fullInfo) return STATUS_NO_MEMORY;

    status = ZwQueryValueKey(KeyHandle, &unicodeValueName, KeyValueFullInformation, fullInfo, bufferSize, &resultLength);
    if (NT_SUCCESS(status)) {
        if (Type) *Type = fullInfo->Type;
        if (*DataSize < fullInfo->DataLength) {
            *DataSize = fullInfo->DataLength;
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            memcpy_s(Data, *DataSize, (PBYTE)fullInfo + fullInfo->DataOffset, fullInfo->DataLength);
            *DataSize = fullInfo->DataLength;
        }
    } else if (status == STATUS_BUFFER_TOO_SMALL) {
        *DataSize = resultLength - FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name);
    }
    HeapFree(GetProcessHeap(), 0, fullInfo);
    return status;
}

inline NTSTATUS RegOpenKeyEx(
    PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, HANDLE RootKey, LPCWSTR SubKey)
{
    UNICODE_STRING unicodeSubKey;
    OBJECT_ATTRIBUTES objectAttributes;
    RtlInitUnicodeString(&unicodeSubKey, SubKey);
    InitializeObjectAttributes(&objectAttributes, &unicodeSubKey, OBJ_CASE_INSENSITIVE, RootKey, NULL);
    return ZwOpenKey(KeyHandle, DesiredAccess, &objectAttributes);
}

inline NTSTATUS RegEnumerateValueKey(
    IN HANDLE KeyHandle, IN ULONG Index,
    OUT PKEY_VALUE_FULL_INFORMATION ValueInformation, IN ULONG Length, OUT PULONG ResultLength)
{
    return ZwEnumerateValueKey(KeyHandle, Index, KeyValueFullInformation, ValueInformation, Length, ResultLength);
}

#endif // WINKERNEL_REGISTRY_H
