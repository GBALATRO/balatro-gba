/**
 * @file list.h
 *
 * @brief A doubly-linked list
 *
 * List Implementation
 * ===================
 *
 *  - This @ref List operates as a linked list @ref ListNodes. It operates as a regular
 * doubly-linked list but doesn't allocate memory and rather gets @ref ListNodes from a pool.
 */
#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

/**
 * @def MAX_LIST_NODES
 * @brief Number of reserved list nodes.
 *
 * Number of list nodes available from the pool of @ref ListNode . This should
 * be set to to the maximum number of list nodes needed at once.
 */
#define MAX_LIST_NODES 128

/**
 * @brief Default list declaration for empty lists
 */
// clang-format off
#define LIST_DEFAULT { .head = NULL, .tail = NULL, .len = 0 }
// clang-format on

typedef struct ListNode ListNode;

/**
 * @brief A single entry in a @ref List
 */
struct ListNode
{
    /**
     * @brief The previous @ref ListNode in the associated @ref List, NULL if at the `head` of the
     * list
     */
    ListNode* prev;

    /**
     * @brief The next @ref ListNode in the associated @ref List, NULL if at the `tail` of the list
     */
    ListNode* next;

    /**
     * @brief Pointer to generic data stored in this node
     */
    void* data;
};

/**
 * @brief A doubly-linked list
 */
typedef struct List
{
    /**
     * @brief The first entry in the list
     */
    ListNode* head;

    /**
     * @brief The last entry in the list
     */
    ListNode* tail;

    /**
     * @brief Number of elements in list
     */
    int len;
} List;

/**
 * @brief @ref ListItr direction
 */
enum ListItrDirection
{
    LIST_ITR_FORWARD,
    LIST_ITR_REVERSE,
};

/**
 * @brief An iterator into a list
 */
typedef struct
{
    /**
     * @brief A pointer to the @ref List this is iterating through
     */
    List* list;

    /**
     * @brief The next node in the list
     */
    ListNode* next_node;

    /**
     * @brief The current node in the list iterator
     *
     * The node of the most recently returned data from  @ref list_itr_next() .
     */
    ListNode* current_node;

    /**
     * @brief The direction of the iterator
     */
    enum ListItrDirection direction;
} ListItr;

/**
 * Initialize a list.
 *
 * Set the values of a list to default.
 *
 * If using this function to reset a list, the list must be freed with @ref list_clear to ensure the
 * list's nodes are deleted properly.
 *
 * @return A @ref List with head and tail reset.
 */
List list_init(void);

/**
 * Clear a list.
 *
 * Go through the list and free each node and set the `head` and `tail` to `NULL`.
 * Note, it doesn't "free" the data at the node.
 *
 * @note To reset an existing list to default values, first call `list_clear` then @ref list_init
 *
 * @param list pointer to a @ref List to clear
 */
void list_clear(List* list);

/**
 * Check if a list is empty
 *
 * @param list pointer to a @ref List
 *
 * @return `true` if the `list` is empty, `false` otherwise.
 */
bool list_is_empty(const List* list);

/**
 * Prepend an entry to the `head` of a @ref list
 *
 * @param list pointer to a @ref List
 * @param data pointer to data to put into the @ref List
 */
void list_push_front(List* list, void* data);

/**
 * Append an entry to the `tail` of a @ref list
 *
 * @param list pointer to a @ref List
 * @param data pointer to data to put into the @ref List
 */
void list_push_back(List* list, void* data);

