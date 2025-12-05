// License nuances.
// I assume this code was originally developed by Microsoft, although it's not entirely clear.
// It was taken from a GitHub project that references the WRK (Windows Research Kernel?) in its name.
// The ReactOS code, on the other hand, is different, though it shares many similarities.
// Anyway, this professionally and effectively written code cannot be ignored as it is perfect for educational purposes.
// For the same educational reason, the code was also commented using AI (Sonnet 4.0), and all comments were reviewed and refined where necessary.

#include <WinKernelLite/Ntrtl.h>

#define SwapPointers(Ptr1, Ptr2) {      \
    PVOID _SWAP_POINTER_TEMP;           \
    _SWAP_POINTER_TEMP = (PVOID)(Ptr1); \
    (Ptr1) = (Ptr2);                    \
    (Ptr2) = _SWAP_POINTER_TEMP; \
    }

// gets the address of the Links element
#define ParentsChildPointerAddress(Links) ( \
    RtlIsLeftChild(Links) ?                 \
        &(((Links)->Parent)->LeftChild)     \
    :                                       \
        &(((Links)->Parent)->RightChild)    \
    )

/**
 * @brief Reorganizes splay tree by moving accessed node to root through rotations.
 * 
 * Performs the fundamental splay operation that maintains the splay tree property
 * by moving a recently accessed node to the root position. Uses zig, zig-zig, 
 * and zig-zag rotations based on the geometric relationship between the target
 * node and its ancestors.
 * 
 * @param[in] Links Pointer to the node to be splayed to root position.
 *                  Must be a valid node within the splay tree structure
 * 
 * @return Pointer to the new root of the tree (the splayed node)
 */
PRTL_SPLAY_LINKS
RtlSplay (
    PRTL_SPLAY_LINKS Links
    )

