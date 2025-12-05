#include <gtest/gtest.h>
#include <Windows.h>
#include <vector>
#include <memory>
#include <WinKernelLite/KernelHeap.h>
#include <WinKernelLite/Debug.h>
#include "WinKernelLiteTestBase.h"

class MemoryAllocationTest : public WinKernelLiteTestBase {
protected:
    void SetUp() override {
        WinKernelLiteTestBase::SetUp();
        LogTestInfo("Memory allocation test suite initialized");
    }
    
    void TearDown() override {
        LogTestInfo("Memory allocation test suite completed");
        WinKernelLiteTestBase::TearDown();
    }
};

TEST_F(MemoryAllocationTest, ExAllocatePool_BasicAllocation) {
    LogTestStep("Basic allocation test");
    
    // Test basic allocation
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, 1024);
    EXPECT_NE(ptr, nullptr);
    LogTestInfo("Allocated 1024 bytes at address: %p", ptr);
    
    if (ptr) {
        LogTestStep("Writing test pattern to allocated memory");
        // Write to the memory to ensure it's valid
        memset(ptr, 0xAB, 1024);
        
        LogTestStep("Verifying memory content");
        // Verify the memory
        BYTE* bytePtr = static_cast<BYTE*>(ptr);
        for (int i = 0; i < 1024; i++) {
            EXPECT_EQ(bytePtr[i], 0xAB);
        }
        
        LogTestStep("Freeing allocated memory");
        ExFreePoolTracked(ptr);
        LogTestInfo("Successfully freed memory at address: %p", ptr);
    }
}

TEST_F(MemoryAllocationTest, ExAllocatePool_ZeroSize) {
    LogTestStep("Zero size allocation test");
    
    // Test allocation with zero size
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, 0);
    LogTestInfo("Zero-size allocation returned: %p", ptr);
    
    // Implementation may return NULL or a valid pointer for zero-size allocations
    // Both behaviors are acceptable, but we should handle gracefully
    if (ptr) {
        LogTestInfo("Zero-size allocation succeeded, freeing memory");
        ExFreePoolTracked(ptr);
    } else {
        LogTestInfo("Zero-size allocation returned NULL (acceptable behavior)");
    }
}

TEST_F(MemoryAllocationTest, ExAllocatePool_LargeAllocation) {
    LogTestStep("Large allocation test");
    
    // Test large allocation (but not too large to cause system issues)
    const SIZE_T largeSize = 1024 * 1024; // 1MB
    PVOID ptr = ExAllocatePoolTracked(NonPagedPool, largeSize);
    LogTestInfo("Large allocation (%zu bytes) returned: %p", largeSize, ptr);
    
    if (ptr) {
        LogTestStep("Testing large memory access patterns");
        // Test writing to different parts of the large allocation
        BYTE* bytePtr = static_cast<BYTE*>(ptr);
        bytePtr[0] = 0x01;
        bytePtr[largeSize / 2] = 0x02;
        bytePtr[largeSize - 1] = 0x03;
        
        EXPECT_EQ(bytePtr[0], 0x01);
        EXPECT_EQ(bytePtr[largeSize / 2], 0x02);
        EXPECT_EQ(bytePtr[largeSize - 1], 0x03);
        
        LogTestInfo("Large memory access test completed successfully");
        ExFreePoolTracked(ptr);
        LogTestInfo("Large allocation freed successfully");
    }
}

TEST_F(MemoryAllocationTest, ExAllocatePoolWithTag_BasicAllocation) {
    LogTestStep("Tagged allocation test");
    
    ULONG tag = 'WKLT'; // WinKernelLite Test tag
    
    PVOID ptr = ExAllocatePoolWithTagTracked(NonPagedPool, 512, tag);
    EXPECT_NE(ptr, nullptr);
    LogTestInfo("Tagged allocation (tag: 0x%08X) returned: %p", tag, ptr);
    
    if (ptr) {
        LogTestStep("Testing tagged memory operations");
        // Verify memory is usable
        memset(ptr, 0xCD, 512);
        
        BYTE* bytePtr = static_cast<BYTE*>(ptr);
        for (int i = 0; i < 512; i++) {
            EXPECT_EQ(bytePtr[i], 0xCD);
        }
        
        LogTestInfo("Tagged memory pattern verification completed");
        ExFreePoolWithTagTracked(ptr, tag);
        LogTestInfo("Tagged allocation freed successfully");
    }
}

