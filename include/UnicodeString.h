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

#ifndef WINKERNEL_UNICODE_STRING_H
#define WINKERNEL_UNICODE_STRING_H

#include <Windows.h>
#include <stdio.h>
#include <WinKernelLite/KernelHeap.h>
#include <WinKernelLite/NtStatus.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Unicode string structure compatible with Windows kernel definition
 */
typedef struct _UNICODE_STRING {
    USHORT Length;        /**< Length of the string in bytes */
    USHORT MaximumLength; /**< Maximum size of buffer in bytes */
    PWSTR  Buffer;        /**< Pointer to string buffer */
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef LONG NTSTATUS;
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER ((NTSTATUS)0xC000000DL)
#endif

#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY ((NTSTATUS)0xC0000017L)
#endif

#ifndef STATUS_NAME_TOO_LONG
#define STATUS_NAME_TOO_LONG ((NTSTATUS)0xC0000106L)
#endif

#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#endif

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE
#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE (0x00000001)
#endif

#ifndef RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING (0x00000002)
#endif

#ifndef UNICODE_NULL
#define UNICODE_NULL ((WCHAR)0)
#endif

#ifndef UNICODE_STRING_MAX_BYTES
#define UNICODE_STRING_MAX_BYTES ((WORD)65534)
#endif

#ifndef UNICODE_STRING_MAX_CHARS
#define UNICODE_STRING_MAX_CHARS (32767)
#endif

/**
 * @brief Initializes an empty UNICODE_STRING with a provided buffer
 * 
 * @param _ucStr Pointer to UNICODE_STRING to initialize
 * @param _buf Buffer to use for the UNICODE_STRING
 * @param _bufSize Size of the buffer in bytes
 * 
 * Sets Length to 0 and MaximumLength to the buffer size
 */
#define RtlInitEmptyUnicodeString(_ucStr,_buf,_bufSize) \
    ((_ucStr)->Buffer = (_buf), \
     (_ucStr)->Length = 0, \
     (_ucStr)->MaximumLength = (USHORT)(_bufSize))

/**
 * @brief Initializes a UNICODE_STRING from a null-terminated wide string
 * 
 * @param DestinationString Pointer to UNICODE_STRING to initialize
 * @param SourceString Optional pointer to null-terminated wide string
 * @return NTSTATUS STATUS_SUCCESS on success, or appropriate error code
 * 
 * If SourceString is NULL, initializes an empty string.
 * The Buffer in DestinationString will point to SourceString's buffer.
 * No memory allocation is performed.
 */
NTSTATUS RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    );

/**
 * @brief Validates a UNICODE_STRING structure
 * 
 * @param Flags Reserved, must be 0
 * @param String Pointer to UNICODE_STRING to validate
 * @return NTSTATUS STATUS_SUCCESS if valid, STATUS_INVALID_PARAMETER if not
 * 
 * Checks:
 * - Length within UNICODE_STRING_MAX_BYTES
 * - MaximumLength >= Length
 * - Buffer alignment
 * - Buffer not NULL if Length > 0
 */
NTSTATUS RtlValidateUnicodeString(
    ULONG Flags,
    PCUNICODE_STRING String
    );

/**
 * @brief Creates a new copy of a UNICODE_STRING
 * 
 * @param Flags Combination of RTL_DUPLICATE_* flags
 * @param StringIn Source string to duplicate
 * @param StringOut Destination for new string
 * @return NTSTATUS STATUS_SUCCESS on success, or appropriate error code
 * 
 * Memory is allocated for the new string's buffer.
 * The caller must free the buffer using FreeUnicodeString when done.
 */
NTSTATUS RtlDuplicateUnicodeString(
    ULONG Flags,
    PCUNICODE_STRING StringIn,
    PUNICODE_STRING StringOut
    );

/**
 * @brief Frees memory allocated for a UNICODE_STRING
 * 
 * @param UnicodeString Pointer to UNICODE_STRING to free
 * 
 * Frees the Buffer and resets the structure fields.
 * Only use this for strings allocated by RtlDuplicateUnicodeString.
 */
