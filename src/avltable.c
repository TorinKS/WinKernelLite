// License nuances.
// I assume this code was originally developed by Microsoft, although it's not entirely clear.
// It was taken from a GitHub project that references the WRK (Windows Research Kernel?) in its name.
// The ReactOS code, on the other hand, is different, though it shares many similarities.
// Anyway, this professionally and effectively written code cannot be ignored as it is perfect for educational purposes.
// For the same educational reason, the code was also commented using AI (Sonnet 4.0), and all comments were reviewed and refined where necessary.

#include <WinKernelLite/Ntrtl.h>

#pragma pack(8)

//
//  Generic table entry header - composite structure for tree nodes
//
//  Structure layout:
//  [RTL_BALANCED_LINKS] - Internal tree pointers and balance factors
//  [LONGLONG UserData]  - Start of caller's data area (8-byte aligned)
//
//  The 8-byte alignment pragma ensures proper data alignment for all
//  common data types when user data is appended after this header.
//

typedef struct _TABLE_ENTRY_HEADER {

    RTL_BALANCED_LINKS BalancedLinks;
    LONGLONG UserData;

} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;

#pragma pack()

/**
 * @brief Universal element matcher that accepts all table entries during enumeration.
 * 
 * This function serves as a default match function when no custom MatchFunction 
 * is specified. It always returns STATUS_SUCCESS regardless of element content,
 * effectively accepting all table entries during enumeration operations.
 * 
 * @param[in] Table Pointer to the AVL table structure
 * @param[in] P1 First comparison parameter (unused)
 * @param[in] P2 Second comparison parameter (unused)
 * 
 * @return STATUS_SUCCESS Always returns success
 */
NTSTATUS
MatchAll (
    IN PRTL_AVL_TABLE Table,
    IN PVOID P1,
    IN PVOID P2
    )

{
    return STATUS_SUCCESS;
}


/**
 * @brief Core search function for the generic table implementation.
 * 
 * Performs binary search traversal through the AVL tree to locate a target node 
 * or determine its insertion point. Used by all table operations requiring node lookup.
 * 
 * @param[in] Table AVL table structure to search within
 * @param[in] Buffer Search key data. Content interpretation is delegated to the
 *                   user-provided comparison function
 * @param[out] NodeOrParent Output pointer that receives either the target node (if found)
 *                          or the prospective parent node (if not found). Remains
 *                          unmodified only when the tree is completely empty
 * 
 * @return TABLE_SEARCH_RESULT Search outcome indicator:
 *         - TableEmptyTree: No nodes exist. NodeOrParent unchanged
 *         - TableFoundNode: Target located. NodeOrParent references the matching node
 *         - TableInsertAsLeft: Target absent. NodeOrParent references insertion parent. 
 *                              New node belongs as left child
 *         - TableInsertAsRight: Target absent. NodeOrParent references insertion parent.
 *                               New node belongs as right child
 */
TABLE_SEARCH_RESULT
FindNodeOrParent(
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer,
    OUT PRTL_BALANCED_LINKS *NodeOrParent
    )

{
    if (RtlIsGenericTableEmptyAvl(Table)) {

        return TableEmptyTree;

    } else {

        // In this AVL tree implementation, the actual tree root is stored as the RIGHT child of a special sentinel node called BalancedRoot.

        PRTL_BALANCED_LINKS NodeToExamine = Table->BalancedRoot.RightChild;

        PRTL_BALANCED_LINKS Child;
        RTL_GENERIC_COMPARE_RESULTS Result;

        ULONG NumberCompares = 0;

        while (TRUE) {

            // The FindNodeOrParent function uses an iterative loop to traverse the AVL tree:

            // Start at root: NodeToExamine = Table->BalancedRoot.RightChild
            // Compare keys: Call the comparison routine
            // Navigate based on result:
            // GenericLessThan -> go left (NodeToExamine = Child)
            // GenericGreaterThan -> go right (NodeToExamine = Child)
            // GenericEqual -> FOUND. Execute your selected code            

            //
            //  Call the user-supplied comparison routine to compare the search key 
            //  (in Buffer) with the key stored in the current tree node. The comparison
            //  result determines which direction to traverse in the binary search tree.
            //

            Result = Table->CompareRoutine(
                         Table,
                         Buffer,
                         &((PTABLE_ENTRY_HEADER) NodeToExamine)->UserData
                         );

            ASSERT(++NumberCompares <= Table->DepthOfTree);

            //
            //  If the search key is less than the current node's key,
            //  traverse to the left subtree.
            //
            if (Result == GenericLessThan) {

                if (Child = NodeToExamine->LeftChild) {

                    NodeToExamine = Child;

                } else {

                    // Consider this AVL tree:
                        //     50
                        //     /  \
                        //  30    70
                        //  / \     \
                        // 20  40    80                    
                    // Scenario: Searching for key 15

                    // Start at root (50): 15 < 50 -> go left
                    // At node (30): 15 < 30 -> try to go left
                    // At node (20): 15 < 20 -> try to go left
                    // Node (20) has no left child -> THIS CODE EXECUTES  
                    // This code serves two functions:

                    // Search Result: Indicates the key was not found
                    // Insertion Preparation: If the caller wants to insert this key, it now knows:
                    // Where to insert (as left child of NodeToExamine)
                    // What the parent would be (NodeToExamine)
                    // Return Values:
                    // *NodeOrParent = Current node (would be the parent of the new node)
                    // Return value = TableInsertAsLeft (indicates new node should be left child)                    

                    *NodeOrParent = NodeToExamine;
                    return TableInsertAsLeft;
                }

            } else if (Result == GenericGreaterThan) {

                if (Child = NodeToExamine->RightChild) {

                    NodeToExamine = Child;

                } else {

                    // the same case as with GenericLessThan see code above

                    *NodeOrParent = NodeToExamine;
                    return TableInsertAsRight;
                }

            } else {
                // The comparison routine returns GenericEqual
                // Meaning: search key exactly matches the current node's key
                // The search is successful - we found the target node                

                ASSERT(Result == GenericEqual);
                *NodeOrParent = NodeToExamine;
                return TableFoundNode;
            }
        }
    }
}


/**
 * @brief Executes a single rotation operation essential for maintaining AVL tree balance.
 * 
 * The operation elevates the specified child node by one tree level while demoting 
 * its parent by one level, establishing the parent as a child of the promoted node. 
 * This rotation redistributes subtree heights to restore balance according to Knuth's 
 * AVL balanced tree algorithms.
 * 
 * Implementation Notes:
 *  - Links updated here, balance factors managed externally
 * 
 * @param[in] C Pointer to the child node designated for promotion in the tree hierarchy.
 *              This function performs a single rotation operation (LL-rotation or RR-rotation)
 */
VOID
PromoteNode (
    IN PRTL_BALANCED_LINKS C
    )
{
    PRTL_BALANCED_LINKS P, G;

    //
    //  Capture the current parent and grandparent (may be the root).
    //

    P = C->Parent;
    G = P->Parent;

    //
    //  Break down the promotion into two cases based upon whether C is a left or right child.

    // All rotation steps promote child C above its parent P to restore balance
    // after P became unbalanced due to an insert or delete operation.
    
    if (P->LeftChild == C) {
        //
        // Right rotation: Promote left child C above its parent P
        // This restructures the tree to maintain AVL balance properties
        // when the left subtree becomes heavier than the right subtree.
        //
        // Transformation pattern (LL rotation):
        //
        //    Grandparent(G)       Grandparent(G)
        //          |                     |
        //       Parent(P)          Promoted(C)
        //         /    \      =>      /       \
        //    Child(C)   z            x       Parent(P)
        //      /   \                            /    \
        //     x     y                          y      z
        //
        // Result: C becomes new subtree root, P becomes C's right child
        //

        P->LeftChild = C->RightChild; // Transfer C's right subtree to P's left

        if (P->LeftChild != NULL) {
            P->LeftChild->Parent = P;
        }

        C->RightChild = P;

        //
        // Continue to common finalization code that establishes the 
        // grandparent-to-child relationship for both rotation cases.
        //

    } else {

        ASSERT(P->RightChild == C);

        //
        // Left rotation: Promote right child C above its parent P
        // This restructures the tree to maintain AVL balance properties
        // when the right subtree becomes heavier than the left subtree.
        //
        // Transformation pattern (RR-rotation):
        //
        //    Grandparent(G)       Grandparent(G)
        //          |                     |
        //       Parent(P)          Promoted(C)
        //         /    \      =>      /        \
        //        x   Child(C)   Parent(P)   Subtree(z)
        //              /   \       /    \
        //             y     z     x   Subtree(y)
        //
        // Result: C becomes new subtree root, P becomes C's left child
        //

        P->RightChild = C->LeftChild;

        if (P->RightChild != NULL) {
            P->RightChild->Parent = P;
        }

        C->LeftChild = P;
    }

    //
    // Establish parent-child relationship: Set promoted node C as the new child of P
    // This completes the structural reorganization for both rotation directions.
    //

    P->Parent = C;

    // Before This Code (after previous rotation steps):
    //       G ← (G still points to P)
    //       |
    //       P ← (P is no longer G's direct child in structure)
    //      ...

    //     C ← (C has been restructured but not yet connected to G)
    //    / \
    //   x   P ← (P is now C's right child)
    //      / \
    //     y   z

    // After This Code:    
    //   G ← (G now correctly points to C)
    //   |
    //   C ← (C now correctly points back to G as parent)
    //  / \
    // x   P
    //    / \
    //   y   z    

    if (G->LeftChild == P) {
        G->LeftChild = C;
    } else {
        ASSERT(G->RightChild == P);
        G->RightChild = C;
    }
    C->Parent = G;
}


/**
 * @brief Restores AVL tree balance when a node has become unbalanced.
 * 
 * This function analyzes unbalance patterns and performs appropriate rotation(s) 
 * to restore the AVL property when a node has a balance factor of ±2.
 * 
 * At entry, the target node S has a balance factor of ±1, but the corresponding 
 * subtree has grown by one level due to insertion or deletion, effectively 
 * creating a ±2 imbalance that violates the AVL constraint.
 * 
 * Three distinct rebalancing scenarios are handled:
 * 
 * Case 1 (Single Rotation): Child and parent have matching balance signs
 *         - Requires one PromoteNode call (LL or RR rotation)  
 *         - Occurs during both insertions and deletions
 * 
 * Case 2 (Double Rotation): Child and parent have opposite balance signs
 *         - Requires two PromoteNode calls (LR or RL rotation)
 *         - Occurs during both insertions and deletions
 * 
 * Case 3 (Delete-Only): Child node is perfectly balanced (balance = 0)
 *         - Requires one PromoteNode call with special balance factor handling
 *         - Can only occur during deletion operations
 * 
 * Implementation follows Knuth's balanced tree algorithm steps A7.iii, A8, and A9,
 * with additional support for deletion-specific Case 3.
 * 
 * @param[in] S Pointer to the node that has become unbalanced and requires rebalancing
 * 
 * @return TRUE if Case 3 was detected (allows deletion algorithm to terminate early),
 *         FALSE for Cases 1 and 2 (rebalancing must continue up the tree)
 */
