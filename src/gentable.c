// License nuances.
// I assume this code was originally developed by Microsoft, although it's not entirely clear.
// It was taken from a GitHub project that references the WRK (Windows Research Kernel?) in its name.
// The ReactOS code, on the other hand, is different, though it shares many similarities.
// Anyway, this professionally and effectively written code cannot be ignored as it is perfect for educational purposes.
// For the same educational reason, the code was also commented using AI (Sonnet 4.0), and all comments were reviewed and refined where necessary.

#include <WinKernelLite/Ntrtl.h>

#pragma pack(8)

//
// Generic table entry header with 8-byte alignment to ensure
// proper data alignment for user payload section.
//

typedef struct _TABLE_ENTRY_HEADER {

    RTL_SPLAY_LINKS SplayLinks;
    LIST_ENTRY ListEntry;
    LONGLONG UserData;

} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;

#pragma pack()


/**
 * @brief     This routine is used by all of the routines of the generic
    table package to locate the a node in the tree.  It will
    find and return (via the NodeOrParent parameter) the node
    with the given key, or if that node is not in the tree it
    will return (via the NodeOrParent parameter) a pointer to
    the parent.

 * 
 * @param[in] Table The generic table structure to search for the specified key
 * @param[in] Buffer Pointer to buffer containing the search key. The table implementation
 *                   does not examine key contents directly, delegating all key interpretation
 *                   to the user-supplied comparison routine
 * @param[out] NodeOrParent Will be set to point to either the node containing the target key
 *                          or the node that should serve as parent if the key were to be
 *                          inserted. Note: this parameter is NOT modified when the search
 *                          result is TableEmptyTree
 * 
 * @return TABLE_SEARCH_RESULT indicating search outcome:
 *         - TableEmptyTree: The tree contains no elements. NodeOrParent is not altered
 *         - TableFoundNode: A node with the specified key exists in the tree.
 *           NodeOrParent points to that node
 *         - TableInsertAsLeft: No node with the key was found. NodeOrParent points to
 *           the intended parent node. The new node should be inserted as the left child
 *         - TableInsertAsRight: No node with the key was found. NodeOrParent points to
 *           the intended parent node. The new node should be inserted as the right child
 */
static
TABLE_SEARCH_RESULT
FindNodeOrParent(
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer,
    OUT PRTL_SPLAY_LINKS *NodeOrParent
    )

{

    if (RtlIsGenericTableEmpty(Table)) {

        return TableEmptyTree;

    } else {

        /* State Machine Variables: Initialize search traversal context */
        
        /* Current position cursor for tree descent navigation */
        PRTL_SPLAY_LINKS NodeToExamine = Table->TableRoot;

        /* Temporary child reference for navigation decision branching */
        PRTL_SPLAY_LINKS Child;

        /* Comparison outcome from user-defined key comparison logic */
        RTL_GENERIC_COMPARE_RESULTS Result;

        while (TRUE) {

            /* Decision Point: Execute key comparison to determine navigation direction */
            Result = Table->CompareRoutine(
                         Table,
                         Buffer,
                         &((PTABLE_ENTRY_HEADER) NodeToExamine)->UserData
                         );

            if (Result == GenericLessThan) {

                if (Child = RtlLeftChild(NodeToExamine)) {

                    NodeToExamine = Child;

                } else {

                    /* Terminal Condition: Left leaf reached, return insertion coordinates */
                    *NodeOrParent = NodeToExamine;
                    return TableInsertAsLeft;

                }

            } else if (Result == GenericGreaterThan) {

                if (Child = RtlRightChild(NodeToExamine)) {

                    NodeToExamine = Child;

                } else {

                    /* Terminal Condition: Right leaf reached, return insertion coordinates */
                    *NodeOrParent = NodeToExamine;
                    return TableInsertAsRight;

                }


            } else {

                /* Terminal Condition: Exact match found, return located element */
                ASSERT(Result == GenericEqual);
                *NodeOrParent = NodeToExamine;
                return TableFoundNode;

            }

        }

    }

}

