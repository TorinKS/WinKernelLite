#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <WinKernelLite/Ntrtl.h>
#include <WinKernelLite/KernelHeap.h>
#include "WinKernelLiteTestBase.h"

class AvlTableTest : public WinKernelLiteTestBase {
protected:
    RTL_AVL_TABLE table;  // Moved to class level for reuse

    void SetUp() override {
        WinKernelLiteTestBase::SetUp();
        LogTestInfo("AVL Table test suite initialized");
        
        // Initialize AVL table for each test
        LogTestStep("Initializing fresh AVL table");
        
        RtlInitializeGenericTableAvl(
            &table,
            CompareRoutine,
            AllocateRoutine,
            FreeRoutine,
            nullptr
        );
        
        LogTestInfo("AVL table initialized successfully");
    }

    void TearDown() override {
        // Clean up AVL table after each test
        LogTestStep("Cleaning up AVL table");
        
        PVOID element;
        int cleaned_elements = 0;
        while ((element = RtlEnumerateGenericTableAvl(&table, TRUE)) != NULL) {
            RtlDeleteElementGenericTableAvl(&table, element);
            cleaned_elements++;
        }
       
        LogTestInfo("AVL table cleanup completed - cleaned %d elements", cleaned_elements);
        
        LogTestInfo("AVL Table test suite completed");
        WinKernelLiteTestBase::TearDown();
    }

    // Test data structure
    struct TestData {
        int key;
        char value[64];
        int category; // For filtering tests
    };

    // Comparison routine for our test data
    static RTL_GENERIC_COMPARE_RESULTS CompareRoutine(
        PRTL_AVL_TABLE Table,
        PVOID FirstStruct,
        PVOID SecondStruct)
    {
        UNREFERENCED_PARAMETER(Table);
        
        TestData* first = (TestData*)FirstStruct;
        TestData* second = (TestData*)SecondStruct;
        
        if (first->key < second->key) return GenericLessThan;
        if (first->key > second->key) return GenericGreaterThan;
        return GenericEqual;
    }

    // Allocation routine
    static PVOID AllocateRoutine(PRTL_AVL_TABLE Table, CLONG ByteSize)
    {
        UNREFERENCED_PARAMETER(Table);
        return ExAllocatePoolTracked(NonPagedPool, ByteSize);
    }

    // Free routine
    static VOID FreeRoutine(PRTL_AVL_TABLE Table, PVOID Buffer)
    {
        UNREFERENCED_PARAMETER(Table);
        ExFreePoolTracked(Buffer);
    }

    // Match function for category filtering
    static NTSTATUS MatchEvenCategory(
        PRTL_AVL_TABLE Table,
        PVOID UserData,
        PVOID MatchData)
    {
        UNREFERENCED_PARAMETER(Table);
        UNREFERENCED_PARAMETER(MatchData);
        
        TestData* data = (TestData*)UserData;
        return (data->category % 2 == 0) ? STATUS_SUCCESS : STATUS_NO_MATCH;
    }

    // Match function for specific value
    static NTSTATUS MatchSpecificValue(
        PRTL_AVL_TABLE Table,
        PVOID UserData,
        PVOID MatchData)
    {
        UNREFERENCED_PARAMETER(Table);
        
        TestData* data = (TestData*)UserData;
        int* targetCategory = (int*)MatchData;
        
        return (data->category == *targetCategory) ? STATUS_SUCCESS : STATUS_NO_MATCH;
    }
};


TEST_F(AvlTableTest, GetElementByIndex) {
    LogTestStep("Get element by index test");
    
    TestData data[] = {
        {30, "Value 30"},
        {10, "Value 10"},
        {20, "Value 20"},
        {40, "Value 40"},
        {15, "Value 15"}
    };
    
    // Insert elements
    for (int i = 0; i < 5; i++) {
        RtlInsertElementGenericTableAvl(&table, &data[i], sizeof(TestData), nullptr);
    }
    LogTestInfo("Inserted 5 elements for index access test");
    
    // Get elements by index (should be in sorted order)
    int expectedKeys[] = {10, 15, 20, 30, 40};
    
    LogTestStep("Accessing elements by index");
    for (ULONG i = 0; i < 5; i++) {
        PVOID element = RtlGetElementGenericTableAvl(&table, i);
        ASSERT_NE(element, nullptr);
        TestData* testData = (TestData*)element;
        EXPECT_EQ(testData->key, expectedKeys[i]);
        LogTestInfo("Index[%lu]: key=%d, value='%s'", i, testData->key, testData->value);
    }
    
    // Test out of bounds access
    PVOID outOfBounds = RtlGetElementGenericTableAvl(&table, 5);
    EXPECT_EQ(outOfBounds, nullptr);
    
    outOfBounds = RtlGetElementGenericTableAvl(&table, MAXULONG);
    EXPECT_EQ(outOfBounds, nullptr);
}

