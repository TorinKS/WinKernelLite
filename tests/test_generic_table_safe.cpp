#include <gtest/gtest.h>
#include <WinKernelLite/ntrtl.h>
#include <vector>

namespace {
    // Global memory tracking for testing
    static size_t g_allocationCount = 0;
    static size_t g_freeCount = 0;

    // Reset memory tracking
    void ResetMemoryTracking() {
        g_allocationCount = 0;
        g_freeCount = 0;
    }

    // Test allocation routine with tracking
    PVOID NTAPI TestAllocateRoutine(PRTL_GENERIC_TABLE Table, CLONG ByteSize) {
        UNREFERENCED_PARAMETER(Table);
        g_allocationCount++;
        return malloc(ByteSize);
    }

    // Test free routine with tracking
    VOID NTAPI TestFreeRoutine(PRTL_GENERIC_TABLE Table, PVOID Buffer) {
        UNREFERENCED_PARAMETER(Table);
        if (Buffer) {
            g_freeCount++;
            free(Buffer);
        }
    }

    // Simple integer data structure
    struct IntegerData {
        int key;
        char value[32];
        
        IntegerData(int k = 0, const char* v = "") : key(k) {
            strncpy_s(value, sizeof(value), v ? v : "", _TRUNCATE);
        }
    };

    // Comparison routine for integer keys
    RTL_GENERIC_COMPARE_RESULTS NTAPI CompareInteger(
        PRTL_GENERIC_TABLE Table,
        PVOID FirstStruct,
        PVOID SecondStruct
    ) {
        UNREFERENCED_PARAMETER(Table);
        
        IntegerData* first = (IntegerData*)FirstStruct;
        IntegerData* second = (IntegerData*)SecondStruct;
        
        if (first->key < second->key) return GenericLessThan;
        if (first->key > second->key) return GenericGreaterThan;
        return GenericEqual;
    }
}

class GenericTableSafeTest : public ::testing::Test {
protected:
    void SetUp() override {
        ResetMemoryTracking();
        RtlInitializeGenericTable(
            &table,
            CompareInteger,
            TestAllocateRoutine,
            TestFreeRoutine,
            nullptr
        );
    }

    void TearDown() override {
        // Safe cleanup - collect keys first, then delete
        std::vector<int> keys;
        
        // Collect all keys without modifying the table
        PVOID restartKey = nullptr;
        PVOID element;
        while ((element = RtlEnumerateGenericTableWithoutSplaying(&table, &restartKey)) != nullptr) {
            IntegerData* data = (IntegerData*)element;
            keys.push_back(data->key);
        }
        
        // Now safely delete all elements
        for (int key : keys) {
            IntegerData deleteKey(key);
            RtlDeleteElementGenericTable(&table, &deleteKey);
        }
        
        // Verify no memory leaks
        EXPECT_EQ(g_allocationCount, g_freeCount) 
            << "Memory leak detected: " << g_allocationCount << " allocations, " 
            << g_freeCount << " frees";
    }

    RTL_GENERIC_TABLE table;
};

TEST_F(GenericTableSafeTest, InitializationTest) {
    EXPECT_TRUE(RtlIsGenericTableEmpty(&table));
    EXPECT_EQ(0, RtlNumberGenericTableElements(&table));
}

TEST_F(GenericTableSafeTest, SingleElementOperations) {
    IntegerData data1(42, "Answer");
    BOOLEAN newElement = FALSE;
    
    // Insert
    PVOID result = RtlInsertElementGenericTable(&table, &data1, sizeof(data1), &newElement);
    EXPECT_NE(nullptr, result);
    EXPECT_TRUE(newElement);
    EXPECT_FALSE(RtlIsGenericTableEmpty(&table));
    EXPECT_EQ(1, RtlNumberGenericTableElements(&table));
    
    // Lookup
    IntegerData searchKey(42);
    PVOID found = RtlLookupElementGenericTable(&table, &searchKey);
    EXPECT_NE(nullptr, found);
    IntegerData* foundData = (IntegerData*)found;
    EXPECT_EQ(42, foundData->key);
    EXPECT_STREQ("Answer", foundData->value);
    
    // Delete
    BOOLEAN deleted = RtlDeleteElementGenericTable(&table, &searchKey);
    EXPECT_TRUE(deleted);
    EXPECT_TRUE(RtlIsGenericTableEmpty(&table));
    EXPECT_EQ(0, RtlNumberGenericTableElements(&table));
}