void FreeUnicodeString(PUNICODE_STRING UnicodeString);

/**
 * @brief Copies a Unicode string to another Unicode string
 * 
 * @param DestinationString Pointer to the destination string
 * @param SourceString Optional pointer to the source string
 * 
 * If SourceString is NULL, the Length field of DestinationString is set to zero.
 * The MaximumLength and Buffer fields of DestinationString are not modified.
 * The number of bytes copied is the smaller of SourceString->Length or DestinationString->MaximumLength.
 */
VOID RtlCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCUNICODE_STRING SourceString OPTIONAL
    );

/**
 * @brief Converts a Unicode string to uppercase
 * 
 * @param DestinationString Pointer to the destination string for the conversion
 * @param SourceString Pointer to the source string to be converted
 * @param AllocateDestinationString TRUE if function should allocate memory for the destination string
 * @return NTSTATUS STATUS_SUCCESS on success, STATUS_NO_MEMORY if memory allocation fails
 * 
 * If AllocateDestinationString is TRUE, caller must free the buffer using FreeUnicodeString.
 * If AllocateDestinationString is FALSE, caller must ensure destination buffer is large enough.
 */
NTSTATUS RtlUpcaseUnicodeString(
    IN OUT PUNICODE_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    );

/**
 * @brief Appends a Unicode string to an existing UNICODE_STRING
 * 
 * @param Destination Pointer to the destination string
 * @param Source Source string to append to the destination
 * @return NTSTATUS STATUS_SUCCESS if successful, STATUS_BUFFER_TOO_SMALL if buffer too small
 * 
 * Copies bytes from the Source string to the destination UNICODE_STRING up to
 * the destination's MaximumLength field.
 */
NTSTATUS RtlAppendUnicodeToString(
    IN PUNICODE_STRING Destination,
    IN PCWSTR Source OPTIONAL
    );

/**
 * @brief Appends one UNICODE_STRING to another
 * 
 * @param Destination Pointer to the destination string
 * @param Source Pointer to the source string to append
 * @return NTSTATUS STATUS_SUCCESS if successful, STATUS_BUFFER_TOO_SMALL if buffer too small
 * 
 * Copies bytes from the Source UNICODE_STRING to the destination UNICODE_STRING up to
 */

/**
 * @brief Compares two UNICODE_STRING structures
 * 
 * @param String1 Pointer to the first string
 * @param String2 Pointer to the second string
 * @param CaseInSensitive TRUE if case should be ignored when doing the comparison
 * @return LONG Zero if strings are equal, <0 if String1 < String2, >0 if String1 > String2
 * 
 * The comparison is based on the values of the characters in the strings, not their lengths.
 * If all characters match but one string is shorter, the shorter string is considered less than the longer string.
 */
inline LONG RtlCompareUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    );

/**
 * @brief Appends one UNICODE_STRING to another
 * 
 * @param Destination Pointer to the destination string
 * @param Source Pointer to the source string to append
 * @return NTSTATUS STATUS_SUCCESS if successful, STATUS_BUFFER_TOO_SMALL if buffer too small
 * 
 * Copies bytes from the Source UNICODE_STRING to the destination UNICODE_STRING up to
 * the destination's MaximumLength field.
 */
NTSTATUS RtlAppendUnicodeStringToString(
    IN OUT PUNICODE_STRING Destination,
    IN PCUNICODE_STRING Source
    );

#ifdef __cplusplus
}
#endif

inline NTSTATUS RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )
{
    if (DestinationString == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    DestinationString->Buffer = (PWSTR)SourceString;  // Set Buffer first, matching Windows implementation

    if (SourceString != NULL) {
        SIZE_T Length = wcslen(SourceString) * sizeof(WCHAR);
        if (Length > UNICODE_STRING_MAX_BYTES) {
            return STATUS_NAME_TOO_LONG;
        }
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
    } else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }
    return STATUS_SUCCESS;
}

