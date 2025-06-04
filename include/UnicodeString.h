#pragma once

// Helper functions for UNICODE_STRING management
inline NTSTATUS DuplicateUnicodeString(PUNICODE_STRING Destination, PCWSTR Source)
{
	USHORT Length = (USHORT)(wcslen(Source) * sizeof(WCHAR));
	USHORT MaximumLength = Length + sizeof(WCHAR); // Add space for null terminator

	// Use tracked allocation 
	Destination->Buffer = ExAllocatePoolTracked(PagedPool, MaximumLength);
	if (!Destination->Buffer) {
		return STATUS_NO_MEMORY;
	}

	RtlCopyMemory(Destination->Buffer, Source, Length);
	Destination->Buffer[Length / sizeof(WCHAR)] = L'\0'; // Add null terminator

	Destination->Length = Length;
	Destination->MaximumLength = MaximumLength;

	return STATUS_SUCCESS;
}

inline VOID FreeUnicodeString(PUNICODE_STRING UnicodeString)
{
	if (UnicodeString && UnicodeString->Buffer) {
		// Use tracked free
		FREE_POOL_TRACKED(UnicodeString->Buffer);
		UnicodeString->Length = 0;
		UnicodeString->MaximumLength = 0;
	}
}

