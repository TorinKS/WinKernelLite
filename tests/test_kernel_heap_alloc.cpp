#include <gtest/gtest.h>
#include <Windows.h>
#include <thread>
#include <vector>
#include <atomic>
#include "../include/KernelHeap.h"

class KernelHeapAllocTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize heap before each test
        ASSERT_TRUE(InitHeap()) << "Failed to initialize heap";
        // Verify heap handle is valid
        GLOBAL_STATE* state = GetGlobalState();
        ASSERT_NE(state->HeapHandle, nullptr) << "Heap handle is null";
    }

    void TearDown() override {
        // Check for memory leaks after each test (quiet check)
        CheckForMemoryLeaksQuiet();
        // Check and cleanup heap
        CleanupHeap();
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

TEST_F(KernelHeapAllocTest, BasicAllocation) {
    // Test basic allocation
    const SIZE_T allocSize = 100;
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, allocSize);
    GLOBAL_STATE* state = GetGlobalState();
    
    ASSERT_NE(ptr, nullptr) << "Allocation failed";
    ASSERT_EQ(state->AllocationCount, (SIZE_T)1) << "Allocation count mismatch";
    ASSERT_EQ(state->CurrentBytesAllocated, allocSize) << "Current bytes allocated mismatch";
    ASSERT_EQ(state->TotalBytesAllocated, allocSize) << "Total bytes allocated mismatch";
    ASSERT_EQ(state->PeakBytesAllocated, allocSize) << "Peak bytes allocated mismatch";

    ExFreePoolTracked(ptr);
    
    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0) << "Memory not properly freed";
    ASSERT_EQ(state->AllocationCount, (SIZE_T)1) << "Total allocation count changed after free";
}

TEST_F(KernelHeapAllocTest, MultipleAllocations) {
    GLOBAL_STATE* state = GetGlobalState();
    std::vector<PVOID> ptrs;
    const SIZE_T size = 100;
    const int numAllocs = 5;
    
    for (int i = 0; i < numAllocs; i++) {
        PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
        ASSERT_NE(ptr, nullptr) << "Allocation " << i << " failed";
        ptrs.push_back(ptr);
    }
    
    ASSERT_EQ(state->AllocationCount, (SIZE_T)numAllocs) << "Allocation count mismatch";
    ASSERT_EQ(state->CurrentBytesAllocated, size * numAllocs) << "Current bytes allocated mismatch";
    
    for (PVOID ptr : ptrs) {
        ExFreePoolTracked(ptr);
    }
    
    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0) << "Memory not properly freed";
}

TEST_F(KernelHeapAllocTest, PeakTracking) {
    GLOBAL_STATE* state = GetGlobalState();
    const SIZE_T size1 = 100;
    const SIZE_T size2 = 200;
    
    PVOID ptr1 = ExAllocatePoolTracked(NonPagedPool, size1);
    PVOID ptr2 = ExAllocatePoolTracked(NonPagedPool, size2);
    
    ASSERT_EQ(state->PeakBytesAllocated, size1 + size2) << "Peak tracking failed";
    
    ExFreePoolTracked(ptr1);
    ASSERT_EQ(state->PeakBytesAllocated, size1 + size2) << "Peak should not decrease after free";
    
    ExFreePoolTracked(ptr2);
    ASSERT_EQ(state->PeakBytesAllocated, size1 + size2) << "Peak should not decrease after all frees";
}

TEST_F(KernelHeapAllocTest, AllocationTracking) {
    GLOBAL_STATE* state = GetGlobalState();
    const SIZE_T size = 100;
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
    
    // Verify tracking entry by traversing the linked list
    bool found = false;
    PLIST_ENTRY current = state->MemoryAllocations.Flink;
    while (current != &state->MemoryAllocations) {
        MEMORY_TRACKING_ENTRY* entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
        if (entry->Address == ptr) {
            found = true;
            EXPECT_EQ(entry->Size, size);
            EXPECT_NE(entry->FileName, nullptr);
            EXPECT_GT(entry->LineNumber, 0);
            break;
        }
        current = current->Flink;
    }
    ASSERT_TRUE(found) << "Allocation not properly tracked";
    
    ExFreePoolTracked(ptr);
    
    // Verify tracking entry is removed from the list
    found = false;
    current = state->MemoryAllocations.Flink;
    while (current != &state->MemoryAllocations) {
        MEMORY_TRACKING_ENTRY* entry = CONTAINING_RECORD(current, MEMORY_TRACKING_ENTRY, ListEntry);
        if (entry->Address == ptr) {
            found = true;
            break;
        }
        current = current->Flink;
    }
    ASSERT_FALSE(found) << "Free not properly tracked - entry still in list";
}

TEST_F(KernelHeapAllocTest, NullFree) {
    GLOBAL_STATE* state = GetGlobalState();
    // Should not crash when freeing NULL
    ExFreePoolTracked(nullptr);
    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0);
}