/**
 * @brief Configures a new generic table instance with user-defined operational callbacks.
 *   Accepts an uninitialized table variable and three essential function pointers
 *   that define the table's behavior. Required initialization step for all
 *   generic table variables prior to any operations.
 * 
 * @param[out] Table Pointer to the generic table variable that requires initialization
 * @param[in] CompareRoutine User-supplied routine that will be used to compare keys
 *                           within the table, determining element ordering and search logic
 * @param[in] AllocateRoutine User-supplied routine to call for allocating memory when
 *                            creating new nodes in the generic table structure
 * @param[in] FreeRoutine User-supplied routine to call for deallocating memory when
 *                        removing nodes from the generic table structure
 * @param[in] TableContext User-supplied context data that will be passed to the
 *                         comparison and memory management routines during operations
 * 
 * @return None
 */
VOID
RtlInitializeGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    IN PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    IN PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
    IN PVOID TableContext
    )

{

    /*
     * Phase 1: Initialize core tree infrastructure to empty state.
     * Establishes foundational structure for splay tree operations.
     */
    Table->TableRoot = NULL;
    InitializeListHead(&Table->InsertOrderList);

    /*
     * Phase 2: Reset element tracking and enumeration state management.
     * Prepares counters and position tracking for insertion operations.
     */
    Table->NumberGenericTableElements = 0;
    Table->OrderedPointer = &Table->InsertOrderList;
    Table->WhichOrderedElement = 0;

    /*
     * Phase 3: Bind user-supplied behavioral contracts to table instance.
     * Establishes runtime polymorphism through function pointer assignments.
     */
    Table->CompareRoutine = CompareRoutine;
    Table->AllocateRoutine = AllocateRoutine;
    Table->FreeRoutine = FreeRoutine;
    Table->TableContext = TableContext;

}


/**
 * @brief Simplified insertion interface that performs element lookup and delegates to full insertion routine.
 * 
 * This wrapper eliminates the need for callers to manage search state by automatically
 * performing the required table traversal and forwarding results to the optimized
 * insertion implementation.
 * 
 * @param[in] Table Target table for insertion operation
 * @param[in] Buffer Search key and source data for potential new element
 * @param[in] BufferSize Allocation size for new element data storage
 * @param[out] NewElement Optional output indicating insertion vs. existing element
 * 
 * @return Pointer to element data (new or existing)
 */
PVOID
RtlInsertElementGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer,
    IN CLONG BufferSize,
    OUT PBOOLEAN NewElement OPTIONAL
    )

{

    // Search state variables for tree traversal
    PRTL_SPLAY_LINKS NodeOrParent;
    TABLE_SEARCH_RESULT Lookup;

    // Perform tree search to locate insertion point
    Lookup = FindNodeOrParent(
                 Table,
                 Buffer,
                 &NodeOrParent
                 );

    
    return RtlInsertElementGenericTableFull(
                Table,
                Buffer,
                BufferSize,
                NewElement,
                NodeOrParent,
                Lookup
                );
}


/**
 * @brief Performs element insertion using pre-computed search metadata for optimization.
    Creates a new table entry by allocating memory and integrating it into the structure,
    or returns the existing element if a duplicate key is found. The NewElement parameter
    indicates whether insertion occurred or an existing entry was returned. Note that
    the Buffer content is copied to the newly allocated element, making the original
    buffer pointer invalid for referencing the inserted data. This function utilizes
    NodeOrParent and SearchResult from a prior RtlLookupElementGenericTableFull call
    to avoid redundant tree traversal operations.
 * 
 * @param[in] Table Target generic table structure for the insertion operation
 * @param[in] Buffer Source data that provides the search key and will be copied
 *                   into the newly allocated element upon successful insertion
 * @param[in] BufferSize Exact byte count for user data allocation, automatically
 *                       extended to include internal header overhead during the
 *                       memory allocation process
 * @param[out] NewElement Optional output flag indicating whether a new element was
 *                        created (TRUE) or an existing element was returned (FALSE)
 * @param[in] NodeOrParent Pre-located node reference from previous search operation,
 *                         containing either the existing element or insertion parent
 * @param[in] SearchResult Search outcome classification that determines the insertion
 *                         strategy or indicates element already exists
 * 
 * @return Direct pointer to the user data area of either the existing element
 *         or the newly inserted element
 */