/*

This code is the main loop of the splay operation in a splay tree:
while (!RtlIsRoot(L)) {
    P = RtlParent(L);
    G = RtlParent(P);
    // ... rotation logic follows ...

The main steps of the algorithm are:
1. Loop Condition: !RtlIsRoot(L)
	- The loop continues as long as node L (the target node) is not the root of the tree
	- Remember: a root node has Parent pointing to itself (L->Parent == L)
	- The goal is to move node L up to become the root through rotations

2. Parent Chain Setup:
    - P = RtlParent(L) - Gets the parent of the current node
    - G = RtlParent(P) - Gets the grandparent (parent of parent)

    
The Splay Process
Each iteration of the loop:
1. Identifies the relationship between L, P, and G
2. Performs appropriate rotations to move L closer to the root
3. Updates the tree structure after rotation
4. Continues until L becomes the root

The splay operation implements the "splay tree property" - recently accessed nodes are moved to the root, which:
    - Improves access time for frequently used nodes
    - Maintains balanced tree performance over time
    - Provides amortized O(log n) performance

In the splay tree implementation, Links (assigned to L) is indeed the element that was just accessed and now needs to be moved to the root through tree reorganization.

So the short code is as follows:
PRTL_SPLAY_LINKS
RtlSplay (
    PRTL_SPLAY_LINKS Links  // ← This is the recently accessed node
    )
{
    PRTL_SPLAY_LINKS L;
    // ...

    L = Links;  // L becomes the target node to move to root

    // Reorganize tree to move L to the root
    while (!RtlIsRoot(L)) {
        // Perform rotations to move L upward...
    }

    return L;  // Return L as the new root
}

Rules:
1. Access Event: Some operation (search, insert, etc.) has just accessed the node pointed to by Links
2. Splay Property: In splay trees, recently accessed nodes should be moved to the root for faster future access
3. Tree Reorganization: The RtlSplay() function reorganizes the entire tree structure through rotations to bring this accessed node to the top

Why do we need these functions?:
- RtlIsLeftChild
- RtlIsRightChild

RtlIsLeftChild determines the position relationship between the target node L and its parent P, 
which is crucial for deciding what type of rotation to perform in the splay operation.

First we check maybe L is root in while, so we return from the cycle. Then we can have two cases L is the 
left child of P or right child of P

The Two Main Cases
if (RtlIsLeftChild(L)) {
    // L is the LEFT child of its parent P
    // Handle left-side rotations (Zig-left, Zig-Zig-left, Zig-Zag-left)
} else {
    // L is the RIGHT child of its parent P
    // Handle right-side rotations (Zig-right, Zig-Zig-right, Zig-Zag-right)
}

The Pattern in the Code:

if (RtlIsLeftChild(L)) {
    if (RtlIsRoot(P)) {                             // case 1
        // Zig rotation (left variant)
    } else if (RtlIsLeftChild(P)) {                 // case 2
        // Zig-Zig rotation (left-left variant)
    } else {                                        // case 3  RtlIsRightChild(Parent)
        // Zig-Zag rotation (left-right variant)
    }
} else { // RtlIsRightChild(L)
	if (RtlIsRoot(P)) { // case 4
        // Zig rotation (right variant)
    } else if (RtlIsRightChild(P)) {  // case 5
        // Zig-Zig rotation (right-right variant)
    } else { // case 6
        // Zig-Zag rotation (right-left variant)
    }
}

So RtlIsLeftChild(L) is the first major branching decision that determines which family of splay rotations 
to use based on the geometric relationship between the target node and its parent.

Case 1 ==> if (RtlIsRoot(P)) -> In this case the L has a parent which is ROOT
                         P            L
                        / \          / \
                       L   t3  ==>  t1  P
                      / \              / \
                     t1  t2          t2   t3

Case 2 ==> } else if (RtlIsLeftChild(P))  -> L is left node of parrent, but parrent is not root
              we have the following case

                      |           |
                      G           L
                     / \         / \
                    P   t4  ==> t1  P
                   / \             / \
                  L   t3          t2  G
                 / \                 / \
                t1  t2              t3  t4

Case 3 ==> the L is the righ child of P , RtlIsRightChild(Parent) ...

                         |                 |
                         G                 L
                        / \              /   \
                      t1   P            G      P
                          / \          / \    / \
                         L   t4  ==>  t1  t2 t3  t4
                        / \
                      t2   t3

In all these 3 cases the L is left node of parent P but we need to check P position also.
So we do:
- RtlIsRoot
- RtlIsLeftChild
- RtlIsRightChild
and check what position of P and of course there could be 3 cases:
- P is root
- P is left child
- P is right child (in case 3, P is right child of G)

Then we have next 3 cases when L is right node of parent P, // RtlIsRightChild(L)

 case 4 ==> if (RtlIsRoot(P)) {

           we have the following case

                 P                 L
                / \               / \
               t1  L             P   t3
                  / \           / \
                 t2  t3  ==>  t1   t2

case 5 => else if (RtlIsRightChild(P)) {
                      |                      |
                      G                      L
                     / \                    / \
                    t1   P                 P   t4
                        / \               / \
                       t2   L            G   t3
                           / \          /  \
                         t3   t4  ==>  t1   t2

case 6 => RtlIsLeftChild(P)
                     |                |
                     G                L
                    / \             /   \
                   P   t4          P      G
                  / \             / \    /  \
                 t1   L    ==>  t1   t2 t3   t4
                     / \
                   t2   t3


RtlSpay is called from the:
 - RtlDelete
 - RtlEnumerateGenericTable
 - RtlLookupElementGenericTableFull
 - RtlInsertElementGenericTableFull


To summarize

1. For New Insertions: The newly inserted node gets splayed to the root, making it the "most recently accessed" element
2. For Existing Nodes: Even if we just found an existing node, we splay it to the root because it was just "accessed"
This implements the core splay tree principle:
    - "Recently accessed elements should be near the root"
    - Every operation that touches a node (insert, lookup, delete, enumerate) splays that node to the root
    - This provides amortized O(log n) performance for sequences of operations
Pattern Across All Operations
You mentioned RtlSplay is called from:
    - RtlInsertElementGenericTableFull: Splay the inserted/found node
    - RtlLookupElementGenericTableFull: Splay the found node
    - RtlDelete: Splay the parent after deletion
    - RtlEnumerateGenericTable: Splay each enumerated node
The Result
Every time you:
    - Insert an element -> It becomes the new root
    - Look up an element -> It moves to the root
    - Delete an element -> Parent gets splayed
    - Enumerate elements-> Each visited element becomes root
This creates a self-optimizing tree where frequently accessed data automatically moves closer to the root, 
improving performance for usage patterns where recently accessed items are likely to be accessed again soon.

*/

