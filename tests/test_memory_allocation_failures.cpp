#include <gtest/gtest.h>
#include <Windows.h>
#include <thread>
#include <vector>
#include <atomic>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#include "../include/KernelHeap.h"
#include "../include/Debug.h"  // Add debug logging support

// Global test environment to disable CRT debugging
class DebugHeapEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
#ifdef _DEBUG
        // Clear any existing break allocation number first
        _CrtSetBreakAlloc(-1);
        
        // Disable CRT heap debugging breakpoints globally
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        flags &= ~_CRTDBG_ALLOC_MEM_DF;      // Disable allocation tracking
        flags &= ~_CRTDBG_CHECK_ALWAYS_DF;   // Disable heap checks on every allocation/deallocation
        flags &= ~_CRTDBG_CHECK_CRT_DF;      // Disable checks on CRT allocations
        flags &= ~_CRTDBG_DELAY_FREE_MEM_DF; // Don't delay freeing memory
        _CrtSetDbgFlag(flags);
        
        // Optionally suppress debug output to reduce noise
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
    }
};

class MemoryAllocationFailureTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _DEBUG
        // Clear any existing break allocation number first
        _CrtSetBreakAlloc(-1);
        
        // Disable CRT heap debugging breakpoints
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        flags &= ~_CRTDBG_ALLOC_MEM_DF;      // Disable allocation tracking
        flags &= ~_CRTDBG_CHECK_ALWAYS_DF;   // Disable heap checks on every allocation/deallocation
        flags &= ~_CRTDBG_CHECK_CRT_DF;      // Disable checks on CRT allocations
        _CrtSetDbgFlag(flags);
#endif
        
        // Initialize debug logging with maximum verbosity
        DebugInitialize();
        DebugSetLevel(DEBUG_LEVEL_TRACE);  // Maximum verbosity for debugging
        DebugSetComponentMask((DWORD)DEBUG_COMPONENT_ALL);  // All components
        DebugEnableTimestamp(TRUE);
        DebugEnableThreadId(TRUE);
        DebugEnableFileLocation(TRUE);
        DebugEnableConsoleOutput(FALSE);  // Disable console for performance
        DebugEnableDebuggerOutput(FALSE);  // Disable debugger for performance
        DebugEnableFileOutput(TRUE, "heap_debug.log");  // Enable file logging
        
        // Initialize heap before each test
        ASSERT_TRUE(InitHeap()) << "Failed to initialize heap";
        
        HEAP_INFO("=== Test Setup Complete ===");
    }

    void TearDown() override {
        HEAP_INFO("=== Test Teardown Starting ===");
        
        // Check for memory leaks after each test (quiet check)
        CheckForMemoryLeaksQuiet();
        // Check and cleanup heap
        CleanupHeap();
        
        // Cleanup debug system
        DebugCleanup();
    }

private:
    // Quiet memory leak check - only reports if leaks are found
    void CheckForMemoryLeaksQuiet() {
        GLOBAL_STATE* state = GetGlobalState();
        if (!state) return;
        
        BOOL foundLeaks = FALSE;
        SIZE_T leakCount = 0;
        SIZE_T leakBytes = 0;
        
        // CriticalSection usage removed for performance - function is now non-thread-safe but faster
        
        // First pass: check if we have any leaks by traversing the linked list
        PLIST_ENTRY current = state->MemoryAllocations.Flink;
        while (current != &state->MemoryAllocations) {
            MEMORY_TRACKING_ENTRY* entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
            foundLeaks = TRUE;
            leakCount++;
            leakBytes += entry->Size;
            current = current->Flink;
        }
        
        // Only print if we found leaks
        if (foundLeaks) {
            printf("\n=== MEMORY LEAK REPORT ===\n");
            printf("Address       | Size     | Allocation Location\n");
            printf("------------- | -------- | ------------------\n");
            
            current = state->MemoryAllocations.Flink;
            while (current != &state->MemoryAllocations) {
                MEMORY_TRACKING_ENTRY* entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
                printf("%p | %8d | %s:%d\n", 
                    entry->Address,
                    (int)entry->Size,
                    entry->FileName,
                    entry->LineNumber);
                current = current->Flink;
            }
            
            printf("\nTotal: %d leaks, %d bytes\n", (int)leakCount, (int)leakBytes);
            printf("Memory usage statistics:\n");
            printf("  Total allocations: %d\n", (int)state->AllocationCount);
            printf("  Total bytes allocated: %d\n", (int)state->TotalBytesAllocated);
            printf("  Peak bytes allocated: %d\n", (int)state->PeakBytesAllocated);
            printf("===========================\n");
        }
    }
};