PVOID
RtlInsertElementGenericTableFull (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer,
    IN CLONG BufferSize,
    OUT PBOOLEAN NewElement OPTIONAL,
    PVOID NodeOrParent,
    TABLE_SEARCH_RESULT SearchResult
    )

{
    /*
     * Local variable to hold the node that will be returned to caller.
     * This will reference either a newly allocated node or the existing
     * matching node depending on search results.
     */

    PRTL_SPLAY_LINKS NodeToReturn;

    if (SearchResult != TableFoundNode) {

        /*
         * Element does not exist - proceed with new node creation.
         * First verify table capacity limits before attempting allocation.
         */

        ASSERT(Table->NumberGenericTableElements != (MAXULONG-1));

        /*
         * Allocate memory for new table entry including overhead.
         * Request total size = user data + internal header structure.
         */

        NodeToReturn = Table->AllocateRoutine(
                           Table,
                           BufferSize+FIELD_OFFSET( TABLE_ENTRY_HEADER, UserData )
                           );

        /*
         * Handle allocation failure scenario by setting appropriate
         * return values and exiting early with NULL pointer.
         */

        if (NodeToReturn == NULL) {

            if (ARGUMENT_PRESENT(NewElement)) {

                *NewElement = FALSE;
            }

            return(NULL);
        }

        RtlInitializeSplayLinks(NodeToReturn);

        /*
         * Add the new element to the tail of the insertion order list.
         * This maintains chronological insertion sequence for enumeration.
         */

        InsertTailList(
            &Table->InsertOrderList,
            &((PTABLE_ENTRY_HEADER) NodeToReturn)->ListEntry
            );

        Table->NumberGenericTableElements++;

        /*
         * Integrate the new node into the splay tree structure based
         * on the search result classification.
         */

        if (SearchResult == TableEmptyTree) {

            Table->TableRoot = NodeToReturn;

        } else {

            if (SearchResult == TableInsertAsLeft) {

                RtlInsertAsLeftChild(
                    NodeOrParent,
                    NodeToReturn
                    );

            } else {

                RtlInsertAsRightChild(
                    NodeOrParent,
                    NodeToReturn
                    );
            }
        }

        /*
         * Transfer caller's data into the user data section of the
         * newly allocated table entry structure.
         */

        RtlCopyMemory(
            &((PTABLE_ENTRY_HEADER) NodeToReturn)->UserData,
            Buffer,
            BufferSize
            );

    } else {

        NodeToReturn = NodeOrParent;
    }

    /*
     * Perform splay operation to promote the target node (new or existing)
     * to the root position for enhanced future access performance.
     */

    Table->TableRoot = RtlSplay(NodeToReturn);

    if (ARGUMENT_PRESENT(NewElement)) {

        *NewElement = ((SearchResult == TableFoundNode)?(FALSE):(TRUE));
    }

    /*
     * Return pointer to the user data portion of the final result node.
     */

    return &((PTABLE_ENTRY_HEADER) NodeToReturn)->UserData;
}


/**
 * @brief Locates and removes a specified element from the generic table.
    Returns TRUE if the element was successfully found and deleted,
    or FALSE if no matching element exists. The input buffer is used
    solely for key-based element identification during the search process.
 * 
 * @param[in] Table Reference to the target generic table structure containing
 *                  the element designated for potential removal operations
 * @param[in] Buffer Search key data utilized by the comparison routine to
 *                   identify the specific element targeted for deletion from the table
 * 
 * @return TRUE upon successful element location and removal, or FALSE when
 *         the specified element cannot be found in the table
 */
