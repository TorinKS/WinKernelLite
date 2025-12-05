#include <gtest/gtest.h>
#include <vector>

// Test WITHOUT RTL_USE_AVL_TABLES (should use splay trees)
#include <WinKernelLite/ntrtl.h>

// Forward declare the internal table entry header structure
typedef struct _TABLE_ENTRY_HEADER {
    RTL_SPLAY_LINKS SplayLinks;
    LIST_ENTRY ListEntry;
    LONGLONG UserData;
} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;

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

// Splay-specific element type and comparison function
struct SplayData {
    RTL_SPLAY_LINKS Links; // must be first
    int key;
};

RTL_GENERIC_COMPARE_RESULTS NTAPI CompareSplayData(
    PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
) {
    UNREFERENCED_PARAMETER(Table);
    SplayData* first = (SplayData*)FirstStruct;
    SplayData* second = (SplayData*)SecondStruct;
    if (first->key < second->key) return GenericLessThan;
    if (first->key > second->key) return GenericGreaterThan;
    return GenericEqual;
}

/**
 * @brief Test splay tree behavior by verifying root changes after lookup
 * 
 * This test verifies that in splay trees, when we access an element,
 * it gets splayed to the root. We examine the TableRoot pointer directly
 * to verify that it changes when different elements are accessed.
 */
