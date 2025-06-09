#ifndef UNICODE_STRING_H
#define UNICODE_STRING_H
/**
 * @file UnicodeString.h
 * @brief Windows kernel-style Unicode string handling functions
 * 
 * This header provides a set of functions for handling Unicode strings in a way that
 * is compatible with Windows kernel conventions. It includes functions for string
 * initialization, validation, and duplication.
 */

#include <Windows.h>
#include "KernelHeapAlloc.h"

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

#ifdef __cplusplus
}
#endif

// Helper functions for UNICODE_STRING management
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
    // Originally in Win kernel, it seemed that ExAllocatePoolWithTag (PagedPool,NumberOfBytes,'grtS') was used for this
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

#endif