ULONG
RebalanceNode (
    IN PRTL_BALANCED_LINKS S
    )
{
    PRTL_BALANCED_LINKS R, P;
    CHAR a;
    
    // This code identifies which subtree has become "too heavy" and needs to be rebalanced. The variable S points to a node that has just become unbalanced (with an effective balance factor of ±2).   
    
    // At this point in the algorithm:
    // Node S has a balance factor of ±1, but one of its subtrees just grew by another level
    // This creates an effective ±2 imbalance that violates the AVL property
    // Variable R will point to the root of the "heavy" subtree that caused the imbalance
    // The algorithm will then examine R's balance factor to determine what type of rotation is needed:
    // Case 1: R->Balance == a → Single rotation needed
    // Case 2: R->Balance == -a → Double rotation needed
    // Case 3: R->Balance == 0 → Special delete-only case    

    a = S->Balance;
    if (a == +1) { // the node is right-heavy
        R = S->RightChild;
    } else { // it is left-heavy
        R = S->LeftChild;
    }

    //
    //  CASE 1: Single Rotation Required (Knuth's Case 1)
    //  
    //  Condition: Both nodes S and R have the same balance factor sign
    //  Solution: Perform one single rotation by promoting R
    //  Algorithm: Execute PromoteNode(R) followed by balance factor updates
    //
    //  Visual Transformation (shown for right-heavy case where a == +1):
    //  Note: Mirror image occurs for left-heavy case (a == -1)
    //  Legend: ++ = critically unbalanced, + = slightly unbalanced, (h) = subtree height
    //
    //      BEFORE Rotation              AFTER Rotation
    //          |                           |
    //          S++                         R
    //         / \                         / \
    //       (h)  R+          ==>         S  (h+1)
    //           / \                     / \
    //         (h) (h+1)               (h) (h)
    //
    //  Operation Context and Height Analysis:
    //  
    //  During INSERT operations:
    //  - Triggered by: Adding node to R's right subtree
    //  - Height before: h+2 → Height after: h+2 (unchanged)
    //  - Result: Rebalancing may terminate (no height increase)
    //  
    //  During DELETE operations:
    //  - Triggered by: Removing node from S's left subtree  
    //  - Height before: h+3 → Height after: h+2 (decreased)
    //  - Result: Rebalancing must continue up the tree (height decreased)
    //

    if (R->Balance == a) {

        PromoteNode( R );
        R->Balance = 0;
        S->Balance = 0;
        return FALSE;

    //
    //  CASE 2: Double Rotation Required (Knuth's Case 2)
    //  
    //  Condition: Nodes S and R have opposite balance factor signs 
    //  Solution: Perform double rotation by promoting P twice
    //  Algorithm: Execute PromoteNode(P) twice, followed by complex balance factor updates
    //
    //  Implementation Details:
    //  - First promotion moves P above R
    //  - Second promotion moves P above S  
    //  - Result: P becomes the new subtree root
    //
    //  Visual Transformation (shown for right-left case where a == +1):
    //  Note: Mirror image occurs for left-right case (a == -1)
    //  Legend: ++ = critically unbalanced, + = right-heavy, - = left-heavy, (h) = subtree height
    //
    //  Subcase 2A: P initially right-heavy (P->Balance == +1)
    //      BEFORE Double Rotation         AFTER Double Rotation
    //          |                               |
    //          S++                             P
    //         /   \                           / \
    //       (h)    R-              ==>      S-    R
    //             /  \                     / \   / \
    //           P+  (h)                 (h)(h-1)(h)(h)
    //          / \
    //      (h-1) (h)
    //
    //  Subcase 2B: P initially left-heavy (P->Balance == -1)  
    //      BEFORE Double Rotation         AFTER Double Rotation
    //          |                                |
    //          S++                              P
    //         /   \                            /  \
    //       (h)    R-              ==>       S     R+
    //             /  \                      / \    / \
    //           P-  (h)                   (h) (h) (h-1)(h)
    //          / \
    //        (h)(h-1)
    //
    //  Operation Context and Height Analysis:
    //  
    //  During INSERT operations:
    //  - Triggered by: Adding node to P's subtree (within R's left subtree)
    //  - Height before: h+2 → Height after: h+2 (unchanged)
    //  - Result: Rebalancing may terminate (no height increase)
    //  
    //  During DELETE operations:
    //  - Triggered by: Removing node from S's left subtree
    //  - Height before: h+3 → Height after: h+2 (decreased)  
    //  - Result: Rebalancing must continue up the tree (height decreased)
    //

    }  else if (R->Balance == -a) {

        //
        //  Identify the pivot node P for the double rotation sequence.
        //  P is the "inside" grandchild that will become the new subtree root.
        //  Selection logic: Choose the child of R on the opposite side from the imbalance direction.
        //

        if (a == 1) {
            P = R->LeftChild;
        } else {
            P = R->RightChild;
        }

        // This is a double rotation that handles:
        // Right-Left (RL) pattern when a == +1
        // Left-Right (LR) pattern when a == -1

        // Visual Example (Right-Left Case):
        // Before (when a == +1, right-heavy with left-heavy right subtree):
        //     S++                   P
        //    / \                 /    \
        // (h)   R-      =>      S      R
        //      / \             / \    /  \
        //     P+ (h)         (h)(h-1)(h) (h)
        //    /  \
        // (h-1) (h)        

        PromoteNode( P );
        PromoteNode( P );

        //
        //  Balance Factor Adjustment After Double Rotation:
        //  
        //  After the double rotation, P is now the new subtree root with perfect balance.
        //  The final balance factors depend on P's original balance before rotation:
        //  
        //  Case 2A: P was originally heavy on same side as overall imbalance (P->Balance == a)
        //           → S becomes light on opposite side (S->Balance = -a)
        //           → R remains balanced (R->Balance = 0)
        //  
        //  Case 2B: P was originally heavy on opposite side from imbalance (P->Balance == -a)  
        //           → S remains balanced (S->Balance = 0)
        //           → R becomes light on same side as original imbalance (R->Balance = a)
        //  
        //  Case 2C: P was originally balanced (P->Balance == 0)
        //           → Both S and R remain balanced (both get Balance = 0)
        //  
        //  In all cases, P achieves perfect balance after becoming the new subtree root.
        //

        S->Balance = 0;
        R->Balance = 0;
        if (P->Balance == a) {
            S->Balance = -a;
        } else if (P->Balance == -a) {
            R->Balance = a;
        }

        P->Balance = 0;
        return FALSE;

    //
    //  CASE 3: Delete-Only Single Rotation (Special Balanced Child Case)
    //  
    //  Condition: Child node R is perfectly balanced (R->Balance == 0)
    //  Solution: Perform single rotation identical to Case 1, with modified balance factors
    //  Algorithm: Execute PromoteNode(R), update balances, and return TRUE to terminate
    //
    //  Key Characteristics:
    //  - Structurally identical to Case 1 but with different balance factor outcomes
    //  - Can ONLY occur during deletion operations, never during insertion
    //  - Results in height preservation (subtree height remains unchanged)
    //  - Allows early termination of rebalancing process
    //
    //  Visual Transformation (shown for right-heavy case where a == +1):
    //  Note: Mirror image occurs for left-heavy case (a == -1)
    //  Legend: ++ = critically unbalanced, + = slightly unbalanced, - = slightly unbalanced, (h) = subtree height
    //
    //      BEFORE Rotation              AFTER Rotation
    //          |                           |
    //          S++                         R-
    //         / \                         / \
    //       (h)  R         ==>          S+  (h+1)
    //           / \                     / \
    //        (h+1)(h+1)               (h) (h+1)
    //

    } else {

        PromoteNode( R );
        R->Balance = -a;
        return TRUE;
    }
}


/**
 * @brief Removes a specified node from the AVL tree while maintaining balance properties.
 * 
 * This function implements a sophisticated deletion strategy that minimizes tree
 * restructuring and preserves optimal search performance.
 * 
 * Deletion Strategy Overview:
 * 
 * Phase 1: Identify Deletion Target
 * - Simple Case: If NodeToDelete has ≤1 child, delete it directly
 * - Complex Case: If NodeToDelete has 2 children, find replacement node
 * 
 * Phase 2: Replacement Node Selection (for Complex Case)  
 * - Chooses predecessor or successor based on balance factors
 * - Optimizes selection to reduce rebalancing probability
 * - Selects from the taller subtree when possible
 * 
 * Phase 3: Node Removal
 * - Removes the identified target node from tree structure
 * - Updates parent-child relationships appropriately
 * 
 * Phase 4: Tree Rebalancing
 * - Traverses upward from deletion point
 * - Applies rotation operations to restore AVL properties
 * - Terminates early when balance is achieved
 * 
 * Phase 5: Node Replacement (for Complex Case)
 * - Substitutes replacement node data into original position
 * - Maintains all external references to NodeToDelete
 * - Preserves tree structure integrity
 * 
 * Performance Characteristics:
 * - Time Complexity: O(log n) for all cases
 * - Rebalancing: At most O(log n) rotations required
 * - Memory: No additional heap allocation needed
 * 
 * @param[in] Table Pointer to the AVL table containing the target node.
 *                  Must be properly initialized with valid tree structure
 * @param[in] NodeToDelete Pointer to the specific node to remove from the tree.
 *                         Must be a valid node currently present in the table.
 *                         Node content will be preserved but relocated if necessary
 * 
 * @note The function always succeeds for valid input parameters.
 *       Tree structure and balance properties are guaranteed upon completion.
 */
VOID
DeleteNodeFromTree (
    IN PRTL_AVL_TABLE Table,
    IN PRTL_BALANCED_LINKS NodeToDelete
    )