{
    PRTL_SPLAY_LINKS L;
    PRTL_SPLAY_LINKS P;
    PRTL_SPLAY_LINKS G;


    L = Links;

    // This code is the main loop of the splay operation in a splay tree.

    while (!RtlIsRoot(L)) {

        P = RtlParent(L); // get the parrent of L
        G = RtlParent(P); // get grand parrent of L

        if (RtlIsLeftChild(L)) {

            if (RtlIsRoot(P)) {

                /*
				  Zig Rotation (Case 1)

                          P             L
                         / \          /   \
                        L   t3  ==>  t1    P
                       / \                / \
                      t1   t2           t2   t3
                */

     
                P->LeftChild = L->RightChild; // (P=b)
                if (P->LeftChild != NULL) // if b!=null
                {
                    P->LeftChild->Parent = P; // (L=P)
                }
   
                L->RightChild = P;
                P->Parent = L;

                //  L becaome root              

                L->Parent = L;

            } else if (RtlIsLeftChild(P)) {

                /*
                  we have the following case

                          |             |
                          G             L
                         / \          /  \
                        P   t4  ==>  t1   P
                       / \               / \
                      L   t3            t2  G
                     / \                   / \
                    t1  t2                t3  t4
                */

                //
                //  Connect P & b
                //

                P->LeftChild = L->RightChild;
                if (P->LeftChild != NULL) {P->LeftChild->Parent = P;}

                //
                //  Connect G & c
                //

                G->LeftChild = P->RightChild;
                if (G->LeftChild != NULL) {G->LeftChild->Parent = G;}

                //
                //  Connect L & Great GrandParent
                //

                if (RtlIsRoot(G)) {
                    L->Parent = L;
                } else {
                    L->Parent = G->Parent;
                    *(ParentsChildPointerAddress(G)) = L;
                }

                //
                //  Connect L & P
                //

                L->RightChild = P;
                P->Parent = L;

                //
                //  Connect P & G
                //

                P->RightChild = G;
                G->Parent = P;

            } else { // RtlIsRightChild(Parent)

                /*
                  we have the following case

                        |                 |
                        G                 L
                       / \              /   \
                      t1   P           G     P
                          / \         / \   /  \
                         L   t4  ==> t1  t2 t3  t4
                        / \
                      t2   t3
                */

                //
                //  Connect G & b
                //

                G->RightChild = L->LeftChild;
                if (G->RightChild != NULL) {G->RightChild->Parent = G;}

                //
                //  Connect P & c
                //

                P->LeftChild = L->RightChild;
                if (P->LeftChild != NULL) {P->LeftChild->Parent = P;}

                //
                //  Connect L & Great GrandParent
                //

                if (RtlIsRoot(G)) {
                    L->Parent = L;
                } else {
                    L->Parent = G->Parent;
                    *(ParentsChildPointerAddress(G)) = L;
                }

                //
                //  Connect L & G
                //

                L->LeftChild = G;
                G->Parent = L;

                //
                //  Connect L & P
                //

                L->RightChild = P;
                P->Parent = L;

            }

        } else { // RtlIsRightChild(L)

            if (RtlIsRoot(P)) {

                /*
                  we have the following case

                        P                L
                       / \              / \
                      t1   L           P   t3
                          / \         / \
                        t2   t3  ==> t1  t2
                */

                //
                //  Connect P & b
                //

                P->RightChild = L->LeftChild;
                if (P->RightChild != NULL) {P->RightChild->Parent = P;}

                //
                //  Connect P & L
                //

                L->LeftChild = P;
                P->Parent = L;

                //
                //  Make L the root
                //

                L->Parent = L;

            } else if (RtlIsRightChild(P)) {

                /*
                  we have the following case

                      |                    |
                      G                    L
                     / \                  / \
                    t1  P                P   t4
                       / \              / \
                      t2  L            G   t3
                         / \          / \
                        t3  t4  ==>  t1  t2
                */

                //
                //  Connect G & b
                //

                G->RightChild = P->LeftChild;
                if (G->RightChild != NULL) {G->RightChild->Parent = G;}

                //
                //  Connect P & c
                //

                P->RightChild = L->LeftChild;
                if (P->RightChild != NULL) {P->RightChild->Parent = P;}

                //
                //  Connect L & Great GrandParent
                //

                if (RtlIsRoot(G)) {
                    L->Parent = L;
                } else {
                    L->Parent = G->Parent;
                    *(ParentsChildPointerAddress(G)) = L;
                }

                //
                //  Connect L & P
                //

                L->LeftChild = P;
                P->Parent = L;

                //
                //  Connect P & G
                //

                P->LeftChild = G;
                G->Parent = P;

            } else { // RtlIsLeftChild(P)

                /*
                  we have the following case

                          |               |
                          G               L
                         / \            /   \
                        P   t4         P      G
                       / \            / \    / \
                      t1  L    ==>   t1  t2 t3  t4
                         / \
                        t2  t3
                */

                //
                //  Connect P & b
                //

                P->RightChild = L->LeftChild;
                if (P->RightChild != NULL) {P->RightChild->Parent = P;}

                //
                //  Connect G & c
                //

                G->LeftChild = L->RightChild;
                if (G->LeftChild != NULL) {G->LeftChild->Parent = G;}

                //
                //  Connect L & Great GrandParent
                //

                if (RtlIsRoot(G)) {
                    L->Parent = L;
                } else {
                    L->Parent = G->Parent;
                    *(ParentsChildPointerAddress(G)) = L;
                }

                //
                //  Connect L & P
                //

                L->LeftChild = P;
                P->Parent = L;

                //
                //  Connect L & G
                //

                L->RightChild = G;
                G->Parent = L;

            }
        }
    }

    return L;
}


/**
 * @brief Removes node from splay tree and splays parent to maintain tree balance.
 * 
 * Deletes the specified node from the splay tree using a three-case strategy
 * based on the number of children. For nodes with two children, swaps with
 * predecessor before deletion. Always splays the parent after removal.
 * 
 * @param[in] Links Pointer to the node to delete from the tree
 * 
 * @return Pointer to the new root of the tree, or NULL if tree becomes empty
 */
PRTL_SPLAY_LINKS
RtlDelete (
    PRTL_SPLAY_LINKS Links
    )

/*
    Deletes node from the splay tree. 

    Links - pointer to the node to delete
    Return pointer to the new root of the tree (or NULL if tree becomes empty)

    The Three-Case Deletion Strategy

    The function handles three distinct cases based on how many children the node has:
    Case 1: Node with Two Children (Hardest Case)
    Case 2: Leaf Node (No Children)
    Case 3: Node with One Child

*/