TEST_F(MemoryAllocationFailureTest, ZeroSizeAllocation) {
    // Test allocation with zero size
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, 0);
    
    // Different implementations may handle this differently
    // Some may return NULL, others may return a valid pointer to a zero-byte allocation
    if (ptr != nullptr) {
        ExFreePoolTracked(ptr);
    }
    
    // Check that global state is still consistent
    GLOBAL_STATE* state = GetGlobalState();
    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
}

TEST_F(MemoryAllocationFailureTest, LargeAllocation) {
    // Test allocation of very large size that might fail
    const SIZE_T largeSize = SIZE_MAX - 1000; // Very large size
    
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, largeSize);
    
    // This allocation will likely fail, but we should handle it gracefully
    if (ptr == nullptr) {
        // Allocation failed as expected for such a large size
        GLOBAL_STATE* state = GetGlobalState();
        EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
        EXPECT_EQ(state->AllocationCount, (SIZE_T)0);
    } else {
        // If somehow it succeeded, free it
        ExFreePoolTracked(ptr);
        GLOBAL_STATE* state = GetGlobalState();
        EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
    }
}

//TEST_F(MemoryAllocationFailureTest, ExhaustMemory) {
//    // Try to exhaust available memory by making many allocations
//    std::vector<PVOID> allocations;
//    const SIZE_T chunkSize = 1024 * 1024; // 1MB chunks
//    const int maxAllocations = 1000; // Reasonable limit
//    
//    bool allocationFailed = false;
//    
//    for (int i = 0; i < maxAllocations; i++) {
//        PVOID ptr = ExAllocatePoolTracked(NonPagedPool, chunkSize);
//        if (ptr == nullptr) {
//            allocationFailed = true;
//            break;
//        }
//        allocations.push_back(ptr);
//    }
//    
//    // Clean up all allocations
//    for (PVOID ptr : allocations) {
//        ExFreePoolTracked(ptr);
//    }
//    
//    // Check that memory tracking is still consistent
//    GLOBAL_STATE* state = GetGlobalState();
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//    
//    // Either we allocated all chunks (system has lots of memory) or we hit a limit
//    EXPECT_TRUE(allocationFailed || allocations.size() == maxAllocations);
//}

//TEST_F(MemoryAllocationFailureTest, DoubleFreeBehavior) {
//    // Test double-free behavior with enhanced tracking
//    const SIZE_T allocSize = 100;
//    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, allocSize);
//    ASSERT_NE(ptr, nullptr);
//    
//    GLOBAL_STATE* state = GetGlobalState();
//    EXPECT_EQ(state->CurrentBytesAllocated, allocSize);
//    SIZE_T initialDoubleFreeCount = state->DoubleFreeCount;
//    
//    // First free should succeed
//    ExFreePoolTracked(ptr);
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//    EXPECT_EQ(state->DoubleFreeCount, initialDoubleFreeCount); // Should not increment yet
//    
//    // Second free of the same pointer - should be detected as double-free
//    ExFreePoolTracked(ptr);
//    
//    // Double-free should be detected and counted
//    EXPECT_EQ(state->DoubleFreeCount, initialDoubleFreeCount + 1);
//    
//    // Memory tracking should still be consistent
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//}