inline NTSTATUS RtlValidateUnicodeString(ULONG Flags, PCUNICODE_STRING String)
{
    // It seems that Flags was not used in the original version either
    UNREFERENCED_PARAMETER(Flags);

    if (String == NULL) {
        return STATUS_SUCCESS;
    }

    // Check Length first - since UNICODE_STRING_MAX_BYTES is a 16-bit value,
    // any value higher than this in a USHORT means overflow
    if (String->Length > UNICODE_STRING_MAX_BYTES) {
        return STATUS_INVALID_PARAMETER;
    }

    // Next check MaximumLength
    if (String->MaximumLength > UNICODE_STRING_MAX_BYTES) {
        return STATUS_INVALID_PARAMETER;
    }

    // MaximumLength must be at least Length
    if (String->MaximumLength < String->Length) {
        return STATUS_INVALID_PARAMETER;
    }

    // WCHAR alignment checks
    if ((String->Length & 1) != 0 ||        // Must be even (WCHAR aligned)
        (String->MaximumLength & 1) != 0) {  // Must be even (WCHAR aligned)
        return STATUS_INVALID_PARAMETER;
    }

    // Buffer validation
    if (String->Length > 0) {
        if (String->Buffer == NULL) {
            return STATUS_INVALID_PARAMETER;
        }
        // Check buffer alignment
        if ((ULONG_PTR)String->Buffer & 1) {  // Must be WCHAR aligned
            return STATUS_INVALID_PARAMETER;
        }
    }

    return STATUS_SUCCESS;
}