{
    PRTL_BALANCED_LINKS EasyDelete;
    PRTL_BALANCED_LINKS P;
    CHAR a;

    //
    //  Phase 1: Determine Deletion Strategy Based on Node Structure
    //  
    //  Simple Deletion Case: Node has ≤1 child (leaf or single-child node)
    //  When a node has at most one child, it can be removed directly from the tree
    //  without requiring a replacement node. This is the optimal case as it minimizes
    //  tree restructuring and preserves existing balance factors where possible.
    //  
    //  Node Categories:
    //  - Leaf Node: Both children are NULL -> Direct removal with parent link update
    //  - Single Child: One child is NULL -> Replace node with its non-NULL child
    //  
    //  This approach avoids the complexity of finding predecessor/successor nodes
    //  and reduces the probability of triggering extensive rebalancing operations.
    //

    if ((NodeToDelete->LeftChild == NULL) || (NodeToDelete->RightChild == NULL)) {

        EasyDelete = NodeToDelete;

    //
    //  Phase 2: Complex Deletion Case - Node Has Two Children
    //  
    //  When the target node has both left and right children, direct deletion would
    //  violate binary search tree properties. Instead, we find a replacement node
    //  (predecessor or successor) that can be safely removed and substituted.
    //  
    //  Replacement Selection Strategy (Performance Optimization):
    //  The algorithm chooses the replacement from the TALLER subtree to minimize
    //  rebalancing probability. This heuristic reduces the likelihood of triggering
    //  cascading rotations up the tree after deletion.
    //  
    //  Decision Logic:
    //  - Balance ≥ 0: Right subtree is taller/equal -> Select in-order successor
    //  - Balance < 0:  Left subtree is taller -> Select in-order predecessor
    //  
    //  Removing a node from the taller subtree brings the tree closer to perfect
    //  balance, while removing from the shorter subtree increases imbalance and
    //  necessitates more extensive rebalancing operations.
    //

    } else if (NodeToDelete->Balance >= 0) {

        //
        //  Successor Selection: Find In-Order Successor from Right Subtree
        //  
        //  When balance ≥ 0 (right subtree is taller or equal), we select the
        //  in-order successor as the replacement node. The successor is the
        //  smallest value in the right subtree, found by traversing right once
        //  then following left pointers to the minimum node.
        //  
        //  Algorithm: Start at right child, then go left until reaching a leaf
        //  
        //  Visual Example:
        //                 NodeToDelete (50)
        //                    /         \
        //                 (30)       (70) ← Right subtree
        //                           /    \
        //                        (60)   (80)
        //                        /
        //                    S(55) ← SUCCESSOR: Leftmost in right subtree
        //  
        //  Result: Node 55 becomes EasyDelete (replacement candidate)
        //

        EasyDelete = NodeToDelete->RightChild;
        while (EasyDelete->LeftChild != NULL) {
            EasyDelete = EasyDelete->LeftChild;
        }
    } else {

        //
        //  Predecessor Selection: Find In-Order Predecessor from Left Subtree
        //  
        //  When balance < 0 (left subtree is taller), we select the in-order
        //  predecessor as the replacement node. The predecessor is the largest
        //  value in the left subtree, found by traversing left once then
        //  following right pointers to the maximum node.
        //  
        //  Algorithm: Start at left child, then go right until reaching a leaf
        //  
        //  Visual Example:
        //                 NodeToDelete (50)
        //                    /         \
        //                 (30) ← Left subtree   (70)
        //                /    \
        //             (20)   (40)
        //                      \
        //                   P(45) ← PREDECESSOR: Rightmost in left subtree
        //  
        //  Result: Node 45 becomes EasyDelete (replacement candidate)
        //

        EasyDelete = NodeToDelete->LeftChild;
        while (EasyDelete->RightChild != NULL) {
            EasyDelete = EasyDelete->RightChild;
        }
    }

    //
    //  Balance Factor Direction Tracking for Rebalancing Phase:
    //  
    //  The rebalancing algorithm requires knowing which subtree (left or right)
    //  had a node removed to properly adjust balance factors during upward traversal.
    //  
    //  Variable 'a' encodes the deletion direction:
    //  - Left deletion (a = -1): Removed node was left child, subtree got shorter
    //  - Right deletion (a = +1): Removed node was right child, subtree got shorter
    //  
    //  Default assumption: Start with left-side deletion (a = -1)
    //  This will be corrected to right-side deletion (a = +1) if needed below
    //

    a = -1;

    //
    //  Node Removal - Case 1: No Left Child Present
    //  
    //  Handle nodes that have at most a right child (includes leaf nodes).
    //  This is the simpler deletion case where we can directly replace the
    //  node with its right subtree (which may be NULL for leaf nodes).
    //  
    //  Process: Replace EasyDelete with its right child in the tree structure
    //  Side Effect: Updates deletion direction tracking variable 'a' if needed
    //

    if (EasyDelete->LeftChild == NULL) {

        if (RtlIsLeftChild(EasyDelete)) {
            EasyDelete->Parent->LeftChild = EasyDelete->RightChild;
        } else {
            EasyDelete->Parent->RightChild = EasyDelete->RightChild;
            a = 1;
        }

        if (EasyDelete->RightChild != NULL) {
            EasyDelete->RightChild->Parent = EasyDelete->Parent;
        }

    //
    //  Node Removal - Case 2: No Right Child Present (Left Child Guaranteed)
    //  
    //  Handle nodes that have only a left child (the complementary case to Case 1).
    //  Since we've already handled the "no left child" case above, we know that if
    //  we reach this point, the node must have exactly one child on the left side.
    //  
    //  Process: Replace EasyDelete with its left child in the tree structure
    //  Side Effect: Updates deletion direction tracking variable 'a' if needed
    //

    } else {

        if (RtlIsLeftChild(EasyDelete)) {
            EasyDelete->Parent->LeftChild = EasyDelete->LeftChild;
        } else {
            EasyDelete->Parent->RightChild = EasyDelete->LeftChild;
            a = 1;
        }

        EasyDelete->LeftChild->Parent = EasyDelete->Parent;
    }

    //
    //  Rebalancing Loop Initialization and Sentinel Root Configuration:
    //  
    //  Prepare for the upward rebalancing traversal by configuring the sentinel root
    //  to serve as a reliable termination condition. Setting the root balance to 0
    //  enables two critical functions:
    //  
    //  1. Loop Termination: Provides a predictable stopping point for the rebalancing
    //     algorithm without requiring special boundary case handling throughout the loop
    //  
    //  2. Tree Depth Detection: When the algorithm reaches the root with a non-zero
    //     balance factor, it indicates that the deletion reduced the overall tree height
    //     by one level (detected via Table->BalancedRoot.Balance != 0 check)
    //  
    //  This initialization simplifies the core rebalancing logic and eliminates the
    //  need for complex root-specific termination tests within the main loop.
    //

    Table->BalancedRoot.Balance = 0;
    P = EasyDelete->Parent;

    //
    //  Upward Rebalancing Traversal Loop:
    //  
    //  Starting from the deletion point, traverse upward through parent nodes
    //  adjusting balance factors and performing rotations as needed to restore
    //  AVL tree properties. The loop continues until either:
    //  
    //  1. A subtree height remains unchanged (termination via balance factor analysis)
    //  2. The sentinel root is reached (guaranteed termination via initialization above)
    //  3. A rotation fully restores balance (early termination from RebalanceNode)
    //  
    //  Each iteration processes one node in the deletion path, propagating height
    //  changes upward and applying corrective rotations when AVL violations are detected.
    //

    while (TRUE) {

        //
        //  Case 1: Deletion Improved Balance - Subtree Height Decreased
        //  
        //  When P->Balance equals the deletion direction 'a', it indicates that
        //  the deletion removed a node from P's previously heavier subtree,
        //  improving the node's balance. This case represents optimal deletion
        //  where the tree becomes more balanced rather than requiring corrections.
        //  
        //  Actions: Reset balance to 0 (perfect balance achieved), then continue
        //  upward traversal since subtree height decreased and may affect ancestors.
        //

        if (P->Balance == a) {

            P->Balance = 0;

        //
        //  Case 2: Deletion from Balanced Node - Subtree Height Unchanged
        //  
        //  When P->Balance equals 0 (node was perfectly balanced), the deletion
        //  occurred from one of two equal-height subtrees. This creates a new
        //  imbalance of ±1 but crucially does NOT change the overall subtree height.
        //  
        //  Condition: P->Balance == 0 (node was balanced before deletion)
        //  Action: Set P->Balance = -a (create slight imbalance in opposite direction)
        //  Termination: Height preservation allows rebalancing algorithm to stop
        //  
        //  Special Case Detection: If P is the sentinel root (BalancedRoot.Balance != 0),
        //  it indicates the deletion reduced the overall tree depth by one level.
        //

        } else if (P->Balance == 0) {

            P->Balance = -a;

            //
            //  Tree Depth Reduction Detection and Adjustment:
            //  
            //  When the rebalancing traversal reaches the sentinel root with a non-zero
            //  balance factor, it indicates that the deletion operation has reduced the
            //  overall tree height by exactly one level. This occurs when:
            //  
            //  1. The deletion propagated height changes all the way to the root
            //  2. The root's subtree became shorter due to the deletion
            //  3. No intermediate rebalancing operations restored the original height
            //  
            //  Detection Mechanism: BalancedRoot.Balance != 0 (set during initialization)
            //  Action Required: Decrement Table->DepthOfTree to reflect new tree height
            //

            if (Table->BalancedRoot.Balance != 0) {
                Table->DepthOfTree -= 1;
            }

            break;

        //
        //  Case 3: Critical Imbalance Requires Rebalancing - Subtree Height Decreased
        //  
        //  When P->Balance is neither 'a' nor 0, it means the node has become critically
        //  unbalanced with a balance factor of ±2. This occurs when deletion removed a
        //  node from the shorter subtree, making the height difference exceed AVL limits.
        //  
        //  Rebalancing Process:
        //  1. Call RebalanceNode(P) to perform necessary rotations (single or double)
        //  2. RebalanceNode returns TRUE if Case 3 (height preservation) was encountered
        //  3. If Case 3: Subtree height unchanged after rotation → Terminate early
        //  4. If Cases 1-2: Subtree height decreased → Continue upward traversal
        //  
        //  Parent Pointer Adjustment:
        //  After rotation, the original node P may have moved in the tree hierarchy.
        //  The new parent to examine becomes P->Parent (P's original parent, now grandparent).
        //  This ensures the algorithm continues from the correct position in the tree.
        //

        } else {

            //
            //  Early Termination Check: Height-Preserving Rotation Detected
            //  
            //  RebalanceNode returns TRUE specifically when Case 3 (delete-only rotation)
            //  was encountered during the rebalancing operation. Case 3 occurs when the
            //  child node being promoted has a balance factor of 0, which results in a
            //  unique outcome: the subtree height remains unchanged after rotation.
            //  
            //  Termination Condition: When subtree height is preserved, no further height
            //  changes will propagate upward in the tree. This means all ancestor nodes
            //  will maintain their current balance factors, and the rebalancing process
            //  can safely terminate at this point.
            //  
            //  Performance Optimization: Early termination prevents unnecessary traversal
            //  up the remaining path to the root, improving deletion efficiency when this
            //  height-preserving case is encountered.
            //

            if (RebalanceNode(P)) {
                break;
            }

            P = P->Parent;
        }

        a = -1;
        if (RtlIsRightChild(P)) {
            a = 1;
        }
        P = P->Parent;
    }

    //
    //  Phase 5: Node Data Substitution (Complex Deletion Case Only)
    //  
    //  This final phase applies only when we performed a complex deletion where
    //  a replacement node (predecessor or successor) was physically removed from
    //  the tree instead of the original target node. The replacement node's data
    //  must now be substituted into the original NodeToDelete position.
    //  
    //  Substitution Process:
    //  1. Copy all structural data from NodeToDelete into the replacement node (EasyDelete)
    //  2. Update parent pointer to reference the replacement node at NodeToDelete's location
    //  3. Update child pointers to reference the replacement node as their new parent
    //  4. Preserve all external references to NodeToDelete (caller's pointer remains valid)
    //  
    //  Critical Note: The original NodeToDelete structure maintains its memory location
    //  and user data content, but its tree links may have been modified during the
    //  rebalancing process. The replacement ensures all tree relationships are correctly
    //  established with the substitute node in the original position.
    //

    if (NodeToDelete != EasyDelete) {
        *EasyDelete = *NodeToDelete;
        if (RtlIsLeftChild(NodeToDelete)) {
            EasyDelete->Parent->LeftChild = EasyDelete;
        } else {
            ASSERT(RtlIsRightChild(NodeToDelete));
            EasyDelete->Parent->RightChild = EasyDelete;
        }
        if (EasyDelete->LeftChild != NULL) {
            EasyDelete->LeftChild->Parent = EasyDelete;
        }
        if (EasyDelete->RightChild != NULL) {
            EasyDelete->RightChild->Parent = EasyDelete;
        }
    }
}


/**
 * @brief Locates the in-order successor of a specified node in the binary search tree.
 * 
 * The successor is defined as the next node in sorted order when traversing 
 * the tree using in-order traversal (left-root-right).
 * 
 * Algorithm Implementation:
 * 
 * Case 1: Node has right subtree
 *         -> Successor is the leftmost node in the right subtree
 *         -> Traverse right once, then left until leaf is reached
 * 
 * Case 2: Node has no right subtree
 *         -> Successor is the first ancestor where current node is in left subtree
 *         -> Traverse upward until finding an ancestor accessed via left child
 * 
 * Case 3: No successor exists
 *         -> Current node is the maximum value in the entire tree
 *         -> Return NULL to indicate end of traversal
 * 
 * Primary Use Cases:
 * - Tree enumeration and iteration operations
 * - Node deletion when replacement node is needed
 * - Range queries and ordered traversal algorithms
 * - Safe enumeration during concurrent modifications
 * 
 * @param[in] Links Pointer to the balanced tree node for which to find the successor.
 *                  Must be a valid node within an AVL tree structure.
 *                  NULL input is handled gracefully and returns NULL
 * 
 * @return Pointer to the successor node in sorted order.
 *         Returns NULL if no successor exists (node is maximum) or if input Links parameter is NULL
 */
PRTL_BALANCED_LINKS
RealSuccessor (
    IN PRTL_BALANCED_LINKS Links
    )

{
    PRTL_BALANCED_LINKS Ptr;

    // Check for NULL input
    if (Links == NULL) {
        return NULL;
    }

    //
    //  Case 1: Right Subtree Exists - Find Leftmost Descendant
    //  
    //  When the target node has a right subtree, the successor is guaranteed to be
    //  the leftmost (minimum) node within that right subtree. This follows from
    //  binary search tree properties where all nodes in the right subtree have
    //  values greater than the current node.
    //  
    //  Algorithm: Navigate right once, then traverse left until reaching a leaf
    //  
    //  Visual Example:
    //  
    //                    Links (50)
    //                       \
    //                     (70) ← Right subtree root
    //                      /  \
    //                   (60)  (80)
    //                   /  \
    //                (55) (65)
    //               /
    //            S(52) ← SUCCESSOR: Leftmost node in right subtree
    //              \
    //             (53)
    //  
    //  Result: S becomes the successor as it's the next value in sorted order.
    //

    if ((Ptr = Links->RightChild) != NULL) {

        while (Ptr->LeftChild != NULL) {
            Ptr = Ptr->LeftChild;
        }

        return Ptr;

    }

    //
    //  Case 2: No Right Subtree - Find Ancestor Where Node Is Left Descendant
    //  
    //  When the target node has no right subtree, we must traverse upward through
    //  parent pointers to locate the successor. The successor is the first ancestor
    //  where the current traversal path originated from the left subtree.
    //  
    //  Algorithm: Move up the tree until finding an ancestor reached via left child
    //  
    //  Visual Example:
    //  
    //                  S (50) ← SUCCESSOR: First ancestor reached via left path
    //                    /  \
    //                 (30)  (60)
    //                 /  \
    //              (20)  (40)
    //                      \
    //                   Links (45) ← Starting node (no right subtree)
    //  
    //  Traversal Path: Links(45) → (40) → (30) → S(50)
    //  - From 45 to 40: 45 is RIGHT child of 40 (continue up)
    //  - From 40 to 30: 40 is RIGHT child of 30 (continue up)  
    //  - From 30 to 50: 30 is LEFT child of 50 (FOUND!)
    //  
    //  Result: S(50) is the successor as it's the next larger value after 45
    //  
    //  Implementation Notes:
    //  - Depends on BalancedRoot sentinel structure for proper termination
    //  - BalancedRoot.Parent points to self (termination condition)
    //  - BalancedRoot.RightChild points to actual tree root
    //  - BalancedRoot.LeftChild does NOT point to self (asymmetric design)
    //

    Ptr = Links;
    while (RtlIsRightChild(Ptr)) {
        Ptr = Ptr->Parent;
    }

    if (RtlIsLeftChild(Ptr)) {
        return Ptr->Parent;
    }

    //
    //  Case 3: No Successor Exists - Maximum Node Reached
    //  
    //  When the upward traversal reaches the sentinel root without finding an ancestor
    //  accessed via left child, it indicates that the original node is the maximum
    //  value in the entire tree. No successor exists in this case.
    //  
    //  Termination Condition: The BalancedRoot sentinel has Parent pointing to itself
    //  (self-referential structure), which serves as the traversal termination marker.
    //  This is detected when Ptr->Parent == Ptr, confirming we've reached the root.
    //  
    //  Return Value: NULL indicates end of ordered traversal (no next element exists)
    //

    ASSERT(Ptr->Parent == Ptr);

    return NULL;
}


