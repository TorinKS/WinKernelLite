#include <gtest/gtest.h>
#include <Windows.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include "../include/Resource.h"

class ResourceConcurrencyTest : public ::testing::Test {
protected:
    ERESOURCE testResource;
    
    void SetUp() override {
        // Initialize the test resource
        NTSTATUS status = ExInitializeResourceLite(&testResource);
        ASSERT_EQ(status, STATUS_SUCCESS);
    }
    
    void TearDown() override {
        // Clean up the test resource
        ExDeleteResourceLite(&testResource);
        
        // Clean up global resources
        CleanupGlobalResources();
    }
};

TEST_F(ResourceConcurrencyTest, BasicExclusiveAcquisition) {
    // Test basic exclusive acquisition and release
    BOOLEAN acquired = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(acquired);
    
    // Verify the resource is marked as owned exclusively
    EXPECT_TRUE(IsOwnedExclusive(&testResource));
    EXPECT_EQ(testResource.ActiveCount, 1);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerThread, (ERESOURCE_THREAD)GetCurrentThreadId());
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerCount, 1);
    
    // Release the resource
    ExReleaseResourceLite(&testResource);
    
    // Verify the resource is released
    EXPECT_FALSE(IsOwnedExclusive(&testResource));
    EXPECT_EQ(testResource.ActiveCount, 0);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerThread, 0);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerCount, 0);
}

TEST_F(ResourceConcurrencyTest, BasicSharedAcquisition) {
    // Test basic shared acquisition and release
    BOOLEAN acquired = ExAcquireResourceSharedLite(&testResource, TRUE);
    EXPECT_TRUE(acquired);
    
    // Verify the resource is not owned exclusively
    EXPECT_FALSE(IsOwnedExclusive(&testResource));
    EXPECT_EQ(testResource.ActiveCount, 1);
    
    // Release the resource
    ExReleaseResourceLite(&testResource);
    
    // Verify the resource is released
    EXPECT_EQ(testResource.ActiveCount, 0);
}

TEST_F(ResourceConcurrencyTest, RecursiveExclusiveAcquisition) {
    // Test recursive acquisition by the same thread
    BOOLEAN acquired1 = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(acquired1);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerCount, 1);
    
    BOOLEAN acquired2 = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(acquired2);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerCount, 2);
    
    BOOLEAN acquired3 = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(acquired3);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerCount, 3);
    
    // Release three times
    ExReleaseResourceLite(&testResource);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerCount, 2);
    EXPECT_TRUE(IsOwnedExclusive(&testResource));
    
    ExReleaseResourceLite(&testResource);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerCount, 1);
    EXPECT_TRUE(IsOwnedExclusive(&testResource));
    
    ExReleaseResourceLite(&testResource);
    EXPECT_EQ(testResource.OwnerThreads[0].OwnerCount, 0);
    EXPECT_FALSE(IsOwnedExclusive(&testResource));
    EXPECT_EQ(testResource.ActiveCount, 0);
}

TEST_F(ResourceConcurrencyTest, NonWaitAcquisition) {
    // Test non-blocking acquisition
    BOOLEAN acquired1 = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(acquired1);
    
    // Another thread should not be able to acquire without waiting
    std::atomic<bool> secondThreadResult{false};
    std::thread secondThread([&]() {
        secondThreadResult = ExAcquireResourceExclusiveLite(&testResource, FALSE);
    });
    
    secondThread.join();
    EXPECT_FALSE(secondThreadResult);
    
    ExReleaseResourceLite(&testResource);
}

TEST_F(ResourceConcurrencyTest, ConcurrentSharedAccess) {
    // Test multiple threads acquiring shared access
    const int numThreads = 4;
    std::atomic<int> successCount{0};
    std::atomic<int> activeCount{0};
    std::vector<std::thread> threads;
    
    auto sharedTask = [&]() {
        BOOLEAN acquired = ExAcquireResourceSharedLite(&testResource, TRUE);
        if (acquired) {
            successCount++;
            activeCount++;
            
            // Hold the resource for a short time
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            activeCount--;
            ExReleaseResourceLite(&testResource);
        }
    };
    
    // Start all threads
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(sharedTask);
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(successCount, numThreads);
    EXPECT_EQ(activeCount, 0);
    EXPECT_EQ(testResource.ActiveCount, 0);
}

