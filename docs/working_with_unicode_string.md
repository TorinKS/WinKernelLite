# Working with UNICODE_STRING in WinKernelLite

This guide explains how to use the UNICODE_STRING implementation provided by WinKernelLite for string manipulation in a Windows kernel-compatible way.

## Overview

The Windows kernel uses the `UNICODE_STRING` structure for string manipulation instead of regular C-style strings. This structure provides length information and allows for non-null-terminated strings, which can be more efficient in kernel mode.

WinKernelLite provides a user-mode implementation of `UNICODE_STRING` and related functions that match the behavior of the kernel versions, allowing you to develop and test code that uses these structures without running in kernel mode.

## UNICODE_STRING Structure

The `UNICODE_STRING` structure is defined as:

```c
typedef struct _UNICODE_STRING {
    USHORT Length;        // Length in bytes (not characters)
    USHORT MaximumLength; // Maximum length in bytes
    PWSTR  Buffer;        // Pointer to string buffer
} UNICODE_STRING, *PUNICODE_STRING;
```

Key points to remember:
- `Length` is in bytes, not characters (divide by 2 to get character count)
- `MaximumLength` includes space for a null terminator if needed
- `Buffer` points to a wide character (UTF-16) string

## Basic Operations

### Initializing a UNICODE_STRING

To initialize a `UNICODE_STRING` from a wide character string literal:

```c
UNICODE_STRING deviceName;
RtlInitUnicodeString(&deviceName, L"My Device");
```

This function:
- Sets the `Length` to the length of the string in bytes (excluding null terminator)
- Sets the `MaximumLength` to the `Length` plus the size of a null terminator
- Sets the `Buffer` to point to the provided string

### Creating a UNICODE_STRING from Scratch

To create a new `UNICODE_STRING` with allocated memory:

```c
UNICODE_STRING myString;
NTSTATUS status = AllocateUnicodeString(&myString, 100); // Allocate buffer for 100 characters
if (NT_SUCCESS(status)) {
    // Use the string...
    
    // Free when done
    FreeUnicodeString(&myString);
}
```

### Duplicating a UNICODE_STRING

To make a copy of an existing `UNICODE_STRING`:

```c
UNICODE_STRING source;
UNICODE_STRING destination;
RtlInitUnicodeString(&source, L"Original String");

NTSTATUS status = RtlDuplicateUnicodeString(
    RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, // Add null terminator
    &source,
    &destination
);

if (NT_SUCCESS(status)) {
    // Use the duplicated string...
    
    // Free when done
    FreeUnicodeString(&destination);
}
```

### Comparing UNICODE_STRING Values

To compare two `UNICODE_STRING` structures:

```c
UNICODE_STRING string1;
UNICODE_STRING string2;
RtlInitUnicodeString(&string1, L"String A");
RtlInitUnicodeString(&string2, L"String B");

// Case-sensitive comparison
if (RtlCompareUnicodeString(&string1, &string2, FALSE) == 0) {
    // Strings are equal
}

// Case-insensitive comparison
if (RtlCompareUnicodeString(&string1, &string2, TRUE) == 0) {
    // Strings are equal (ignoring case)
}
```

### Copying a UNICODE_STRING

To copy the contents of one `UNICODE_STRING` to another:

```c
UNICODE_STRING source;
UNICODE_STRING destination;
RtlInitUnicodeString(&source, L"Source String");

// Allocate destination with enough space
AllocateUnicodeString(&destination, source.Length / sizeof(WCHAR));

// Copy the string
RtlCopyUnicodeString(&destination, &source);

// Use destination...

// Free when done
FreeUnicodeString(&destination);
```

## Memory Management

### Allocating a UNICODE_STRING Buffer

WinKernelLite provides helper functions to allocate memory for `UNICODE_STRING` buffers:

```c
UNICODE_STRING string;
NTSTATUS status = AllocateUnicodeString(&string, 100); // 100 characters

if (NT_SUCCESS(status)) {
    // String buffer is allocated and ready to use
    RtlCopyMemory(string.Buffer, L"Hello", 5 * sizeof(WCHAR));
    string.Length = 5 * sizeof(WCHAR);
}
```

### Freeing a UNICODE_STRING Buffer

When you're done with a `UNICODE_STRING` that has an allocated buffer, free it:

```c
FreeUnicodeString(&string);
```

This function:
- Frees the `Buffer` using `ExFreePool`
- Sets `Buffer` to NULL
- Sets `Length` and `MaximumLength` to 0

## Practical Examples

### Building a Device ID String

```c
UNICODE_STRING deviceId;
WCHAR buffer[100];
UNICODE_STRING prefix, suffix;

// Initialize components
RtlInitUnicodeString(&prefix, L"USB\\");
RtlInitUnicodeString(&suffix, L"\\Device");

// Allocate destination string with enough space
AllocateUnicodeString(&deviceId, 
    (prefix.Length + suffix.Length) / sizeof(WCHAR) + 10);

// Copy prefix
RtlCopyUnicodeString(&deviceId, &prefix);

// Append a number
RtlAppendUnicodeToString(&deviceId, L"001");

// Append suffix
RtlAppendUnicodeStringToString(&deviceId, &suffix);

// Use the complete ID...

// Free when done
FreeUnicodeString(&deviceId);
```

### Searching Within a UNICODE_STRING

```c
UNICODE_STRING haystack, needle;
RtlInitUnicodeString(&haystack, L"Windows Kernel Programming");
RtlInitUnicodeString(&needle, L"Kernel");

if (FindUnicodeSubstring(&haystack, &needle)) {
    // "Kernel" was found in "Windows Kernel Programming"
}
```

### Displaying UNICODE_STRING Content

To print the contents of a `UNICODE_STRING` for debugging:

```c
UNICODE_STRING deviceName;
RtlInitUnicodeString(&deviceName, L"My Device");

// Option 1: Use DumpUnicodeString utility
DumpUnicodeString(&deviceName, "Device Name");

// Option 2: Manual conversion for printf
WCHAR tempBuffer[256];
ULONG charsToCopy = min(deviceName.Length / sizeof(WCHAR), 255);
wcsncpy(tempBuffer, deviceName.Buffer, charsToCopy);
tempBuffer[charsToCopy] = L'\0';
wprintf(L"Device name: %s\n", tempBuffer);
```

## Best Practices

1. **Always check return values**: Functions like `RtlDuplicateUnicodeString` return `NTSTATUS` values indicating success or failure.

2. **Free allocated strings**: Call `FreeUnicodeString` on any string that had memory allocated for its buffer.

3. **Remember byte vs. character counts**: `Length` and `MaximumLength` are in bytes, not characters. Divide by `sizeof(WCHAR)` to get character counts.

4. **Use NULL termination appropriately**: Windows kernel code often uses non-null-terminated strings. Use `RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE` when you need null termination.

5. **Initialize strings before use**: Always initialize a `UNICODE_STRING` with `RtlInitUnicodeString` or similar before using it.

6. **Check for buffer overflow**: When manipulating string contents directly, ensure you don't exceed `MaximumLength`.

By following these guidelines, you can effectively work with `UNICODE_STRING` structures in a way that's compatible with Windows kernel development practices while still running in user mode.