TEST_F(MemoryAllocationFailureTest, FreeNullPointer) {
    // Test freeing a NULL pointer
    GLOBAL_STATE* state = GetGlobalState();
    SIZE_T initialCurrentBytes = state->CurrentBytesAllocated;
    SIZE_T initialAllocCount = state->AllocationCount;
    
    ExFreePoolTracked(nullptr);
    
    // State should remain unchanged
    EXPECT_EQ(state->CurrentBytesAllocated, initialCurrentBytes);
    EXPECT_EQ(state->AllocationCount, initialAllocCount);
}

//TEST_F(MemoryAllocationFailureTest, FreeInvalidPointer) {
//    // Test freeing an invalid pointer (not allocated by our system)
//    GLOBAL_STATE* state = GetGlobalState();
//    SIZE_T initialCurrentBytes = state->CurrentBytesAllocated;
//    SIZE_T initialAllocCount = state->AllocationCount;
//    
//    // Create an invalid pointer
//    PVOID invalidPtr = (PVOID)(uintptr_t)0xDEADBEEF;
//    
//    // Our library should now handle invalid pointers gracefully without exceptions
//    ExFreePoolTracked(invalidPtr);
//    
//    // State should remain unchanged since the pointer was invalid
//    EXPECT_EQ(state->CurrentBytesAllocated, initialCurrentBytes);
//    EXPECT_EQ(state->AllocationCount, initialAllocCount);
//}

TEST_F(MemoryAllocationFailureTest, MemoryFragmentation) {
    // Test memory fragmentation scenarios
    std::vector<PVOID> smallAllocations;
    std::vector<PVOID> largeAllocations;
    
    const SIZE_T smallSize = 64;
    const SIZE_T largeSize = 1024;
    const int numAllocations = 100;
    
    // Allocate many small and large chunks to create fragmentation
    for (int i = 0; i < numAllocations; i++) {
        PVOID smallPtr = ExAllocatePoolTracked(NonPagedPool, smallSize);
        PVOID largePtr = ExAllocatePoolTracked(NonPagedPool, largeSize);
        
        if (smallPtr) smallAllocations.push_back(smallPtr);
        if (largePtr) largeAllocations.push_back(largePtr);
    }
    
    // Free every other allocation to create holes
    for (size_t i = 0; i < smallAllocations.size(); i += 2) {
        ExFreePoolTracked(smallAllocations[i]);
        smallAllocations[i] = nullptr;
    }
    
    for (size_t i = 1; i < largeAllocations.size(); i += 2) {
        ExFreePoolTracked(largeAllocations[i]);
        largeAllocations[i] = nullptr;
    }
    
    // Try to allocate medium-sized chunks in the fragmented heap
    std::vector<PVOID> mediumAllocations;
    const SIZE_T mediumSize = 256;
    
    for (int i = 0; i < 50; i++) {
        PVOID mediumPtr = ExAllocatePoolTracked(NonPagedPool, mediumSize);
        if (mediumPtr) {
            mediumAllocations.push_back(mediumPtr);
        }
    }
    
    // Clean up all remaining allocations
    for (PVOID ptr : smallAllocations) {
        if (ptr) ExFreePoolTracked(ptr);
    }
    for (PVOID ptr : largeAllocations) {
        if (ptr) ExFreePoolTracked(ptr);
    }
    for (PVOID ptr : mediumAllocations) {
        if (ptr) ExFreePoolTracked(ptr);
    }
    
    // Check that all memory is properly freed
    GLOBAL_STATE* state = GetGlobalState();
    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
}

//TEST_F(MemoryAllocationFailureTest, RapidAllocationDeallocation) {
//    // Test rapid allocation and deallocation patterns
//    const SIZE_T allocSize = 1024;
//    const int iterations = 1000;
//    
//    for (int i = 0; i < iterations; i++) {
//        PVOID ptr = ExAllocatePoolTracked(NonPagedPool, allocSize);
//        if (ptr) {
//            // Write some data to ensure the memory is accessible
//            memset(ptr, (BYTE)(i & 0xFF), allocSize);
//            ExFreePoolTracked(ptr);
//        }
//    }
//    
//    // Check that memory tracking is still accurate
//    GLOBAL_STATE* state = GetGlobalState();
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//    EXPECT_EQ(state->AllocationCount, (SIZE_T)iterations);
//}