{
    PRTL_SPLAY_LINKS Predecessor;
    PRTL_SPLAY_LINKS Parent;
    PRTL_SPLAY_LINKS Child;

    PRTL_SPLAY_LINKS *ParentChildPtr;

    // Case 1. 

    //1.	Find the predecessor(largest value smaller than current node)
    //2.	Swap positions with the predecessor using SwapSplayLinks
    //3.	Transform the problem - now the original node has ≤1 child


    //    50  ← Has 2 children 
    //   /  \
    //  30   70
    // /  \
    //20   40
    //   /  \
    //  35   45

    //   Step 1: Handle two-children case
    //   •	Find predecessor of 50 = 45
    //   •	Swap 45 and 50 positions

    //   After swap:
    //        45  ← 45 is now root
    //       /  \
    //      30   70
    //     /  \
    //    20   40
    //       /  \
    //      35   50  ← 50 now has ≤1 child!

    //   Step 2: Handle simple case
    //   •	50 now has no children (leaf node)
    //   •	Disconnect 50 from parent (40)
    //   •	Splay parent (40)
    //   Final result:

    //        40  ← 40 became new root after splay
    //       /  \
    //      30   45
    //     /      \
    //    20       70
    //            /
    //           35

    // Node 40 becomes the root because it was the last accessed node during the deletion of 50, 
    // and splay trees automatically splay recently accessed nodes to the root.


    if ((RtlLeftChild(Links) != NULL) && (RtlRightChild(Links) != NULL)) {

        // Find the predecessor(largest value smaller than current node)
        // Swap positions with the predecessor using SwapSplayLinks    
        Predecessor = RtlSubtreePredecessor(Links);
        SwapSplayLinks(Predecessor, Links);
    }

    // Case 2, node has not children
    if ((RtlLeftChild(Links) == NULL) && (RtlRightChild(Links) == NULL)) {

        // return NULL if node is root

        if (RtlIsRoot(Links)) {

            return NULL;
        }

		// set parent's child pointer to NULL , splay the parent and make it the new root

        Parent = RtlParent(Links);

        ParentChildPtr = ParentsChildPointerAddress(Links);
        *ParentChildPtr = NULL;

        return RtlSplay(Parent);

    }

    //
    //  otherwise Links has one child.  If it is the root then make the child
    //  the new root, otherwise link together the child and parent, and splay
    //  the parent.  But first remember who our child is.
    //

	// Case 3, node has one child. 
    // Identify which child exists, either the left child OR the right child (but not both, since we're in the 
    // "one child" case).
    // Remember the child because:
    // 1. We're about to delete the parent node (Links)
    // 2. We need to reconnect the child to the grandparent
    // 3. We need the child pointer for later operations



    if (RtlLeftChild(Links) != NULL) {
        Child = RtlLeftChild(Links);
    } else {
        Child = RtlRightChild(Links);
    }

    // Subcase 3a: Node to delete is root
    // Make child the new root

    if (RtlIsRoot(Links)) {
        Child->Parent = Child;
        return Child;
    }

    //
    //  Links is not the root, so set link's parent child pointer to be
    //  the child and the set child's parent to be link's parent, and splay
    //  the parent.
    //

    // Sub-case 3b: Node to delete is not root

    ParentChildPtr = ParentsChildPointerAddress(Links);
    *ParentChildPtr = Child;    // Connect grandparent → child
    Child->Parent = Links->Parent;  // Connect child → grandparent

    return RtlSplay(RtlParent(Child));  // Splay the grandparent

}


/**
 * @brief Removes node from splay tree without splaying, preserving tree structure.
 * 
 * Deletes the specified node without performing splay operations, useful for
 * bulk deletions or when tree structure preservation is desired. Updates the
 * root pointer through the Root parameter.
 * 
 * @param[in] Links Pointer to the node to delete from the tree
 * @param[in,out] Root Pointer to root pointer that will be updated if root changes
 */
VOID
RtlDeleteNoSplay (
    PRTL_SPLAY_LINKS Links,
    PRTL_SPLAY_LINKS *Root
    )

    /*
         Deletes node from the splay tree without splaying which is not always needed.
         Updates root via pointer parameter

         Use RtlDeleteNoSplay when:
            - You're doing bulk operations
            - You don't want tree structure changes
            - You're probably batch deleting many nodes
            - You want predictable tree shape

        RtlDelete (with splay):
        Tree before:        Tree after (40 splayed to root):
             45                     40
            /  \                   /  \
           30   70                30   45
          /  \                   /      \
         20   40                20       70
             /  \                       /
            35   [deleted]             35

        RtlDeleteNoSplay (no splay):

        Tree before:        Tree after (structure preserved):
             45                     45
            /  \                   /  \
           30   70                30   70
          /  \                   /
         20   40                20
             /  \                 \
            35   [deleted]         35

    */