inline NTSTATUS RtlDuplicateUnicodeString(
    ULONG Flags,
    PCUNICODE_STRING StringIn,
    PUNICODE_STRING StringOut
    )
{
    // Validate output parameter
    if (StringOut == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Initialize output structure
    StringOut->Buffer = NULL;
    StringOut->Length = 0;
    StringOut->MaximumLength = 0;

    // Validate flags
    const ULONG VALID_FLAGS = RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                             RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING;
    
    if ((Flags & ~VALID_FLAGS) != 0) {
        return STATUS_INVALID_PARAMETER;
    }

    const BOOL bNullTerminate = (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE) != 0;
    const BOOL bAllocateNull = (Flags & RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING) != 0;

    // Must null terminate if allocating null string
    if (bAllocateNull && !bNullTerminate) {
        return STATUS_INVALID_PARAMETER;
    }

    // Validate input string if provided
    NTSTATUS Status = RtlValidateUnicodeString(0, StringIn);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    // Calculate required buffer sizes
    USHORT SourceLength = (StringIn != NULL) ? StringIn->Length : 0;
    USHORT AllocLength = SourceLength;

    // Check for overflow when adding null terminator
    if (bNullTerminate) {
        if (SourceLength >= UNICODE_STRING_MAX_BYTES) {
            return STATUS_NAME_TOO_LONG;
        }
        AllocLength += sizeof(WCHAR);
    }

    // Don't allocate for empty strings unless explicitly requested
    if (SourceLength == 0 && !bAllocateNull) {
        return STATUS_SUCCESS;
    }

    // Allocate and copy buffer
    // Originally in Win kernel, it seemed that ExAllocatePoolWithTag (PagedPool,NumberOfBytes,....) was used for this
    PWSTR NewBuffer = (PWSTR)ExAllocatePoolTracked(PagedPool, AllocLength);
    if (NewBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    // Copy source data if it exists
    if (SourceLength > 0) {
        RtlCopyMemory(NewBuffer, StringIn->Buffer, SourceLength);
    }

    // Add null terminator if requested
    if (bNullTerminate) {
        NewBuffer[SourceLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    // Update output structure
    StringOut->Buffer = NewBuffer;
    StringOut->Length = SourceLength;
    StringOut->MaximumLength = AllocLength;    
    return STATUS_SUCCESS;
}

inline void FreeUnicodeString(PUNICODE_STRING UnicodeString)
{
	if (UnicodeString && UnicodeString->Buffer) {
		// Use tracked free
		FREE_POOL_TRACKED(UnicodeString->Buffer);
		UnicodeString->Length = 0;
		UnicodeString->MaximumLength = 0;

        memset( UnicodeString, 0, sizeof( *UnicodeString ) );

	}
}

// Implementation of RtlAppendUnicodeToString
inline NTSTATUS
RtlAppendUnicodeToString(
    IN PUNICODE_STRING Destination,
    IN PCWSTR Source OPTIONAL
    )
{
    // Check for NULL destination
    if (Destination == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Early return if source is NULL or empty
    if (Source == NULL || *Source == UNICODE_NULL) {
        return STATUS_SUCCESS;
    }
    
    // Validate destination buffer exists - this prevents access violations
    if (Destination->Buffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Calculate source length manually (in bytes)
    USHORT sourceLength = 0;
    for (const WCHAR* p = Source; *p != UNICODE_NULL; p++) {
        sourceLength += sizeof(WCHAR);
    }
    
    // Verify there's enough space in destination (with space for null terminator)
    if ((Destination->Length + sourceLength) >= Destination->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    // Get pointer to destination buffer position for append
    PWCHAR destPtr = (PWCHAR)((BYTE*)Destination->Buffer + Destination->Length);
    
    // Copy the source string
    RtlCopyMemory(destPtr, Source, sourceLength);
    
    // Update destination length
    Destination->Length += sourceLength;
    
    // Null terminate if there's room
    if (Destination->Length < Destination->MaximumLength) {
        Destination->Buffer[Destination->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
      return STATUS_SUCCESS;
}

// Implementation of RtlAppendUnicodeStringToString
inline NTSTATUS
RtlAppendUnicodeStringToString(
    IN OUT PUNICODE_STRING Destination,
    IN PCUNICODE_STRING Source
    )
{
    // Validate input parameters
    if (Destination == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    
    if (Source == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Validate source string
    NTSTATUS status = RtlValidateUnicodeString(0, Source);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Validate destination string
    status = RtlValidateUnicodeString(0, Destination);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Early return if there's nothing to append
    if (Source->Length == 0) {
        return STATUS_SUCCESS;
    }
    
    // Check if destination buffer has enough space
    USHORT requiredSpace = Source->Length;
    USHORT availableSpace = Destination->MaximumLength - Destination->Length;
    
    if (requiredSpace > availableSpace) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    // Get character count for source (divide byte length by sizeof(WCHAR))
    USHORT sourceCharCount = Source->Length / sizeof(WCHAR);
    
    // Get the current end position in the destination buffer
    USHORT destCharPos = Destination->Length / sizeof(WCHAR);
    
    // Manual character-by-character copy instead of using RtlCopyMemory
    for (USHORT i = 0; i < sourceCharCount; i++) {
        Destination->Buffer[destCharPos + i] = Source->Buffer[i];
    }
    
    // Update destination length
    Destination->Length += Source->Length;
    
    // Add null terminator if there's space (note: this is not required by the API contract
    // but is a courtesy for debugging and compatibility with C-style strings)
    if (Destination->Length < Destination->MaximumLength) {
        Destination->Buffer[Destination->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
      return STATUS_SUCCESS;
}

/**
 * @brief Helper function to convert a wide character to uppercase
 * 
 * @param c The character to convert
 * @return WCHAR The uppercase version of the character
 */
static inline WCHAR RtlUpcaseUnicodeChar(WCHAR c) {
    
    if (c >= L'a' && c <= L'z') {
        return c - L'a' + L'A';  // Convert to uppercase
    }

    return c;
}

/**
 * @brief Compares two UNICODE_STRING structures
 * 
 * The function compares two counted strings. The return value indicates if the 
 * strings are equal, String1 is less than String2, or String1 is greater than String2.
 * 
 * @param String1 Pointer to the first string
 * @param String2 Pointer to the second string
 * @param CaseInSensitive TRUE if case should be ignored when doing the comparison
 * @return LONG Zero if strings are equal, <0 if String1 < String2, >0 if String1 > String2
 */
inline LONG RtlCompareUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )
{
    // Handle NULL pointer cases
    if (String1 == NULL && String2 == NULL) {
        return 0;  // Both NULL, consider equal
    }
    if (String1 == NULL) {
        return -1; // NULL is less than non-NULL
    }
    if (String2 == NULL) {
        return 1;  // Non-NULL is greater than NULL
    }
    
    // Handle NULL buffer cases
    WCHAR *s1 = String1->Buffer;
    WCHAR *s2 = String2->Buffer;
    
    if (s1 == NULL && s2 == NULL) {
        return 0;  // Both empty, consider equal
    }
    if (s1 == NULL) {
        return -1; // Empty is less than non-empty
    }
    if (s2 == NULL) {
        return 1;  // Non-empty is greater than empty
    }
    
    // Get string lengths in characters
    USHORT n1 = String1->Length / sizeof(WCHAR);
    USHORT n2 = String2->Length / sizeof(WCHAR);
    
    // Compare characters up to the length of the shorter string
    USHORT minLength = (n1 < n2) ? n1 : n2;
    
    for (USHORT i = 0; i < minLength; i++) {
        WCHAR c1 = s1[i];
        WCHAR c2 = s2[i];
        
        // Apply case-insensitive comparison if requested
        if (CaseInSensitive) {
            c1 = RtlUpcaseUnicodeChar(c1);
            c2 = RtlUpcaseUnicodeChar(c2);
        }
        
        // Return difference at first non-matching character
        if (c1 != c2) {
            return (LONG)c1 - (LONG)c2;
        }
    }
    
    // If all characters matched up to the shorter length,
    // the shorter string is considered less than the longer one
    return (LONG)n1 - (LONG)n2;
}

/**
 * @brief Copies a Unicode string to another Unicode string
 * 
 * This function copies the source string to the destination string. If the source string
 * is not provided, the destination string's Length is set to zero. The MaximumLength and
 * Buffer fields of the destination string are not modified.
 * 
 * The number of bytes copied is limited by the smaller of the source string's Length or
 * the destination string's MaximumLength.
 * 
 * @param DestinationString Pointer to the destination string
 * @param SourceString Optional pointer to the source string
 */
inline VOID RtlCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCUNICODE_STRING SourceString OPTIONAL
    )
{
    // Parameter validation
    if (DestinationString == NULL) {
        return;
    }

    // If no source string, set destination length to zero and return
    if (SourceString == NULL) {
        DestinationString->Length = 0;
        // Do not modify the buffer contents when source is NULL
        return;
    }
    
    // Validate that destination has a buffer
    if (DestinationString->Buffer == NULL) {
        DestinationString->Length = 0;
        return;
    }
    
    // Calculate how many bytes we can copy
    USHORT bytesToCopy = SourceString->Length;
    if (bytesToCopy > DestinationString->MaximumLength) {
        bytesToCopy = DestinationString->MaximumLength;
    }
    
    // If source has data to copy
    if (bytesToCopy > 0 && SourceString->Buffer != NULL) {
        // Copy the string data
        RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, bytesToCopy);
        
        // Update destination length
        DestinationString->Length = bytesToCopy;
        
        // Null-terminate if there's room
        if ((bytesToCopy + sizeof(WCHAR)) <= DestinationString->MaximumLength) {
            DestinationString->Buffer[bytesToCopy / sizeof(WCHAR)] = UNICODE_NULL;
        }
    } else {
        // Empty source (zero length), set destination length to zero
        DestinationString->Length = 0;
        
        // Always null-terminate empty strings if there's room
        if (DestinationString->MaximumLength >= sizeof(WCHAR)) {
            DestinationString->Buffer[0] = UNICODE_NULL;
        }
    }
}

/**
 * @brief Converts a Unicode string to uppercase
 * 
 * The function converts a copy of the source string to uppercase and 
 * writes the converted string in the destination buffer.
 * 
 * @param DestinationString Pointer to the destination string for the conversion
 * @param SourceString Pointer to the source string to be converted
 * @param AllocateDestinationString TRUE if function should allocate memory for the destination string
 * @return NTSTATUS STATUS_SUCCESS on success, or appropriate error code
 */
inline NTSTATUS RtlUpcaseUnicodeString(
    IN OUT PUNICODE_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )
{
    // Parameter validation
    if (DestinationString == NULL || SourceString == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Validate source string
    NTSTATUS status = RtlValidateUnicodeString(0, SourceString);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Handle allocation if requested
    if (AllocateDestinationString) {
        // Allocate buffer for destination string
        PWSTR newBuffer = NULL;
        
        // Only allocate if source has content
        if (SourceString->Length > 0) {
            newBuffer = (PWSTR)ExAllocatePoolTracked(PagedPool, SourceString->Length);
            if (newBuffer == NULL) {
                return STATUS_NO_MEMORY;
            }
        }
        
        // Setup destination string
        DestinationString->Buffer = newBuffer;
        DestinationString->Length = SourceString->Length;
        DestinationString->MaximumLength = SourceString->Length;
    } else {
        // Ensure destination buffer is large enough
        if (DestinationString->MaximumLength < SourceString->Length) {
            return STATUS_BUFFER_TOO_SMALL;
        }
        
        // Update destination string length
        DestinationString->Length = SourceString->Length;
    }
    
    // If source is empty, we're done
    if (SourceString->Length == 0 || SourceString->Buffer == NULL) {
        return STATUS_SUCCESS;
    }
    
    // Get character count
    USHORT charCount = SourceString->Length / sizeof(WCHAR);

    // Perform the uppercase conversion
    for (USHORT i = 0; i < charCount; i++) {
        DestinationString->Buffer[i] = RtlUpcaseUnicodeChar(SourceString->Buffer[i]);
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Converts a UNICODE_STRING to an unsigned long integer
 * 
 * This implementation uses a different approach than the standard Windows version,
 * utilizing a character-by-character validation and conversion method with
 * enhanced error checking and overflow detection.
 */
inline NTSTATUS RtlUnicodeStringToInteger(
    IN PCUNICODE_STRING String,
    IN ULONG Base OPTIONAL,
    OUT PULONG Value
    )
{
    if (!String || !Value) {
        return STATUS_INVALID_PARAMETER;
    }
    
    if (!String->Buffer || String->Length == 0) {
        *Value = 0;
        return STATUS_SUCCESS;
    }
    
    // Calculate number of characters
    SIZE_T numChars = String->Length / sizeof(WCHAR);
    if (numChars == 0) {
        *Value = 0;
        return STATUS_SUCCESS;
    }
    
    SIZE_T pos = 0;
    BOOLEAN isNegative = FALSE;
    ULONG result = 0;
    ULONG actualBase = Base;
    
    // Skip leading whitespace
    while (pos < numChars && (String->Buffer[pos] == L' ' || 
                             String->Buffer[pos] == L'\t' || 
                             String->Buffer[pos] == L'\n' || 
                             String->Buffer[pos] == L'\r')) {
        pos++;
    }
    
    if (pos >= numChars) {
        *Value = 0;
        return STATUS_SUCCESS;
    }
    
    // Check for sign
    if (String->Buffer[pos] == L'+') {
        pos++;
    } else if (String->Buffer[pos] == L'-') {
        isNegative = TRUE;
        pos++;
    }
    
    if (pos >= numChars) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Auto-detect base if not specified
    if (actualBase == 0) {
        actualBase = 10; // Default to decimal
        
        if (String->Buffer[pos] == L'0' && pos + 1 < numChars) {
            WCHAR nextChar = String->Buffer[pos + 1];
            if (nextChar == L'x' || nextChar == L'X') {
                actualBase = 16;
                pos += 2;
            } else if (nextChar == L'b' || nextChar == L'B') {
                actualBase = 2;
                pos += 2;
            } else if (nextChar == L'o' || nextChar == L'O') {
                actualBase = 8;
                pos += 2;
            }
        }
    } else {
        // Validate explicitly specified base
        if (actualBase != 2 && actualBase != 8 && actualBase != 10 && actualBase != 16) {
            return STATUS_INVALID_PARAMETER;
        }
        
        // Skip standard prefixes even with explicit base
        if (actualBase == 16 && String->Buffer[pos] == L'0' && pos + 1 < numChars &&
            (String->Buffer[pos + 1] == L'x' || String->Buffer[pos + 1] == L'X')) {
            pos += 2;
        }
    }
    
    if (pos >= numChars) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Convert digits
    BOOLEAN foundValidDigit = FALSE;
    while (pos < numChars) {
        WCHAR ch = String->Buffer[pos];
        ULONG digitValue;
        
        // Convert character to digit value
        if (ch >= L'0' && ch <= L'9') {
            digitValue = ch - L'0';
        } else if (ch >= L'A' && ch <= L'F') {
            digitValue = ch - L'A' + 10;
        } else if (ch >= L'a' && ch <= L'f') {
            digitValue = ch - L'a' + 10;
        } else {
            // Invalid character - stop processing
            break;
        }
        
        // Check if digit is valid for this base
        if (digitValue >= actualBase) {
            break;
        }
        
        // Check for overflow before multiplication
        if (result > (ULONG_MAX / actualBase)) {
            return STATUS_INVALID_PARAMETER;
        }
        
        ULONG newResult = result * actualBase;
        
        // Check for overflow before addition
        if (newResult > ULONG_MAX - digitValue) {
            return STATUS_INVALID_PARAMETER;
        }
        
        result = newResult + digitValue;
        foundValidDigit = TRUE;
        pos++;
    }
    
    if (!foundValidDigit) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Apply sign for negative numbers (convert to unsigned representation)
    if (isNegative) {
        result = (ULONG)(-(LONG)result);
    }
    
    *Value = result;
    return STATUS_SUCCESS;
}

/**
 * @brief Debug utility function to dump Unicode string contents
 * 
 * This function prints detailed information about a UNICODE_STRING structure,
 * including its length, maximum length, buffer address, and string content
 * in both hexadecimal and text format.
 * 
 * @param String Pointer to the UNICODE_STRING to dump
 * @param Name Optional name/label to print before the dump (can be NULL)
 */
__forceinline void DumpUnicodeString(PCUNICODE_STRING String, const char* Name) {
    if (String == NULL) {
        printf("NULL UNICODE_STRING pointer\n");
        return;
    }

    if (Name != NULL) {
        printf("%s:\n", Name);
    }

    printf("Length: %u (0x%04x)\n", String->Length, String->Length);
    printf("MaximumLength: %u (0x%04x)\n", String->MaximumLength, String->MaximumLength);
    printf("Buffer: %p\n", (void*)String->Buffer);

    if (String->Buffer != NULL) {
        SIZE_T chars = String->Length / sizeof(WCHAR);
        printf("Content (%Iu chars):\n", chars);
        
        // Hex dump
        printf("Hex: ");
        for (SIZE_T i = 0; i < chars; i++) {
            printf("%04x ", (unsigned short)String->Buffer[i]);
        }
        printf("\n");

        printf("Text: ");
        for (SIZE_T i = 0; i < chars; i++) {
            WCHAR ch = String->Buffer[i];
            if (ch >= 32 && ch <= 126) { 
                printf("%c", (char)ch);
            } else {
                printf(".");
            }
        }
        printf("\n\n");
    }
}

#ifdef __cplusplus
#endif

#endif // WINKERNEL_UNICODE_STRING_H