#include <stdio.h>
#include <Windows.h>
#include <WinKernelLite/File.h>
#include <WinKernelLite/Registry.h>

int main() {
    // Initialize a UNICODE_STRING for a test registry key
    UNICODE_STRING keyName;
    WCHAR keyNameBuffer[] = L"Software\\WinKernelLiteTest";
    keyName.Buffer = keyNameBuffer;
    keyName.Length = (USHORT)(wcslen(keyNameBuffer) * sizeof(WCHAR));
    keyName.MaximumLength = (USHORT)((wcslen(keyNameBuffer) + 1) * sizeof(WCHAR));
    
    // Initialize object attributes
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &keyName, OBJ_CASE_INSENSITIVE, HKEY_CURRENT_USER, NULL);
    
    // Create the registry key
    HANDLE keyHandle;
    ULONG disposition;    
	
	NTSTATUS status = ZwCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        &objAttr,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        &disposition
    );
    
    if (NT_SUCCESS(status)) {
        printf("Registry key %s. Handle: 0x%p\n", 
               disposition == REG_CREATED_NEW_KEY ? "created" : "opened",
               keyHandle);
        
        // Set a string value in the key
        UNICODE_STRING valueName;
        WCHAR valueNameBuffer[] = L"TestValue";
        valueName.Buffer = valueNameBuffer;
        valueName.Length = (USHORT)(wcslen(valueNameBuffer) * sizeof(WCHAR));
        valueName.MaximumLength = (USHORT)((wcslen(valueNameBuffer) + 1) * sizeof(WCHAR));
        
        WCHAR valueData[] = L"This is a test value";        
        
        status = ZwSetValueKey(
            keyHandle,
            &valueName,
            0,
            REG_SZ,
            valueData,
            static_cast<ULONG>((wcslen(valueData) + 1) * sizeof(WCHAR))
        );
        
        if (NT_SUCCESS(status)) {
            printf("Registry value set successfully\n");
        } else {
            printf("Failed to set registry value. Status: 0x%08X\n", status);
        }
        
        // Enumerate subkeys (there shouldn't be any yet)
        printf("Enumerating subkeys:\n");
        ULONG index = 0;
        KEY_BASIC_INFORMATION keyInfo;
        ULONG resultLength;
        
        while (NT_SUCCESS(ZwEnumerateKey(
            keyHandle,
            index,
            KeyBasicInformation,
            &keyInfo,
            sizeof(keyInfo),
            &resultLength)))
        {
            // Display key information
            WCHAR tempName[1024] = {0};
            wcsncpy_s(tempName, 1024, keyInfo.Name, keyInfo.NameLength / sizeof(WCHAR));
            printf("  Subkey %d: %ls\n", index, tempName);
            index++;
        }
        
        if (index == 0) {
            printf("  No subkeys found\n");
        }
        
        // Close the key
        status = ZwClose(keyHandle);
        if (NT_SUCCESS(status)) {
            printf("Registry key closed successfully\n");
        } else {
            printf("Failed to close registry key. Status: 0x%08X\n", status);
        }
    } else {
        printf("Failed to create registry key. Status: 0x%08X\n", status);
    }
    
    return 0;
}