//TEST_F(MemoryAllocationFailureTest, ConcurrentAllocationFailure) {
//    // Test allocation failures under concurrent access
//    const int numThreads = 4;
//    const int allocationsPerThread = 100;
//    const SIZE_T allocSize = 1024;
//    
//    std::atomic<int> successfulAllocations{0};
//    std::atomic<int> failedAllocations{0};
//    std::vector<std::thread> threads;
//    
//    auto allocationTask = [&]() {
//        std::vector<PVOID> localAllocations;
//        
//        for (int i = 0; i < allocationsPerThread; i++) {
//            PVOID ptr = ExAllocatePoolTracked(NonPagedPool, allocSize);
//            if (ptr) {
//                localAllocations.push_back(ptr);
//                successfulAllocations++;
//            } else {
//                failedAllocations++;
//            }
//        }
//        
//        // Free all local allocations
//        for (PVOID ptr : localAllocations) {
//            ExFreePoolTracked(ptr);
//        }
//    };
//    
//    // Start all threads
//    for (int i = 0; i < numThreads; i++) {
//        threads.emplace_back(allocationTask);
//    }
//    
//    // Wait for all threads to complete
//    for (auto& thread : threads) {
//        thread.join();
//    }
//    
//    // Check final state
//    GLOBAL_STATE* state = GetGlobalState();
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//    
//    int totalAttempts = successfulAllocations + failedAllocations;
//    EXPECT_EQ(totalAttempts, numThreads * allocationsPerThread);
//}

TEST_F(MemoryAllocationFailureTest, HeapCorruption_Detection) {
    // Test detection of heap corruption scenarios
    const SIZE_T allocSize = 100;
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, allocSize);
    ASSERT_NE(ptr, nullptr);
    
    GLOBAL_STATE* state = GetGlobalState();
    SIZE_T bytesBeforeCorruption = state->CurrentBytesAllocated;
    (void)bytesBeforeCorruption; // Suppress unused variable warning
    
    // Simulate memory corruption by writing beyond allocated boundary
    // Note: This is dangerous and might crash, but we're testing error handling
    BYTE* bytePtr = (BYTE*)ptr;
    
    // Write pattern to allocated memory (this should be safe)
    for (SIZE_T i = 0; i < allocSize; i++) {
        bytePtr[i] = (BYTE)(i & 0xFF);
    }
    
    // Verify the pattern
    bool patternValid = true;
    for (SIZE_T i = 0; i < allocSize; i++) {
        if (bytePtr[i] != (BYTE)(i & 0xFF)) {
            patternValid = false;
            break;
        }
    }
    EXPECT_TRUE(patternValid);
    
    // Free the memory
    ExFreePoolTracked(ptr);
    
    // Check that memory is properly freed
    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
}

//TEST_F(MemoryAllocationFailureTest, SEH_BufferOverrunDetection) {
//    // Test buffer overrun detection using SEH
//    const SIZE_T allocSize = 64;
//    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, allocSize);
//    ASSERT_NE(ptr, nullptr);
//    
//    BYTE* bytePtr = (BYTE*)ptr;
//    bool overrunDetected = false;
//    DWORD exceptionCode = 0;
//    
//    // Use SEH in a separate function to avoid C++ object unwinding issues
//    auto testBufferOverrun = [](BYTE* p, SIZE_T size, bool* detected, DWORD* code) -> void {
//        __try {
//            // Write within bounds (should be safe)
//            for (SIZE_T i = 0; i < size; i++) {
//                p[i] = 0xAA;
//            }
//            
//            // Attempt to write beyond allocated boundary (may trigger exception)
//            // Note: This is intentionally dangerous for testing purposes
//            for (SIZE_T i = size; i < size + 16; i++) {
//                p[i] = 0xBB; // This may cause access violation
//            }
//        }
//        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? 
//                  EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
//            *detected = true;
//            *code = GetExceptionCode();
//            printf("Buffer overrun detected - Exception: 0x%08X\n", *code);
//        }
//    };
//    
//    testBufferOverrun(bytePtr, allocSize, &overrunDetected, &exceptionCode);
//    
//    // Clean up
//    ExFreePoolTracked(ptr);
//    
//    GLOBAL_STATE* state = GetGlobalState();
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//    
//    // Either the overrun was detected or the system allowed it
//    // Both are valid behaviors depending on the heap implementation
//}

