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

#ifndef WINKERNEL_DEBUG_H_
#define WINKERNEL_DEBUG_H_

#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Debug level definitions */
typedef enum _DEBUG_LEVEL {
    DEBUG_LEVEL_NONE = 0,     /* No debug output */
    DEBUG_LEVEL_ERROR = 1,    /* Only errors */
    DEBUG_LEVEL_WARN = 2,     /* Warnings and errors */
    DEBUG_LEVEL_INFO = 3,     /* Info, warnings and errors */
    DEBUG_LEVEL_VERBOSE = 4,  /* All output including verbose */
    DEBUG_LEVEL_TRACE = 5     /* Extremely detailed tracing */
} DEBUG_LEVEL;

/* Debug component flags - can be combined */
typedef enum _DEBUG_COMPONENT {
    DEBUG_COMPONENT_NONE = 0,
    DEBUG_COMPONENT_HEAP = 0x01,        /* Heap allocation/deallocation */
    DEBUG_COMPONENT_REGISTRY = 0x02,    /* Registry operations */
    DEBUG_COMPONENT_UNICODE = 0x04,     /* Unicode string operations */
    DEBUG_COMPONENT_LIST = 0x08,        /* Linked list operations */
    DEBUG_COMPONENT_CRITICAL = 0x10,    /* Critical section operations */
    DEBUG_COMPONENT_TRACKING = 0x20,    /* Memory tracking operations */
    DEBUG_COMPONENT_ALL = 0xFFFFFFFF    /* All components */
} DEBUG_COMPONENT;

/* Global debug state */
typedef struct _DEBUG_STATE {
    DEBUG_LEVEL Level;
    DWORD ComponentMask;
    BOOL EnableTimestamp;
    BOOL EnableThreadId;
    BOOL EnableFileLocation;
    BOOL OutputToConsole;
    BOOL OutputToDebugger;
    BOOL OutputToFile;
    HANDLE LogFileHandle;
    CRITICAL_SECTION LogLock;
    char LogBuffer[2048];  /* Thread-safe log buffer */
} DEBUG_STATE;

/* Function declarations */
__forceinline void DebugInitialize(void);
__forceinline void DebugCleanup(void);
__forceinline void DebugSetLevel(DEBUG_LEVEL level);
__forceinline DEBUG_LEVEL DebugGetLevel(void);
__forceinline void DebugSetComponentMask(DWORD mask);
__forceinline DWORD DebugGetComponentMask(void);
__forceinline void DebugEnableConsoleOutput(BOOL enable);
__forceinline void DebugEnableDebuggerOutput(BOOL enable);
__forceinline void DebugEnableFileOutput(BOOL enable, const char* filename);
__forceinline void DebugEnableTimestamp(BOOL enable);
__forceinline void DebugEnableThreadId(BOOL enable);
__forceinline void DebugEnableFileLocation(BOOL enable);
inline void DebugPrintInternal(DEBUG_LEVEL level, DEBUG_COMPONENT component, 
                                     const char* file, int line, const char* format, ...);

/* Global debug state - external declaration */
extern DEBUG_STATE* g_WinKernelLite_DebugState;

/* Debug state accessor */
__forceinline DEBUG_STATE* GetDebugState(void) {
    if (g_WinKernelLite_DebugState == NULL) {
        DEBUG_STATE* temp = (DEBUG_STATE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEBUG_STATE));
        if (temp != NULL) {
            InitializeCriticalSection(&temp->LogLock);
            temp->Level = DEBUG_LEVEL_WARN;  /* Default to warnings and errors */
            temp->ComponentMask = (DWORD)DEBUG_COMPONENT_ALL;  /* All components by default */
            temp->EnableTimestamp = TRUE;
            temp->EnableThreadId = TRUE;
            temp->EnableFileLocation = TRUE;
            temp->OutputToConsole = FALSE;  /* Disable console output for performance */
            temp->OutputToDebugger = FALSE;  /* Disable debugger output for performance */
            temp->OutputToFile = FALSE;  /* Will be enabled explicitly when needed */
            temp->LogFileHandle = INVALID_HANDLE_VALUE;
            g_WinKernelLite_DebugState = temp;
        }
    }
    return g_WinKernelLite_DebugState;
}

__forceinline void DebugInitialize(void) {
    GetDebugState();  /* Ensure state is initialized */
}

