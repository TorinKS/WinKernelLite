/*
 * Test program to debug heap corruption with extensive logging
 */

#include <gtest/gtest.h>
#include <WinKernelLite/Debug.h>
#include <WinKernelLite/KernelHeap.h>
#include "WinKernelLiteTestBase.h"

class HeapDebuggingTest : public WinKernelLiteTestBase {
protected:
    void SetUp() override {
        WinKernelLiteTestBase::SetUp();
        LogTestInfo("Heap debugging test suite initialized with enhanced tracking");
    }
    
    void TearDown() override {
        LogTestInfo("Heap debugging test suite completed");
        WinKernelLiteTestBase::TearDown();
    }
};

TEST_F(HeapDebuggingTest, AllocateAndFreeWithTracking) {
    LogTestStep("Allocation and free tracking test");
    
    // Allocate some memory
    PVOID ptr1 = ExAllocatePoolTracked(NonPagedPool, 64);
    ASSERT_NE(ptr1, nullptr);
    LogTestInfo("Allocated ptr1: %p (64 bytes)", ptr1);
    
    PVOID ptr2 = ExAllocatePoolTracked(NonPagedPool, 128);
    ASSERT_NE(ptr2, nullptr);
    LogTestInfo("Allocated ptr2: %p (128 bytes)", ptr2);
    
    LogTestStep("Freeing first allocation");
    // Free the first pointer
    ExFreePoolTracked(ptr1);
    LogTestInfo("Freed ptr1: %p", ptr1);
    
    LogTestStep("Freeing second allocation");
    // Free the second pointer
    ExFreePoolTracked(ptr2);
    LogTestInfo("Freed ptr2: %p", ptr2);
    
    LogTestInfo("Basic allocation and free tracking test completed successfully");
}

TEST_F(HeapDebuggingTest, MultipleAllocationsAndFrees) {
    LogTestStep("Multiple allocations tracking test");
    
    const int numAllocations = 10;
    PVOID ptrs[numAllocations];
    
    LogTestInfo("Starting %d sequential allocations for tracking test", numAllocations);
    
    // Allocate multiple blocks
    for (int i = 0; i < numAllocations; i++) {
        SIZE_T size = 32 * (i + 1);
        ptrs[i] = ExAllocatePoolTracked(NonPagedPool, size);
        ASSERT_NE(ptrs[i], nullptr);
        LogTestInfo("Allocated ptr[%d]: %p (%zu bytes)", i, ptrs[i], size);
    }
    
    LogTestStep("Freeing allocations in reverse order");
    // Free them in reverse order
    for (int i = numAllocations - 1; i >= 0; i--) {
        LogTestInfo("About to free ptr[%d]: %p", i, ptrs[i]);
        ExFreePoolTracked(ptrs[i]);
        LogTestInfo("Successfully freed ptr[%d]", i);
    }
    
    LogTestInfo("Multiple allocations tracking test completed successfully");
}

//TEST_F(HeapDebuggingTest, FreedMemoryTracking) {
//    LogTestStep("Double-free detection test");
//    
//    // Enable freed memory tracking
//    SetFreedMemoryTracking(TRUE);
//    SetMaxFreedEntries(100);
//    LogTestInfo("Freed memory tracking enabled with max 100 entries");
//    
//    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, 256);
//    ASSERT_NE(ptr, nullptr);
//    LogTestInfo("Allocated tracking test ptr: %p (256 bytes)", ptr);
//    
//    LogTestStep("Performing first free operation");
//    // Free it
//    ExFreePoolTracked(ptr);
//    LogTestInfo("Successfully freed tracking test ptr: %p", ptr);
//    
//    LogTestStep("Attempting double-free (should be detected)");
//    // Try to free it again (should be caught as double-free)
//    LogTestInfo("About to attempt double-free of ptr: %p", ptr);
//    ExFreePoolTracked(ptr);
//    LogTestInfo("Double-free attempt completed - check logs for detection");
//    
//    LogTestInfo("Freed memory tracking test completed");
//}