BOOLEAN
RtlDeleteElementGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )

{

    //
    // Target node reference or potential insertion position for search operation.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Search operation result classification and outcome descriptor.
    //
    TABLE_SEARCH_RESULT Lookup;

    Lookup = FindNodeOrParent(
                 Table,
                 Buffer,
                 &NodeOrParent
                 );

    if ((Lookup == TableEmptyTree) || (Lookup != TableFoundNode)) {

        return FALSE;

    } else {

        //
        // Execute tree structure modification by removing target node.
        //

        Table->TableRoot = RtlDelete(NodeOrParent);

        //
        // Eliminate node from insertion-order tracking list and update counters.
        //

        RemoveEntryList(&((PTABLE_ENTRY_HEADER) NodeOrParent)->ListEntry);
        Table->NumberGenericTableElements--;
        Table->WhichOrderedElement = 0;
        Table->OrderedPointer = &Table->InsertOrderList;

        //
        // Delegate memory cleanup to user-defined deallocation routine.
        // CRITICAL: Deallocation callback receives complete node structure
        // pointer (including splay links) rather than isolated user data
        // section, enabling proper memory management of entire allocation.
        //

        Table->FreeRoutine(Table,NodeOrParent);
        return TRUE;

    }

}


/**
 * @brief Searches for an element within the generic table using the provided key.
    Returns a pointer to the associated user data structure if found,
    or NULL if the element does not exist. The input buffer serves
    exclusively as a search key for element identification.
 * 
 * @param[in] Table Reference to the target generic table structure for search
 * @param[in] Buffer Search key data for element comparison operations performed
 *                   by the registered user-defined comparison routine
 * 
 * @return Pointer to user data of the successfully located element, or NULL
 *         if the element does not exist within the table structure
 */
PVOID
RtlLookupElementGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )

{
    //
    // Storage for node reference or potential insertion position.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Container for detailed search operation outcome.
    //
    TABLE_SEARCH_RESULT Lookup;

    return RtlLookupElementGenericTableFull(
                Table,
                Buffer,
                &NodeOrParent,
                &Lookup
                );
}


/**
 * @brief Executes element search within the generic table structure with dual-purpose functionality.
 * 
 * Provides both retrieval and insertion optimization capabilities. This enhanced lookup
 * operation returns either the target element's user data (on successful location) or
 * critical insertion metadata (when element is absent) to facilitate subsequent optimized
 * insertion operations.
 * 
 * The function leverages the internal tree search mechanism to locate elements while
 * simultaneously capturing positional information that can be reused to eliminate
 * redundant tree traversals during subsequent insertion calls. Upon successful element
 * discovery, the target node is automatically promoted to the root position through
 * splaying to enhance future access performance.
 * 
 * This routine serves as the foundation for optimized insert-or-update patterns where
 * applications need to conditionally insert elements based on their current presence
 * in the table.
 * 
 * @param[in] Table Pointer to the target generic table structure for search operations
 * @param[in] Buffer Pointer to the search key data that will be compared against
 *                   existing table elements using the registered comparison routine
 * @param[out] NodeOrParent Receives either the located target node (on successful search)
 *                          or the appropriate parent node position (for potential insertion),
 *                          enabling optimized subsequent operations
 * @param[out] SearchResult Receives detailed search outcome classification indicating whether
 *                          the element was found, the table was empty, or the insertion
 *                          position relationship (left/right child)
 * 
 * @return Pointer to user data section of the located element on successful search,
 *         or NULL when element is not present in the table structure
 */
PVOID
NTAPI
RtlLookupElementGenericTableFull (
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer,
    OUT PVOID *NodeOrParent,
    OUT TABLE_SEARCH_RESULT *SearchResult
    )