{
    PRTL_SPLAY_LINKS Predecessor;
    PRTL_SPLAY_LINKS Parent;
    PRTL_SPLAY_LINKS Child;

    PRTL_SPLAY_LINKS *ParentChildPtr;

 
    if ((RtlLeftChild(Links) != NULL) && (RtlRightChild(Links) != NULL)) {

        Predecessor = RtlSubtreePredecessor(Links);

        if (RtlIsRoot(Links)) {
            *Root = Predecessor;
        }

        SwapSplayLinks(Predecessor, Links);

    }

    if ((RtlLeftChild(Links) == NULL) && (RtlRightChild(Links) == NULL)) {

        if (RtlIsRoot(Links)) {

            *Root = NULL;

            return;
        }

        ParentChildPtr = ParentsChildPointerAddress(Links);
        *ParentChildPtr = NULL;

        return;
    }


    if (RtlLeftChild(Links) != NULL) {
        Child = RtlLeftChild(Links);
    } else {
        Child = RtlRightChild(Links);
    }

 
    if (RtlIsRoot(Links)) {
        Child->Parent = Child;

        *Root = Child;

        return;
    }

    ParentChildPtr = ParentsChildPointerAddress(Links);
    *ParentChildPtr = Child;
    Child->Parent = Links->Parent;

    return;
}


/**
 * @brief Finds successor within the node's right subtree only.
 * 
 * Locates the node with the smallest value greater than the specified node,
 * searching only within the right subtree rooted at the given node. Returns
 * NULL if no successor exists in the subtree.
 * 
 * @param[in] Links Pointer to the node whose subtree successor to find
 * 
 * @return Pointer to successor node within subtree, or NULL if none exists
 */
PRTL_SPLAY_LINKS
RtlSubtreeSuccessor (
    PRTL_SPLAY_LINKS Links
    )

    /*
        The RtlSubtreeSuccessor function finds the successor of a node within only its own subtree (not the entire tree). Let me explain what this means and how it differs from RtlRealSuccessor.

        What is a "Subtree Successor"?
        The subtree successor is the node with the smallest value that is still larger than the current node,
         but only looking within the subtree rooted at that node.
        Key difference: It only searches downward in the tree, not upward to ancestors.


        Visual Example
        Tree with node values:
                50
               /  \
              30   70  ← Links points to this node (70)
             /  \    \
            20   40   80
                /  \    \
               35   45   90
                        /
                       85

        Sorted order: 20, 30, 35, 40, 45, 50, 70, 80, 85, 90

        Example 1: RtlSubtreeSuccessor(70)
        - Node 70 has a right child (80)
        - Go right to 80, then go left as far as possible
        - 80 has no left child, so 80 is the subtree successor of 70

        Example 2: RtlSubtreeSuccessor(50)
        - Node 50 has a right child (70)
        - Go right to 70, then go left as far as possible
        - 70 has no left child, so 70 is the subtree successor of 50

        Example 3: RtlSubtreeSuccessor(45)
        - Node 45 has no right child
        - Return NULL (no subtree successor)

        Algorithm Walkthrough
        Let's trace through RtlSubtreeSuccessor(50):
        1. Start at node 50

                50  ← Starting here
               /  \
              30   70
             /  \
            20   40
                /  \
               35   45

        2. Check: Does 50 have a right child?
           YES → 70

        3. Go to right child (70)

                50
               /  \
              30   70  ← Now here
             /  \
            20   40
                /  \
               35   45

        4. Find leftmost node in this subtree
           while (RtlLeftChild(Ptr) != NULL)

           70 has no left child, so we stop at 70

        5. Return 70 as the subtree successor of 50

        **Purpose and Usage**
        RtlSubtreeSuccessor is primarily used in:
        1.	Tree deletion algorithms - Finding replacement nodes within a specific subtree
        2.	Subtree operations - When you only care about nodes below the current node
        3.	Local tree restructuring - Operations that don't need to consider the entire tree
    */

{
    PRTL_SPLAY_LINKS Ptr;


    // Step 1: Check if there's a right subtree
    if ((Ptr = RtlRightChild(Links)) != NULL) {

        // Step 2: Find the leftmost node in the right subtree
        while (RtlLeftChild(Ptr) != NULL) {
            Ptr = RtlLeftChild(Ptr);
        }

        return Ptr;

    }

    // Step 3: No right subtree = no subtree successors

    return NULL;

}


/**
 * @brief Finds predecessor within the node's left subtree only.
 * 
 * Locates the node with the largest value smaller than the specified node,
 * searching only within the left subtree rooted at the given node. Returns
 * NULL if no predecessor exists in the subtree.
 * 
 * @param[in] Links Pointer to the node whose subtree predecessor to find
 * 
 * @return Pointer to predecessor node within subtree, or NULL if none exists
 */
PRTL_SPLAY_LINKS
RtlSubtreePredecessor (
    PRTL_SPLAY_LINKS Links
    )

/*
    This function gives us the largest value that's still smaller than our original node.
    It is used in deletion function:
    - the predecessor is guaranteed to have at most one child (only a left child)
    - after swapping, the original node (now in predecessor's position) is easier to delete
    - the tree's binary search property is maintained because the predecessor's value fits perfectly in the original node's position

    The Swap Example Revisited
    Before deletion of 50:
         50  ← Want to delete (has 2 children)
        /  \
       30   70
      /  \
     20   40
        /  \
       35   45 ← Predecessor (has at most 1 child)

    After SwapSplayLinks(45, 50):
         45  ← 45 moved to root position
        /  \
       30   70
      /  \
     20   40
        /  \
       35   50 ← 50 moved here (now has 0 children!)

    Now 50 is easy to delete because it has no children, and 45 maintains the tree's order because it was the largest value smaller than 50.

*/

