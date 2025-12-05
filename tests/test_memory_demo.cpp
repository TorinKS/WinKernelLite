#include <gtest/gtest.h>
#include <WinKernelLite/KernelHeap.h>
#include <WinKernelLite/Debug.h>
#include "WinKernelLiteTestBase.h"

class MemoryDemoTest : public WinKernelLiteTestBase {
protected:
    void SetUp() override {
        WinKernelLiteTestBase::SetUp();
        LogTestInfo("Memory demonstration test initialized");
    }
    
    void TearDown() override {
        LogTestInfo("Memory demonstration test completed");
        WinKernelLiteTestBase::TearDown();
    }
};

TEST_F(MemoryDemoTest, CleanMemoryTest) {
    LogTestStep("Testing clean memory usage");
    
    // Allocate memory and properly free it
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, 512);
    ASSERT_NE(ptr, nullptr);
    LogTestInfo("Allocated 512 bytes at address: %p", ptr);
    
    // Use the memory
    memset(ptr, 0xAA, 512);
    
    // Properly free the memory
    ExFreePoolTracked(ptr);
    LogTestInfo("Freed memory - this should show MEMORY_CLEAN status");
}

TEST_F(MemoryDemoTest, MemoryLeakTest) {
    LogTestStep("Testing memory leak detection");
    
    // Allocate memory but don't free it (intentional leak)
    PVOID ptr1 = ExAllocatePoolTracked(NonPagedPool, 256);
    PVOID ptr2 = ExAllocatePoolTracked(NonPagedPool, 128);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    LogTestInfo("Allocated 256 bytes at: %p", ptr1);
    LogTestInfo("Allocated 128 bytes at: %p", ptr2);
    LogTestInfo("Intentionally NOT freeing memory - should show MEMORY_PROBLEMS");
    
    // Don't free ptr1 and ptr2 - this will create a memory leak
}

//TEST_F(MemoryDemoTest, DoubleFreeTest) {
//    LogTestStep("Testing double-free detection");
//    
//    // Allocate memory
//    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, 64);
//    ASSERT_NE(ptr, nullptr);
//    LogTestInfo("Allocated 64 bytes at: %p", ptr);
//    
//    // Free it once (properly)
//    ExFreePoolTracked(ptr);
//    LogTestInfo("Freed memory once (proper)");
//    
//    // Try to free it again (double-free)
//    ExFreePoolTracked(ptr);
//    LogTestInfo("Attempted double-free - should show MEMORY_PROBLEMS");
//}