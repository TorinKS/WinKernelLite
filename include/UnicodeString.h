#pragma once

#include <Windows.h>
#include "KernelHeapAlloc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef LONG NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_NO_MEMORY ((NTSTATUS)0xC0000017L)

NTSTATUS DuplicateUnicodeString(PUNICODE_STRING Destination, PCWSTR Source);
void FreeUnicodeString(PUNICODE_STRING UnicodeString);

#ifdef __cplusplus
}
#endif

// Helper functions for UNICODE_STRING management
inline NTSTATUS DuplicateUnicodeString(PUNICODE_STRING Destination, PCWSTR Source)
{
	USHORT Length = (USHORT)(wcslen(Source) * sizeof(WCHAR));
	USHORT MaximumLength = Length + sizeof(WCHAR); // Add space for null terminator

	// Use tracked allocation 
	Destination->Buffer = (PWSTR)ExAllocatePoolTracked(PagedPool, MaximumLength);
	if (!Destination->Buffer) {
		return STATUS_NO_MEMORY;
	}

	RtlCopyMemory(Destination->Buffer, Source, Length);
	Destination->Buffer[Length / sizeof(WCHAR)] = L'\0'; // Add null terminator

	Destination->Length = Length;
	Destination->MaximumLength = MaximumLength;

	return STATUS_SUCCESS;
}

inline void FreeUnicodeString(PUNICODE_STRING UnicodeString)
{
	if (UnicodeString && UnicodeString->Buffer) {
		// Use tracked free
		FREE_POOL_TRACKED(UnicodeString->Buffer);
		UnicodeString->Length = 0;
		UnicodeString->MaximumLength = 0;
	}
}