{
    PRTL_SPLAY_LINKS Ptr;

    //Step 1: Go to Left Subtree
    //    if ((Ptr = RtlLeftChild(Links)) != NULL)
    //        -	All values in the left subtree are smaller than current node
    //        - So the predecessor must be in the left subtree

    if ((Ptr = RtlLeftChild(Links)) != NULL) {
        // Step 2 : Find Rightmost Node in Left Subtree
        // In the left subtree, go as far right as possible
        // The rightmost node has the largest value in that subtree
        // This gives us the largest value that's still smaller than our original node

        // Visual Walkthrough
        //    To find predecessor of 50:
        //       50  ← Finding predecessor of this
        //      / \
        //     30   70
        //    / \
        //   20   40  ← Go to left subtree
        //  / \
        // 35  45 ← Keep going right : 40 → 45

        // 1. Start at 50
        // 2. Go left to 30 (left subtree contains smaller values)
        // 3. From 30, go right to 40 (find largest in left subtree)
        // 4. From 40, go right to 45 (keep going right)
        // 5. 45 has no right child → 45 is the predecessor of 50

        while (RtlRightChild(Ptr) != NULL) {
            Ptr = RtlRightChild(Ptr);
        }

        return Ptr;
    }

    // we don't have a subtree predecessor 
    return NULL;
}


/**
 * @brief Finds the next node in sorted order across the entire tree.
 * 
 * Locates the successor of the specified node by searching the entire tree
 * structure. First checks the right subtree, then traverses up the tree to
 * find the first ancestor where the path came from the left side.
 * 
 * @param[in] Links Pointer to the node whose successor to find
 * 
 * @return Pointer to successor node in the entire tree, or NULL if node is maximum
 */
PRTL_SPLAY_LINKS
RtlRealSuccessor (
    PRTL_SPLAY_LINKS Links
    )

    /*
        The RtlRealSuccessor function is designed to find the successor of a node within the entire tree (not just a subtree). 
        Let me explain what this means and how it works:
        What is a "Real Successor"? The successor of a node is the node with the smallest value that is still larger than 
        the current node's value - essentially the "next" node in sorted order. "Real" means it searches the entire tree, not just the 
        subtree rooted at the given node (unlike RtlSubtreeSuccessor).

        Example:
        Tree with values:
             50
            /  \
           30   70
          /  \    \
         20   40   80
             /  \
            35   45

        Sorted order: 20, 30, 35, 40, 45, 50, 70, 80

	        - RealSuccessor of 30 = 35 (next larger value in entire tree)
            - RealSuccessor of 45 = 50 (next larger value in entire tree)
            - RealSuccessor of 70 = 80 (next larger value in entire tree)
	        - RealSuccessor of 80 = NULL (largest value, no successor)

        How the Algorithm Works
        The function uses a two-step approach:
        
        Step 1: Check Right Subtree
        if ((Ptr = RtlRightChild(Links)) != NULL) {
            while (RtlLeftChild(Ptr) != NULL) {
                Ptr = RtlLeftChild(Ptr);
            }
            return Ptr;
        }

        If the node has a right child, the successor is the leftmost node in the right subtree.
        Example: For node 30, go right to 40, then left to 35.

        Step 2: Search Up the Tree
        Ptr = Links;
        while (RtlIsRightChild(Ptr)) {
            Ptr = RtlParent(Ptr);
        }

        if (RtlIsLeftChild(Ptr)) {
            return RtlParent(Ptr);
        }

        return NULL;

        If there's no right child, move up the tree until you find an ancestor where you're coming from the left side.

        Example: For node 45:
        1.	45 has no right child
        2.	Go up to parent 40 (45 is right child of 40)
        3.	Go up to parent 30 (40 is right child of 30)
        4.	Go up to parent 50 (30 is left child of 50) ✓
        5.	Return 50 as the successor


        Usage
        This function is primarily used in:
        1.	Tree enumeration - Walking through all nodes in sorted order
        2.	Range queries - Finding the next element after a given key
        3.	Iterator operations - Moving to the next element in sequence
        The function is essential for implementing ordered traversal of the splay tree, allowing you to iterate through all elements in ascending order efficiently.

    */

{
    PRTL_SPLAY_LINKS Ptr;

    // Step 1: Check Right Subtree, see above
 
    if ((Ptr = RtlRightChild(Links)) != NULL) {

        while (RtlLeftChild(Ptr) != NULL) {
            Ptr = RtlLeftChild(Ptr);
        }

        return Ptr;

    }

	// Step 2: Search Up the Tree, see above

    Ptr = Links;
    while (RtlIsRightChild(Ptr)) {
        Ptr = RtlParent(Ptr);
    }

    if (RtlIsLeftChild(Ptr)) {
        return RtlParent(Ptr);
    }

    //  we don't have a real successor 

    return NULL;

}


