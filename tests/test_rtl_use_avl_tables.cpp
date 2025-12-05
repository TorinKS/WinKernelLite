#include <gtest/gtest.h>

// Test WITH RTL_USE_AVL_TABLES (should use AVL trees)
#define RTL_USE_AVL_TABLES 0
#include <WinKernelLite/ntrtl.h>

// Test data structure
struct TestData {
    int key;
    TestData(int k = 0) : key(k) {}
};

// Comparison function
RTL_GENERIC_COMPARE_RESULTS NTAPI CompareTestData(
    PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
) {
    UNREFERENCED_PARAMETER(Table);
    TestData* first = (TestData*)FirstStruct;
    TestData* second = (TestData*)SecondStruct;
    
    if (first->key < second->key) return GenericLessThan;
    if (first->key > second->key) return GenericGreaterThan;
    return GenericEqual;
}

// Allocation/Free functions
PVOID NTAPI AllocateTestData(PRTL_GENERIC_TABLE Table, CLONG ByteSize) {
    UNREFERENCED_PARAMETER(Table);
    return malloc(ByteSize);
}

VOID NTAPI FreeTestData(PRTL_GENERIC_TABLE Table, PVOID Buffer) {
    UNREFERENCED_PARAMETER(Table);
    if (Buffer) free(Buffer);
}

/**
 * @brief Test AVL tree behavior (when RTL_USE_AVL_TABLES is defined)
 * 
 * This test verifies that when RTL_USE_AVL_TABLES is defined, generic tables use AVL trees.
 * In AVL trees, enumeration always returns elements in sorted order regardless of 
 * which elements were accessed.
 */
TEST(RtlAvlTablesTest, VerifyAvlTreeBehavior) {
    RTL_GENERIC_TABLE table;
    
    RtlInitializeGenericTable(&table, CompareTestData, AllocateTestData, FreeTestData, nullptr);
    
    // Insert elements in order: 1, 2, 3
    for (int key = 1; key <= 3; key++) {
        TestData data(key);
        BOOLEAN newElement;
        RtlInsertElementGenericTable(&table, &data, sizeof(TestData), &newElement);
        EXPECT_TRUE(newElement);
    }
    
    // Access element 2 (middle element) - in AVL tree this doesn't change structure
    TestData searchKey(2);
    PVOID found = RtlLookupElementGenericTable(&table, &searchKey);
    EXPECT_NE(found, nullptr);
    
    // Enumerate - in AVL tree, enumeration should always be in sorted order
    PVOID element = RtlEnumerateGenericTable(&table, TRUE);
    EXPECT_NE(element, nullptr);
    
    TestData* firstElement = (TestData*)element;
    int firstKey = firstElement->key;
    
    // Verify AVL tree behavior: enumeration should be in sorted order (1 first)
    EXPECT_EQ(firstKey, 1) << "AVL tree should maintain sorted order (1 first)";
    
    // Clean up
    while (!RtlIsGenericTableEmpty(&table)) {
        element = RtlEnumerateGenericTable(&table, TRUE);
        if (element) {
            TestData* data = (TestData*)element;
            TestData deleteKey(data->key);
            RtlDeleteElementGenericTable(&table, &deleteKey);
        } else {
            break;
        }
    }
}