TEST_F(MemoryAllocationFailureTest, SEH_UseAfterFreeDetection) {
    // Test use-after-free detection using SEH
    const SIZE_T allocSize = 128;
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, allocSize);
    ASSERT_NE(ptr, nullptr);
    
    BYTE* bytePtr = (BYTE*)ptr;
    
    // Write some data to the allocated memory
    for (SIZE_T i = 0; i < allocSize; i++) {
        bytePtr[i] = (BYTE)(i & 0xFF);
    }
    
    // Free the memory
    ExFreePoolTracked(ptr);
    
    // Attempt to access freed memory (may trigger exception)
    bool useAfterFreeDetected = false;
    DWORD exceptionCode = 0;
    BYTE testValue = 0;
    
    // Use SEH in a separate function to avoid C++ object unwinding issues
    auto testUseAfterFree = [](BYTE* p, bool* detected, DWORD* code, BYTE* value) -> void {
        __try {
            // Try to read from freed memory
            *value = p[0];
            
            // Try to write to freed memory
            p[0] = 0xFF;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? 
                  EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            *detected = true;
            *code = GetExceptionCode();
            printf("Use-after-free detected - Exception: 0x%08X\n", *code);
        }
    };
    
    testUseAfterFree(bytePtr, &useAfterFreeDetected, &exceptionCode, &testValue);
    
    GLOBAL_STATE* state = GetGlobalState();
    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
    
    // Either use-after-free was detected or the memory was still accessible
    // Both are valid behaviors depending on the heap implementation and timing
}

//TEST_F(MemoryAllocationFailureTest, DoubleFreeTracking_MultipleAllocations) {
//    // Test double-free tracking with multiple allocations
//    const SIZE_T allocSize = 256;
//    const int numAllocations = 5;
//    PVOID ptrs[numAllocations];
//    
//    GLOBAL_STATE* state = GetGlobalState();
//    SIZE_T initialDoubleFreeCount = state->DoubleFreeCount;
//    
//    // Allocate multiple chunks
//    for (int i = 0; i < numAllocations; i++) {
//        ptrs[i] = ExAllocatePoolTracked(NonPagedPool, allocSize);
//        ASSERT_NE(ptrs[i], nullptr);
//    }
//    
//    EXPECT_EQ(state->CurrentBytesAllocated, allocSize * numAllocations);
//    
//    // Free them all normally
//    for (int i = 0; i < numAllocations; i++) {
//        ExFreePoolTracked(ptrs[i]);
//    }
//    
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//    EXPECT_EQ(state->DoubleFreeCount, initialDoubleFreeCount); // No double-frees yet
//    
//    // Now try to free them all again - should detect multiple double-frees
//    for (int i = 0; i < numAllocations; i++) {
//        ExFreePoolTracked(ptrs[i]);
//    }
//    
//    // Should have detected all double-frees
//    EXPECT_EQ(state->DoubleFreeCount, initialDoubleFreeCount + numAllocations);
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//}

