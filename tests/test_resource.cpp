#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include "../include/Resource.h"

class ResourceTest : public ::testing::Test {
protected:
    ERESOURCE resource = {0};

    void SetUp() override {
        EXPECT_EQ(ExInitializeResourceLite(&resource), STATUS_SUCCESS);
    }

    void TearDown() override {
        EXPECT_EQ(ExDeleteResourceLite(&resource), STATUS_SUCCESS);
    }
};

// Test basic exclusive acquisition
TEST_F(ResourceTest, ExclusiveAcquisition) {
    EXPECT_TRUE(ExAcquireResourceExclusiveLite(&resource, TRUE));
    ExReleaseResourceLite(&resource);
}

// Test recursive exclusive acquisition
TEST_F(ResourceTest, RecursiveExclusiveAcquisition) {
    EXPECT_TRUE(ExAcquireResourceExclusiveLite(&resource, TRUE));
    EXPECT_TRUE(ExAcquireResourceExclusiveLite(&resource, TRUE));
    ExReleaseResourceLite(&resource);
    ExReleaseResourceLite(&resource);
}

// Test basic shared acquisition
TEST_F(ResourceTest, SharedAcquisition) {
    EXPECT_TRUE(ExAcquireResourceSharedLite(&resource, TRUE));
    ExReleaseResourceLite(&resource);
}

// Test multiple shared acquisitions
TEST_F(ResourceTest, MultipleSharedAcquisitions) {
    EXPECT_TRUE(ExAcquireResourceSharedLite(&resource, TRUE));
    EXPECT_TRUE(ExAcquireResourceSharedLite(&resource, TRUE));
    ExReleaseResourceLite(&resource);
    ExReleaseResourceLite(&resource);
}

// Test exclusive acquisition after shared (should wait)
TEST_F(ResourceTest, ExclusiveAfterSharedWithWait) {
    // Acquire shared in a separate thread
    std::thread sharedThread([this]() {
        EXPECT_TRUE(ExAcquireResourceSharedLite(&resource, TRUE));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Hold the resource
        ExReleaseResourceLite(&resource);
    });

    // Give the shared thread time to acquire
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Try to acquire exclusive with wait
    EXPECT_TRUE(ExAcquireResourceExclusiveLite(&resource, TRUE));
    ExReleaseResourceLite(&resource);

    sharedThread.join();
}

// Test critical regions
TEST_F(ResourceTest, CriticalRegions) {
    EXPECT_EQ(GetKernelApcDisableCount(), 0);
    
    KeEnterCriticalRegion();
    EXPECT_EQ(GetKernelApcDisableCount(), 1);
    
    KeEnterCriticalRegion();
    EXPECT_EQ(GetKernelApcDisableCount(), 2);
    
    KeLeaveCriticalRegion();
    EXPECT_EQ(GetKernelApcDisableCount(), 1);
    
    KeLeaveCriticalRegion();
    EXPECT_EQ(GetKernelApcDisableCount(), 0);
}

// Test null parameter handling
TEST_F(ResourceTest, NullParameterHandling) {
    EXPECT_FALSE(ExAcquireResourceExclusiveLite(nullptr, TRUE));
    EXPECT_FALSE(ExAcquireResourceSharedLite(nullptr, TRUE));
    
    // These should not crash
    ExReleaseResourceLite(nullptr);
    EXPECT_EQ(ExInitializeResourceLite(nullptr), STATUS_INVALID_PARAMETER);
    EXPECT_EQ(ExDeleteResourceLite(nullptr), STATUS_INVALID_PARAMETER);
}