__forceinline void DebugCleanup(void) {
    DEBUG_STATE* state = g_WinKernelLite_DebugState;
    if (state) {
        EnterCriticalSection(&state->LogLock);
        
        if (state->LogFileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(state->LogFileHandle);
            state->LogFileHandle = INVALID_HANDLE_VALUE;
        }
        
        LeaveCriticalSection(&state->LogLock);
        DeleteCriticalSection(&state->LogLock);
        HeapFree(GetProcessHeap(), 0, state);
        g_WinKernelLite_DebugState = NULL;
    }
}

__forceinline void DebugSetLevel(DEBUG_LEVEL level) {
    DEBUG_STATE* state = GetDebugState();
    if (state) {
        state->Level = level;
    }
}

__forceinline DEBUG_LEVEL DebugGetLevel(void) {
    DEBUG_STATE* state = GetDebugState();
    return state ? state->Level : DEBUG_LEVEL_NONE;
}

__forceinline void DebugSetComponentMask(DWORD mask) {
    DEBUG_STATE* state = GetDebugState();
    if (state) {
        state->ComponentMask = mask;
    }
}

__forceinline DWORD DebugGetComponentMask(void) {
    DEBUG_STATE* state = GetDebugState();
    return state ? state->ComponentMask : DEBUG_COMPONENT_NONE;
}

__forceinline void DebugEnableConsoleOutput(BOOL enable) {
    DEBUG_STATE* state = GetDebugState();
    if (state) {
        state->OutputToConsole = enable;
    }
}

__forceinline void DebugEnableDebuggerOutput(BOOL enable) {
    DEBUG_STATE* state = GetDebugState();
    if (state) {
        state->OutputToDebugger = enable;
    }
}

__forceinline void DebugEnableFileOutput(BOOL enable, const char* filename) {
    DEBUG_STATE* state = GetDebugState();
    if (!state) return;
    
    // EnterCriticalSection(&state->LogLock);
    
    if (state->LogFileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(state->LogFileHandle);
        state->LogFileHandle = INVALID_HANDLE_VALUE;
    }
    
    if (enable && filename) {
        state->LogFileHandle = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ,
                                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (state->LogFileHandle != INVALID_HANDLE_VALUE) {
            state->OutputToFile = TRUE;
        } else {
            state->OutputToFile = FALSE;
        }
    } else {
        state->OutputToFile = FALSE;
    }
    
    // LeaveCriticalSection(&state->LogLock);
}

__forceinline void DebugEnableTimestamp(BOOL enable) {
    DEBUG_STATE* state = GetDebugState();
    if (state) {
        state->EnableTimestamp = enable;
    }
}

__forceinline void DebugEnableThreadId(BOOL enable) {
    DEBUG_STATE* state = GetDebugState();
    if (state) {
        state->EnableThreadId = enable;
    }
}

__forceinline void DebugEnableFileLocation(BOOL enable) {
    DEBUG_STATE* state = GetDebugState();
    if (state) {
        state->EnableFileLocation = enable;
    }
}

