#include <stdio.h>
#include <windows.h>
#include <WinKernelLite/Registry.h>

int main(int /* argc */, char* /* argv */[]) {
    printf("Registry simple test program\n");
    
    // Create or open a registry key
    HANDLE keyHandle;
    DWORD disposition;    NTSTATUS status = RegCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        HKEY_CURRENT_USER,
        L"Software\\WinKernelLiteTest",
        REG_OPTION_NON_VOLATILE,
        &disposition
    );
    
    if (NT_SUCCESS(status)) {
        printf("Registry key %s. Handle: 0x%p\n", 
               disposition == REG_CREATED_NEW_KEY ? "created" : "opened",
               keyHandle);
        
        // Set a string value in the key
        WCHAR valueData[] = L"This is a test value";        status = RegSetValueKey(
            keyHandle,
            L"TestValue",
            REG_SZ,
            valueData,
            static_cast<ULONG>((wcslen(valueData) + 1) * sizeof(WCHAR))
        );
        
        if (NT_SUCCESS(status)) {
            printf("Registry value set successfully\n");
        } else {
            printf("Failed to set registry value. Status: 0x%08X\n", status);
        }
        
        // Close the key
        status = ZwClose(keyHandle);
        if (NT_SUCCESS(status)) {        printf("Registry key closed successfully\n");
        } else {
            printf("Failed to close registry key. Status: 0x%08X\n", status);
        }
    } else {
        printf("Failed to create registry key. Status: 0x%08X\n", status);
    }
    
    // Test enumeration
    printf("\nEnumerating registry keys under HKEY_CURRENT_USER\\Software:\n");
    HANDLE enumKeyHandle;    status = RegOpenKey(
        &enumKeyHandle,
        KEY_READ,
        HKEY_CURRENT_USER,
        L"Software"
    );
    
    if (NT_SUCCESS(status)) {
        DWORD index = 0;
        WCHAR subkeyName[1024];
        
        while (NT_SUCCESS(RegEnumerateKey(enumKeyHandle, index, subkeyName, 1024))) {
            printf("  Subkey %d: %ls\n", index, subkeyName);
            index++;
            if (index >= 10) {
                printf("  (First 10 keys shown)\n");
                break;
            }
        }
        
        ZwClose(enumKeyHandle);
    } else {
        printf("Failed to open registry key for enumeration. Status: 0x%08X\n", status);
    }
    
    // Test ZwEnumerateValueKey functionality
    printf("\nEnumerating registry values under HKEY_CURRENT_USER\\Software\\WinKernelLiteTest:\n");
    
    // Open the test key we created earlier
    HANDLE valueKeyHandle;
    status = RegOpenKey(
        &valueKeyHandle,
        KEY_READ,
        HKEY_CURRENT_USER,
        L"Software\\WinKernelLiteTest"
    );
    
    if (NT_SUCCESS(status)) {
        DWORD index = 0;
        BYTE buffer[1024]; // Buffer for value information
        ULONG resultLength;
        
        // First, make sure we have at least one value to enumerate
        // Add another test value
        WCHAR additionalValue[] = L"Another test value";
        status = RegSetValueKey(
            valueKeyHandle,
            L"AnotherTestValue",
            REG_SZ,
            additionalValue,
            static_cast<ULONG>((wcslen(additionalValue) + 1) * sizeof(WCHAR))
        );
        
        if (!NT_SUCCESS(status)) {
            printf("Failed to set additional registry value. Status: 0x%08X\n", status);
        }
        
        // Enumerate values using ZwEnumerateValueKey directly
        printf("  Using ZwEnumerateValueKey directly:\n");
        index = 0;
        while (true) {
            status = ZwEnumerateValueKey(
                valueKeyHandle,
                index,
                KeyValueFullInformation,
                buffer,
                sizeof(buffer),
                &resultLength
            );
            
            if (!NT_SUCCESS(status)) {
                if (status == STATUS_NO_MORE_ENTRIES) {
                    printf("  (No more values)\n");
                } else {
                    printf("  Error enumerating values. Status: 0x%08X\n", status);
                }
                break;
            }
            
            PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
            WCHAR valueName[1024] = {0};
            
            // Extract the name (copying it to ensure null termination)
            if (valueInfo->NameLength > 0) {
                wcsncpy_s(valueName, 1024, valueInfo->Name, min(1023, valueInfo->NameLength / sizeof(WCHAR)));
                valueName[min(1023, valueInfo->NameLength / sizeof(WCHAR))] = L'\0';
            }
            
            printf("  Value %d: Name='%ls', Type=%d, DataLength=%d\n", 
                   index, 
                   valueName, 
                   valueInfo->Type, 
                   valueInfo->DataLength);
            
            // If it's a string value, display the content
            if (valueInfo->Type == REG_SZ && valueInfo->DataLength > 0) {
                WCHAR* data = (WCHAR*)((BYTE*)valueInfo + valueInfo->DataOffset);
                printf("    Data: '%ls'\n", data);
            }
            
            index++;
        }
        
        // Now test the simplified wrapper
        printf("\n  Using RegEnumerateValueKey wrapper:\n");
        index = 0;
        while (true) {
            status = RegEnumerateValueKey(
                valueKeyHandle,
                index,
                (PKEY_VALUE_FULL_INFORMATION)buffer,
                sizeof(buffer),
                &resultLength
            );
            
            if (!NT_SUCCESS(status)) {
                if (status == STATUS_NO_MORE_ENTRIES) {
                    printf("  (No more values)\n");
                } else {
                    printf("  Error enumerating values. Status: 0x%08X\n", status);
                }
                break;
            }
            
            PKEY_VALUE_FULL_INFORMATION valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
            WCHAR valueName[1024] = {0};
            
            // Extract the name (copying it to ensure null termination)
            if (valueInfo->NameLength > 0) {
                wcsncpy_s(valueName, 1024, valueInfo->Name, min(1023, valueInfo->NameLength / sizeof(WCHAR)));
                valueName[min(1023, valueInfo->NameLength / sizeof(WCHAR))] = L'\0';
            }
            
            printf("  Value %d: Name='%ls', Type=%d, DataLength=%d\n", 
                   index, 
                   valueName, 
                   valueInfo->Type, 
                   valueInfo->DataLength);
            
            // If it's a string value, display the content
            if (valueInfo->Type == REG_SZ && valueInfo->DataLength > 0) {
                WCHAR* data = (WCHAR*)((BYTE*)valueInfo + valueInfo->DataOffset);
                printf("    Data: '%ls'\n", data);
            }
            
            index++;
        }
        
        ZwClose(valueKeyHandle);
    } else {
        printf("Failed to open registry key for value enumeration. Status: 0x%08X\n", status);
    }
    
    return 0;
}