/**
 * @brief Finds the predecessor of a given node in the binary search tree.
 * 
 * The RealPredecessor function finds the predecessor of a given node in the binary search tree - 
 * that is, the node that comes immediately before the given node in sorted order.
 * 
 * Key Use Cases:
 * 1. Node Deletion Strategy - The most critical use is during node deletion. When deleting a node 
 *    that has two children, the algorithm needs to find a replacement node. The predecessor is one 
 *    of the two possible replacements (the other being the successor).
 * 
 * 2. Safe Enumeration During Deletion - When a node is deleted during enumeration, the enumeration 
 *    state needs to be updated safely.
 * 
 * @param[in] Links Pointer to the balanced tree node for which to find the predecessor.
 *                  Must be a valid node within an AVL tree structure.
 *                  NULL input is handled gracefully and returns NULL
 * 
 * @return Pointer to the predecessor node in sorted order.
 *         Returns NULL if no predecessor exists or if input Links parameter is NULL
 */
PRTL_BALANCED_LINKS
RealPredecessor (
    IN PRTL_BALANCED_LINKS Links
    )

{
    PRTL_BALANCED_LINKS Ptr;

    if (Links == NULL) {
        return NULL;
    }

    if ((Ptr = Links->LeftChild) != NULL) {

        while (Ptr->RightChild != NULL) {
            Ptr = Ptr->RightChild;
        }

        return Ptr;
    }

    /*
      Without a left subtree, we must traverse upward through parent nodes
      to locate the predecessor. The algorithm searches for the first ancestor
      where the current node appears in its RIGHT subtree. Visually:

                       P <- Target predecessor found here
                        \
                         . <- Traversal path upward
                        .
                       .
                    Links <- Starting point (must be RIGHT descendant of P)

        Implementation Note: This algorithm relies on the sentinel root structure
        where BalancedRoot.Parent references itself, and BalancedRoot.RightChild
        contains the actual tree root, ensuring proper termination conditions.
    */

    Ptr = Links;
    while (RtlIsLeftChild(Ptr)) {
        Ptr = Ptr->Parent;
    }

    if (RtlIsRightChild(Ptr) && (Ptr->Parent->Parent != Ptr->Parent)) {
        return Ptr->Parent;
    }

    //
    //  otherwise we are do not have a real predecessor so we simply return
    //  NULL
    //

    return NULL;

}


/**
 * @brief Establishes a fully functional AVL table by initializing all required data structures.
 * 
 * This function transforms an uninitialized table structure into a ready-to-use balanced 
 * binary search tree with complete operational capabilities by registering user-provided 
 * callback routines and preparing the table for subsequent element operations.
 * 
 * 
 * @param[in,out] Table Pointer to the uninitialized AVL table structure requiring setup.
 *                      Must point to valid memory with sufficient space for RTL_AVL_TABLE structure.
 *                      The memory contents are completely overwritten during initialization, so any
 *                      existing data will be lost. After successful initialization, this structure
 *                      becomes the primary handle for all subsequent table operations and must
 *                      remain valid throughout the table's operational lifetime
 * 
 * @param[in] CompareRoutine User-defined element comparison function for tree ordering operations.
 *                           Must implement three-way comparison semantics returning negative values
 *                           for less-than relationships, zero for equality, and positive values for
 *                           greater-than relationships. This routine determines element positioning
 *                           within the tree and must provide consistent, transitive ordering for
 *                           proper tree operation. The function will be called frequently during
 *                           search, insertion, and deletion operations, so performance is critical
 * 
 * @param[in] AllocateRoutine User-defined memory allocation function for dynamic node creation.
 *                            Must return properly aligned memory of the requested size or NULL
 *                            to indicate allocation failure. The allocated memory will be used to
 *                            store both internal tree management structures and user element data.
 *                            Applications can implement custom allocation strategies including
 *                            memory pools, debugging allocators, or specialized heap management
 *                            through this interface
 * 
 * @param[in] FreeRoutine User-defined memory deallocation function for node cleanup operations.
 *                        Must properly release memory previously allocated by AllocateRoutine.
 *                        The function receives pointers to complete node structures including
 *                        both internal tree links and user data areas. Proper implementation
 *                        is essential for preventing memory leaks during element deletion and
 *                        table destruction operations
 * 
 * @param[in] TableContext Application-specific context data passed to callback routines.
 *                         This opaque pointer is stored within the table structure and provided
 *                         to comparison, allocation, and deallocation functions during their
 *                         invocation. Applications can use this mechanism to pass configuration
 *                         data, state information, or object references required by callback
 *                         implementations. May be NULL if no context is required
 * 
 * @note This function always succeeds for valid input parameters.
 *       The table structure is guaranteed to be fully initialized and ready for element
 *       operations upon completion. Invalid parameters (such as NULL function pointers)
 *       may cause undefined behavior during subsequent table operations rather than
 *       immediate failure during initialization.
 */
VOID
RtlInitializeGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    IN PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
    IN PRTL_AVL_FREE_ROUTINE FreeRoutine,
    IN PVOID TableContext
    )

{
    RtlZeroMemory( Table, sizeof(RTL_AVL_TABLE) );
    
    //  The actual tree root will be stored as BalancedRoot.RightChild when the
    //  first element is inserted.

    Table->BalancedRoot.Parent = &Table->BalancedRoot;

    Table->CompareRoutine = CompareRoutine;
    Table->AllocateRoutine = AllocateRoutine;
    Table->FreeRoutine = FreeRoutine;
    Table->TableContext = TableContext;

}


/**
 * @brief Simplified element insertion with streamlined interface for standard table operations.
 * 
 * Provides a convenient, high-level interface for inserting elements into AVL tables
 * without requiring pre-computed search context. This function automatically performs
 * element lookup, handles duplicate detection, and delegates to the comprehensive
 * insertion implementation for complete table management.
 * 
 * Operation Overview:
 * 
 * Phase 1: Automated Element Search and Context Generation
 * • Executes FindNodeOrParent to locate insertion point or existing element
 * • Generates complete search context including position and relationship information
 * 
 * Phase 2: Delegation to Advanced Insertion Implementation
 * • Passes all parameters plus generated search context to RtlInsertElementGenericTableFullAvl
 * • Leverages sophisticated insertion logic with pre-computed positioning information
 * • Maintains full functionality including balance restoration and state management
 * 
 * 
 * @param[in] Table Pointer to the target AVL table structure for element insertion.
 *                  Must be a properly initialized table with registered comparison, allocation,
 *                  and deallocation routines. The table will be modified to include the new
 *                  element and updated with current metadata including element counts and
 *                  tree depth information. Supports both empty and populated table states
 * 
 * @param[in] Buffer Complete element data buffer containing both key and value information.
 *                   Serves dual purposes: provides comparison data for element positioning and
 *                   supplies content for storage in the newly created table entry. The buffer
 *                   contents are copied into allocated table memory, ensuring data persistence
 *                   independent of the original buffer lifecycle. Format and interpretation
 *                   are application-specific and must align with comparison routine expectations
 * 
 * @param[in] BufferSize Size in bytes of the user data portion for the new table element.
 *                       Specifies the amount of memory to allocate for element storage beyond
 *                       the internal tree management structures (RTL_BALANCED_LINKS). The actual
 *                       allocation includes both this user data space and the required balanced
 *                       tree link overhead. Applications must avoid accessing or modifying the
 *                       initial sizeof(RTL_BALANCED_LINKS) bytes of allocated memory, as this
 *                       space is reserved for internal tree operations. Must be > 0 for valid operation
 * 
 * @param[out] NewElement Optional output parameter reporting insertion operation outcome.
 *                        When provided, receives TRUE if a new element was successfully created
 *                        and added to the table, or FALSE if an existing element with matching
 *                        key was located and returned. This flag enables applications to distinguish
 *                        between insertion operations that create new table entries and lookup
 *                        operations that access existing data. May be NULL if status information
 *                        is not required by the calling application
 * 
 * @return Element access pointer with context-dependent semantics:
 *         - Successful Insertion (Non-NULL): Points directly to the user data portion of the newly created table element.
 *           Contains a copy of the provided Buffer contents and remains valid until explicit element deletion.
 *           The returned pointer can be used for immediate data access and modification within the table context.
 *         - Existing Element Access (Non-NULL): Points to the user data portion of a previously stored element with matching
 *           key. Provides direct access to existing table data without modification to table structure or element counts.
 *           Useful for update-or-insert operation patterns.
 *         - Allocation Failure (NULL): Indicates insufficient memory for new element creation. The table remains
 *           completely unmodified with no structural changes, counter updates, or side effects. Provides atomic failure
 *           semantics for robust error handling in resource-constrained environments.
 */
PVOID
RtlInsertElementGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer,
    IN CLONG BufferSize,
    OUT PBOOLEAN NewElement OPTIONAL
    )

{

    //
    //  Local Variables: Search Context Storage for Insertion Optimization
    //  
    //  These variables capture the results of the initial tree search operation,
    //  providing the necessary context for efficient element insertion without
    //  requiring redundant tree traversal in the subsequent insertion phase.
    //

    //
    //  Search Position Context: Located Element or Insertion Parent
    //  
    //  This variable will receive either the existing element node (if a matching
    //  key is found) or the parent node where insertion should occur (if no match
    //  exists). This dual-purpose result eliminates the need for separate search
    //  operations during the insertion process.
    //
    PRTL_BALANCED_LINKS NodeOrParent;

    //
    //  Search Operation Outcome: Element Existence and Positioning Information
    //  
    //  Captures the precise result of the tree search, indicating whether an element
    //  was found or providing specific positioning context for new element insertion.
    //  This information guides the subsequent insertion logic and optimization strategies.
    //
    TABLE_SEARCH_RESULT Lookup;

    //
    //  Phase 1: Element Search and Context Generation
    //  
    //  Execute comprehensive tree search to determine element existence and generate
    //  positioning context for potential insertion. This search provides both the
    //  element location (if found) and the optimal insertion point (if not found),
    //  eliminating redundant traversal in subsequent operations.
    //  
    //  FindNodeOrParent Operations:
    //  • Performs binary search traversal using registered comparison routine
    //  • Returns exact element location for existing keys
    //  • Identifies parent node and insertion direction for new elements
    //  • Handles empty table scenarios with appropriate context generation
    //

    Lookup = FindNodeOrParent(
                 Table,
                 Buffer,
                 &NodeOrParent
                 );

    //
    //  Phase 2: Delegation to Advanced Insertion Implementation
    //  
    //  Forward all insertion parameters plus the generated search context to the
    //  comprehensive insertion function. This delegation pattern provides the full
    //  functionality of the advanced implementation while maintaining a simplified
    //  interface for standard insertion operations.
    //  
    //  RtlInsertElementGenericTableFullAvl Operations:
    //  • Processes search results for immediate insertion or existing element access
    //  • Handles memory allocation, tree balancing, and state management
    //  • Returns direct pointer to user data within the table structure
    //

    return RtlInsertElementGenericTableFullAvl(
                Table,
                Buffer,
                BufferSize,
                NewElement,
                NodeOrParent,
                Lookup
                );
}


PVOID
RtlInsertElementGenericTableFullAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer,
    IN CLONG BufferSize,
    OUT PBOOLEAN NewElement OPTIONAL,
    IN PVOID NodeOrParent,
    IN TABLE_SEARCH_RESULT SearchResult
    )