TEST_F(ResourceConcurrencyTest, ExclusiveBlocksShared) {
    // Test that exclusive ownership blocks shared acquisition
    std::atomic<bool> exclusiveAcquired{false};
    std::atomic<bool> sharedAcquired{false};
    std::atomic<bool> exclusiveReleased{false};
    
    std::thread exclusiveThread([&]() {
        BOOLEAN acquired = ExAcquireResourceExclusiveLite(&testResource, TRUE);
        exclusiveAcquired = acquired;
        
        if (acquired) {
            // Hold for a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ExReleaseResourceLite(&testResource);
            exclusiveReleased = true;
        }
    });
    
    // Wait for exclusive thread to acquire
    while (!exclusiveAcquired) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::thread sharedThread([&]() {
        // This should initially fail/wait
        BOOLEAN acquired = ExAcquireResourceSharedLite(&testResource, FALSE);
        if (!acquired && !exclusiveReleased) {
            // Try again after exclusive is released
            while (!exclusiveReleased) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            acquired = ExAcquireResourceSharedLite(&testResource, TRUE);
        }
        
        if (acquired) {
            sharedAcquired = true;
            ExReleaseResourceLite(&testResource);
        }
    });
    
    exclusiveThread.join();
    sharedThread.join();
    
    EXPECT_TRUE(exclusiveAcquired);
    EXPECT_TRUE(exclusiveReleased);
    EXPECT_TRUE(sharedAcquired);
    EXPECT_EQ(testResource.ActiveCount, 0);
}