/**
 * @brief Finds the previous node in sorted order across the entire tree.
 * 
 * Locates the predecessor of the specified node by searching the entire tree
 * structure. First checks the left subtree, then traverses up the tree to
 * find the first ancestor where the path came from the right side.
 * 
 * @param[in] Links Pointer to the node whose predecessor to find
 * 
 * @return Pointer to predecessor node in the entire tree, or NULL if node is minimum
 */
PRTL_SPLAY_LINKS
RtlRealPredecessor (
    PRTL_SPLAY_LINKS Links
    )
{
    /*
        Finds the predecessor of a node within the entire tree (not just a subtree).
        What is a "Real Predecessor"? The predecessor of a node is the node with the largest value that is still smaller than 
        the current node's value - essentially the "previous" node in sorted order.
        "Real" means it searches the entire tree, not just the subtree rooted at the given node (unlike RtlSubtreePredecessor).

        Example:
        Tree with values:
             50
            /  \
           30   70
          /  \    \
         20   40   80
             /  \
            35   45

        Sorted order: 20, 30, 35, 40, 45, 50, 70, 80
        So:
            - RealPredecessor of 50 = 45 (previous smaller value in entire tree)
            - RealPredecessor of 40 = 35 (previous smaller value in entire tree)
            - RealPredecessor of 30 = 20 (previous smaller value in entire tree)
            - RealPredecessor of 20 = NULL (smallest value, no predecessor)

        The function uses a two-step approach:
        Step 1: Check Left Subtree:
        If the node has a left child, the predecessor is the rightmost node in the left subtree.
        Example: For node 50, go left to 30, then right to 40, then right to 45.

        Step 2: Search Up the Tree
        If there's no left child, move up the tree until you find an ancestor where you're coming from the right side.
        Example: For node 20:
        1.	20 has no left child
        2.	Go up to parent 30 (20 is left child of 30)
        3.	30 is left child of 50, so continue up
        4.	50 has no parent or we've reached root
        5.	Return NULL (no predecessor)

        Visual Walkthrough for Node 45
        Step 1: Does 45 have a left child? NO
        Step 2: Search up the tree
        Tree:           Path taken:
            50              50
           /  \            /
          30   70    =>   30  ← Stop here (45 → 40 → 30)
         /  \            /  \
        20   40         20   40
            /  \             /  \
           35   45  ← Start  35   45

        1.	Start at 45
        2.	Go up to parent 40 (45 is right child of 40) ✓
        3.	Return 40 as the predecessor
        For node 35:
        1.	Start at 35
        2.	Go up to parent 40 (35 is left child of 40)
        3.	Go up to parent 30 (40 is right child of 30) ✓
        4.	Return 30 as the predecessor

        This function is primarily used in:
        1.	Tree enumeration - Walking through all nodes in reverse sorted order
        2.	Range queries - Finding the previous element before a given key
        3.	Iterator operations - Moving to the previous element in sequence
        4.	Deletion algorithms - Finding replacement nodes when deleting
        The function is essential for implementing ordered traversal of the splay tree, allowing you to iterate through all elements in descending order efficiently.
    */

    PRTL_SPLAY_LINKS Ptr;

    // Step 1: Check Left Subtree
    if ((Ptr = RtlLeftChild(Links)) != NULL) {

        while (RtlRightChild(Ptr) != NULL) {
            Ptr = RtlRightChild(Ptr);
        }

        return Ptr;
    }

    // Step 2: Search Up the Tree
    Ptr = Links;
    while (RtlIsLeftChild(Ptr)) {
        Ptr = RtlParent(Ptr);
    }

    if (RtlIsRightChild(Ptr)) {
        return RtlParent(Ptr);
    }

    //  there is no real predecessor
    return NULL;
}


/**
 * @brief Exchanges positions of two nodes in the splay tree structure.
 * 
 * Completely swaps the tree positions of two nodes while maintaining all
 * parent-child relationships and tree integrity. Handles various node
 * relationship cases including independent nodes and parent-child pairs.
 * 
 * @param[in,out] Link1 First node to swap
 * @param[in,out] Link2 Second node to swap
 */
VOID
SwapSplayLinks (
    PRTL_SPLAY_LINKS Link1,
    PRTL_SPLAY_LINKS Link2
    )