/**
 * @brief Advanced element insertion with optimized insertion using pre-computed search context.
 * 
 * Performs sophisticated element insertion into the AVL table using pre-computed
 * search results from a previous lookup operation. This function combines insertion
 * logic with comprehensive tree balancing to maintain optimal search performance
 * while supporting both new element creation and duplicate element detection.
 * 
 * Insertion Process Overview:
 * 
 * Phase 1: Search Result Validation and Element Existence Check
 * 
 * Phase 2: Dynamic Memory Allocation and Node Construction
 * 
 * Phase 3: Tree Structure Integration and Positioning
 * 
 * Phase 4: AVL Balance Restoration and Tree Optimization
 * 
 * Phase 5: Data Integration and State Finalization
 * 
 * Optimization Features:
 * 
 * Search Context Reuse:
 * Accepts NodeOrParent and SearchResult parameters from previous lookup operations,
 * eliminating redundant tree traversal and providing significant performance gains
 * for insert-or-find operation patterns.
 * 
 * 
 * Balance Factor Efficiency:
 * Implements Knuth's A6-A10 algorithm steps for minimal rebalancing overhead,
 * with early termination when tree balance is naturally maintained.
 * 
 * 
 * @param[in] Table Pointer to the AVL table structure where the element will be inserted.
 *                  Must be a properly initialized table with valid comparison, allocation,
 *                  and deallocation routines. The table structure will be modified to
 *                  include the new element and updated metadata including element counts
 *                  and tree depth information
 * @param[in] Buffer Element key and data buffer used for both comparison and data storage.

 * @param[in] BufferSize Size in bytes of the user data portion to allocate for the new element.
 *                       The actual memory allocation will include additional space for internal
 *                       balanced tree link structures (RTL_BALANCED_LINKS). 
 * @param[out] NewElement Optional output parameter indicating element insertion status.
 *                        If provided, receives TRUE when a new element was successfully created
 *                        and inserted into the table, or FALSE when an existing element with
 *                        the same key was found. This parameter allows applications to distinguish
 *                        between insert operations and find operations on existing elements.
 *                        May be NULL if insertion status information is not required
 * @param[in] NodeOrParent Pre-computed search context from RtlLookupElementGenericTableFullAvl.
 *                         Contains either the existing element node (if found) or the parent node
 *                         where insertion should occur (if not found). 
 * @param[in] SearchResult Pre-computed search outcome from RtlLookupElementGenericTableFullAvl.
 *                         Specifies the exact insertion context: TableFoundNode (element exists),
 *                         TableInsertAsLeft (insert as left child), TableInsertAsRight (insert
 *                         as right child), or TableEmptyTree (first element insertion). 
 * 
 * @return Element data pointer with the following semantics:
 *         Success (Non-NULL): Points directly to the user data portion of the element within the table.
 *         For new insertions, this points to a copy of the provided Buffer contents.
 *         For existing elements, this points to the previously stored data. The returned
 *         pointer remains valid until the element is deleted from the table and can be
 *         used for direct data access and modification.
 *         Failure (NULL): Indicates memory allocation failure during new element creation. The table
 *         structure remains unmodified - no element insertion, counter updates, or
 *         tree modifications occur.
 */

{
    //  This variable will hold the pointer to the complete node structure that
    //  will be returned to the caller. It represents either a newly allocated
    //  and inserted node or an existing node found during the search operation.
    //

    PTABLE_ENTRY_HEADER NewNode;

    if (SearchResult != TableFoundNode) {

        //
        //  Element Count Validation: Prevent Integer Overflow in Table Size
        //  
        //  Ensure the table size remains within valid bounds before attempting
        //  new element insertion. 

        ASSERT(Table->NumberGenericTableElements != (MAXULONG-1));


        //  Memory Layout: [RTL_BALANCED_LINKS][User Data Area]
        //  
        //  The user-defined allocation routine handles the actual memory management,
        //  allowing applications to implement custom allocation strategies such as
        //  memory pools, debugging allocators, or specialized heap management.
        //

        NewNode = Table->AllocateRoutine(
                           Table,
                           BufferSize+FIELD_OFFSET( TABLE_ENTRY_HEADER, UserData )
                           );

        //  When memory allocation fails, ensure complete rollback with no side effects.
        //  This provides atomic insertion semantics where either the operation fully
        //  succeeds or leaves the table in its original state. The NewElement flag
        //  is set to FALSE to indicate no insertion occurred, maintaining consistent
        //  API semantics even during failure conditions.
        //

        if (!NewNode) {

            if (!NewElement) {

                *NewElement = FALSE;
            }

            return NULL;
        }

        //
        //  Table Metadata Update: Element Count Increment
        //  
        //  Update the table's element counter to reflect the addition of the new node.
        //  This counter is used for table size queries, validation checks, and
        //  performance monitoring. The increment occurs after successful allocation
        //  but before potential tree insertion failures to maintain accurate state.
        //

        Table->NumberGenericTableElements++;
        //
        //  Node Structure Initialization: Clean Balanced Tree Link Setup
        //  
        //  Initialize the balanced tree link portion of the newly allocated node
        //  to ensure predictable starting state. This zero-initialization prevents
        //  random memory content from affecting tree operations and provides a
        //  clean foundation for subsequent parent-child relationship establishment.
        //

        RtlZeroMemory(NewNode, sizeof(RTL_BALANCED_LINKS) );

        //
        //  Phase 3: Tree Structure Integration Based on Search Context
        //  
        //  Insert the newly allocated node into the tree structure using the precise
        //  positioning information provided by the SearchResult parameter. This phase
        //  handles different insertion scenarios including empty tree initialization
        //  and positioned insertion relative to existing nodes.
        //

        if (SearchResult == TableEmptyTree) {

            //
            //  Empty Tree Initialization: First Element Insertion
            //  
            //  When inserting into an empty table, establish the new node as the root
            //  element by connecting it directly to the sentinel BalancedRoot structure.
            //  This initialization creates the foundation for all subsequent tree
            //  operations and sets the initial tree depth to 1.
            //  
            //  Tree Structure After First Insertion:
            //  
            //      BalancedRoot (Sentinel)
            //             |
            //             | (RightChild)
            //             ↓
            //         NewNode (Root)
            //        (Parent points to BalancedRoot)
            //

            Table->BalancedRoot.RightChild = &NewNode->BalancedLinks;
            NewNode->BalancedLinks.Parent = &Table->BalancedRoot;
            ASSERT(Table->DepthOfTree == 0);
            Table->DepthOfTree = 1;

        } else {

            //
            //  Positioned Insertion: Parent-Child Relationship Establishment
            //  
            //  Insert the new node at the specific position determined by the previous
            //  search operation. The SearchResult indicates whether the new node should
            //  become the left or right child of the identified parent node.
            //  
            //  This positioning maintains binary search tree ordering properties while
            //  preparing for subsequent balance factor adjustments and potential rotations.
            //

            PRTL_BALANCED_LINKS R = &NewNode->BalancedLinks;
            PRTL_BALANCED_LINKS S = (PRTL_BALANCED_LINKS)NodeOrParent;

            if (SearchResult == TableInsertAsLeft) {

                //
                //  Left Child Insertion: New Node Becomes Left Subtree
                //  
                //  Position the new node as the left child of the identified parent.
                //  This placement indicates that the new element's key value is less
                //  than the parent's key value according to the comparison routine.
                //

                ((PRTL_BALANCED_LINKS)NodeOrParent)->LeftChild = &NewNode->BalancedLinks;

            } else {

                //
                //  Right Child Insertion: New Node Becomes Right Subtree
                //  
                //  Position the new node as the right child of the identified parent.
                //  This placement indicates that the new element's key value is greater
                //  than the parent's key value according to the comparison routine.
                //

                ((PRTL_BALANCED_LINKS)NodeOrParent)->RightChild = &NewNode->BalancedLinks;
            }

            //
            //  Bidirectional Link Completion: Parent Reference Establishment
            //  
            //  Complete the parent-child relationship by setting the new node's parent
            //  pointer to reference the identified parent node. This bidirectional
            //  linking enables efficient tree traversal in both directions and supports
            //  the upward traversal required for balance factor adjustment.
            //

            NewNode->BalancedLinks.Parent = NodeOrParent;

            //
            //  Phase 4: AVL Balance Restoration
            //  
            //  The basic binary tree insertion is now complete (corresponding to Knuth's
            //  algorithm steps A1-A5). The following section implements the critical
            //  balance factor adjustment and rotation operations (steps A6-A10) required
            //  to maintain AVL tree properties and ensure optimal search performance.
            //  
            //  AVL Property: The height difference between left and right subtrees of
            //  any node must not exceed 1. Violations require corrective rotations.
            //

            //
            //  Sentinel Root Balance Configuration: Loop Control Optimization
            //  
            //  Set the sentinel root's balance factor to a distinctive value (-1) that
            //  simplifies the upward traversal termination logic. This initialization
            //  enables detection of tree height changes and provides a reliable boundary
            //  condition for the balance adjustment algorithm.
            //

            Table->BalancedRoot.Balance = -1;

            //
            //  Balance Factor Adjustment Loop: Upward Propagation of Height Changes
            //  
            //  Starting from the newly inserted node, traverse upward through parent
            //  nodes adjusting balance factors to reflect the tree modification. This
            //  loop implements Knuth's steps A6-A10, handling three distinct cases:
            //  
            //  Case 1: Previously balanced nodes become unbalanced (continue upward)
            //  Case 2: Previously unbalanced nodes become balanced (terminate)
            //  Case 3: Critical imbalance requiring rotation (rebalance and terminate)
            //

            while (TRUE) {

                //  Variable 'a' encodes the direction of balance factor adjustment:
                //  • +1: Right subtree grew taller (right-heavy adjustment needed)
                //  • -1: Left subtree grew taller (left-heavy adjustment needed)
                //  
                //  The direction is determined by examining whether the current node R
                //  is a left child or right child of its parent node S.
                //

                CHAR a;

                //
                //  Direction Calculation: Determine Balance Factor Adjustment Direction
                //  
                //  Calculate the appropriate balance factor adjustment based on the
                //  structural relationship between the current node and its parent.
                //  This direction determines how the parent's balance factor should
                //  be modified to reflect the insertion in this subtree.
                //

                a = 1;
                if (RtlIsLeftChild(R)) {
                    a = -1;
                }

                //
                //  Case 1: Previously Balanced Node - Continue Upward Propagation
                //  
                //  When the parent node S was previously balanced (Balance == 0), the
                //  insertion has made it unbalanced but not critically so. Update the
                //  balance factor and continue upward since this height change may affect
                //  ancestors. This corresponds to Knuth's step A6.
                //  
                //  Tree height has increased in this subtree, requiring continued
                //  propagation of the height change up the tree.
                //

                if (S->Balance == 0) {

                    S->Balance = a;
                    R = S;
                    S = S->Parent;

                //
                //  Case 2: Height Change Neutralized - Terminate Successfully
                //  
                //  When the parent node S has an opposite balance factor from the adjustment
                //  direction, the insertion has actually improved the tree balance by filling
                //  in a shorter subtree. The height change is neutralized, and no further
                //  propagation is needed. This corresponds to Knuth's step A7.ii.
                //  
                //  Special Case: If we've reached the sentinel root (indicated by non-zero
                //  BalancedRoot.Balance), the overall tree depth has increased by one level.
                //

                } else if (S->Balance != a) {

                    //
                    //  Balance Neutralization: Height Stabilization Achieved
                    //  
                    //  The insertion has filled a previously shorter subtree, bringing
                    //  the node into perfect balance. No further height changes will
                    //  propagate upward from this point, allowing early termination.
                    //

                    S->Balance = 0;

                    //
                    //  Tree Depth Increase Detection: Overall Height Growth
                    //  
                    //  When the balance adjustment reaches the sentinel root and finds
                    //  it unbalanced, this indicates that the insertion has increased
                    //  the overall tree height by exactly one level. Update the depth
                    //  counter to maintain accurate tree metrics.
                    //

                    if (Table->BalancedRoot.Balance == 0) {
                        Table->DepthOfTree += 1;
                    }

                    break;

                //
                //  Case 3: Critical Imbalance - Rebalancing Required
                //  
                //  When the parent node S has the same balance factor as the adjustment
                //  direction, a critical imbalance (±2) has been created that violates
                //  AVL constraints. Execute rebalancing operations to restore tree
                //  properties. This corresponds to Knuth's steps A7.iii, A8, and A9.
                //  
                //  The RebalanceNode function performs the necessary single or double
                //  rotations and adjusts balance factors throughout the affected subtree.
                //

                } else {

                    //
                    //  Critical Imbalance Resolution: Rotation Operations
                    //  
                    //  Execute sophisticated rebalancing algorithm to restore AVL properties.
                    //  The RebalanceNode function analyzes the imbalance pattern and performs
                    //  appropriate single or double rotations to eliminate the height
                    //  violation while maintaining binary search tree ordering.
                    //

                    RebalanceNode( S );
                    break;
                }
            }
        }

        //
        //  Phase 5: User Data Integration and Content Preservation
        //  
        //  Copy the complete buffer contents into the newly allocated node's user data
        //  area. This operation preserves the original element data while ensuring it
        //  persists independently of the caller's buffer. The data copy creates a
        //  permanent table entry that remains valid until explicit deletion.
        //  

        RtlCopyMemory( &NewNode->UserData, Buffer, BufferSize );

    } else {

        //
        //  Existing Element Access: Return Previously Stored Data
        //  
        //  When the search operation located an existing element with a matching key,
        //  return a pointer to that element without performing any insertion, allocation,
        //  or tree modification. This provides efficient access to existing data while
        //  maintaining read-only semantics for duplicate key scenarios.
        //

        NewNode = NodeOrParent;
    }

    //
    //  Insertion Status Reporting: NewElement Flag Management
    //  
    //  Update the optional NewElement output parameter to reflect the operation outcome.
    //  This flag provides crucial information for applications that need to distinguish
    //  between successful new insertions and access to existing elements.
    //  
    //  Status Values:
    //  • TRUE: New element was successfully created and inserted into the table
    //  • FALSE: Existing element was found and returned without modification
    //

    if (ARGUMENT_PRESENT(NewElement)) {
        *NewElement = ((SearchResult == TableFoundNode)?(FALSE):(TRUE));
    }

    //
    //  Return Value: User Data Pointer with Proper Alignment
    //  
    //  Return a pointer directly to the user data portion of the node, skipping
    //  over the internal balanced tree link structures. This pointer provides
    //  direct access to the stored element data and remains valid until the
    //  element is explicitly deleted from the table.
    //  
    //  The returned pointer is properly aligned for the user data type and
    //  points to either newly copied data (for insertions) or existing data
    //  (for duplicate key access).
    //

    return &NewNode->UserData;
}