TEST_F(GenericTableSafeTest, DuplicateKeyHandling) {
    IntegerData data1(100, "First");
    IntegerData data2(100, "Second");
    BOOLEAN newElement = FALSE;
    
    // Insert first
    PVOID result1 = RtlInsertElementGenericTable(&table, &data1, sizeof(data1), &newElement);
    EXPECT_NE(nullptr, result1);
    EXPECT_TRUE(newElement);
    
    // Insert duplicate key
    PVOID result2 = RtlInsertElementGenericTable(&table, &data2, sizeof(data2), &newElement);
    EXPECT_NE(nullptr, result2);
    EXPECT_FALSE(newElement); // Should not be new
    EXPECT_EQ(result1, result2); // Should return existing element
    EXPECT_EQ(1, RtlNumberGenericTableElements(&table)); // Count should not increase
}

TEST_F(GenericTableSafeTest, MultipleElementOperations) {
    std::vector<IntegerData> testData = {
        IntegerData(10, "Ten"),
        IntegerData(5, "Five"),
        IntegerData(15, "Fifteen"),
        IntegerData(3, "Three"),
        IntegerData(7, "Seven")
    };
    
    // Insert all elements
    for (const auto& data : testData) {
        BOOLEAN newElement = FALSE;
        PVOID result = RtlInsertElementGenericTable(&table, const_cast<IntegerData*>(&data), sizeof(data), &newElement);
        EXPECT_NE(nullptr, result);
        EXPECT_TRUE(newElement);
    }
    
    EXPECT_EQ(testData.size(), RtlNumberGenericTableElements(&table));
    
    // Verify all elements can be found
    for (const auto& data : testData) {
        IntegerData searchKey(data.key);
        PVOID found = RtlLookupElementGenericTable(&table, &searchKey);
        EXPECT_NE(nullptr, found);
        IntegerData* foundData = (IntegerData*)found;
        EXPECT_EQ(data.key, foundData->key);
        EXPECT_STREQ(data.value, foundData->value);
    }
}

TEST_F(GenericTableSafeTest, EnumerationOrder) {
    std::vector<int> keys = {50, 25, 75, 10, 30, 60, 80};
    
    // Insert elements
    for (int key : keys) {
        IntegerData data(key, "Value");
        RtlInsertElementGenericTable(&table, &data, sizeof(data), nullptr);
    }
    
    // Collect enumerated keys (should be in sorted order, not insertion order)
    std::vector<int> enumeratedKeys;
    PVOID element = RtlEnumerateGenericTable(&table, TRUE); // Start enumeration
    while (element != nullptr) {
        IntegerData* data = (IntegerData*)element;
        enumeratedKeys.push_back(data->key);
        element = RtlEnumerateGenericTable(&table, FALSE); // Continue enumeration
    }
    
    // Should have all keys in sorted order (not insertion order)
    std::vector<int> expectedSortedKeys = {10, 25, 30, 50, 60, 75, 80};
    EXPECT_EQ(expectedSortedKeys.size(), enumeratedKeys.size());
    EXPECT_EQ(expectedSortedKeys, enumeratedKeys);
}

TEST_F(GenericTableSafeTest, EnumerationWithoutSplaying) {
    std::vector<int> keys = {40, 20, 60, 10, 30};
    
    // Insert elements
    for (int key : keys) {
        IntegerData data(key, "Value");
        RtlInsertElementGenericTable(&table, &data, sizeof(data), nullptr);
    }
    
    // Test enumeration without splaying (should return elements in sorted order)
    std::vector<int> enumeratedKeys;
    PVOID restartKey = nullptr;
    PVOID element;
    while ((element = RtlEnumerateGenericTableWithoutSplaying(&table, &restartKey)) != nullptr) {
        IntegerData* data = (IntegerData*)element;
        enumeratedKeys.push_back(data->key);
    }
    
    // Should have all keys in sorted order (not insertion order)
    std::vector<int> expectedSortedKeys = {10, 20, 30, 40, 60};
    EXPECT_EQ(expectedSortedKeys.size(), enumeratedKeys.size());
    EXPECT_EQ(expectedSortedKeys, enumeratedKeys);
}