{

    //
    // Execute search operation and capture comprehensive result metadata.
    //

    *SearchResult = FindNodeOrParent(
                        Table,
                        Buffer,
                        (PRTL_SPLAY_LINKS *)NodeOrParent
                        );

    if ((*SearchResult == TableEmptyTree) || (*SearchResult != TableFoundNode)) {

        return NULL;

    } else {

        //
        // Promote located element to root position for optimized future access.
        //

        Table->TableRoot = RtlSplay(*NodeOrParent);

        //
        // Extract and return user data pointer from the discovered element.
        //

        return &((PTABLE_ENTRY_HEADER)*NodeOrParent)->UserData;
    }
}


/**
 * @brief Provides sequential access to generic table elements through destructive enumeration with automatic tree restructuring.
 * 
 * Each call returns a pointer to the user data of the next element in sorted order while
 * simultaneously performing splay operations to optimize future access patterns. The
 * enumeration process modifies the tree structure by splaying accessed nodes to the root
 * position, which may improve performance for subsequent operations on recently enumerated
 * elements.
 * 
 * The Restart parameter controls whether to begin enumeration from the minimum element or
 * continue from the current root position. This function is designed for comprehensive
 * table traversal where tree restructuring is acceptable or beneficial.
 * 
 * Standard enumeration loop structure:
 * 
 *     ptr = EnumerateGenericTable(Table, TRUE);
 *     while (ptr != NULL) {
 *         // Process ptr (user data)
 *         ptr = EnumerateGenericTable(Table, FALSE);
 *     }
 * 
 * @param[in] Table Pointer to the generic table structure for enumeration
 * @param[in] Restart Controls enumeration behavior: TRUE initiates enumeration from
 *                    the minimum element, FALSE continues from the current root and
 *                    advances to its in-order successor while splaying the result
 * 
 * @return Pointer to user data of the next element in enumeration sequence,
 *         or NULL when no additional elements remain
 */
PVOID
RtlEnumerateGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN BOOLEAN Restart
    )

{

    if (RtlIsGenericTableEmpty(Table)) {

        //
        // Handle empty table case - immediate return with no processing.
        //

        return NULL;

    } else {

        //
        // Enumeration target node for current iteration step.
        //
        PRTL_SPLAY_LINKS NodeToReturn;

        //
        // Determine enumeration strategy based on restart parameter.
        //

        if (Restart) {

            //
            // Initialize enumeration by descending to leftmost node (minimum key).
            //

            for (
                NodeToReturn = Table->TableRoot;
                RtlLeftChild(NodeToReturn);
                NodeToReturn = RtlLeftChild(NodeToReturn)
                ) {
                ;
            }

            Table->TableRoot = RtlSplay(NodeToReturn);

        } else {

            //
            // Continue enumeration by finding the next element in sorted sequence.
            // Current root represents the previously returned element; locate its
            // immediate successor and promote it to root via splaying operation.
            // Handle end-of-sequence condition when no successor exists.
            //

            NodeToReturn = RtlRealSuccessor(Table->TableRoot);

            if (NodeToReturn) {

                Table->TableRoot = RtlSplay(NodeToReturn);

            }

        }

        //
        // Return user data pointer from current enumeration node, or NULL
        // if enumeration sequence has been exhausted.
        //

        return ((NodeToReturn)?
                   ((PVOID)&((PTABLE_ENTRY_HEADER)NodeToReturn)->UserData)
                  :((PVOID)(NULL)));

    }

}


/**
 * @brief Check whether the input table is empty
 * 
 * @param[in] Table Pointer to the target generic table structure for emptiness analysis
 * 
 * @return TRUE if the table contains no elements (empty state),
 *         FALSE if the table contains one or more elements
 */
BOOLEAN
RtlIsGenericTableEmpty (
    IN PRTL_GENERIC_TABLE Table
    )

/*
    Returns TRUE if the table is
    empty, otherwise it returns FALSE.
*/