/**
 * @brief Key-based element removal with comprehensive deletion and enumeration safety.
 * 
 * Performs complete element removal from the AVL table using key-based lookup
 * followed by sophisticated tree deletion with automatic enumeration state
 * management. 
 * 
 * Deletion Process Overview:
 * 
 * Phase 1: Element Lookup and Validation
 * 
 * Phase 2: Enumeration Safety Protocols
 * 
 * Phase 3: Structural Tree Deletion
 * 
 * Phase 4: State Management and Cleanup
 * 
 * Phase 5: Memory Deallocation
 * 
 * Enumeration Safety Features:
 * 
 * RestartKey Protection:
 * When the deleted element is currently referenced by an active enumeration
 * (RestartKey), the function automatically adjusts the enumeration state to
 * point to the predecessor element, ensuring safe continuation of enumeration.
 * 
 * 
 * @param[in] Table Pointer to the AVL table structure containing the element to delete.
 *                  Must be a valid, properly initialized table with registered comparison
 *                  and deallocation routines. The table structure will be modified to
 *                  reflect the element removal and updated state information
 * 
 * @param[in] Buffer Search key data buffer passed to the user-defined comparison routine.
 *                   Contains the key information necessary to locate the target element
 *                   for deletion. The buffer contents and structure are application-specific
 *                   and must match the format expected by the registered comparison function.
 *                   This data is used solely for element identification and is not modified
 * 
 * @return Deletion operation outcome with the following semantics:
 *         - TRUE (Successful Deletion): The target element was located and completely removed from the table.
 *           All enumeration states have been updated for safety, tree balance has been restored, element
 *           counters decremented, and memory deallocated through the user-defined FreeRoutine.
 *         - FALSE (Element Not Found): No element matching the provided key buffer was found in the table.
 *           The table structure remains completely unmodified - no deletions, state changes, or side effects occur.
 *           The return value enables callers to distinguish between successful deletion and element absence,
 *           supporting conditional logic and error handling in calling code.
 */
BOOLEAN
RtlDeleteElementGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer
    )

{

   
    //  
    //  This variable will receive either the located target element (on successful
    //  search) or the parent node where insertion would occur (on failed search).
    //  For deletion operations, only successful searches are processed further.
    //
    PRTL_BALANCED_LINKS NodeOrParent;

    //
    //  Search Outcome Classification: Element Presence Determination
    //  
    //  Captures the precise result of the tree search operation, indicating whether
    //  the target element was found or providing insertion context if not found.
    //  Only TableFoundNode results proceed to the deletion phase.
    //
    TABLE_SEARCH_RESULT Lookup;

    //
    //  Phase 1: Element Lookup and Existence Verification
    //  
    //  Execute comprehensive tree search using the provided key buffer and the
    //  table's registered comparison routine. This search operation determines
    //  both element existence and provides structural context for deletion.
    //  
    //

    Lookup = FindNodeOrParent(
                 Table,
                 Buffer,
                 &NodeOrParent
                 );

    //
    //  Early Termination: Handle Element Absence with No Side Effects
    //  
    //  When the search operation fails to locate a matching element, terminate
    //  the deletion process immediately without any table modifications. This
    //  ensures atomic semantics where failed deletions leave the table completely
    //  unchanged in both structure and state.
    //  
    //

    if (Lookup != TableFoundNode) {

        return FALSE;

    } else {

        //
        //  Phase 2: Enumeration Safety - RestartKey Protection Protocol
        //  
        //  Before performing the actual deletion, ensure that any active enumeration
        //  states remain valid after the target element is removed. This critical
        //  safety mechanism prevents enumeration corruption that could cause
        //  application crashes or infinite loops.
        //  
        //  RestartKey Management Strategy:
        //  When the element to be deleted is currently serving as the enumeration
        //  restart position, update the RestartKey to point to the immediate
        //  predecessor in sorted order. This allows enumeration to continue
        //  seamlessly from the correct position after deletion.
        //  
        //  Predecessor Selection Rationale:
        //  Using the predecessor (rather than successor) ensures that enumeration
        //  will not skip any remaining elements, providing complete traversal
        //  coverage even when deletions occur during enumeration.
        //

        if (NodeOrParent == Table->RestartKey) {
            Table->RestartKey = RealPredecessor( NodeOrParent );
        }

        //
        //  Phase 2 (Continued): Concurrent Enumeration Tracking
        //  
        //

        Table->DeleteCount += 1;

        //
        //  Phase 3: Structural Tree Deletion with Balance Restoration
        //  
        //  Delegate the complex tree deletion operation to the specialized
        //  DeleteNodeFromTree function, which handles sophisticated node removal
        //  scenarios including predecessor/successor replacement and AVL rebalancing.
        //  
        //

        DeleteNodeFromTree( Table, NodeOrParent );
        Table->NumberGenericTableElements--;

        //
        //  Phase 4: Position Cache Invalidation and State Reset
        //  
        //  Reset the indexed access caching mechanism to prevent stale position
        //  references after the table structure has been modified. This ensures
        //  that subsequent calls to RtlGetElementGenericTableAvl will recalculate
        //  positions from the beginning rather than using potentially invalid cache.
        //  
        //

        Table->WhichOrderedElement = 0;
        Table->OrderedPointer = NULL;

 
        //
        //  Phase 4: Memory Deallocation through User-Defined Routine
        //  
        //  Invoke the application-provided memory deallocation routine to free
        //  the memory associated with the deleted element. The function receives
        //  a pointer to the complete node structure (including tree links) rather
        //  than just the user data portion.
        //  

        Table->FreeRoutine(Table,NodeOrParent);
        return TRUE;
    }
}


/**
 * @brief Simplified element search with basic lookup interface for AVL table operations.
 * 
 * Provides a streamlined interface for element lookup operations within AVL tables,
 * focusing solely on element retrieval without exposing internal tree navigation
 * details. This function serves as the primary entry point for standard lookup
 * operations where only element presence/absence and data access are required.
 * 
 * @param[in] Table Pointer to the AVL table structure containing elements to search.
 *                  Must be a valid, properly initialized table with registered comparison
 *                  routine. The table state remains unmodified during the search operation
 * 
 * @param[in] Buffer Search key data passed to the user-defined comparison routine.
 *                   The buffer contents and structure are application-specific and should
 *                   contain the key fields necessary for element identification. This data
 *                   is used exclusively for comparison and is not stored or modified during
 *                   the search process
 * 
 * @return Element access result with the following semantics:
 *         - SUCCESS (Element Found): Returns pointer to the user data portion of the located element.
 *           This pointer provides direct access to the stored data and remains valid until the element is deleted from the table.
 *         - FAILURE (Element Not Found): Returns NULL to indicate absence of matching element. No additional
 *           context about insertion position or tree structure is provided in this simplified interface.
 *         - Data Validity Note: In concurrent environments, successful lookups return pointers that
 *           may become invalid immediately if other threads delete the referenced element. Proper synchronization
 *           mechanisms are required to ensure data integrity and prevent access violations.
 */
PVOID
RtlLookupElementGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID Buffer
    )

{

    //
    //  Node Reference Storage: Container for Tree Navigation Context
    //  
    //  This variable will receive either the located element node (on success) or
    //  the optimal parent node for insertion (on failure). While this advanced
    //  context is generated during the search process, it is not exposed through
    //  the simplified interface and will be discarded upon function completion.
    //
    PRTL_BALANCED_LINKS NodeOrParent;

    //
    //  Search Outcome Classification: Detailed Result Analysis Storage
    //  
    //  This variable captures the precise search outcome including element location
    //  status and insertion position context. The comprehensive search engine
    //  provides detailed classification (TableFoundNode, TableInsertAsLeft, etc.)
    //  but this simplified interface only utilizes the basic found/not-found distinction.
    //
    TABLE_SEARCH_RESULT Lookup;

    return RtlLookupElementGenericTableFullAvl(
                Table,
                Buffer,
                &NodeOrParent,
                &Lookup
                );
}


/**
 * @brief searches a generic table for an element that matches the specified data.
 * 
 * @param[in] Table Pointer to the AVL table structure to search within.
 *                  Must be a valid, properly initialized table. The table state
 *                  is not modified during this read-only operation
 * 
 * @param[in] Buffer Search key data buffer passed to the user-defined comparison routine.
 *                   The contents and structure are application-specific but typically
 *                   contain the key fields used for element identification and ordering.
 *                   This buffer is used solely for comparison and is not stored or modified
 * 
 * @param[out] NodeOrParent Output parameter receiving contextual node reference.
 *                          On Success (element found): Points to the located table node
 *                          On Failure (element not found): Points to optimal parent for insertion
 *                          This dual-purpose output enables both immediate data access and
 *                          optimized insertion context for subsequent operations
 * 
 * @param[out] SearchResult Output parameter describing the search outcome and positional context.
 *                          Possible values and meanings:
 *                          • TableFoundNode: Element located successfully
 *                          • TableInsertAsLeft: Element not found, insert as left child of NodeOrParent
 *                          • TableInsertAsRight: Element not found, insert as right child of NodeOrParent
 *                          • TableEmptyTree: Table is empty, special insertion handling required
 *                          This result guides subsequent insertion operations and eliminates
 *                          the need for redundant tree traversal
 * 
 * @return Context-dependent return value based on search outcome:
 *         - SUCCESS (Element Found): Returns pointer to the user data portion of the located element.
 *           This pointer provides immediate access to the stored data and remains valid until the element is deleted from the table.
 *         - FAILURE (Element Not Found): Returns NULL to indicate absence of matching element. The NodeOrParent
 *           and SearchResult parameters contain insertion context for optimized subsequent insertion operations.
 *         - Note: In concurrent environments, successful lookups may return pointers to elements that could be deleted
 *           immediately by other threads. Proper synchronization is required to ensure data validity.
 */
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl (
    PRTL_AVL_TABLE Table,
    PVOID Buffer,
    OUT PVOID *NodeOrParent,
    OUT TABLE_SEARCH_RESULT *SearchResult
    )

{

    *SearchResult = FindNodeOrParent(
                        Table,
                        Buffer,
                        (PRTL_BALANCED_LINKS *)NodeOrParent
                        );

    if (*SearchResult != TableFoundNode) {

        return NULL;

    } else {

        return &((PTABLE_ENTRY_HEADER)*NodeOrParent)->UserData;
    }
}


/**
 * @brief Sequential element enumeration with state-based tree traversal and restart control.
 * 
 * Provides ordered iteration through all table elements in sorted sequence, maintaining
 * enumeration state between calls to enable efficient sequential access patterns.
 * This function acts as a simplified wrapper around the core enumeration engine,
 * managing restart logic and state initialization automatically.
 *  
 * @param[in] Table Pointer to the AVL table structure containing elements to enumerate.
 *                  Must be a valid, properly initialized table. The table's internal
 *                  enumeration state (RestartKey) will be modified during operation
 * 
 * @param[in] Restart Enumeration control flag specifying traversal behavior:
 *                    TRUE: Initialize enumeration from the beginning (minimum element)
 *                          Resets all internal state and starts fresh traversal
 *                    FALSE: Continue enumeration from current cached position
 *                           Advances to next element in sorted sequence
 *                    Note: First call to this function should always use Restart=TRUE
 *                          to establish proper enumeration context
 * 
 * @return Pointer to user data portion of the current enumeration element.
 *         Returns NULL when enumeration is complete (no more elements)
 *         or when the table is empty.
 *         The returned pointer remains valid until the corresponding element
 *         is deleted from the table. In single-threaded environments, this
 *         pointer is safe to use until the next enumeration call.
 */
PVOID
RtlEnumerateGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN BOOLEAN Restart
    )
{
    if (Restart) {
        Table->RestartKey = NULL;
    }

 

    return RtlEnumerateGenericTableWithoutSplayingAvl( Table, &Table->RestartKey );
}


BOOLEAN
RtlIsGenericTableEmptyAvl (
    IN PRTL_AVL_TABLE Table
    )

