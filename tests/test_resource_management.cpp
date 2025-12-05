#include <gtest/gtest.h>
#include <Windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include "../include/Resource.h"

class ResourceManagementTest : public ::testing::Test {
protected:
    ERESOURCE testResource;
    
    void SetUp() override {
        // Initialize a test resource
        ExInitializeResourceLite(&testResource);
    }
    
    void TearDown() override {
        // Clean up the resource
        ExDeleteResourceLite(&testResource);
    }
};

TEST_F(ResourceManagementTest, ExInitializeResourceLite_BasicInitialization) {
    ERESOURCE resource;
    
    // Test basic initialization
    NTSTATUS status = ExInitializeResourceLite(&resource);
    EXPECT_EQ(status, STATUS_SUCCESS);
    
    // Clean up
    ExDeleteResourceLite(&resource);
}

TEST_F(ResourceManagementTest, ExInitializeResourceLite_NullPointer) {
    // Test with NULL pointer
    NTSTATUS status = ExInitializeResourceLite(nullptr);
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(ResourceManagementTest, ExAcquireResourceSharedLite_Basic) {
    // Test basic shared acquisition
    BOOLEAN acquired = ExAcquireResourceSharedLite(&testResource, TRUE);
    EXPECT_TRUE(acquired);
    
    if (acquired) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, ExAcquireResourceSharedLite_NullPointer) {
    // Test with NULL resource
    BOOLEAN acquired = ExAcquireResourceSharedLite(nullptr, TRUE);
    EXPECT_FALSE(acquired);
}

TEST_F(ResourceManagementTest, ExAcquireResourceExclusiveLite_Basic) {
    // Test basic exclusive acquisition
    BOOLEAN acquired = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(acquired);
    
    if (acquired) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, ExAcquireResourceExclusiveLite_NullPointer) {
    // Test with NULL resource
    BOOLEAN acquired = ExAcquireResourceExclusiveLite(nullptr, TRUE);
    EXPECT_FALSE(acquired);
}

TEST_F(ResourceManagementTest, ExReleaseResourceLite_Basic) {
    // Acquire and then release
    BOOLEAN acquired = ExAcquireResourceSharedLite(&testResource, TRUE);
    ASSERT_TRUE(acquired);
    
    // Test release
    ExReleaseResourceLite(&testResource);
    
    // Should be able to acquire again
    acquired = ExAcquireResourceSharedLite(&testResource, TRUE);
    EXPECT_TRUE(acquired);
    
    if (acquired) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, ExReleaseResourceLite_NullPointer) {
    // Test releasing NULL resource - should handle gracefully
    ExReleaseResourceLite(nullptr);
    // If we reach here without crashing, the test passes
    SUCCEED();
}

TEST_F(ResourceManagementTest, ExDeleteResourceLite_Basic) {
    ERESOURCE resource;
    
    NTSTATUS status = ExInitializeResourceLite(&resource);
    ASSERT_EQ(status, STATUS_SUCCESS);
    
    // Test deletion
    status = ExDeleteResourceLite(&resource);
    EXPECT_EQ(status, STATUS_SUCCESS);
}

TEST_F(ResourceManagementTest, ExDeleteResourceLite_NullPointer) {
    // Test deleting NULL resource
    NTSTATUS status = ExDeleteResourceLite(nullptr);
    EXPECT_EQ(status, STATUS_INVALID_PARAMETER);
}

TEST_F(ResourceManagementTest, MultipleSharedAcquisitions) {
    // Test multiple shared acquisitions
    std::vector<BOOLEAN> acquisitions;
    const int numAcquisitions = 5;
    
    for (int i = 0; i < numAcquisitions; i++) {
        BOOLEAN acquired = ExAcquireResourceSharedLite(&testResource, TRUE);
        acquisitions.push_back(acquired);
        EXPECT_TRUE(acquired);
    }
    
    // Release all acquisitions
    for (int i = 0; i < numAcquisitions; i++) {
        if (acquisitions[i]) {
            ExReleaseResourceLite(&testResource);
        }
    }
}

TEST_F(ResourceManagementTest, ExclusiveAfterShared) {
    // Acquire shared
    BOOLEAN sharedAcquired = ExAcquireResourceSharedLite(&testResource, TRUE);
    ASSERT_TRUE(sharedAcquired);
    
    // Try to acquire exclusive (should fail with wait=FALSE)
    BOOLEAN exclusiveAcquired = ExAcquireResourceExclusiveLite(&testResource, FALSE);
    EXPECT_FALSE(exclusiveAcquired);
    
    // Release shared
    ExReleaseResourceLite(&testResource);
    
    // Now exclusive should succeed
    exclusiveAcquired = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(exclusiveAcquired);
    
    if (exclusiveAcquired) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, SharedAfterExclusive) {
    // Acquire exclusive
    BOOLEAN exclusiveAcquired = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    ASSERT_TRUE(exclusiveAcquired);
    
    // Try to acquire shared (should fail with wait=FALSE)
    BOOLEAN sharedAcquired = ExAcquireResourceSharedLite(&testResource, FALSE);
    EXPECT_FALSE(sharedAcquired);
    
    // Release exclusive
    ExReleaseResourceLite(&testResource);
    
    // Now shared should succeed
    sharedAcquired = ExAcquireResourceSharedLite(&testResource, TRUE);
    EXPECT_TRUE(sharedAcquired);
    
    if (sharedAcquired) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, NestedAcquisitions) {
    // Test nested acquisitions (same thread)
    BOOLEAN acquired1 = ExAcquireResourceSharedLite(&testResource, TRUE);
    ASSERT_TRUE(acquired1);
    
    BOOLEAN acquired2 = ExAcquireResourceSharedLite(&testResource, TRUE);
    EXPECT_TRUE(acquired2);
    
    // Release in reverse order
    if (acquired2) {
        ExReleaseResourceLite(&testResource);
    }
    if (acquired1) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, ExclusiveNestedAcquisition) {
    // Test nested exclusive acquisitions
    BOOLEAN acquired1 = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    ASSERT_TRUE(acquired1);
    
    BOOLEAN acquired2 = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(acquired2);
    
    // Release in reverse order
    if (acquired2) {
        ExReleaseResourceLite(&testResource);
    }
    if (acquired1) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, MultipleResources_Independent) {
    ERESOURCE resource1, resource2;
    
    // Initialize both resources
    NTSTATUS status1 = ExInitializeResourceLite(&resource1);
    NTSTATUS status2 = ExInitializeResourceLite(&resource2);
    ASSERT_EQ(status1, STATUS_SUCCESS);
    ASSERT_EQ(status2, STATUS_SUCCESS);
    
    // Acquire resource1 exclusive
    BOOLEAN acquired1 = ExAcquireResourceExclusiveLite(&resource1, TRUE);
    ASSERT_TRUE(acquired1);
    
    // Should be able to acquire resource2 independently
    BOOLEAN acquired2 = ExAcquireResourceSharedLite(&resource2, TRUE);
    EXPECT_TRUE(acquired2);
    
    // Release both
    if (acquired2) {
        ExReleaseResourceLite(&resource2);
    }
    if (acquired1) {
        ExReleaseResourceLite(&resource1);
    }
    
    // Clean up
    ExDeleteResourceLite(&resource1);
    ExDeleteResourceLite(&resource2);
}

TEST_F(ResourceManagementTest, StressTest_MultipleOperations) {
    const int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        // Alternate between shared and exclusive acquisitions
        if (i % 2 == 0) {
            BOOLEAN acquired = ExAcquireResourceSharedLite(&testResource, TRUE);
            EXPECT_TRUE(acquired);
            if (acquired) {
                ExReleaseResourceLite(&testResource);
            }
        } else {
            BOOLEAN acquired = ExAcquireResourceExclusiveLite(&testResource, TRUE);
            EXPECT_TRUE(acquired);
            if (acquired) {
                ExReleaseResourceLite(&testResource);
            }
        }
    }
}