{ 
    /*
        Swaps the positions of two nodes in a splay tree while maintaining all the 
        tree structure relationships. This is a critical operation used during splay tree deletion.

        What This Function Does
        Purpose: Completely exchange the positions of Link1 and Link2 in the tree, including:
            - their parent-child relationships
            - their positions relative to their parents
            - their own children
            - all pointer connections  

        Swapping two nodes in a tree is complex because they can have various relationships:
        Case 1: Independent nodes (Link1 and Link2)
            Parent1        Parent2
               |              |
             Link1          Link2
             /   \          /   \
           LC1   RC1      LC2   RC2

        Case 2: Parent-child relationship
            Parent1
               |
             Link2 (parent)
             /   \
           Link1  RC2 (child)
           /   \
         LC1   RC1

        Case 3: One is root
             Link2 (root)
             /   \
           Link1  RC2
           /   \
         LC1   RC1
    */
    
    PRTL_SPLAY_LINKS *Parent1ChildPtr;
    PRTL_SPLAY_LINKS *Parent2ChildPtr;
    
    // Step 1: Normalize the inputs
    // This ensures Link2 is always the "higher" node(root or parent) and Link1 is the "lower" node(child), simplifying the logic.
    // Also as we will see later this leads to only 4 cases

    if ((RtlIsRoot(Link1)) || (RtlParent(Link2) == Link1)) {
        SwapPointers(Link1, Link2);
    }

    // Step 2: Handle four distinct cases
    // Four possible scenarios :
    // Case 1. Link1 is NOT a child of Link2 AND Link2 is NOT root(independent nodes)
    // Case 2. Link1 is NOT a child of Link2 AND Link2 IS root(Link2 is root, Link1 elsewhere)
    // Case 3. Link1 IS a child of Link2 AND Link2 is NOT root(parent - child, Link2 has parent)
    // Case 4. Link1 IS a child of Link2 AND Link2 IS root(parent - child, Link2 is root)

    if (RtlParent(Link1) != Link2) {

        if (!RtlIsRoot(Link2)) {
            //  Case 1, indepedent nodes:
            
            // Get pointers to parent's child pointers
            Parent1ChildPtr = ParentsChildPointerAddress(Link1);
            Parent2ChildPtr = ParentsChildPointerAddress(Link2);

            // Swap what the parents point to so parents will point to swapped children
            SwapPointers(*Parent1ChildPtr, *Parent2ChildPtr);
            
            // Swap the parent pointers so links will point to swapped parents
            SwapPointers(Link1->Parent, Link2->Parent);

        } else {

            // Case 2

            Parent1ChildPtr = ParentsChildPointerAddress(Link1);
            *Parent1ChildPtr = Link2;

            Link2->Parent = Link1->Parent;
            Link1->Parent = Link1;

        }

        // Swap Child Pointers (Common Step)

        SwapPointers(Link1->LeftChild, Link2->LeftChild);
        SwapPointers(Link1->RightChild, Link2->RightChild);

    } else { 
        // Case 3 and Case 4, Link1 IS a child of Link2 

        if (!RtlIsRoot(Link2)) {

            // Case 3

            //  Step 1: Get parent's child pointer address         
            //    GrandParent
            //        |
            //        ↓
            //   [ChildPtr] ← Parent2ChildPtr points to this address
            //        |
            //      Link2
            //      /   \
            //   Link1   RC2
            //   /   \
            // LC1   RC1

            //Parent2ChildPtr = &(GrandParent->LeftChild)  // or RightChild

            Parent2ChildPtr = ParentsChildPointerAddress(Link2);
            
            // Step 2: Make GrandParent point to Link1
            *Parent2ChildPtr = Link1;

            // Step 3: Make Link1 point to GrandParent
            Link1->Parent = Link2->Parent;
            // Resultant tree
            //    GrandParent ←─────────── Link1->Parent
            //        |                      ↑
            //        ↓                      |
            //      Link1                 Link1
            //      /   \                 /   \
            //    LC1  RC1              LC1  RC1
            // Link1 now considers GrandParent as its parent!

        } else {
            // Case 4

            // Initial state
            //     Link2 (root - no parent)
            //     /   \
            //  Link1   RC2
            //  /   \
            //LC1   RC1            

			// Simple step: Make Link1 the new root, resultant tree:
            //      Link1 (now root - points to itself)
            //      /   \
            //    LC1  RC1

            //Link1->Parent = Link1  ← Self-reference indicates root status
            Link1->Parent = Link1;
        }


        //  Case 3 and Case 4 common steps
        // Link1 gets Link2's children (Link1 and RC2)
        //  - Link2 gets Link1's children (LC1 and RC1)
        //  - After swapping child pointers :
        //  Link2                    Link1
        //  /   \                    /     \
        //LC1   RC1                Link1   RC2  ← PROBLEM!
        // The Problem: Link1 now points to itself as a child! This creates a circular reference.

        SwapPointers(Link1->LeftChild, Link2->LeftChild);
        SwapPointers(Link1->RightChild, Link2->RightChild);


        // Replace the self-reference with Link2:
        if (Link1->LeftChild == Link1) {
            Link1->LeftChild = Link2;
        } else {
            Link1->RightChild = Link2;
        }

        // Now:
        //   Link1
        //  /     \
        // Link2  RC2  ← Fixed: Link1 points to Link2

    }

    //  Case 1, 2, 3, 4 final steps:
    if (Link1->LeftChild  != NULL) {Link1->LeftChild->Parent  = Link1;}
    if (Link1->RightChild != NULL) {Link1->RightChild->Parent = Link1;}
    if (Link2->LeftChild  != NULL) {Link2->LeftChild->Parent  = Link2;}
    if (Link2->RightChild != NULL) {Link2->RightChild->Parent = Link2;}
}