TEST_F(MemoryAllocationTest, ExAllocatePoolWithTag_DifferentTags) {
    LogTestStep("Multiple tagged allocations test");
    
    ULONG tag1 = 'TAG1';
    ULONG tag2 = 'TAG2';
    
    PVOID ptr1 = ExAllocatePoolWithTagTracked(NonPagedPool, 256, tag1);
    PVOID ptr2 = ExAllocatePoolWithTagTracked(NonPagedPool, 256, tag2);
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2); // Should be different allocations
    
    LogTestInfo("Allocation 1 (tag: 0x%08X): %p", tag1, ptr1);
    LogTestInfo("Allocation 2 (tag: 0x%08X): %p", tag2, ptr2);
    
    if (ptr1) {
        ExFreePoolWithTagTracked(ptr1, tag1);
        LogTestInfo("Tagged allocation 1 freed");
    }
    if (ptr2) {
        ExFreePoolWithTagTracked(ptr2, tag2);
        LogTestInfo("Tagged allocation 2 freed");
    }
}

TEST_F(MemoryAllocationTest, ExFreePool_NullPointer) {
    LogTestStep("NULL pointer free test");
    
    // Test freeing NULL pointer - should handle gracefully
    ExFreePoolTracked(nullptr);
    LogTestInfo("NULL pointer free completed without crash");
    // If we reach here without crashing, the test passes
    SUCCEED();
}

TEST_F(MemoryAllocationTest, ExFreePoolWithTag_NullPointer) {
    LogTestStep("NULL pointer tagged free test");
    
    // Test freeing NULL pointer with tag - should handle gracefully
    ExFreePoolWithTagTracked(nullptr, 'TEST');
    LogTestInfo("NULL pointer tagged free completed without crash");
    // If we reach here without crashing, the test passes
    SUCCEED();
}

TEST_F(MemoryAllocationTest, MultipleAllocationsAndFrees) {
    LogTestStep("Multiple allocations and frees test");
    
    std::vector<PVOID> allocations;
    const int numAllocations = 10;
    
    LogTestInfo("Starting %d sequential allocations", numAllocations);
    
    // Allocate multiple blocks
    for (int i = 0; i < numAllocations; i++) {
        SIZE_T size = static_cast<SIZE_T>(i + 1) * 64; // Cast to SIZE_T before multiplication
        PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
        if (ptr) {
            allocations.push_back(ptr);
            LogTestInfo("Allocation %d: %zu bytes at %p", i + 1, size, ptr);
            
            // Write pattern to verify memory integrity
            BYTE* bytePtr = static_cast<BYTE*>(ptr);
            for (SIZE_T j = 0; j < size; j++) {
                bytePtr[j] = static_cast<BYTE>(i + 1);
            }
        }
    }
    
    LogTestStep("Verifying memory integrity");
    // Verify memory integrity
    for (size_t i = 0; i < allocations.size(); i++) {
        BYTE* bytePtr = static_cast<BYTE*>(allocations[i]);
        SIZE_T size = (i + 1) * 64;
        
        for (SIZE_T j = 0; j < size; j++) {
            EXPECT_EQ(bytePtr[j], static_cast<BYTE>(i + 1));
        }
    }
    LogTestInfo("Memory integrity verification completed");
    
    LogTestStep("Freeing all allocations");
    // Free all allocations
    for (size_t i = 0; i < allocations.size(); i++) {
        ExFreePoolTracked(allocations[i]);
        LogTestInfo("Freed allocation %zu", i + 1);
    }
}

TEST_F(MemoryAllocationTest, PagedVsNonPagedPool) {
    LogTestStep("Paged vs NonPaged pool test");
    
    // Test both paged and non-paged pool allocations
    PVOID pagedPtr = ExAllocatePoolTracked(PagedPool, 1024);
    PVOID nonPagedPtr = ExAllocatePoolTracked(NonPagedPool, 1024);
    
    LogTestInfo("Paged pool allocation: %p", pagedPtr);
    LogTestInfo("NonPaged pool allocation: %p", nonPagedPtr);
    
    // Both should succeed (or fail gracefully)
    if (pagedPtr) {
        memset(pagedPtr, 0xEF, 1024);
        ExFreePoolTracked(pagedPtr);
        LogTestInfo("Paged pool allocation freed");
    }
    
    if (nonPagedPtr) {
        memset(nonPagedPtr, 0xFE, 1024);
        ExFreePoolTracked(nonPagedPtr);
        LogTestInfo("NonPaged pool allocation freed");
    }
    
    SUCCEED();
}