// Test edge cases
TEST_F(AvlTableTest, EmptyTableOperations) {
    LogTestStep("Empty table operations test");
    
    // Test operations on empty table
    EXPECT_TRUE(RtlIsGenericTableEmptyAvl(&table));
    EXPECT_EQ(RtlNumberGenericTableElementsAvl(&table), 0UL);
    
    // Lookup in empty table should return NULL
    TestData lookup = {1, ""};
    PVOID found = RtlLookupElementGenericTableAvl(&table, &lookup);
    EXPECT_EQ(found, nullptr);
    
    // Delete from empty table should return FALSE
    BOOLEAN deleted = RtlDeleteElementGenericTableAvl(&table, &lookup);
    EXPECT_FALSE(deleted);
    
    // Enumeration should return NULL
    PVOID element = RtlEnumerateGenericTableAvl(&table, TRUE);
    EXPECT_EQ(element, nullptr);
    
    // Get element by index should return NULL
    PVOID indexed = RtlGetElementGenericTableAvl(&table, 0);
    EXPECT_EQ(indexed, nullptr);
}

// Test single element operations
TEST_F(AvlTableTest, SingleElementOperations) {
    LogTestStep("Single element operations test");
    
    TestData data = {42, "Single Element"};
    BOOLEAN newElement;
    
    // Insert single element
    PVOID result = RtlInsertElementGenericTableAvl(&table, &data, sizeof(TestData), &newElement);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(newElement);
    EXPECT_FALSE(RtlIsGenericTableEmptyAvl(&table));
    EXPECT_EQ(RtlNumberGenericTableElementsAvl(&table), 1UL);
    
    // Lookup the element
    TestData lookup = {42, ""};
    PVOID found = RtlLookupElementGenericTableAvl(&table, &lookup);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(((TestData*)found)->key, 42);
    EXPECT_STREQ(((TestData*)found)->value, "Single Element");
    
    // Get by index
    PVOID indexed = RtlGetElementGenericTableAvl(&table, 0);
    ASSERT_NE(indexed, nullptr);
    EXPECT_EQ(((TestData*)indexed)->key, 42);
   
    // Out of bounds access
    PVOID outOfBounds = RtlGetElementGenericTableAvl(&table, 1);
    EXPECT_EQ(outOfBounds, nullptr);
    
    // Enumeration
    PVOID enumerated = RtlEnumerateGenericTableAvl(&table, TRUE);
    ASSERT_NE(enumerated, nullptr);
    EXPECT_EQ(((TestData*)enumerated)->key, 42);
   
    // Continue enumeration should return NULL
    enumerated = RtlEnumerateGenericTableAvl(&table, FALSE);
    EXPECT_EQ(enumerated, nullptr);
    
    // Delete the element
    BOOLEAN deleted = RtlDeleteElementGenericTableAvl(&table, &lookup);
    EXPECT_TRUE(deleted);
    EXPECT_TRUE(RtlIsGenericTableEmptyAvl(&table));
    EXPECT_EQ(RtlNumberGenericTableElementsAvl(&table), 0UL);
}

