#ifndef UNICODE_STRING_UTILS_H
#define UNICODE_STRING_UTILS_H

#include "UnicodeString.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dumps the contents of a UNICODE_STRING structure to stdout
 * 
 * @param String Pointer to UNICODE_STRING to dump
 * @param Name Optional name/label for the string being dumped
 * 
 * Outputs:
 * - String name (if provided)
 * - Length and MaximumLength values
 * - Buffer pointer
 * - Hex dump of string contents
 * - Actual string content (if printable)
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

        // String content (if printable)
        printf("Text: ");
        for (SIZE_T i = 0; i < chars; i++) {
            WCHAR ch = String->Buffer[i];
            if (ch >= 32 && ch <= 126) { // ASCII printable range
                printf("%c", (char)ch);
            } else {
                printf(".");
            }
        }
        printf("\n\n");
    }
}

#ifdef __cplusplus
}
#endif

#endif