/**
 * Insert data into a @ref List a specific index
 *
 * If the index specified is larger than the length of the list
 * it will @ref list_push_back() the data instead;
 *
 * Performs the following operation:
 *
 *                    ┌─────┐
 *                    │ node│
 *                    └─────┘
 *          ┌─────┐   ┌─────┐   ┌─────┐
 *          │idx-1│◄─►│ idx │◄─►│idx+1│
 *          └─────┘   └─────┘   └─────┘
 *
 *  1. Set new `node` `prev` to the node at idx - 1
 *  2. Set new `node` `next` to the node at idx
 *  3. Set node at idx - 1 `next` to new `node`
 *  4. Set node at idx `prev` to the new `node`
 *
 * Result:
 *
 *     ┌─────┐   ┌─────┐   ┌─────┐   ┌─────┐
 *     │idx-1│◄─►│ node│◄─►│ idx │◄─►│idx+1│
 *     └─────┘   └─────┘   └─────┘   └─────┘
 *
 * Finally, the list is now updated with new `node` now at the labeled idx:
 *
 *     ┌─────┐   ┌─────┐   ┌─────┐   ┌─────┐
 *     │idx-1│◄─►│ idx │◄─►│idx+1│◄─►│idx+2│
 *     └─────┘   └─────┘   └─────┘   └─────┘
 *
 * @param list pointer to a @ref List
 * @param data pointer to data to put into the @ref List
 * @param idx desired index to insert
 */
void list_insert(List* list, void* data, unsigned int idx);

/**
 * @brief Swaps the data of two ListNodes.
 *
 * It is not imperative that the two nodes belong to the same List, but since this is mainly used
 * when sorting a List they will most likely be anyway.
 *
 * @param node_a pointer to the first ListNode, cannot be **NULL*
 * @param node_b pointer to the second ListNode, cannot be **NULL*
 * @return true if the data could be swapped, false if either pointer was **NULL**
 */
bool list_swap(ListNode* node_a, ListNode* node_b);

/**
 * Swap the data pointers at the specified indices of a @ref List
 *
 * If either indices are larger than the length of the list, return false.
 *
 * @param list pointer to a @ref List
 * @param idx_a desired index to swap with idx_b
 * @param idx_b desired index to swap with idx_a
 *
 * @return true if successful, false otherwise
 */
bool list_swap_at_idx(List* list, unsigned int idx_a, unsigned int idx_b);

/**
 * Get a List's node at the specified index
 *
 * @param list pointer to a @ref List
 * @param idx index of the desired @ref ListNode in the list
 *
 * @return a pointer to the data at the index of the list, or NULL if out-of-bounds
 */
void* list_get_at_idx(List* list, unsigned int idx);

/**
 * Remove a List's node at the specified index
 *
 * @param list pointer to a @ref List
 * @param idx index of the desired @ref ListNode in the list
 *
 * @return `true` if successfully removed, `false` if out-of-bounds
 */
bool list_remove_at_idx(List* list, unsigned int idx);

/**
 * Remove a List's node with the matching pointer
 *
 * @param list pointer to a @ref List
 * @param data pointer to data in node in list
 *
 * @return `true` if successfully removed, `false` otherwise
 *
 * @note When working with @ref ListItr, use @ref list_itr_remove_current_node()
 */
bool list_remove_data(List* list, void* data);

/**
 * Get the number of elements in a @ref List
 *
 * @param list pointer to a @ref List
 *
 * @return The number of elements in the list
 */
int list_get_len(const List* list);

/**
 * Declare a @ref ListItr
 *
 * @param list pointer to a @ref List
 *
 * @return A new @ref ListItr
 */
ListItr list_itr_create(List* list);

/**
 * Declare a reverse @ref ListItr
 *
 * @param list pointer to a @ref List
 *
 * @return A new reverse @ref ListItr
 */
ListItr rev_list_itr_create(List* list);

/**
 * Get the next data entry in a @ref ListItr
 *
 * @param itr pointer to the @ref ListItr
 *
 * @return A pointer to the data pointer at the next @ref ListNode if valid, otherwise return NULL.
 */
void* list_itr_next(ListItr* itr);

/**
 * Remove the current @ref ListNode from the iterator.
 *
 * The "current node" corresponds to the list node associated with the
 * most recently returned valu from @ref list_itr_next()
 *
 * @param itr pointer to the @ref ListItr
 *
 * @note When working with @ref ListItr, use this and not @ref list_remove_at() as it will
 * "break" the iterator.
 */
void list_itr_remove_current_node(ListItr* itr);

#endif