{
    return ((Table->TableRoot)?(FALSE):(TRUE));
}

/**
 * @brief Retrieves an element from the generic table at the specified insertion-order position.
 * 
 * Provides indexed access to elements based on their chronological insertion sequence
 * rather than their sorted tree order. The function performs bounds checking and returns
 * NULL for invalid indices. Valid index range is 0 to (element_count - 1), where 0
 * corresponds to the first inserted element and the maximum index corresponds to the
 * most recently inserted element.
 * 
 * Important: Element removal operations cause index values to shift for all elements
 * that were inserted after the removed element, potentially invalidating previously
 * stored index references.
 * 
 * @param[in] Table Pointer to the target generic table structure for element retrieval
 * @param[in] I Zero-based index position specifying which element to retrieve based
 *              on insertion chronology (0 = first inserted, 1 = second inserted, etc.)
 * 
 * @return Pointer to user data section of the element at the specified index position,
 *         or NULL if the index is out of bounds or the table is empty
 */
PVOID
RtlGetElementGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN ULONG I
    )

{

    //
    // Track the enumeration position within the insertion order list.
    //
    ULONG CurrentLocation = Table->WhichOrderedElement;

    //
    // Store total count of elements present in the table.
    //
    ULONG NumberInTable = Table->NumberGenericTableElements;

    //
    // Convert zero-based index to one-based for internal calculations.
    //
    // Overflow protection: If overflow occurs, the bounds check below
    // will catch it and return NULL appropriately.
    //
    ULONG NormalizedI = I + 1;

    //
    // Variables to calculate optimal traversal path distances.
    //
    ULONG ForwardDistance,BackwardDistance;

    //
    // Pointer to traverse the insertion order linked list.
    //
    PLIST_ENTRY CurrentNode = Table->OrderedPointer;


    //
    // Perform bounds validation - reject invalid indices immediately.
    //

    if ((I == MAXULONG) || (NormalizedI > NumberInTable)) return NULL;

    //
    // Optimization: Return immediately if already positioned at target element.
    //

    if (NormalizedI == CurrentLocation) {

        return &((PTABLE_ENTRY_HEADER) CONTAINING_RECORD(CurrentNode, TABLE_ENTRY_HEADER, ListEntry))->UserData;
    }

    //
    // Determine optimal traversal strategy based on current position.
    //

    if (CurrentLocation > NormalizedI) {

        //
        // Target is before current position - choose between backward traversal
        // from current position or forward traversal from list head.
        //
        // The decision depends on which path requires fewer steps. Since
        // backward traversal from current position may need to wrap around
        // the circular list, we only use it when the target is closer to
        // current position than to the list head.
        //
        // Backward traversal is optimal when target index exceeds the
        // midpoint between list head and current position.
        //

        if (NormalizedI > (CurrentLocation/2)) {

            //
            // Target is closer to current position - traverse backward.
            //

            for (
                BackwardDistance = CurrentLocation - NormalizedI;
                BackwardDistance;
                BackwardDistance--
                ) {

                CurrentNode = CurrentNode->Blink;

            }
        } else {

            //
            // Target is closer to list head - restart from beginning.
            //

            for (
                CurrentNode = &Table->InsertOrderList;
                NormalizedI;
                NormalizedI--
                ) {

                CurrentNode = CurrentNode->Flink;

            }

        }

    } else {


        //
        // Target is after current position - choose between forward traversal
        // from current position or backward traversal from list tail.
        //
        // Similar to above case, we select the path requiring fewer steps.
        // Backward traversal from tail may be more efficient for elements
        // near the end of the list.
        //

        ForwardDistance = NormalizedI - CurrentLocation;

        //
        // Calculate cost of backward traversal from list tail.
        //

        BackwardDistance = (NumberInTable - NormalizedI) + 1;

        if (ForwardDistance <= BackwardDistance) {

            for (
                ;
                ForwardDistance;
                ForwardDistance--
                ) {

                CurrentNode = CurrentNode->Flink;

            }


        } else {

            for (
                CurrentNode = &Table->InsertOrderList;
                BackwardDistance;
                BackwardDistance--
                ) {

                CurrentNode = CurrentNode->Blink;

            }

        }

    }

    //
    // Update enumeration state and extract user data from located element.
    //

    Table->OrderedPointer = CurrentNode;
    Table->WhichOrderedElement = I+1;

    return &((PTABLE_ENTRY_HEADER) CONTAINING_RECORD(CurrentNode, TABLE_ENTRY_HEADER, ListEntry))->UserData;

}