TEST_F(ResourceConcurrencyTest, StressTestConcurrentAccess) {
    // Stress test with many threads doing various operations
    const int numThreads = 8;
    const int operationsPerThread = 50;
    std::atomic<int> totalOperations{0};
    std::atomic<int> successfulOperations{0};
    std::vector<std::thread> threads;
    
    auto stressTask = [&](int threadId) {
        for (int i = 0; i < operationsPerThread; i++) {
            totalOperations++;
            
            // Randomly choose exclusive or shared access
            bool useExclusive = (threadId + i) % 3 == 0;
            
            BOOLEAN acquired;
            if (useExclusive) {
                acquired = ExAcquireResourceExclusiveLite(&testResource, FALSE);
            } else {
                acquired = ExAcquireResourceSharedLite(&testResource, FALSE);
            }
            
            if (acquired) {
                successfulOperations++;
                
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                
                ExReleaseResourceLite(&testResource);
            }
            
            // Small delay between operations
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    };
    
    // Start all threads
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(stressTask, i);
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(totalOperations, numThreads * operationsPerThread);
    EXPECT_GT(successfulOperations, 0); // At least some operations should succeed
    EXPECT_EQ(testResource.ActiveCount, 0);
}

TEST_F(ResourceConcurrencyTest, InvalidParameterHandling) {
    // Test with NULL resource pointer
    BOOLEAN result = ExAcquireResourceExclusiveLite(nullptr, TRUE);
    EXPECT_FALSE(result);
    
    result = ExAcquireResourceSharedLite(nullptr, TRUE);
    EXPECT_FALSE(result);
    
    // Release with NULL should not crash
    ExReleaseResourceLite(nullptr);
    
    // Initialize with NULL should fail
    NTSTATUS status = ExInitializeResourceLite(nullptr);
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
    
    // Delete with NULL should fail
    status = ExDeleteResourceLite(nullptr);
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(ResourceConcurrencyTest, CriticalRegionFunctionality) {
    // Test critical region functions
    LONG initialCount = GetKernelApcDisableCount();
    
    KeEnterCriticalRegion();
    LONG count1 = GetKernelApcDisableCount();
    EXPECT_EQ(count1, initialCount + 1);
    
    KeEnterCriticalRegion();
    LONG count2 = GetKernelApcDisableCount();
    EXPECT_EQ(count2, initialCount + 2);
    
    KeLeaveCriticalRegion();
    LONG count3 = GetKernelApcDisableCount();
    EXPECT_EQ(count3, initialCount + 1);
    
    KeLeaveCriticalRegion();
    LONG count4 = GetKernelApcDisableCount();
    EXPECT_EQ(count4, initialCount);
}

TEST_F(ResourceConcurrencyTest, CriticalRegionConcurrency) {
    // Test critical region functions under concurrent access
    const int numThreads = 4;
    const int operationsPerThread = 100;
    std::atomic<int> maxObservedCount{0};
    std::vector<std::thread> threads;
    
    auto criticalRegionTask = [&]() {
        for (int i = 0; i < operationsPerThread; i++) {
            KeEnterCriticalRegion();
            
            // Check the count and update max if needed
            LONG currentCount = GetKernelApcDisableCount();
            int currentMax = maxObservedCount.load();
            while (currentCount > currentMax && 
                   !maxObservedCount.compare_exchange_weak(currentMax, currentCount)) {
                // Retry if another thread updated maxObservedCount
            }
            
            // Small delay to increase chance of contention
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            
            KeLeaveCriticalRegion();
        }
    };
    
    // Start all threads
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(criticalRegionTask);
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Final count should be back to initial value
    LONG finalCount = GetKernelApcDisableCount();
    EXPECT_EQ(finalCount, 0); // Assuming we started at 0
    EXPECT_GE(maxObservedCount, numThreads); // Should have seen concurrent increments
}

TEST_F(ResourceConcurrencyTest, ResourceDeadlockAvoidance) {
    // Test scenario that could lead to deadlock
    ERESOURCE resource1, resource2;
    
    NTSTATUS status1 = ExInitializeResourceLite(&resource1);
    NTSTATUS status2 = ExInitializeResourceLite(&resource2);
    ASSERT_EQ(status1, STATUS_SUCCESS);
    ASSERT_EQ(status2, STATUS_SUCCESS);
    
    std::atomic<bool> thread1Done{false};
    std::atomic<bool> thread2Done{false};
    std::atomic<bool> deadlockDetected{false};
    
    auto thread1Task = [&]() {
        // Acquire resource1 first
        BOOLEAN acquired1 = ExAcquireResourceExclusiveLite(&resource1, TRUE);
        if (acquired1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Try to acquire resource2 (potential deadlock point)
            BOOLEAN acquired2 = ExAcquireResourceExclusiveLite(&resource2, FALSE);
            if (acquired2) {
                ExReleaseResourceLite(&resource2);
            } else {
                deadlockDetected = true;
            }
            
            ExReleaseResourceLite(&resource1);
        }
        thread1Done = true;
    };
    
    auto thread2Task = [&]() {
        // Small delay to ensure thread1 gets resource1 first
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        // Acquire resource2 first
        BOOLEAN acquired2 = ExAcquireResourceExclusiveLite(&resource2, TRUE);
        if (acquired2) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Try to acquire resource1 (potential deadlock point)
            BOOLEAN acquired1 = ExAcquireResourceExclusiveLite(&resource1, FALSE);
            if (acquired1) {
                ExReleaseResourceLite(&resource1);
            } else {
                deadlockDetected = true;
            }
            
            ExReleaseResourceLite(&resource2);
        }
        thread2Done = true;
    };
    
    std::thread t1(thread1Task);
    std::thread t2(thread2Task);
    
    t1.join();
    t2.join();
    
    EXPECT_TRUE(thread1Done);
    EXPECT_TRUE(thread2Done);
    // deadlockDetected being true means our non-blocking approach avoided actual deadlock
    
    // Clean up
    ExDeleteResourceLite(&resource1);
    ExDeleteResourceLite(&resource2);
}