TEST_F(GenericTableSafeTest, GetElementByIndex) {
    std::vector<int> keys = {100, 200, 300};
    
    // Insert elements
    for (int key : keys) {
        IntegerData data(key, "Value");
        RtlInsertElementGenericTable(&table, &data, sizeof(data), nullptr);
    }
    
    // Test indexed access
    for (ULONG i = 0; i < keys.size(); i++) {
        PVOID element = RtlGetElementGenericTable(&table, i);
        EXPECT_NE(nullptr, element);
        IntegerData* data = (IntegerData*)element;
        EXPECT_EQ(keys[i], data->key);
    }
    
    // Test out of bounds access
    PVOID outOfBounds = RtlGetElementGenericTable(&table, static_cast<ULONG>(keys.size()));
    EXPECT_EQ(nullptr, outOfBounds);
}

TEST_F(GenericTableSafeTest, SequentialDeletion) {
    std::vector<int> keys = {1, 2, 3, 4, 5};
    
    // Insert all elements
    for (int key : keys) {
        IntegerData data(key, "Value");
        RtlInsertElementGenericTable(&table, &data, sizeof(data), nullptr);
    }
    
    EXPECT_EQ(keys.size(), RtlNumberGenericTableElements(&table));
    
    // Delete elements one by one
    for (int key : keys) {
        IntegerData deleteKey(key);
        BOOLEAN deleted = RtlDeleteElementGenericTable(&table, &deleteKey);
        EXPECT_TRUE(deleted);
    }
    
    EXPECT_TRUE(RtlIsGenericTableEmpty(&table));
    EXPECT_EQ(0, RtlNumberGenericTableElements(&table));
}

TEST_F(GenericTableSafeTest, DeleteNonExistentElements) {
    IntegerData data(123, "Exists");
    RtlInsertElementGenericTable(&table, &data, sizeof(data), nullptr);
    
    // Try to delete non-existent element
    IntegerData nonExistent(456);
    BOOLEAN deleted = RtlDeleteElementGenericTable(&table, &nonExistent);
    EXPECT_FALSE(deleted);
    
    // Original element should still exist
    EXPECT_EQ(1, RtlNumberGenericTableElements(&table));
    PVOID found = RtlLookupElementGenericTable(&table, &data);
    EXPECT_NE(nullptr, found);
}

TEST_F(GenericTableSafeTest, EmptyTableOperations) {
    // Test operations on empty table
    EXPECT_TRUE(RtlIsGenericTableEmpty(&table));
    EXPECT_EQ(0, RtlNumberGenericTableElements(&table));
    
    IntegerData searchKey(999);
    PVOID found = RtlLookupElementGenericTable(&table, &searchKey);
    EXPECT_EQ(nullptr, found);
    
    BOOLEAN deleted = RtlDeleteElementGenericTable(&table, &searchKey);
    EXPECT_FALSE(deleted);
    
    PVOID element = RtlEnumerateGenericTable(&table, TRUE);
    EXPECT_EQ(nullptr, element);
    
    PVOID indexed = RtlGetElementGenericTable(&table, 0);
    EXPECT_EQ(nullptr, indexed);
}

TEST_F(GenericTableSafeTest, MemoryUsage) {
    const int numElements = 10;
    
    // Insert elements and track memory usage
    for (int i = 0; i < numElements; i++) {
        IntegerData data(i, "TestValue");
        RtlInsertElementGenericTable(&table, &data, sizeof(data), nullptr);
    }
    
    // Should have made exactly numElements allocations
    EXPECT_EQ(numElements, g_allocationCount);
    EXPECT_EQ(0, g_freeCount); // No frees yet
    
    // Memory tracking will be verified in TearDown()
}