/**
 * @brief Retrieves the current element count within the generic table structure.
 * 
 * Provides a count of elements currently stored within the specified generic table
 * structure. 
 * 
 * @param[in] Table Pointer to an initialized generic table structure for which
 *                  the element count is requested
 * 
 * @return Current number of elements contained in the table as a 32-bit unsigned integer value
 */
ULONG
RtlNumberGenericTableElements(
    IN PRTL_GENERIC_TABLE Table
    )

{

    return Table->NumberGenericTableElements;

}


/**
 * @brief Performs sequential enumeration of generic table elements without modifying the underlying splay tree structure.
 * 
 * Enables iteration through all table entries in sorted order while preserving the current
 * tree topology, making it suitable for scenarios where tree restructuring operations
 * should be avoided. 
 * 
 * The enumeration mechanism utilizes a continuation token (RestartKey) to maintain position
 * state between successive calls. When RestartKey contains NULL, enumeration begins from
 * the smallest element; otherwise, iteration continues from the node referenced by the
 * restart token.
 * 
 * Typical enumeration pattern:
 * 
 *     PVOID RestartKey = NULL;
 *     PVOID Element;
 *     
 *     while ((Element = RtlEnumerateGenericTableWithoutSplaying(Table, &RestartKey)) != NULL) {
 *         // Process Element
 *     }
 * 
 * @param[in] Table Pointer to the target generic table structure
 * @param[in,out] RestartKey Enumeration state pointer. Initialize to NULL for first call,
 *                           then pass unmodified value from previous call to continue
 *                           enumeration sequence. Set to NULL when enumeration completes.
 * 
 * @return Pointer to user data section of next element in sequence,
 *         or NULL when enumeration is complete
 */
PVOID
RtlEnumerateGenericTableWithoutSplaying (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID *RestartKey
    )

{

    if (RtlIsGenericTableEmpty(Table)) {

        return NULL;

    } else {

        //
        // Node pointer for traversing tree structure during enumeration.
        //
        PRTL_SPLAY_LINKS NodeToReturn;

        //
        // Check restart token to determine enumeration starting point.
        //

        if (*RestartKey == NULL) {

            //
            // Begin enumeration by locating the minimum element (leftmost leaf).
            //

            for (
                NodeToReturn = Table->TableRoot;
                RtlLeftChild(NodeToReturn);
                NodeToReturn = RtlLeftChild(NodeToReturn)
                ) {
                ;
            }

            *RestartKey = NodeToReturn;

        } else {

            //
            // Continue enumeration from stored position by advancing to
            // the next node in in-order sequence using successor operation.
            //

            NodeToReturn = RtlRealSuccessor(*RestartKey);

            if (NodeToReturn) {

                *RestartKey = NodeToReturn;

            } else {

                //
                // Enumeration completed - reset RestartKey to NULL to indicate
                // end of sequence for subsequent calls.
                // Note: it differs from the original code

                *RestartKey = NULL;

            }

        }

        //
        // Extract user data from current node, or return NULL if enumeration
        // has reached the end of the sequence.
        //

        return ((NodeToReturn)?
                   ((PVOID)&((PTABLE_ENTRY_HEADER)NodeToReturn)->UserData)
                  :((PVOID)(NULL)));

    }

}
