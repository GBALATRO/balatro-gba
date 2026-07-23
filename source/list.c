/**
 * @file list.c
 *
 * @brief List functions implementation.
 */

#include "list.h"

#include "pool.h"

#include <stdbool.h>

/**
 * Remove a node from a list.
 *
 * Remove a @ref ListNode from a @ref List. There are no checks to ensure that the
 * passed `node` is actually part of the passed `list`. Handle with care.
 * This is used with the @ref ListItr specifically.
 *
 * @param list pointer to a @ref List
 * @param node pointer to a @ref ListNode
 */
static void s_list_remove_node(List* list, ListNode* node);

/**
 * Get the next @ref ListNode in a @ref ListItr
 *
 * Note: Use of this function outside of testing is strongly discouraged. Unless
 * you really want to access the @ref ListNode itself, it's preferred to just use
 * @ref list_itr_next .
 *
 * @param itr pointer to the @ref ListItr
 *
 * @return A pointer to the @ref ListNode in the itr, otherwise return NULL.
 */
static ListNode* s_list_itr_node_next(ListItr* itr);

List list_init(void)
{
    List list = LIST_DEFAULT;
    return list;
}

void list_clear(List* list)
{
    if (list_is_empty(list))
        return;

    ListItr itr = list_itr_create(list);
    ListNode* ln;

    while ((ln = s_list_itr_node_next(&itr)))
    {
        POOL_FREE(ListNode, ln);
    }

    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
}

bool list_is_empty(const List* list)
{
    return list->len == 0;
}

void list_push_front(List* list, void* data)
{
    ListNode* node = POOL_GET(ListNode);

    node->data = data;
    node->prev = NULL;
    node->next = list->head;

    if (list_is_empty(list))
    {
        list->tail = node;
    }
    else
    {
        list->head->prev = node;
    }

    list->head = node;

    list->len++;
}

void list_push_back(List* list, void* data)
{
    ListNode* node = POOL_GET(ListNode);
    node->data = data;
    node->prev = list->tail;
    node->next = NULL;

    if (list_is_empty(list))
    {
        list->head = node;
    }
    else
    {
        list->tail->next = node;
    }

    list->tail = node;

    list->len++;
}

void list_insert(List* list, void* data, unsigned int idx)
{
    if (idx >= list->len)
    {
        list_push_back(list, data);
        return;
    }

    if (idx == 0)
    {
        list_push_front(list, data);
        return;
    }

    // After the above two checks the index is guaranteed to be inbetween the
    // `head` and `tail` of the `list`. This means the actual list doesn't need
    // to modify it's head and tail, only it's length. Simplifying the code below:

    unsigned int curr_idx = 0;
    ListItr itr = list_itr_create(list);
    ListNode* ln;

    while ((ln = s_list_itr_node_next(&itr)))
    {
        if (idx == curr_idx++)
        {
            ListNode* node = POOL_GET(ListNode);
            node->prev = ln->prev;
            node->next = ln;
            ln->prev->next = node;
            ln->prev = node;
            node->data = data;
            list->len++;
            return;
        }
    }
}

bool list_swap(ListNode* node_a, ListNode* node_b)
{
    if (node_a == NULL || node_b == NULL)
        return false;

    void* tmp = node_a->data;
    node_a->data = node_b->data;
    node_b->data = tmp;

    return true;
}

bool list_swap_at_idx(List* list, unsigned int idx_a, unsigned int idx_b)
{
    if (idx_a >= list->len || idx_b >= list->len)
        return false;
    if (idx_a == idx_b)
        return true; // swapping with yourself isn't technically an error

    unsigned int curr_idx = 0;
    unsigned int max_idx = idx_a > idx_b ? idx_a : idx_b;
    ListNode* node_a = NULL;
    ListNode* node_b = NULL;

    ListItr itr = list_itr_create(list);
    ListNode* ln;

    do
    {
        ln = s_list_itr_node_next(&itr);
        if (idx_a == curr_idx)
        {
            node_a = ln;
            continue;
        }
        if (idx_b == curr_idx)
        {
            node_b = ln;
            continue;
        }
    } while (max_idx != curr_idx++);

    // Just swap the data pointers
    void* tmp = node_a->data;
    node_a->data = node_b->data;
    node_b->data = tmp;

    return true;
}

static void s_list_remove_node(List* list, ListNode* node)
{
    if (node->prev && !node->next) // end of list
    {
        node->prev->next = NULL;
        list->tail = node->prev;
    }
    else if (node->prev && node->next) // somewhere in between
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    else if (node->next && !node->prev) // beginning of list
    {
        node->next->prev = NULL;
        list->head = node->next;
    }
    else if (!node->prev && !node->next) // only element in list
    {
        list->head = NULL;
        list->tail = NULL;
    }

    POOL_FREE(ListNode, node);

    list->len--;
}

int list_get_len(const List* list)
{
    return list->len;
}

void* list_get_at_idx(List* list, unsigned int idx)
{
    if (idx >= list_get_len(list))
        return NULL;

    int curr_idx = 0;
    ListItr itr = list_itr_create(list);
    void* data = NULL;

    while ((data = list_itr_next(&itr)))
    {
        if (idx == curr_idx++)
            return data;
    }

    return NULL;
}

bool list_remove_at_idx(List* list, unsigned int idx)
{
    if (idx >= list_get_len(list))
        return false;

    int len = 0;
    ListItr itr = list_itr_create(list);
    ListNode* ln;

    while ((ln = s_list_itr_node_next(&itr)))
    {
        if (idx == len++)
        {
            s_list_remove_node(list, ln);
            return true;
        }
    }
    return false;
}

ListItr list_itr_create(List* list)
{
    ListItr itr = {
        .list = list,
        .next_node = !list_is_empty(list) ? list->head : NULL,
        .current_node = NULL,
        .direction = LIST_ITR_FORWARD,
    };

    return itr;
}

ListItr rev_list_itr_create(List* list)
{
    ListItr itr = {
        .list = list,
        .next_node = !list_is_empty(list) ? list->tail : NULL,
        .current_node = NULL,
        .direction = LIST_ITR_REVERSE,
    };

    return itr;
}

void* list_itr_next(ListItr* itr)
{
    ListNode* ln = s_list_itr_node_next(itr);
    return ln ? ln->data : NULL;
}

static ListNode* s_list_itr_node_next(ListItr* itr)
{
    if (!itr->next_node)
        return NULL;

    itr->current_node = itr->next_node;

    ListNode* ln = itr->next_node;
    ListNode* next_itr_node = (itr->direction == LIST_ITR_FORWARD) ? ln->next : ln->prev;

    if (next_itr_node)
    {
        itr->next_node = next_itr_node;
        return ln;
    }

    itr->next_node = NULL;
    return ln;
}

void list_itr_remove_current_node(ListItr* itr)
{
    if (!itr || !itr->current_node)
        return;
    ListNode* tmp_prev = itr->current_node->prev;
    s_list_remove_node(itr->list, itr->current_node);
    itr->current_node = tmp_prev;
}

bool list_remove_data(List* list, void* data)
{
    ListItr itr = list_itr_create(list);
    ListNode* ln;

    while ((ln = s_list_itr_node_next(&itr)))
    {
        if (ln->data == data)
        {
            s_list_remove_node(list, ln);
            return true;
        }
    }

    return false;
}