TEST_F(ResourceManagementTest, ConversionTest_SharedToExclusive) {
    // Acquire shared
    BOOLEAN sharedAcquired = ExAcquireResourceSharedLite(&testResource, TRUE);
    ASSERT_TRUE(sharedAcquired);
    
    // Release shared
    ExReleaseResourceLite(&testResource);
    
    // Acquire exclusive
    BOOLEAN exclusiveAcquired = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(exclusiveAcquired);
    
    if (exclusiveAcquired) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, ConversionTest_ExclusiveToShared) {
    // Acquire exclusive
    BOOLEAN exclusiveAcquired = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    ASSERT_TRUE(exclusiveAcquired);
    
    // Release exclusive
    ExReleaseResourceLite(&testResource);
    
    // Acquire shared
    BOOLEAN sharedAcquired = ExAcquireResourceSharedLite(&testResource, TRUE);
    EXPECT_TRUE(sharedAcquired);
    
    if (sharedAcquired) {
        ExReleaseResourceLite(&testResource);
    }
}

TEST_F(ResourceManagementTest, WaitParameter_Testing) {
    // Test wait=FALSE behavior with recursive acquisition
    BOOLEAN acquired1 = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    ASSERT_TRUE(acquired1);
    
    // Recursive acquisition should succeed even with wait=FALSE
    // because the same thread already owns the resource
    BOOLEAN acquired2 = ExAcquireResourceExclusiveLite(&testResource, FALSE);
    EXPECT_TRUE(acquired2);
    
    // Release the second acquisition
    if (acquired2) {
        ExReleaseResourceLite(&testResource);
    }
    
    // Release the first acquisition
    ExReleaseResourceLite(&testResource);
    
    // Now it should succeed again
    acquired2 = ExAcquireResourceExclusiveLite(&testResource, TRUE);
    EXPECT_TRUE(acquired2);
    
    if (acquired2) {
        ExReleaseResourceLite(&testResource);
    }
}