inline void DebugPrintInternal(DEBUG_LEVEL level, DEBUG_COMPONENT component, 
                                     const char* file, int line, const char* format, ...) {
    DEBUG_STATE* state = GetDebugState();
    if (!state || level > state->Level || !(state->ComponentMask & component)) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    EnterCriticalSection(&state->LogLock);
    
    char* buffer = state->LogBuffer;
    int bufferSize = sizeof(state->LogBuffer);
    int pos = 0;
    
    /* Add timestamp if enabled */
    if (state->EnableTimestamp) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE,
                          "[%02d:%02d:%02d.%03d] ", 
                          st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }
    
    /* Add thread ID if enabled */
    if (state->EnableThreadId) {
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE,
                          "[TID:%lu] ", GetCurrentThreadId());
    }
    
    /* Add debug level */
    const char* levelStr = "UNKNOWN";
    switch (level) {
        case DEBUG_LEVEL_ERROR: levelStr = "ERROR"; break;
        case DEBUG_LEVEL_WARN: levelStr = "WARN"; break;
        case DEBUG_LEVEL_INFO: levelStr = "INFO"; break;
        case DEBUG_LEVEL_VERBOSE: levelStr = "VERBOSE"; break;
        case DEBUG_LEVEL_TRACE: levelStr = "TRACE"; break;
        default: break;
    }
    pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, "[%s] ", levelStr);
    
    /* Add component */
    if (component & DEBUG_COMPONENT_HEAP) {
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, "[HEAP] ");
    }
    if (component & DEBUG_COMPONENT_REGISTRY) {
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, "[REGISTRY] ");
    }
    if (component & DEBUG_COMPONENT_UNICODE) {
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, "[UNICODE] ");
    }
    if (component & DEBUG_COMPONENT_LIST) {
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, "[LIST] ");
    }
    if (component & DEBUG_COMPONENT_CRITICAL) {
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, "[CRITICAL] ");
    }
    if (component & DEBUG_COMPONENT_TRACKING) {
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, "[TRACKING] ");
    }
    
    /* Add file location if enabled */
    if (state->EnableFileLocation && file) {
        const char* fileName = strrchr(file, '\\');
        if (!fileName) fileName = strrchr(file, '/');
        if (!fileName) fileName = file; else fileName++;
        pos += _snprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, "[%s:%d] ", fileName, line);
    }
    
    /* Add the actual message */
    pos += _vsnprintf_s(buffer + pos, bufferSize - pos, _TRUNCATE, format, args);
    
    /* Ensure newline at end */
    if (pos > 0 && buffer[pos - 1] != '\n') {
        if (pos < bufferSize - 2) {
            buffer[pos++] = '\n';
            buffer[pos] = '\0';
        }
    }
    
    /* Output to console */
    if (state->OutputToConsole) {
        printf("%s", buffer);
        fflush(stdout);
    }
    
    /* Output to debugger */
    if (state->OutputToDebugger) {
        OutputDebugString(buffer);
    }
    
    /* Output to file */
    if (state->OutputToFile && state->LogFileHandle != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(state->LogFileHandle, buffer, (DWORD)strlen(buffer), &bytesWritten, NULL);
        FlushFileBuffers(state->LogFileHandle);
    }
    
    LeaveCriticalSection(&state->LogLock);
    
    va_end(args);
}

/* Convenience macros for different debug levels and components */
#define DEBUG_ERROR(component, format, ...) \
    DebugPrintInternal(DEBUG_LEVEL_ERROR, component, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define DEBUG_WARN(component, format, ...) \
    DebugPrintInternal(DEBUG_LEVEL_WARN, component, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define DEBUG_INFO(component, format, ...) \
    DebugPrintInternal(DEBUG_LEVEL_INFO, component, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define DEBUG_VERBOSE(component, format, ...) \
    DebugPrintInternal(DEBUG_LEVEL_VERBOSE, component, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define DEBUG_TRACE(component, format, ...) \
    DebugPrintInternal(DEBUG_LEVEL_TRACE, component, __FILE__, __LINE__, format, ##__VA_ARGS__)

/* Heap-specific convenience macros */
#define HEAP_ERROR(format, ...) DEBUG_ERROR(DEBUG_COMPONENT_HEAP, format, ##__VA_ARGS__)
#define HEAP_WARN(format, ...) DEBUG_WARN(DEBUG_COMPONENT_HEAP, format, ##__VA_ARGS__)
#define HEAP_INFO(format, ...) DEBUG_INFO(DEBUG_COMPONENT_HEAP, format, ##__VA_ARGS__)
#define HEAP_VERBOSE(format, ...) DEBUG_VERBOSE(DEBUG_COMPONENT_HEAP, format, ##__VA_ARGS__)
#define HEAP_TRACE(format, ...) DEBUG_TRACE(DEBUG_COMPONENT_HEAP, format, ##__VA_ARGS__)

/* Tracking-specific convenience macros */
#define TRACKING_ERROR(format, ...) DEBUG_ERROR(DEBUG_COMPONENT_TRACKING, format, ##__VA_ARGS__)
#define TRACKING_WARN(format, ...) DEBUG_WARN(DEBUG_COMPONENT_TRACKING, format, ##__VA_ARGS__)
#define TRACKING_INFO(format, ...) DEBUG_INFO(DEBUG_COMPONENT_TRACKING, format, ##__VA_ARGS__)
#define TRACKING_VERBOSE(format, ...) DEBUG_VERBOSE(DEBUG_COMPONENT_TRACKING, format, ##__VA_ARGS__)
#define TRACKING_TRACE(format, ...) DEBUG_TRACE(DEBUG_COMPONENT_TRACKING, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif  /* WINKERNEL_DEBUG_H_ */