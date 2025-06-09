#include <gtest/gtest.h>
#include <Windows.h>
#include "../include/LinkedList.h"

// Helper struct for testing
struct TestItem {
    LIST_ENTRY ListEntry;
    int Value;
};

// Helper macro to get containing record
// #define CONTAINING_RECORD(address, type, field) \
//     ((type *)((char*)(address) - (size_t)(&((type *)0)->field)))

class LinkedListTest : public ::testing::Test {
protected:
    LIST_ENTRY ListHead;
    std::vector<TestItem*> items;  // For cleanup

    void SetUp() override {
        InitializeListHead(&ListHead);
    }

    void TearDown() override {
        for (auto item : items) {
            delete item;
        }
        items.clear();
    }

    TestItem* CreateTestItem(int value) {
        TestItem* item = new TestItem();
        item->Value = value;
        items.push_back(item);
        return item;
    }
};

TEST_F(LinkedListTest, InitializeListHead_CreatesEmptyList) {
    EXPECT_TRUE(IsListEmpty(&ListHead));
    EXPECT_EQ(ListHead.Flink, &ListHead);
    EXPECT_EQ(ListHead.Blink, &ListHead);
}

TEST_F(LinkedListTest, InsertHeadList_SingleItem) {
    TestItem* item = CreateTestItem(1);
    
    InsertHeadList(&ListHead, &item->ListEntry);
    
    EXPECT_FALSE(IsListEmpty(&ListHead));
    EXPECT_EQ(ListHead.Flink, &item->ListEntry);
    EXPECT_EQ(ListHead.Blink, &item->ListEntry);
    EXPECT_EQ(item->ListEntry.Flink, &ListHead);
    EXPECT_EQ(item->ListEntry.Blink, &ListHead);
}

TEST_F(LinkedListTest, InsertTailList_SingleItem) {
    TestItem* item = CreateTestItem(1);
    
    InsertTailList(&ListHead, &item->ListEntry);
    
    EXPECT_FALSE(IsListEmpty(&ListHead));
    EXPECT_EQ(ListHead.Flink, &item->ListEntry);
    EXPECT_EQ(ListHead.Blink, &item->ListEntry);
    EXPECT_EQ(item->ListEntry.Flink, &ListHead);
    EXPECT_EQ(item->ListEntry.Blink, &ListHead);
}

TEST_F(LinkedListTest, MultipleItems_InsertHead) {
    TestItem* item1 = CreateTestItem(1);
    TestItem* item2 = CreateTestItem(2);
    
    InsertHeadList(&ListHead, &item1->ListEntry);
    InsertHeadList(&ListHead, &item2->ListEntry);
    
    EXPECT_FALSE(IsListEmpty(&ListHead));
    EXPECT_EQ(ListHead.Flink, &item2->ListEntry);
    EXPECT_EQ(ListHead.Blink, &item1->ListEntry);
    EXPECT_EQ(item2->ListEntry.Flink, &item1->ListEntry);
    EXPECT_EQ(item1->ListEntry.Blink, &item2->ListEntry);
}

TEST_F(LinkedListTest, MultipleItems_InsertTail) {
    TestItem* item1 = CreateTestItem(1);
    TestItem* item2 = CreateTestItem(2);
    
    InsertTailList(&ListHead, &item1->ListEntry);
    InsertTailList(&ListHead, &item2->ListEntry);
    
    EXPECT_FALSE(IsListEmpty(&ListHead));
    EXPECT_EQ(ListHead.Flink, &item1->ListEntry);
    EXPECT_EQ(ListHead.Blink, &item2->ListEntry);
    EXPECT_EQ(item1->ListEntry.Flink, &item2->ListEntry);
    EXPECT_EQ(item2->ListEntry.Blink, &item1->ListEntry);
}

TEST_F(LinkedListTest, RemoveHeadList_SingleItem) {
    TestItem* item = CreateTestItem(1);
    InsertHeadList(&ListHead, &item->ListEntry);
    
    PLIST_ENTRY removed = RemoveHeadList(&ListHead);
    
    EXPECT_TRUE(IsListEmpty(&ListHead));
    EXPECT_EQ(removed, &item->ListEntry);
    EXPECT_EQ(ListHead.Flink, &ListHead);
    EXPECT_EQ(ListHead.Blink, &ListHead);
}

TEST_F(LinkedListTest, RemoveTailList_SingleItem) {
    TestItem* item = CreateTestItem(1);
    InsertTailList(&ListHead, &item->ListEntry);
    
    PLIST_ENTRY removed = RemoveTailList(&ListHead);
    
    EXPECT_TRUE(IsListEmpty(&ListHead));
    EXPECT_EQ(removed, &item->ListEntry);
    EXPECT_EQ(ListHead.Flink, &ListHead);
    EXPECT_EQ(ListHead.Blink, &ListHead);
}