TEST_F(MemoryAllocationTest, StressAllocation) {
    LogTestStep("Stress allocation test");
    
    const int iterations = 100;
    
    LogTestInfo("Starting stress test with %d iterations", iterations);
    
    for (int i = 0; i < iterations; i++) {
        SIZE_T size = static_cast<SIZE_T>(i % 16 + 1) * 64; // Cast to SIZE_T before multiplication
        
        PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
        if (ptr) {
            // Quick write test
            memset(ptr, static_cast<int>(i & 0xFF), size);
            
            // Immediate free
            ExFreePoolTracked(ptr);
            
            if ((i + 1) % 20 == 0) {
                LogTestInfo("Completed %d/%d stress iterations", i + 1, iterations);
            }
        }
    }
    
    LogTestInfo("Stress allocation test completed successfully");
    SUCCEED();
}

TEST_F(MemoryAllocationTest, AllocateWithTagStress) {
    LogTestStep("Tagged allocation stress test");
    
    const int iterations = 50;
    std::vector<std::pair<PVOID, ULONG>> allocations;
    
    LogTestInfo("Starting tagged stress test with %d iterations", iterations);
    
    for (int i = 0; i < iterations; i++) {
        ULONG tag = 'TST' + (i << 8); // Generate different tags
        SIZE_T size = static_cast<SIZE_T>(i % 8 + 1) * 128; // Cast to SIZE_T before multiplication
        
        PVOID ptr = ExAllocatePoolWithTagTracked(NonPagedPool, size, tag);
        if (ptr) {
            allocations.push_back({ptr, tag});
            
            // Write test pattern
            BYTE* bytePtr = static_cast<BYTE*>(ptr);
            for (SIZE_T j = 0; j < size; j++) {
                bytePtr[j] = static_cast<BYTE>(i);
            }
            
            if ((i + 1) % 10 == 0) {
                LogTestInfo("Allocated %d/%d tagged stress blocks", i + 1, iterations);
            }
        }
    }
    
    LogTestStep("Freeing all tagged allocations");
    // Free all allocations with their respective tags
    for (size_t i = 0; i < allocations.size(); i++) {
        ExFreePoolWithTagTracked(allocations[i].first, allocations[i].second);
        if ((i + 1) % 10 == 0) {
            LogTestInfo("Freed %zu/%zu tagged stress blocks", i + 1, allocations.size());
        }
    }
    
    LogTestInfo("Tagged allocation stress test completed successfully");
    SUCCEED();
}

TEST_F(MemoryAllocationTest, EdgeCaseSizes) {
    LogTestStep("Edge case sizes test");
    
    // Test edge case sizes
    std::vector<SIZE_T> testSizes = {
        1,       // Minimum size
        2,       // Small size
        3,       // Odd size
        4,       // Word aligned
        8,       // Double word aligned
        16,      // Paragraph aligned
        32,      // Cache line size (typical)
        64,      // Common allocation size
        128,     // Power of 2
        255,     // Just under 256
        256,     // Power of 2
        1023,    // Just under 1KB
        1024,    // 1KB
        4095,    // Just under 4KB
        4096     // Page size
    };
    
    LogTestInfo("Testing %zu different edge case sizes", testSizes.size());
    
    for (SIZE_T size : testSizes) {
        PVOID ptr = ExAllocatePoolTracked(NonPagedPool, size);
        LogTestInfo("Size %zu: allocation at %p", size, ptr);
        
        if (ptr) {
            // Test first and last byte access
            BYTE* bytePtr = static_cast<BYTE*>(ptr);
            bytePtr[0] = 0xAA;
            if (size > 1) {
                bytePtr[size - 1] = 0xBB;
            }
            
            EXPECT_EQ(bytePtr[0], 0xAA);
            if (size > 1) {
                EXPECT_EQ(bytePtr[size - 1], 0xBB);
            }
            
            ExFreePoolTracked(ptr);
        }
    }
    
    LogTestInfo("Edge case sizes test completed successfully");
}
