/**
 * @file LinkedList.h
 * @brief Implementation of a generic doubly-linked list using Windows LIST_ENTRY structure
 *
 * This header provides standard operations for manipulating doubly-linked lists.
 * It uses the Windows kernel-style LIST_ENTRY structure where each list entry
 * contains forward and backward links (Flink and Blink).
 *
 * The list is circular, with the list head serving as a sentinel node.
 * When the list is empty, both Flink and Blink of the list head point to itself.
 */

#pragma once

/**
 * @brief Initializes a list head
 *
 * Sets up an empty list by making the list head's Flink and Blink point to itself,
 * creating an empty circular linked list.
 *
 * @param[out] ListHead Pointer to the LIST_ENTRY that serves as the list head
 */
static __forceinline
void
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

/**
 * @brief Checks if a list is empty
 *
 * A list is considered empty when the list head's Flink points to itself,
 * meaning there are no entries in the list.
 *
 * @param[in] ListHead Pointer to the LIST_ENTRY that serves as the list head
 * @return TRUE if the list is empty, FALSE otherwise
 */
_Must_inspect_result_
static __forceinline
BOOLEAN
IsListEmpty(
    _In_ const LIST_ENTRY* const ListHead
)
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

/**
 * @brief Removes an entry from a linked list
 *
 * Removes the specified entry by updating the Flink/Blink pointers of adjacent entries.
 * Does not free any memory associated with the entry.
 *
 * @param[in] Entry Pointer to the LIST_ENTRY to be removed
 * @return TRUE if the list is empty after removal, FALSE otherwise
 */
static __forceinline
BOOLEAN
RemoveEntryList(
    _In_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY const Blink = Entry->Blink;
    PLIST_ENTRY const Flink = Entry->Flink;

    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

/**
 * @brief Removes and returns the first entry in the list
 *
 * Removes the entry immediately after the list head (the head of the list)
 * and returns a pointer to it. Does not free any memory.
 *
 * @param[in,out] ListHead Pointer to the LIST_ENTRY that serves as the list head
 * @return Pointer to the removed LIST_ENTRY
 * @note Caller must not call this function on an empty list
 */
static __forceinline
PLIST_ENTRY
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead
)
{
    PLIST_ENTRY const Entry = ListHead->Flink;
    PLIST_ENTRY const Flink = Entry->Flink;

    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

/**
 * @brief Removes and returns the last entry in the list
 *
 * Removes the entry immediately before the list head (the tail of the list)
 * and returns a pointer to it. Does not free any memory.
 *
 * @param[in,out] ListHead Pointer to the LIST_ENTRY that serves as the list head
 * @return Pointer to the removed LIST_ENTRY
 * @note Caller must not call this function on an empty list
 */
static __forceinline
PLIST_ENTRY
RemoveTailList(
    _Inout_ PLIST_ENTRY ListHead
)
{
    PLIST_ENTRY const Entry = ListHead->Blink;
    PLIST_ENTRY const Blink = Entry->Blink;

    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

/**
 * @brief Inserts a new entry at the end of the list
 *
 * Places the new entry before the list head, making it the last entry in the list.
 *
 * @param[in,out] ListHead Pointer to the LIST_ENTRY that serves as the list head
 * @param[in,out] Entry Pointer to the LIST_ENTRY to be inserted
 */
static __forceinline
void
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
)
{
    PLIST_ENTRY const Blink = ListHead->Blink;

    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

/**
 * @brief Inserts a new entry at the beginning of the list
 *
 * Places the new entry after the list head, making it the first entry in the list.
 *
 * @param[in,out] ListHead Pointer to the LIST_ENTRY that serves as the list head
 * @param[in,out] Entry Pointer to the LIST_ENTRY to be inserted
 */
static __forceinline
void
InsertHeadList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
)
{
    PLIST_ENTRY const Flink = ListHead->Flink;

    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

/**
 * @brief Appends one list to the end of another
 *
 * Takes all entries from ListToAppend and adds them to the end of the list 
 * identified by ListHead.
 *
 * @param[in,out] ListHead Pointer to the head of the first list
 * @param[in,out] ListToAppend Pointer to the head of the list to be appended
 * @note After this operation, ListToAppend should not be used without re-initialization
 */
static __forceinline
void
AppendTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY ListToAppend
)
{
    // If the list to append is empty (Flink points to self), nothing to do
    if (ListToAppend->Flink == ListToAppend) {
        return;
    }

    // Use block scope for temporary variables (C99)
    {
        PLIST_ENTRY const FirstNew = ListToAppend->Flink;     // First node of list to append
        PLIST_ENTRY const LastNew = ListToAppend->Blink;      // Last node of list to append
        PLIST_ENTRY const LastOld = ListHead->Blink;          // Last node of original list

        // Connect end of original list to start of appended list
        LastOld->Flink = FirstNew;
        FirstNew->Blink = LastOld;

        // Connect end of appended list to list head
        LastNew->Flink = ListHead;
        ListHead->Blink = LastNew;
    }
}