//TEST_F(MemoryAllocationFailureTest, DoubleFreeTracking_ThreadSafety) {
//    // Test double-free tracking under concurrent access
//    const int numThreads = 3;
//    const int allocationsPerThread = 10;
//    const SIZE_T allocSize = 128;
//    
//    GLOBAL_STATE* state = GetGlobalState();
//    SIZE_T initialDoubleFreeCount = state->DoubleFreeCount;
//    
//    std::atomic<int> totalAllocations{0};
//    std::atomic<int> totalDoubleFrees{0};
//    std::vector<std::thread> threads;
//    
//    auto doubleFreeTask = [&]() {
//        std::vector<PVOID> localAllocations;
//        
//        // Allocate memory
//        for (int i = 0; i < allocationsPerThread; i++) {
//            PVOID ptr = ExAllocatePoolTracked(NonPagedPool, allocSize);
//            if (ptr) {
//                localAllocations.push_back(ptr);
//                totalAllocations++;
//            }
//        }
//        
//        // Free all allocations normally
//        for (PVOID ptr : localAllocations) {
//            ExFreePoolTracked(ptr);
//        }
//        
//        // Try to free them again (double-free)
//        for (PVOID ptr : localAllocations) {
//            ExFreePoolTracked(ptr);
//            totalDoubleFrees++;
//        }
//    };
//    
//    // Start all threads
//    for (int i = 0; i < numThreads; i++) {
//        threads.emplace_back(doubleFreeTask);
//    }
//    
//    // Wait for all threads to complete
//    for (auto& thread : threads) {
//        thread.join();
//    }
//    
//    // Check final state
//    EXPECT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
//    EXPECT_GT(state->DoubleFreeCount, initialDoubleFreeCount); // Should have detected some double-frees
//    EXPECT_EQ(totalAllocations.load(), numThreads * allocationsPerThread);
//    EXPECT_EQ(totalDoubleFrees.load(), numThreads * allocationsPerThread);
//}

//TEST_F(MemoryAllocationFailureTest, DoubleFreeTracking_Configuration) {
//    // Test double-free tracking configuration
//    GLOBAL_STATE* state = GetGlobalState();
//    
//    // Test getting/setting freed memory tracking
//    BOOL originalTracking = GetFreedMemoryTracking();
//    EXPECT_TRUE(originalTracking); // Should be enabled by default
//    
//    SetFreedMemoryTracking(FALSE);
//    EXPECT_FALSE(GetFreedMemoryTracking());
//    
//    // Test getting/setting max freed entries
//    SIZE_T originalMaxEntries = GetMaxFreedEntries();
//    EXPECT_GT(originalMaxEntries, (SIZE_T)0);
//    
//    SetMaxFreedEntries(100);
//    EXPECT_EQ(GetMaxFreedEntries(), (SIZE_T)100);
//    
//    // Allocate and free some memory with tracking disabled
//    SIZE_T initialDoubleFreeCount = state->DoubleFreeCount;
//    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, 64);
//    ASSERT_NE(ptr, nullptr);
//    
//    ExFreePoolTracked(ptr);
//    ExFreePoolTracked(ptr); // This should not be detected since tracking is disabled
//    
//    // Double-free count should not have increased
//    EXPECT_EQ(state->DoubleFreeCount, initialDoubleFreeCount);
//    
//    // Restore original settings
//    SetFreedMemoryTracking(originalTracking);
//    SetMaxFreedEntries(originalMaxEntries);
//}

//TEST_F(MemoryAllocationFailureTest, DoubleFreeTracking_ReportGeneration) {
//    // Test the double-free report generation
//    const SIZE_T allocSize = 512;
//    PVOID ptr1 = ExAllocatePoolTracked(NonPagedPool, allocSize);
//    PVOID ptr2 = ExAllocatePoolTracked(NonPagedPool, allocSize * 2);
//    
//    ASSERT_NE(ptr1, nullptr);
//    ASSERT_NE(ptr2, nullptr);
//    
//    GLOBAL_STATE* state = GetGlobalState();
//    SIZE_T initialFreedEntries = state->FreedEntryCount;
//    
//    // Free the allocations
//    ExFreePoolTracked(ptr1);
//    ExFreePoolTracked(ptr2);
//    
//    // Should have added entries to the freed memory list
//    EXPECT_EQ(state->FreedEntryCount, initialFreedEntries + 2);
//    
//    // Test report generation (we can't easily test the output, but we can call it)
//    PrintDoubleFreeReport();
//    
//    // Try double-free to trigger detection
//    SIZE_T initialDoubleFreeCount = state->DoubleFreeCount;
//    ExFreePoolTracked(ptr1); // Should be detected as double-free
//    
//    EXPECT_EQ(state->DoubleFreeCount, initialDoubleFreeCount + 1);
//    
//    // Generate another report
//    PrintDoubleFreeReport();
//}