TEST_F(LinkedListTest, RemoveEntryList_FromMiddle) {
    TestItem* item1 = CreateTestItem(1);
    TestItem* item2 = CreateTestItem(2);
    TestItem* item3 = CreateTestItem(3);
    
    InsertTailList(&ListHead, &item1->ListEntry);
    InsertTailList(&ListHead, &item2->ListEntry);
    InsertTailList(&ListHead, &item3->ListEntry);
    
    RemoveEntryList(&item2->ListEntry);
    
    EXPECT_FALSE(IsListEmpty(&ListHead));
    EXPECT_EQ(item1->ListEntry.Flink, &item3->ListEntry);
    EXPECT_EQ(item3->ListEntry.Blink, &item1->ListEntry);
}

TEST_F(LinkedListTest, AppendTailList_EmptyToEmpty) {
    LIST_ENTRY otherList;
    InitializeListHead(&otherList);
    
    AppendTailList(&ListHead, &otherList);
    
    EXPECT_TRUE(IsListEmpty(&ListHead));
}

TEST_F(LinkedListTest, AppendTailList_NonEmptyToEmpty) {
    LIST_ENTRY otherList;
    InitializeListHead(&otherList);
    
    TestItem* item1 = CreateTestItem(1);
    TestItem* item2 = CreateTestItem(2);
    InsertTailList(&otherList, &item1->ListEntry);
    InsertTailList(&otherList, &item2->ListEntry);
    
    AppendTailList(&ListHead, &otherList);
    
    EXPECT_FALSE(IsListEmpty(&ListHead));
    EXPECT_EQ(ListHead.Flink, &item1->ListEntry);
    EXPECT_EQ(ListHead.Blink, &item2->ListEntry);
    EXPECT_EQ(item1->ListEntry.Flink, &item2->ListEntry);
    EXPECT_EQ(item2->ListEntry.Blink, &item1->ListEntry);
}

TEST_F(LinkedListTest, AppendTailList_EmptyToNonEmpty) {
    LIST_ENTRY otherList;
    InitializeListHead(&otherList);
    
    TestItem* item1 = CreateTestItem(1);
    InsertTailList(&ListHead, &item1->ListEntry);
    
    AppendTailList(&ListHead, &otherList);
    
    EXPECT_FALSE(IsListEmpty(&ListHead));
    EXPECT_EQ(ListHead.Flink, &item1->ListEntry);
    EXPECT_EQ(ListHead.Blink, &item1->ListEntry);
}

TEST_F(LinkedListTest, AppendTailList_NonEmptyToNonEmpty) {
    LIST_ENTRY otherList;
    InitializeListHead(&otherList);
    
    TestItem* item1 = CreateTestItem(1);
    TestItem* item2 = CreateTestItem(2);
    TestItem* item3 = CreateTestItem(3);
    TestItem* item4 = CreateTestItem(4);
    
    InsertTailList(&ListHead, &item1->ListEntry);
    InsertTailList(&ListHead, &item2->ListEntry);
    InsertTailList(&otherList, &item3->ListEntry);
    InsertTailList(&otherList, &item4->ListEntry);
    
    AppendTailList(&ListHead, &otherList);
    
    EXPECT_FALSE(IsListEmpty(&ListHead));
    EXPECT_EQ(ListHead.Flink, &item1->ListEntry);
    EXPECT_EQ(ListHead.Blink, &item4->ListEntry);
    EXPECT_EQ(item2->ListEntry.Flink, &item3->ListEntry);
    EXPECT_EQ(item3->ListEntry.Blink, &item2->ListEntry);
}

TEST_F(LinkedListTest, TraverseForward) {
    TestItem* items[3] = {
        CreateTestItem(1),
        CreateTestItem(2),
        CreateTestItem(3)
    };
    
    for (auto item : items) {
        InsertTailList(&ListHead, &item->ListEntry);
    }
    
    int i = 0;
    for (PLIST_ENTRY entry = ListHead.Flink; entry != &ListHead; entry = entry->Flink) {
        TestItem* item = CONTAINING_RECORD(entry, TestItem, ListEntry);
        EXPECT_EQ(item->Value, items[i]->Value);
        i++;
    }
    EXPECT_EQ(i, 3);
}

TEST_F(LinkedListTest, TraverseBackward) {
    TestItem* items[3] = {
        CreateTestItem(1),
        CreateTestItem(2),
        CreateTestItem(3)
    };
    
    for (auto item : items) {
        InsertTailList(&ListHead, &item->ListEntry);
    }
    
    int i = 2;
    for (PLIST_ENTRY entry = ListHead.Blink; entry != &ListHead; entry = entry->Blink) {
        TestItem* item = CONTAINING_RECORD(entry, TestItem, ListEntry);
        EXPECT_EQ(item->Value, items[i]->Value);
        i--;
    }
    EXPECT_EQ(i, -1);
}