/**
 * @brief Determines whether a generic table is empty.
 * 
 * @param[in] Table Pointer to the AVL table structure to query for emptiness.
 *                  Must be a valid, properly initialized AVL table. NULL pointers
 *                  will cause access violations. The table state is not modified
 *                  during this operation
 * 
 * @return Element presence indicator with the following semantics:
 *         TRUE: Table contains zero elements (completely empty)
 *         FALSE: Table contains one or more elements (non-empty)
 *         Note: Return value represents state at call time and may
 *               change immediately in multi-threaded environments
 */

{
    return ((Table->NumberGenericTableElements)?(FALSE):(TRUE));
}


PVOID
RtlGetElementGenericTableAvl (
    IN PRTL_AVL_TABLE Table,
    IN ULONG I
    )

/**
 * @brief Index-based element access with position optimization 
 * 
 * This function provides direct access to table elements by zero-based index position
 * in sorted order, implementing intelligent navigation optimization to minimize
 * traversal overhead. 
 * 
 * @param[in] Table Pointer to the initialized AVL table structure for indexed access.
 *                  Must be a valid table that has been properly initialized and
 *                  contains at least one element for non-zero indices
 * @param[in] I Zero-based index of the desired element in sorted order.
 *              Valid range: 0 to (RtlNumberGenericTableElementsAvl(Table) - 1)
 *              Index Semantics:
 *              • 0: Minimum element (leftmost in sorted order)
 *              • 1: Second smallest element
 *              • N-1: Maximum element (rightmost in sorted order)
 * 
 * @return Pointer to user data portion of the element at index I, or NULL when:
 *         • Index I is out of bounds (≥ number of elements)
 *         • Index I equals MAXULONG (invalid index sentinel)
 *         • Table is empty
 *         • Internal navigation error
 */

{
    //
    //  Position Cache State: Previously accessed index and corresponding node reference
    //  
    //  Maintains the last successfully accessed position to optimize subsequent calls
    //  with nearby indices. When accessing sequential or nearby elements, this cache
    //  enables O(1) or O(distance) performance instead of O(log n) tree traversal.
    //

    ULONG CurrentLocation = Table->WhichOrderedElement;

    //
    //  Table Size Snapshot: Total element count for bounds validation and optimization
    //  
    //  Captures the current table size to prevent out-of-bounds access and
    //  assists in distance calculation for traversal optimization heuristics.
    //

    ULONG NumberInTable = Table->NumberGenericTableElements;

    //
    //  Navigation Distance Metrics: Traversal cost analysis variables
    //  
    //  Used to calculate and compare the cost of reaching the target index
    //  via forward traversal (successor operations) versus backward traversal  
    //  (predecessor operations) to select the optimal navigation strategy.
    //

    ULONG ForwardDistance,BackwardDistance;

    //
    //  Current Position Reference: Active tree node pointer for navigation operations
    //  
    //  Points to the current node during traversal operations. Initialized from
    //  the cached position state and updated during navigation to reach the target.
    //  NULL indicates that no valid cached position exists (first access).
    //

    PRTL_BALANCED_LINKS CurrentNode = (PRTL_BALANCED_LINKS)Table->OrderedPointer;

    //
    //  Input Validation: Bounds Checking and Invalid Index Detection
    //  
    //  Perform early validation to reject invalid indices before expensive tree
    //  operations. This prevents access violations and provides consistent error
    //  handling for out-of-range requests.
    //  
    //  Validation Cases:
    //  • MAXULONG: Reserved sentinel value indicating invalid/uninitialized index
    //  • Index ≥ ElementCount: Beyond valid range of existing elements
    //  
    //  Early return optimization prevents unnecessary processing for invalid requests.
    //

    if ((I == MAXULONG) || ((I + 1) > NumberInTable)) return NULL;

    if (CurrentNode == NULL) {

        //
        //  Minimum Element Location: Find Leftmost Node in Sorted Order
        //  
        //  Navigate to the smallest element by following left child pointers
        //  from the tree root until reaching a leaf. This implements the
        //  standard binary search tree minimum-finding algorithm.
        //  
        //  Tree Structure: BalancedRoot.RightChild points to the actual root,
        //  while BalancedRoot itself serves as a sentinel structure.
        //

        for (
            CurrentNode = Table->BalancedRoot.RightChild;
            CurrentNode->LeftChild;
            CurrentNode = CurrentNode->LeftChild
            ) {
            NOTHING;
        }
        CurrentLocation = 0;

        //
        //  Cache State Update: Store Position for Future Optimization
        //  
        //  Save the discovered minimum element position in the table's position
        //  cache to accelerate subsequent accesses. 

        Table->OrderedPointer = CurrentNode;
        Table->WhichOrderedElement = 0;
    }

    //
    //  Cache Hit Optimization: Direct Return for Matching Index
    //  
    //  When the requested index matches the currently cached position, return
    //  immediately without any tree navigation. This provides O(1) performance
    //  for repeated access to the same element or sequential enumeration patterns.

    if (I == CurrentLocation) {

        return &((PTABLE_ENTRY_HEADER)CurrentNode)->UserData;
    }

    //
    //  Navigation Strategy Selection: Optimal Path Analysis for Target Index
    //  
    //  Determine the most efficient traversal method to reach the target index
    //  by analyzing the relative positions and calculating traversal costs for
    //  different navigation strategies.
    //  
    //  Strategy Categories:
    //  1. Backward from current position (CurrentLocation > I)
    //  2. Forward from current position (CurrentLocation < I)  
    //  3. Fresh start from minimum/maximum element (optimization boundary cases)
    //

    if (CurrentLocation > I) {

        //
        //  Backward Navigation Analysis: Current Position > Target Index
        //  
        //  When the cached position is beyond the target, evaluate whether backward
        //  traversal from current position or forward traversal from the table start
        //  provides better performance.
        //

        if (I >= (CurrentLocation/2)) {

            //
            //  Backward Traversal Optimization: Direct Predecessor Navigation
            //  
            //  Target index is closer to current position than to table start.
            //  Use RealPredecessor operations to move backward through sorted order
            //  until reaching the desired index position.

            for (
                BackwardDistance = CurrentLocation - I;
                BackwardDistance;
                BackwardDistance--
                ) {

                CurrentNode = RealPredecessor(CurrentNode);
            }

        } else {

            //
            //  Fresh Start Optimization: Restart from Table Minimum
            //  
            //  Target index is closer to table start than current position.
            //  Navigate to minimum element and traverse forward to target, avoiding
            //  the longer backward traversal from current position.

            for (
                CurrentNode = Table->BalancedRoot.RightChild;
                CurrentNode->LeftChild;
                CurrentNode = CurrentNode->LeftChild
                ) {
                NOTHING;
            }

            //
            //  Forward Traversal from Minimum: Sequential Successor Navigation
            //  
            //  Starting from the minimum element (index 0), use RealSuccessor
            //  operations to advance through sorted order until reaching
            //  the target index position.
            //

            for (
                ;
                I;
                I--
                ) {

                CurrentNode = RealSuccessor(CurrentNode);
            }
        }

    } else {

        //
        //  Forward Navigation Analysis: Current Position < Target Index
        //  
        //  When the cached position is before the target, evaluate whether forward
        //  traversal from current position or backward traversal from the table end
        //  provides better performance.
        //  
 
        ForwardDistance = I - CurrentLocation;

        //
        //  Backward Distance Calculation: Cost of Traversal from Table Maximum
        //  
        //  Calculate the number of predecessor operations needed to reach the
        //  target index when starting from the maximum element (rightmost node).
        //  
        //  Formula: BackwardDistance = TotalElements - (TargetIndex + 1)
        //  This represents the distance from the end of the sorted sequence.
        //

        BackwardDistance = NumberInTable - (I + 1);

        //
        //  Navigation Strategy Selection: Choose Optimal Traversal Direction
        //  
        //  Compare forward and backward traversal costs to select the most efficient
        //  path. The bias factor (+1) prevents constant switching between strategies
        //  for boundary cases and optimizes for the common case of accessing the
        //  last element without requiring expensive rightmost node discovery.
        //

        if (ForwardDistance <= (BackwardDistance + 1)) {

            //
            //  Forward Traversal Optimization: Direct Successor Navigation
            //  
            //  Forward distance is shorter or equivalent to backward distance.
            //  Use RealSuccessor operations to advance from current position
            //  through sorted order until reaching the target index.

            for (
                ;
                ForwardDistance;
                ForwardDistance--
                ) {

                CurrentNode = RealSuccessor(CurrentNode);
            }

        } else {

            //
            //  Backward from Maximum Optimization: Start from Table End
            //  
            //  Backward distance is significantly shorter than forward distance.
            //  Navigate to maximum element and traverse backward to target, avoiding
            //  the longer forward traversal from current position.
            //  

            for (
                CurrentNode = Table->BalancedRoot.RightChild;
                CurrentNode->RightChild;
                CurrentNode = CurrentNode->RightChild
                ) {
                NOTHING;
            }

            //
            //  Backward Traversal from Maximum: Sequential Predecessor Navigation
            //  
            //  Starting from the maximum element (index N-1), use RealPredecessor
            //  operations to move backward through sorted order until reaching
            //  the target index position.
            //

            for (
                ;
                BackwardDistance;
                BackwardDistance--
                ) {

                CurrentNode = RealPredecessor(CurrentNode);
            }
        }
    }

    //
    //  Result Processing and Cache Update: Finalize Position and Return User Data
    //  
    //  Update the table's position cache with the newly discovered location to
    //  optimize future accesses, then extract and return the user data portion
    //  from the located element.
    //  

    Table->OrderedPointer = CurrentNode;
    Table->WhichOrderedElement = I;

    return &((PTABLE_ENTRY_HEADER)CurrentNode)->UserData;
}


ULONG
RtlNumberGenericTableElementsAvl (
    IN PRTL_AVL_TABLE Table
    )

/**
 * @brief Table size query with efficient element count retrieval for AVL tables.
 * 
 * This function provides constant-time access to the current number of elements
 * stored in an AVL table. The count represents active elements and is maintained
 * automatically during all insertion and deletion operations.
 * 
 * Implementation Notes:
 * 
 * • Count maintained in Table->NumberGenericTableElements field
 * • Incremented during successful insertions (new elements only)
 * • Decremented during successful deletions  
 * • Unaffected by lookup operations or enumeration activities
 * • Maximum value constrained by ULONG range (typically 4,294,967,295)
 * 
 * Relationship to Other Functions:
 * • RtlIsGenericTableEmptyAvl(): Optimized boolean empty check
 * • RtlGetElementGenericTableAvl(): Uses count for bounds validation
 * • Insert/Delete operations: Automatically maintain accurate count
 * 
 * @param[in] Table Pointer to the initialized AVL table structure for size query.
 *                  Must be a valid table that has been properly initialized via
 *                  RtlInitializeGenericTableAvl. The table structure must remain
 *                  stable during the function call (basic read synchronization)
 * 
 * @return Current number of elements stored in the table (0 to MAXULONG).
 *         Return Values:
 *         • 0: Empty table (no elements present)
 *         • 1+: Actual count of active elements in the table
 *         • MAXULONG: Theoretical maximum (constrained by available memory)
 *         The returned value represents a snapshot of the table size at the
 *         moment of the function call. In concurrent environments, this value
 *         may change immediately after return if modifications are ongoing
 */

{
    return Table->NumberGenericTableElements;
}


PVOID
RtlEnumerateGenericTableWithoutSplayingAvl (
    IN PRTL_AVL_TABLE Table,
    IN PVOID *RestartKey
    )

/**
 * @brief AVL table enumeration with explicit position management.
 * 
 * This function provides stateless enumeration of AVL table elements in sorted order. This * function requires explicit position management through the RestartKey parameter and does * not modify table state.
 * 
 * Historical Context (it might be):
 * The "WithoutSplaying" terminology originates from the original splay tree 
 * implementation where enumeration would perform tree rotations. This AVL
 * variant preserves the naming convention to indicate non-modifying behavior,
 * even though AVL trees don't perform splaying operations.
 * 
 * 
 * @param[in] Table Pointer to the initialized AVL table structure for enumeration.
 *                  Must remain stable throughout the enumeration process when used
 *                  concurrently (protected by appropriate synchronization)
 * @param[in,out] RestartKey Bidirectional position tracking pointer for enumeration state.
 *                           INPUT: NULL = begin enumeration from smallest element
 *                           Non-NULL = continue from previously returned position
 *                           OUTPUT: Updated with current element pointer for next call
 *                           NULL when enumeration completes (no more elements)
 *                           CRITICAL: Caller must not modify the referenced memory between calls.
 *                           The pointer value itself serves as the position bookmark
 * 
 * @return Pointer to user data portion of current element, or NULL indicating:
 *         • Enumeration completed (no more elements in sorted order)
 *         • Table is empty
 *         • Invalid input parameters
 *         The returned pointer remains valid until the element is deleted
 *         or the table is destroyed. Multiple concurrent enumerations may
 *         return pointers to the same elements
 */

