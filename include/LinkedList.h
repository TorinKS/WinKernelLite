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
FORCEINLINE
VOID
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead->Blink = ListHead;
    return;
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
BOOLEAN
CFORCEINLINE
IsListEmpty(
    _In_ const LIST_ENTRY* ListHead
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
FORCEINLINE
BOOLEAN
RemoveEntryList(
    _In_ PLIST_ENTRY Entry
)

{

    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
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
FORCEINLINE
PLIST_ENTRY
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead
)

{

    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
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
FORCEINLINE
PLIST_ENTRY
RemoveTailList(
    _Inout_ PLIST_ENTRY ListHead
)

{

    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
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
FORCEINLINE
VOID
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
)
{

    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
    return;
}


/**
 * @brief Inserts a new entry at the beginning of the list
 *
 * Places the new entry after the list head, making it the first entry in the list.
 *
 * @param[in,out] ListHead Pointer to the LIST_ENTRY that serves as the list head
 * @param[in,out] Entry Pointer to the LIST_ENTRY to be inserted
 */
FORCEINLINE
VOID
InsertHeadList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
)
{

    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
    return;
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
FORCEINLINE
VOID
AppendTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY ListToAppend
)
{

    PLIST_ENTRY ListEnd = ListHead->Blink;

    ListHead->Blink->Flink = ListToAppend;
    ListHead->Blink = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink = ListEnd;
    return;
}