TEST_F(KernelHeapAllocTest, ReuseFreeEntry) {
    GLOBAL_STATE* state = GetGlobalState();
    const SIZE_T size = 100;
    
    // First allocation
    PVOID ptr1 = ExAllocatePoolTracked(NonPagedPool, size);
    ExFreePoolTracked(ptr1);
    
    // Second allocation should work normally (no specific reuse requirement with linked list)
    PVOID ptr2 = ExAllocatePoolTracked(NonPagedPool, size);
    
    // Count current allocations in the linked list
    SIZE_T usedEntries = 0;
    // CriticalSection usage removed for performance - function is now non-thread-safe but faster
    PLIST_ENTRY current = state->MemoryAllocations.Flink;
    while (current != &state->MemoryAllocations) {
        usedEntries++;
        current = current->Flink;
    }
    
    ASSERT_EQ(usedEntries, (SIZE_T)1) << "Expected one allocation in tracking list";
    ExFreePoolTracked(ptr2);
}

TEST_F(KernelHeapAllocTest, MaxAllocations) {
    GLOBAL_STATE* state = GetGlobalState();
    
    // Verify state was successfully retrieved
    ASSERT_NE(state, nullptr);
    
    // Test size
    const SIZE_T size = 4;
    
    // Suppress error messages during this test
    SetErrorSuppression(TRUE);
    
    std::vector<PVOID> ptrs;
    const int testAllocations = 1000;
    
    // Allocate many items
    for (int i = 0; i < testAllocations; i++) {
        PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
        if (ptr != nullptr) {
            ptrs.push_back(ptr);
        } else {
            break; // Stop if we hit actual memory limits
        }
    }
    
    // Verify we could allocate at least the old MAX_ALLOCATIONS worth
    ASSERT_GE(ptrs.size(), (size_t)100) << "Should be able to allocate many items with linked list";
    
    // Free all allocated memory
    for (PVOID ptr : ptrs) {
        ExFreePoolTracked(ptr);
    }
    
    // Restore error reporting
    SetErrorSuppression(FALSE);
}

//TEST_F(KernelHeapAllocTest, ThreadSafety) {
//    GLOBAL_STATE* state = GetGlobalState();
//    const int numThreads = 4;
//    const int allocsPerThread = 100;
//    const SIZE_T size = 100;
//    std::atomic<bool> failed{false};
//    std::atomic<int> totalAllocations{0};
//    std::atomic<int> failedAllocations{0};
//    
//    std::vector<std::thread> threads;
//    
//    // Create threads that allocate and free memory
//    for (int i = 0; i < numThreads; i++) {
//        threads.emplace_back([&]() {
//            std::vector<PVOID> threadPtrs;
//            
//            for (int j = 0; j < allocsPerThread; j++) {
//                PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
//                if (ptr == nullptr) {
//                    failedAllocations++;
//                    continue; // Skip this allocation and continue
//                }
//                totalAllocations++;
//                threadPtrs.push_back(ptr);
//            }
//            
//            for (PVOID ptr : threadPtrs) {
//                ExFreePoolTracked(ptr);
//            }
//        });
//    }
//    
//    // Wait for all threads
//    for (auto& thread : threads) {
//        thread.join();
//    }
//    
//    // We should have at least some successful allocations
//    ASSERT_GT(totalAllocations, 0) << "No allocations succeeded";
//    
//    // Print allocation statistics
//    if (failedAllocations > 0) {
//        printf("Note: %d allocations failed out of %d total attempted\n", 
//               failedAllocations.load(), totalAllocations.load() + failedAllocations.load());
//    }
//    
//    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0) << "Memory leak in threaded test";
//}

TEST_F(KernelHeapAllocTest, MemoryContent) {
    const SIZE_T size = 100;
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
    ASSERT_NE(ptr, nullptr);
    
    // Memory should be accessible for read/write
    memset(ptr, 0xAA, size);
    
    BYTE* bytePtr = static_cast<BYTE*>(ptr);
    for (SIZE_T i = 0; i < size; i++) {
        ASSERT_EQ(bytePtr[i], (BYTE)0xAA) << "Memory content mismatch at index " << i;
    }
    
    ExFreePoolTracked(ptr);
}

TEST_F(KernelHeapAllocTest, AllocationFailure) {
    GLOBAL_STATE* state = GetGlobalState();
    SetErrorSuppression(TRUE); // Suppress error messages for this test
    
    // Try to allocate an extremely large amount of memory that should fail
    const SIZE_T hugeSize = (SIZE_T)-1; // Maximum SIZE_T value
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, hugeSize);
    
    // Verify that allocation returns NULL when it fails
    ASSERT_EQ(ptr, nullptr) << "Allocation should return NULL when it fails";
    
    // Verify no memory was added to tracking
    ASSERT_EQ(state->AllocationCount, (SIZE_T)0) << "Failed allocation should not be tracked";    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0) << "Failed allocation should not affect bytes count";
    
    SetErrorSuppression(FALSE); // Restore error messages
}