{
    //
    //  Early Termination: Empty Table Detection and Immediate Return
    //  
    //  Perform upfront validation to avoid unnecessary processing when the table
    //  contains no elements. This optimization prevents execution of enumeration
    //  logic against empty data structures and provides consistent behavior
    //  across all enumeration entry points.
    //  
    //  Return NULL immediately to indicate enumeration completion for empty tables.
    //

    if (RtlIsGenericTableEmptyAvl(Table)) {

        return NULL;

    } else {

        //
        //  Enumeration Position Holder: Current Element Reference for Return Processing
        //  
        //  This variable maintains the reference to the current element being processed
        //  during enumeration. It serves as both the target for tree navigation operations
        //  and the source for user data extraction in the final return statement.
        //
        PRTL_BALANCED_LINKS NodeToReturn;

        //
        //  Enumeration Initialization: Navigate to Minimum Element for Fresh Start
        //  
        //  When RestartKey is NULL, begin enumeration from the beginning by locating
        //  the smallest element in the sorted table. In a binary search tree, the
        //  minimum element is always the leftmost node, found by traversing left
        //  children until reaching a leaf.
        // 
        //  State Update: Cache the starting position in RestartKey for subsequent calls.
        //

        if (*RestartKey == NULL) {

            //
            //  Minimum Element Discovery: Traverse Left Until Reaching Leftmost Leaf
            //  
            //  Binary search tree property guarantees that the leftmost node contains
            //  the smallest key value. The sentinel root structure requires accessing
            //  the actual tree root via BalancedRoot.RightChild before beginning
            //  the leftward traversal to the minimum element.
            //

            for (
                NodeToReturn = Table->BalancedRoot.RightChild;
                NodeToReturn->LeftChild;
                NodeToReturn = NodeToReturn->LeftChild
                ) {
                ;
            }

            *RestartKey = NodeToReturn;

        } else {

            //
            //  Enumeration Continuation: Advance to Next Element in Sorted Order
            //  
            //  When RestartKey contains a valid position from a previous call, calculate
            //  the immediate successor in the sorted sequence. The RealSuccessor function
            //  implements in-order traversal logic to find the next larger element.
            //  
            //  Successor Calculation Process:
            //  1. If current node has right subtree -> find leftmost node in right subtree
            //  2. If no right subtree -> traverse up until finding ancestor via left path
            //  3. If no successor exists -> return NULL (reached maximum element)
            //  
            //  Position State Management:
            //  • Update RestartKey with new position for subsequent calls
            //  • Preserve NULL result when enumeration reaches the end
            //

            NodeToReturn = RealSuccessor(*RestartKey);

            if (NodeToReturn) {
                *RestartKey = NodeToReturn;
            }
        }

        //
        //  Result Processing: Extract User Data from Valid Elements or Return NULL
        //  
        //  Convert the internal tree node reference into a user-accessible data pointer
        //  or return NULL to indicate enumeration completion.
        //  
        //  Data Extraction Process:
        //  • Valid NodeToReturn: Cast to TABLE_ENTRY_HEADER and extract UserData portion
        //  • NULL NodeToReturn: Return NULL to signal end of enumeration sequence
        //  

        return ((NodeToReturn)?
                   ((PVOID)&((PTABLE_ENTRY_HEADER)NodeToReturn)->UserData)
                  :((PVOID)(NULL)));
    }
}


PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory (
    IN PRTL_AVL_TABLE Table,
    IN PRTL_AVL_MATCH_FUNCTION MatchFunction OPTIONAL,
    IN PVOID MatchData OPTIONAL,
    IN ULONG NextFlag,
    IN OUT PVOID *RestartKey,
    IN OUT PULONG DeleteCount,
    IN PVOID Buffer
    )

/**
 * @brief Advanced directory-style table enumeration, in collation order.
 * 
 * @param[in] Table Pointer to the AVL table structure containing elements to enumerate.
 *                  Must be properly initialized via RtlInitializeGenericTableAvl
 * @param[in] MatchFunction Optional filtering function to determine element inclusion.
 *                          Function receives table pointer, element data, and MatchData.
 *                          Return STATUS_SUCCESS to include element in results.
 *                          Return STATUS_NO_MATCH to skip element and continue.
 *                          Return STATUS_NO_MORE_MATCHES to terminate enumeration.
 *                          If NULL, all elements are included (universal match)
 * @param[in] MatchData Arbitrary data pointer passed to MatchFunction for filtering logic.
 *                      Common usage: wildcard patterns, search criteria, filter state.
 *                      Ignored if MatchFunction is NULL
 * @param[in] NextFlag Enumeration position control flag:
 *                     FALSE: Return current element at RestartKey/Buffer position
 *                     TRUE: Skip current element and return next matching element
 *                     Set to FALSE for first call, TRUE for subsequent continuation calls
 * @param[in,out] RestartKey Bidirectional position cache for efficient enumeration resumption.
 *                           INPUT: NULL = start from Buffer position (fresh/fallback navigation)
 *                           Non-NULL = resume from cached position (if still valid)
 *                           OUTPUT: Updated with current element pointer for next call
 *                           Automatically cleared (NULL) when enumeration completes
 * @param[in,out] DeleteCount Concurrent modification detection mechanism for position validation.
 *                            INPUT: Previous table delete count (0 for first call)
 *                            OUTPUT: Current table delete count for next call validation
 *                            When input != current count, cached RestartKey is invalidated
 *                            and enumeration falls back to Buffer-based positioning
 * @param[in] Buffer Navigation key for position establishment within the sorted table.
 *                   Must contain valid key data compatible with table's comparison function.
 *                   Usage Patterns:
 *                   • Table Start: Initialize with minimum possible key value
 *                   • Arbitrary Start: Set to desired starting key (exact or approximate)
 *                   • Resumption: Set to last known key when RestartKey is invalid
 *                   • Pattern Search: Set to pattern prefix (e.g., "TXF" for "TXF*" search)
 *                   Key Behavior: If exact key doesn't exist, enumeration begins from
 *                   next larger key in sorted order (standard binary search semantics)
 * 
 * @return Pointer to user data portion of matching element, or NULL when:
 *         • No more elements exist beyond current position
 *         • MatchFunction returned STATUS_NO_MORE_MATCHES (enumeration termination)
 *         • Table is empty
 *         • Invalid input parameters
 */

{
    NTSTATUS Status;

    //
    //  Enumeration State Management: Cached position pointer for efficient resumption
    //  between successive enumeration calls. When non-NULL, represents the last
    //  returned element position. When NULL, triggers fresh positioning using Buffer.
    //

    PTABLE_ENTRY_HEADER NodeOrParent = (PTABLE_ENTRY_HEADER)*RestartKey;

    //
    //  Tree Navigation Result: Outcome classification from binary search operation
    //  when establishing position within the sorted table structure using Buffer key.
    //

    TABLE_SEARCH_RESULT Lookup;

    //
    //  Prerequisite Validation: Empty Table Early Exit Optimization
    //  
    //  Before initiating complex enumeration logic, verify that the table contains
    //  at least one element. This prevents unnecessary processing overhead and
    //  provides immediate termination for degenerate cases.
    //  
    //  Side Effect: Clear RestartKey to maintain consistent state for caller
    //

    if (RtlIsGenericTableEmptyAvl(Table)) {

        *RestartKey = NULL;
        return NULL;
    }

    //
    //  Filter Function Initialization: Universal Match Default Assignment
    //  
    //  When no custom filtering function is provided, assign the universal matcher
    //  that accepts all elements. This eliminates the need for NULL checks during
    //  the enumeration loop and simplifies the core filtering logic.
    //  
    //  MatchAll function always returns STATUS_SUCCESS for any element.
    //

    if (MatchFunction == NULL) {
        MatchFunction = &MatchAll;
    }

    if (*DeleteCount != Table->DeleteCount) {
        NodeOrParent = NULL;
    }

    ASSERT(FIELD_OFFSET(TABLE_ENTRY_HEADER, BalancedLinks) == 0);

    if (NodeOrParent == NULL) {

        Lookup = FindNodeOrParent(
                     Table,
                     Buffer,
                     (PRTL_BALANCED_LINKS *)&NodeOrParent
                     );

        //
        //  Enumeration Start Position Adjustment: Handle Inexact Key Matches
        //  
        //  When the Buffer key doesn't exactly match an existing element, adjust
        //  the starting position based on binary search outcome to ensure proper
        //  enumeration ordering and prevent element skipping.
        //  
        //  Position Logic:
        //  • TableFoundNode: Start from exact match (no adjustment needed)
        //  • TableInsertAsLeft: Current node is the immediate successor (use as-is)
        //  • TableInsertAsRight: Current node < Buffer key, advance to successor
        //  
        //  Override NextFlag to prevent skipping the determined starting element,
        //  since we're establishing initial position rather than continuing enumeration.
        //

        if (Lookup != TableFoundNode) {

            NextFlag = FALSE;

            //
            //  Successor Position Calculation for Right Insertion Point
            //  
            //  When Buffer key would insert to the right of the found node,
            //  the enumeration must begin from the next larger element in sorted order.
            //  Advance to the immediate successor to maintain correct traversal sequence.
            //

            if (Lookup == TableInsertAsRight) {
                NodeOrParent = (PTABLE_ENTRY_HEADER)RealSuccessor((PRTL_BALANCED_LINKS)NodeOrParent);
            }
        }
    }

    //
    //  Enumeration Continuation: Skip Current Element When Advancing
    //  
    //  When NextFlag is TRUE, the caller requests the next element after the current
    //  position, requiring advancement before applying filter criteria. This typically
    //  occurs on subsequent enumeration calls after the first call has established
    //  the initial position.
    //  
    //  The ASSERT ensures that a valid position exists before attempting advancement,
    //  preventing null pointer dereference during successor calculation.
    //

    if (NextFlag) {
        ASSERT(NodeOrParent != NULL);
        NodeOrParent = (PTABLE_ENTRY_HEADER)RealSuccessor((PRTL_BALANCED_LINKS)NodeOrParent);
    }

    //
    //  Element Filtering Loop: Sequential Search for Matching Elements
    //  
    //  Traverse the sorted table in ascending order, testing each element against
    //  the match criteria until finding an acceptable element or reaching enumeration
    //  termination conditions.
    //  
    //  Loop Continuation Conditions:
    //  • NodeOrParent != NULL: More elements exist in traversal direction
    //  • MatchFunction returns STATUS_NO_MATCH: Current element rejected, continue search
    //  
    //  Loop Termination Conditions:
    //  • NodeOrParent == NULL: Reached end of table (no more elements)
    //  • MatchFunction returns STATUS_SUCCESS: Found matching element
    //  • MatchFunction returns STATUS_NO_MORE_MATCHES: Explicit enumeration termination
    //  
    //  The double condition check ensures both existence validation and filter
    //  application occur atomically, preventing evaluation of the match function
    //  against NULL elements.
    //

    while ((NodeOrParent != NULL) && ((Status = (*MatchFunction)(Table, &NodeOrParent->UserData, MatchData)) == STATUS_NO_MATCH)) {
        NodeOrParent = (PTABLE_ENTRY_HEADER)RealSuccessor((PRTL_BALANCED_LINKS)NodeOrParent);
    }

    //
    //  Enumeration Result Finalization: Output Parameter Updates and Return Value Determination
    //  
    //  Process the filtering loop outcome and update caller's state variables to maintain
    //  enumeration consistency across successive function calls.
    //  
    //  Success Path (NodeOrParent != NULL):
    //  The filtering loop found a valid element, either through successful matching
    //  (STATUS_SUCCESS) or explicit termination request (STATUS_NO_MORE_MATCHES).
    //  
    //  State Updates:
    //  • RestartKey: Cache current position for efficient next call resumption
    //  • DeleteCount: Synchronize with table's current modification count
    //  
    //  Return Logic:
    //  • STATUS_SUCCESS: Return pointer to user data (successful match)
    //  • STATUS_NO_MORE_MATCHES: Return NULL (termination request honored)
    //  
    //  Failure Path (NodeOrParent == NULL):
    //  Reached end of table without finding matching elements. Return NULL to
    //  indicate enumeration completion, with output parameters left unchanged.
    //

    if (NodeOrParent != NULL) {
        ASSERT((Status == STATUS_SUCCESS) || (Status == STATUS_NO_MORE_MATCHES));
        *RestartKey = NodeOrParent;
        *DeleteCount = Table->DeleteCount;
        if (Status == STATUS_SUCCESS) {
            return &NodeOrParent->UserData;
        }
    }

    return NULL;
}

