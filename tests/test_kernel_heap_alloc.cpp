#include <gtest/gtest.h>
#include <Windows.h>
#include <thread>
#include <vector>
#include <atomic>
#include "../include/KernelHeapAlloc.h"

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
        // Check for memory leaks after each test
        PrintMemoryLeaks();
        // Check and cleanup heap
        CleanupHeap();
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
    
    // Verify tracking entry
    bool found = false;
    for (SIZE_T i = 0; i < MAX_ALLOCATIONS; i++) {
        if (state->MemoryAllocations[i].Address == ptr && state->MemoryAllocations[i].IsAllocated) {
            found = true;
            EXPECT_EQ(state->MemoryAllocations[i].Size, size);
            EXPECT_NE(state->MemoryAllocations[i].FileName, nullptr);
            EXPECT_GT(state->MemoryAllocations[i].LineNumber, 0);
            break;
        }
    }
    ASSERT_TRUE(found) << "Allocation not properly tracked";
    
    ExFreePoolTracked(ptr);
    
    // Verify tracking entry is marked as freed
    found = false;
    for (SIZE_T i = 0; i < MAX_ALLOCATIONS; i++) {
        if (state->MemoryAllocations[i].Address == ptr && !state->MemoryAllocations[i].IsAllocated) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found) << "Free not properly tracked";
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
    
    // Second allocation should reuse the freed entry
    PVOID ptr2 = ExAllocatePoolTracked(NonPagedPool, size);
    
    SIZE_T usedEntries = 0;
    for (SIZE_T i = 0; i < MAX_ALLOCATIONS; i++) {
        if (state->MemoryAllocations[i].Address != nullptr) {
            usedEntries++;
        }
    }
    
    ASSERT_EQ(usedEntries, (SIZE_T)1) << "Freed entry not reused";
    ExFreePoolTracked(ptr2);
}

TEST_F(KernelHeapAllocTest, MaxAllocations) {
    GLOBAL_STATE* state = GetGlobalState();
    
    // Test size
    const SIZE_T size = 4;
    
    // Suppress error messages during this test
    SetErrorSuppression(TRUE);
    
    // Force the tracking table to be marked as full
    state->TrackingTableFull = TRUE;
    
    // Allocate when tracking table is full - should succeed
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
    
    // Verify allocation succeeded
    ASSERT_NE(ptr, nullptr) << "Allocation failed when tracking table is full";
    
    // Free the memory
    ExFreePoolTracked(ptr);
    
    // Reset tracking table state
    state->TrackingTableFull = FALSE;
    
    // Restore error reporting
    SetErrorSuppression(FALSE);
}

TEST_F(KernelHeapAllocTest, ThreadSafety) {
    GLOBAL_STATE* state = GetGlobalState();
    const int numThreads = 4;
    const int allocsPerThread = 100;
    const SIZE_T size = 100;
    std::atomic<bool> failed{false};
    std::atomic<int> totalAllocations{0};
    std::atomic<int> failedAllocations{0};
    
    std::vector<std::thread> threads;
    
    // Create threads that allocate and free memory
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([&]() {
            std::vector<PVOID> threadPtrs;
            
            for (int j = 0; j < allocsPerThread; j++) {
                PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
                if (ptr == nullptr) {
                    failedAllocations++;
                    continue; // Skip this allocation and continue
                }
                totalAllocations++;
                threadPtrs.push_back(ptr);
            }
            
            for (PVOID ptr : threadPtrs) {
                ExFreePoolTracked(ptr);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // We should have at least some successful allocations
    ASSERT_GT(totalAllocations, 0) << "No allocations succeeded";
    
    // Print allocation statistics
    if (failedAllocations > 0) {
        printf("Note: %d allocations failed out of %d total attempted\n", 
               failedAllocations.load(), totalAllocations.load() + failedAllocations.load());
    }
    
    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0) << "Memory leak in threaded test";
}

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
    ASSERT_EQ(state->AllocationCount, (SIZE_T)0) << "Failed allocation should not be tracked";
    ASSERT_EQ(state->CurrentBytesAllocated, (SIZE_T)0) << "Failed allocation should not affect bytes count";
    
    SetErrorSuppression(FALSE); // Restore error messages
}