// Test boundary values
TEST_F(AvlTableTest, BoundaryValues) {
    LogTestStep("Boundary values test");
    
    std::vector<int> boundaryKeys = {
        INT_MIN, INT_MIN + 1, -1, 0, 1, INT_MAX - 1, INT_MAX
    };
    
    LogTestInfo("Testing boundary values: INT_MIN to INT_MAX");
    
    // Insert boundary values
    for (int key : boundaryKeys) {
        TestData data = {key, ""};
        sprintf_s(data.value, sizeof(data.value), "Value_%d", key);
        PVOID result = RtlInsertElementGenericTableAvl(&table, &data, sizeof(TestData), nullptr);
        ASSERT_NE(result, nullptr) << "Failed to insert key: " << key;
        LogTestInfo("Inserted boundary key: %d", key);
    }
    
    EXPECT_EQ(RtlNumberGenericTableElementsAvl(&table), boundaryKeys.size());
    
    // Verify all can be found
    for (int key : boundaryKeys) {
        TestData lookup = {key, ""};
        PVOID found = RtlLookupElementGenericTableAvl(&table, &lookup);
        ASSERT_NE(found, nullptr) << "Failed to find key: " << key;
        EXPECT_EQ(((TestData*)found)->key, key);
    }
    
    // Verify enumeration order (should be sorted)
    std::sort(boundaryKeys.begin(), boundaryKeys.end());
    
    size_t index = 0;
    PVOID element = RtlEnumerateGenericTableAvl(&table, TRUE);
    while (element != nullptr && index < boundaryKeys.size()) {
        EXPECT_EQ(((TestData*)element)->key, boundaryKeys[index]);
        element = RtlEnumerateGenericTableAvl(&table, FALSE);
        index++;
    }
    EXPECT_EQ(index, boundaryKeys.size());
}

// Test random insertion and deletion patterns
TEST_F(AvlTableTest, RandomOperations) {
    LogTestStep("Random operations test");
    
    const int NUM_OPERATIONS = 50;
    std::vector<int> keys;
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> dist(1, 1000);
    
    // Generate random keys
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        keys.push_back(dist(rng));
    }
    LogTestInfo("Generated %d random keys for testing", NUM_OPERATIONS);
    
    // Insert all keys
    for (int key : keys) {
        TestData data = {key, ""};
        sprintf_s(data.value, sizeof(data.value), "Random_%d", key);
        RtlInsertElementGenericTableAvl(&table, &data, sizeof(TestData), nullptr);
    }
    LogTestInfo("Inserted all random keys");
    
    // Remove duplicates for verification
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    
    EXPECT_EQ(RtlNumberGenericTableElementsAvl(&table), keys.size());
    LogTestInfo("After removing duplicates: %zu unique keys", keys.size());
    
    // Verify all unique keys can be found
    for (int key : keys) {
        TestData lookup = {key, ""};
        PVOID found = RtlLookupElementGenericTableAvl(&table, &lookup);
        EXPECT_NE(found, nullptr) << "Failed to find key: " << key;
    }
    LogTestInfo("All unique keys verified in table");
    
    // Randomly delete half the elements
    std::shuffle(keys.begin(), keys.end(), rng);
    size_t halfSize = keys.size() / 2;
    
    for (size_t i = 0; i < halfSize; i++) {
        TestData lookup = {keys[i], ""};
        BOOLEAN deleted = RtlDeleteElementGenericTableAvl(&table, &lookup);
        EXPECT_TRUE(deleted) << "Failed to delete key: " << keys[i];
    }
    LogTestInfo("Randomly deleted %zu elements", halfSize);
    
    EXPECT_EQ(RtlNumberGenericTableElementsAvl(&table), keys.size() - halfSize);
    
    // Verify deleted elements are gone and remaining elements exist
    for (size_t i = 0; i < keys.size(); i++) {
        TestData lookup = {keys[i], ""};
        PVOID found = RtlLookupElementGenericTableAvl(&table, &lookup);
        
        if (i < halfSize) {
            EXPECT_EQ(found, nullptr) << "Deleted key still found: " << keys[i];
        } else {
            EXPECT_NE(found, nullptr) << "Remaining key not found: " << keys[i];
        }
    }
    LogTestInfo("Verified deletion results - deleted elements gone, remaining elements present");
}