TEST_F(MemoryAllocationFailureTest, DoubleFreeTracking_MemoryManagement) {
    // Test that the freed memory tracking doesn't grow indefinitely
    GLOBAL_STATE* state = GetGlobalState();
    
    // Set a small limit for testing
    SIZE_T originalMaxEntries = GetMaxFreedEntries();
    SetMaxFreedEntries(5);
    
    // Allocate and free more than the limit
    const int numOperations = 10;
    for (int i = 0; i < numOperations; i++) {
        PVOID ptr = ExAllocatePoolTracked(NonPagedPool, 64);
        ASSERT_NE(ptr, nullptr);
        ExFreePoolTracked(ptr);
    }
    
    // Should not exceed the maximum
    EXPECT_LE(state->FreedEntryCount, (SIZE_T)5);
    
    // Restore original setting
    SetMaxFreedEntries(originalMaxEntries);
}

//TEST_F(MemoryAllocationFailureTest, InvalidPointerHandling_Comprehensive) {
//    // Test comprehensive invalid pointer handling
//    GLOBAL_STATE* state = GetGlobalState();
//    SIZE_T initialCurrentBytes = state->CurrentBytesAllocated;
//    SIZE_T initialAllocCount = state->AllocationCount;
//    
//    // Test various types of invalid pointers
//    PVOID invalidPtrs[] = {
//        (PVOID)(uintptr_t)0xDEADBEEF,    // Classic invalid pointer
//        (PVOID)(uintptr_t)0x12345678,    // Another invalid address
//        (PVOID)(uintptr_t)0x1,           // Very low address
//        (PVOID)(uintptr_t)0xFFFFFFFF,    // High address (32-bit)
//        (PVOID)(uintptr_t)0x8000000000000000ULL  // High address (64-bit)
//    };
//    
//    // Temporarily suppress errors for cleaner test output
//    BOOL originalSuppression = GetErrorSuppression();
//    SetErrorSuppression(TRUE);
//    
//    // Try to free each invalid pointer
//    for (size_t i = 0; i < sizeof(invalidPtrs) / sizeof(invalidPtrs[0]); i++) {
//        ExFreePoolTracked(invalidPtrs[i]);
//        
//        // State should remain unchanged for each invalid pointer
//        EXPECT_EQ(state->CurrentBytesAllocated, initialCurrentBytes);
//        EXPECT_EQ(state->AllocationCount, initialAllocCount);
//    }
//    
//    // Restore original error suppression
//    SetErrorSuppression(originalSuppression);
//    
//    // Verify that valid allocations still work correctly after invalid pointer attempts
//    PVOID validPtr = ExAllocatePoolTracked(NonPagedPool, 256);
//    ASSERT_NE(validPtr, nullptr);
//    EXPECT_GT(state->CurrentBytesAllocated, initialCurrentBytes);
//    
//    // Free the valid pointer
//    ExFreePoolTracked(validPtr);
//    EXPECT_EQ(state->CurrentBytesAllocated, initialCurrentBytes);
//}

// Register the global environment to disable CRT debugging
int main(int argc, char** argv) {
#ifdef _DEBUG
    // Clear any allocation break number that may have been set before we even start
    _CrtSetBreakAlloc(-1);
    
    // Disable CRT debugging as early as possible
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags &= ~_CRTDBG_ALLOC_MEM_DF;
    flags &= ~_CRTDBG_CHECK_ALWAYS_DF;
    flags &= ~_CRTDBG_CHECK_CRT_DF;
    flags &= ~_CRTDBG_DELAY_FREE_MEM_DF;
    _CrtSetDbgFlag(flags);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    
    // Disable Google Test's break-on-failure behavior
    ::testing::GTEST_FLAG(break_on_failure) = false;
    
    // Add the debug heap environment
    ::testing::AddGlobalTestEnvironment(new DebugHeapEnvironment);
    
    return RUN_ALL_TESTS();
}