TEST(RtlSplayTablesTest, VerifySplayingToRoot) {
    RTL_GENERIC_TABLE table;
    
    // Use SplayData and compare routine so TableRoot can be cast to SplayData*
    RtlInitializeGenericTable(&table, CompareSplayData, AllocateTestData, FreeTestData, nullptr);
    
    // Insert SplayData elements: 10, 20, 30
    std::vector<int> keys = {10, 20, 30};
    for (int key : keys) {
        SplayData sd;
        RtlZeroMemory(&sd, sizeof(sd));
        RtlInitializeSplayLinks(&sd.Links);
        sd.key = key;
        BOOLEAN newElement;
        PVOID inserted = RtlInsertElementGenericTable(&table, &sd, sizeof(SplayData), &newElement);
        EXPECT_TRUE(newElement);
        EXPECT_NE(inserted, nullptr);
    }
     
    EXPECT_EQ(RtlNumberGenericTableElements(&table), 3);
    
    // Get the initial root pointer
    PRTL_SPLAY_LINKS initialRoot = table.TableRoot;
    EXPECT_NE(initialRoot, nullptr) << "Table should have a root after insertions";
    
    // Look up element 10 - this should splay it to the root in a splay tree
    SplayData searchKey;
    RtlZeroMemory(&searchKey, sizeof(searchKey));
    RtlInitializeSplayLinks(&searchKey.Links);
    searchKey.key = 10;
    PVOID found = RtlLookupElementGenericTable(&table, &searchKey);
    EXPECT_NE(found, nullptr) << "Should find element 10";
    EXPECT_EQ(((SplayData*)found)->key, 10) << "Found element should have key 10";
    
    // Check that the root has changed - in splay trees, the looked-up element becomes root
    PRTL_SPLAY_LINKS newRoot = table.TableRoot;
    EXPECT_NE(newRoot, nullptr) << "Table should still have a root after lookup";
    
    // Verify that the root element is now the element we just looked up (key 10)
    // Use CONTAINING_RECORD to get the TABLE_ENTRY_HEADER from the RTL_SPLAY_LINKS
    // Then access the UserData which contains our SplayData
    TABLE_ENTRY_HEADER* rootHeader = CONTAINING_RECORD(newRoot, TABLE_ENTRY_HEADER, SplayLinks);
    SplayData* rootElem = (SplayData*)&rootHeader->UserData;
    EXPECT_NE(rootElem, nullptr) << "Root element pointer should not be null";
    EXPECT_EQ(rootElem->key, 10) << "After lookup, root should be element with key 10 (splaying occurred)";
    
    // The key insight: in a splay tree, accessing element 10 should move it closer to or at the root
    // We verify this by checking that the tree structure is being modified by operations
    
    // Look up a different element (30) - this should also change the root in a splay tree
    SplayData searchKey30;
    RtlZeroMemory(&searchKey30, sizeof(searchKey30));
    RtlInitializeSplayLinks(&searchKey30.Links);
    searchKey30.key = 30;
    found = RtlLookupElementGenericTable(&table, &searchKey30);
    EXPECT_NE(found, nullptr) << "Should find element 30";
    EXPECT_EQ(((SplayData*)found)->key, 30) << "Found element should have key 30";
    
    // Verify that element 30 is now the root (splaying occurred)
    PRTL_SPLAY_LINKS newRoot30 = table.TableRoot;
    EXPECT_NE(newRoot30, nullptr) << "Table should still have a root after lookup of 30";
    TABLE_ENTRY_HEADER* rootHeader30 = CONTAINING_RECORD(newRoot30, TABLE_ENTRY_HEADER, SplayLinks);
    SplayData* rootElem30 = (SplayData*)&rootHeader30->UserData;
    EXPECT_NE(rootElem30, nullptr) << "Root element pointer should not be null after lookup of 30";
    EXPECT_EQ(rootElem30->key, 30) << "After lookup, root should be element with key 30 (splaying occurred)";
    
    // Verify that the root changed from element 10 to element 30
    EXPECT_NE(newRoot, newRoot30) << "Root should have changed after looking up different element";
    
    // Check that we can access all elements correctly and verify splaying for element 20
    SplayData searchKey20;
    RtlZeroMemory(&searchKey20, sizeof(searchKey20));
    RtlInitializeSplayLinks(&searchKey20.Links);
    searchKey20.key = 20;
    found = RtlLookupElementGenericTable(&table, &searchKey20);
    EXPECT_NE(found, nullptr) << "Should find element 20";
    EXPECT_EQ(((SplayData*)found)->key, 20) << "Found element should have key 20";
    
    // Verify that element 20 is now the root (splaying occurred)
    PRTL_SPLAY_LINKS newRoot20 = table.TableRoot;
    EXPECT_NE(newRoot20, nullptr) << "Table should still have a root after lookup of 20";
    TABLE_ENTRY_HEADER* rootHeader20 = CONTAINING_RECORD(newRoot20, TABLE_ENTRY_HEADER, SplayLinks);
    SplayData* rootElem20 = (SplayData*)&rootHeader20->UserData;
    EXPECT_NE(rootElem20, nullptr) << "Root element pointer should not be null after lookup of 20";
    EXPECT_EQ(rootElem20->key, 20) << "After lookup, root should be element with key 20 (splaying occurred)";
    
    // Verify that the root changed from element 30 to element 20
    EXPECT_NE(newRoot30, newRoot20) << "Root should have changed after looking up element 20";
    
    // Test that the table maintains correct functionality regardless of splaying
    EXPECT_EQ(RtlNumberGenericTableElements(&table), 3) << "Should still have 3 elements";
    
    // Clean up - enumerate and delete all elements using the non-splaying enumerator
    PVOID restartKey = NULL;
    PVOID element;
    std::vector<int> keysToDelete;
    while ((element = RtlEnumerateGenericTableWithoutSplaying(&table, &restartKey)) != NULL) {
        SplayData* data = (SplayData*)element;
        keysToDelete.push_back(data->key);
    }
    
    for (int key : keysToDelete) {
        SplayData deleteKey;
        RtlZeroMemory(&deleteKey, sizeof(deleteKey));
        RtlInitializeSplayLinks(&deleteKey.Links);
        deleteKey.key = key;
        BOOLEAN deleted = RtlDeleteElementGenericTable(&table, &deleteKey);
        EXPECT_TRUE(deleted) << "Should be able to delete element " << key;
    }
    
    EXPECT_TRUE(RtlIsGenericTableEmpty(&table)) << "Table should be empty after cleanup";
}