TEST_F(AvlTableTest, EnumerationEdgeCases) {
    LogTestStep("Enumeration edge cases test");
    
    // Test enumeration restart
    std::vector<int> keys = {5, 2, 8, 1, 3, 7, 9};
    
    for (int key : keys) {
        TestData data = {key, ""};
        sprintf_s(data.value, sizeof(data.value), "Val_%d", key);
        RtlInsertElementGenericTableAvl(&table, &data, sizeof(TestData), nullptr);
    }
    LogTestInfo("Inserted test data for enumeration: %zu elements", keys.size());
    
    // First enumeration
    std::vector<int> firstEnum;
    PVOID element = RtlEnumerateGenericTableAvl(&table, TRUE);
    while (element != nullptr) {
        firstEnum.push_back(((TestData*)element)->key);
        element = RtlEnumerateGenericTableAvl(&table, FALSE);
        if (firstEnum.size() == 3) break; // Stop after 3 elements
    }
    LogTestInfo("First partial enumeration completed: %zu elements", firstEnum.size());
    
    // Restart enumeration
    std::vector<int> secondEnum;
    element = RtlEnumerateGenericTableAvl(&table, TRUE);
    while (element != nullptr) {
        secondEnum.push_back(((TestData*)element)->key);
        element = RtlEnumerateGenericTableAvl(&table, FALSE);
    }
    LogTestInfo("Second complete enumeration completed: %zu elements", secondEnum.size());
    
    // Second enumeration should be complete and sorted
    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(secondEnum.size(), keys.size());
    for (size_t i = 0; i < keys.size(); i++) {
        EXPECT_EQ(secondEnum[i], keys[i]);
    }
    LogTestInfo("Verified second enumeration is complete and correctly sorted");
}

TEST_F(AvlTableTest, FullVersionFunctions) {
    LogTestStep("Full version functions test");
    
    TestData data = {100, "Full Version Test"};
    strcpy_s(data.value, sizeof(data.value), "Full Version Test");
    
    // Test lookup with search result (should not find)
    PVOID nodeOrParent;
    TABLE_SEARCH_RESULT searchResult;
    PVOID found = RtlLookupElementGenericTableFullAvl(&table, &data, &nodeOrParent, &searchResult);
    EXPECT_EQ(found, nullptr);
    EXPECT_NE(searchResult, TableFoundNode);
    LogTestInfo("Initial lookup correctly returned NULL for non-existent element");
    
    // Insert using full version
    BOOLEAN newElement;
    PVOID inserted = RtlInsertElementGenericTableFullAvl(
        &table, &data, sizeof(TestData), &newElement, nodeOrParent, searchResult);
    ASSERT_NE(inserted, nullptr);
    EXPECT_TRUE(newElement);
    LogTestInfo("Element inserted using full version function: key=%d", data.key);
    
    // Lookup again (should find this time)
    found = RtlLookupElementGenericTableFullAvl(&table, &data, &nodeOrParent, &searchResult);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(searchResult, TableFoundNode);
    EXPECT_EQ(((TestData*)found)->key, 100);
    LogTestInfo("Element found using full version lookup: key=%d", ((TestData*)found)->key);
    
    // Try to insert duplicate using full version
    PVOID duplicate = RtlInsertElementGenericTableFullAvl(
        &table, &data, sizeof(TestData), &newElement, nodeOrParent, searchResult);
    ASSERT_NE(duplicate, nullptr);
    EXPECT_FALSE(newElement);
    EXPECT_EQ(duplicate, found); // Should return the same element
    LogTestInfo("Duplicate insertion correctly returned existing element");
}


 // Test large scale indexed access performance patterns
 TEST_F(AvlTableTest, IndexedAccessPatterns) {
     const int NUM_ELEMENTS = 100;

     // Insert elements
     for (int i = 0; i < NUM_ELEMENTS; i++) {
         TestData data = {i, "", 0};
         sprintf_s(data.value, sizeof(data.value), "Pattern_%d", i);
         RtlInsertElementGenericTableAvl(&table, &data, sizeof(TestData), nullptr);
     }

     // Test sequential access (should use caching optimization)
     for (ULONG i = 0; i < NUM_ELEMENTS; i++) {
         PVOID element = RtlGetElementGenericTableAvl(&table, i);
         ASSERT_NE(element, nullptr) << "Sequential access failed at index " << i;
         EXPECT_EQ(((TestData*)element)->key, i);
     }

     // Test reverse sequential access
     for (ULONG i = NUM_ELEMENTS; i > 0; i--) {
         PVOID element = RtlGetElementGenericTableAvl(&table, i - 1);
         ASSERT_NE(element, nullptr) << "Reverse access failed at index " << (i - 1);
         EXPECT_EQ(((TestData*)element)->key, i - 1);
     }

     // Test random access pattern
     std::vector<ULONG> indices = {50, 10, 90, 25, 75, 5, 95, 1, 99, 0};
     for (ULONG index : indices) {
         PVOID element = RtlGetElementGenericTableAvl(&table, index);
         ASSERT_NE(element, nullptr) << "Random access failed at index " << index;
         EXPECT_EQ(((TestData*)element)->key, index);
     }
 }

