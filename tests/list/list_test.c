#include "list.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// As simple as it gets, just needs to be initialized correctly
// - list_init
// - list_is_empty
// - list_get_len
// - list_clear
void create_and_clear_list(void)
{
    List my_cool_list = list_init();

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_get_len(&my_cool_list) == 0);
    assert(list_is_empty(&my_cool_list));

    list_clear(&my_cool_list);

    assert(list_is_empty(&my_cool_list));
}

// Push back one entry, make sure it looks as expected
// tests:
// - list_init
// - list_push_back
// - list_is_empty
// - list_get_len
// - list_get_at_idx
// - list_clear
void push_back_one_entry(void)
{
    List my_cool_list = list_init();

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_get_len(&my_cool_list) == 0);
    assert(list_is_empty(&my_cool_list));

    int test_data = 1337;

    list_push_back(&my_cool_list, &test_data);

    // should have data and be the same head/tail
    assert(my_cool_list.head != NULL);
    assert(my_cool_list.tail != NULL);
    assert(my_cool_list.head == my_cool_list.tail);
    assert(list_get_len(&my_cool_list) == 1);

    // pointer should be the same
    assert(my_cool_list.head->data == &test_data);
    // and consequently the value should be the same
    assert(*(int*)(my_cool_list.head->data) == test_data);
    assert(list_get_at_idx(&my_cool_list, 0) == &test_data);
    assert(list_get_at_idx(&my_cool_list, 1) == NULL);
    assert(!list_is_empty(&my_cool_list));
    // clear the list
    list_clear(&my_cool_list);

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_is_empty(&my_cool_list));
}

// Push front one entry, make sure it looks as expected
// - list_init
// - list_push_front
// - list_is_empty
// - list_get_len
// - list_get_at_idx
// - list_clear
void push_front_one_entry(void)
{
    List my_cool_list = list_init();

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_get_len(&my_cool_list) == 0);
    assert(list_is_empty(&my_cool_list));

    int test_data = 1337;

    list_push_front(&my_cool_list, &test_data);

    // should have data and be the same head/tail
    assert(my_cool_list.head != NULL);
    assert(my_cool_list.tail != NULL);
    assert(my_cool_list.head == my_cool_list.tail);
    assert(list_get_len(&my_cool_list) == 1);
    assert(!list_is_empty(&my_cool_list));

    // pointer should be the same
    assert(my_cool_list.head->data == &test_data);
    // and consequently the value should be the same
    assert(*(int*)(my_cool_list.head->data) == test_data);
    assert(list_get_at_idx(&my_cool_list, 0) == &test_data);
    assert(list_get_at_idx(&my_cool_list, 1) == NULL);
    // clear the list
    list_clear(&my_cool_list);

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_is_empty(&my_cool_list));
}

// This is meant to be a full test of every function including the use of
// the ListItr iterator
// 
// Push back three entries. To cover the 3 states of nodes outside of single node
// lists. 3 states being, node at the front, node at the middle, and node at the end
//
// Afterwards remove one entry with `list_remove_at_idx` and then remove another
// with `list_remove_node`
//
// Lastly, push to the front the same 3 entries and verify the state of the list
// before detroying.
//
// - list_init
// - list_push_front
// - list_is_empty
// - list_get_len
// - list_get_at_idx
// - list_remove_at_idx
// - list_itr_create
// - list_itr_next
// - list_clear
// - list_itr_remove_node_current;
void push_back_three_remove_push_front_three_entries(void)
{
    List my_cool_list = list_init();

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);

    int test_data[3] = {0, 1, 2};

    list_push_back(&my_cool_list, &test_data[0]);

    // Make sure with one entry it looks right
    assert(my_cool_list.head == my_cool_list.tail);

    list_push_back(&my_cool_list, &test_data[1]);

    // Now with two it should be different
    assert(my_cool_list.head != my_cool_list.tail);

    list_push_back(&my_cool_list, &test_data[2]);

    // 3 entries?
    assert(list_get_len(&my_cool_list) == 3);

    // now use an iterator to examine each node
    int* data;

    ListItr list_itr = list_itr_create(&my_cool_list);

    int itr = 0;

    ListNode* prev_ln = NULL;

    while((data = list_itr_next(&list_itr)))
    {
        assert(data != NULL);
        assert(data == &test_data[itr]);
        ListNode* ln = list_itr.current_node;
        switch(itr)
        {
            case 0:
                assert(ln->prev == NULL);
                assert(ln->next != NULL);
                assert(ln->next->prev == ln);
                break;
            case 1:
                assert(ln->prev == prev_ln);
                assert(ln->prev != NULL);
                assert(ln->next != NULL);
                assert(ln->next->prev == ln);
                assert(ln->prev->next == ln);
                break;
            case 2:
                assert(ln->prev != NULL);
                assert(ln->next == NULL);
                assert(ln->prev->next == ln);
                assert(ln->prev == prev_ln);
                break;
            default:
                assert(false); // shouldn't get here
                break;
        }
        itr++;
        prev_ln = ln;
    }

    // remove the middle entry
    assert(list_remove_at_idx(&my_cool_list, 1));
    // can't remove entry 2 now (doesn't exist)
    assert(!list_remove_at_idx(&my_cool_list, 2));
    assert(list_get_len(&my_cool_list) == 2);

    list_itr = list_itr_create(&my_cool_list);
    data = NULL;
    itr = 0;
    prev_ln = NULL;
    while((data = list_itr_next(&list_itr)))
    {
        assert(data != NULL);
        ListNode* ln = list_itr.current_node;
        switch(itr)
        {
            case 0:
                assert(ln->prev == NULL);
                assert(ln->next != NULL);
                assert(ln->next->prev == ln);
                break;
            case 1:
                assert(ln->prev != NULL);
                assert(ln->next == NULL);
                assert(ln->prev->next == ln);
                assert(ln->prev == prev_ln);
                list_itr_remove_current_node(&list_itr);
                break;
            default:
                assert(false); // shouldn't get here
                break;
        }
        itr++;
        prev_ln = ln;
    }

    assert(list_get_len(&my_cool_list) == 1);
    assert(my_cool_list.head == my_cool_list.tail);

    // now push the same 3 pointers to the front of the list
    list_push_front(&my_cool_list, &test_data[0]);
    list_push_front(&my_cool_list, &test_data[1]);
    list_push_front(&my_cool_list, &test_data[2]);

    // now, the list should be in the order...
    // test_data[2] -> test_data[1] -> test_data[0] -> test_data[0]
    list_itr = list_itr_create(&my_cool_list);
    data = NULL;
    itr = 0;
    while((data = list_itr_next(&list_itr)))
    {
        assert(data != NULL);
        ListNode* ln = list_itr.current_node;

        switch(itr)
        {
            case 0:
                assert(ln->data == &test_data[2]);
                break;
            case 1:
                assert(ln->data == &test_data[1]);
                break;
            case 2:
                assert(ln->data == &test_data[0]);
                break;
            case 3:
                assert(ln->data == &test_data[0]);
                break;
            default:
                assert(false); // shouldn't get here
                break;
        }
        itr++;
    }

    assert(list_get_len(&my_cool_list) == 4);

    // clear the list
    list_clear(&my_cool_list);

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_is_empty(&my_cool_list));
}

// Test inserting at head, middle, and tail of list
// - list_init
// - list_push_back
// - list_insert
// - list_is_empty
// - list_get_len
// - list_itr_create
// - rev_list_itr_create
// - list_itr_next
// - list_clear
// - list_get_at_idx
void test_list_insertion(void)
{
    List my_cool_list = list_init();

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_get_len(&my_cool_list) == 0);
    assert(list_is_empty(&my_cool_list));

    const int initial_list_size = 5;
    int test_data[initial_list_size];

    for(int i = 0; i < initial_list_size; i++)
    {
        test_data[i] = i;
        list_push_back(&my_cool_list, &test_data[i]);
    }

    assert(list_get_len(&my_cool_list) == initial_list_size);

    int head_val = 0xDEADBEEF;
    int middle_val = 0x1337;
    int tail_val = 0x10101010;
    int last_tail_val = 0x12345678;

    // insert at head of list
    list_insert(&my_cool_list, &head_val, 0);

    ListItr itr = list_itr_create(&my_cool_list);
    void* data;
    int test_data_itr = 0;

    int index = 0;

    while((data = list_itr_next(&itr)))
    {
        int data_itr = *(int*)data;
        int data_from_get_at_idx = *(int*)list_get_at_idx(&my_cool_list, index);

        if(index == 0)
        {
            assert(data_itr == head_val);
            assert(data_from_get_at_idx == head_val);
        }
        else
        {
            assert(data_itr == test_data[test_data_itr]);
            assert(data_from_get_at_idx == test_data[test_data_itr]);
            test_data_itr++;
        }
        index++;
    }

    assert(list_get_len(&my_cool_list) == initial_list_size + 1);
    
    // insert at tail of list, use large value of 100 to force it to append it
    // to the end of the list
    list_insert(&my_cool_list, &tail_val, 100);

    itr = list_itr_create(&my_cool_list);
    index = 0;
    test_data_itr = 0;

    while((data = list_itr_next(&itr)))
    {
        int data_itr = *(int*)data;
        int data_from_get_at_idx = *(int*)list_get_at_idx(&my_cool_list, index);

        if(index == 0)
        {
            assert(data_itr == head_val);
            assert(data_from_get_at_idx == head_val);
        }
        else if(index == initial_list_size + 1) // for head and tail insertion
        {
            assert(data_itr == tail_val);
            assert(data_from_get_at_idx == tail_val);
        }
        else
        {
            assert(data_itr == test_data[test_data_itr]);
            assert(data_from_get_at_idx == test_data[test_data_itr]);
            test_data_itr++;
        }
        index++;
    }

    assert(list_get_len(&my_cool_list) == initial_list_size + 2);
    
    // insert at "middle" of list
    int insert_loc = 2;
    test_data_itr = 0;
    list_insert(&my_cool_list, &middle_val, insert_loc);

    itr = list_itr_create(&my_cool_list);
    index = 0;

    while((data = list_itr_next(&itr)))
    {
        int data_itr = *(int*)data;
        int data_from_get_at_idx = *(int*)list_get_at_idx(&my_cool_list, index);

        if(index == 0)
        {
            assert(data_itr == head_val);
            assert(data_from_get_at_idx == head_val);
        }
        else if(index == insert_loc)
        {
            assert(data_itr == middle_val);
            assert(data_from_get_at_idx == middle_val);
        }
        else if(index == initial_list_size + 2) // for head+middle+tail
        {
            assert(data_itr == tail_val);
            assert(data_from_get_at_idx == tail_val);
        }
        else
        {
            assert(data_itr == test_data[test_data_itr]);
            assert(data_from_get_at_idx == test_data[test_data_itr]);
            test_data_itr++;
        }
        index++;
    }

    printf("\n");

    ListItr rev_itr = rev_list_itr_create(&my_cool_list);
    index = initial_list_size + 2;
    test_data_itr = initial_list_size - 1;
    while((data = list_itr_next(&rev_itr)))
    {
        int data_itr = *(int*)data;
        int data_from_get_at_idx = *(int*)list_get_at_idx(&my_cool_list, index);

        if(index == 0)
        {
            assert(data_itr == head_val);
            assert(data_from_get_at_idx == head_val);
        }
        else if(index == insert_loc)
        {
            assert(data_itr == middle_val);
            assert(data_from_get_at_idx == middle_val);
        }
        else if(index == initial_list_size + 2) // for head+middle+tail
        {
            assert(data_itr == tail_val);
            assert(data_from_get_at_idx == tail_val);
        }
        else
        {
            assert(data_itr == test_data[test_data_itr]);
            assert(data_from_get_at_idx == test_data[test_data_itr]);
            test_data_itr--;
        }
        index--;
    }

    assert(list_get_len(&my_cool_list) == initial_list_size + 3);

    // insert at the real end of the list, to make sure
    // it works in conjunction with the previous "out of range" insertion for the tail
    int tail_idx = list_get_len(&my_cool_list) - 1;
    test_data_itr = 0;
    list_insert(&my_cool_list, &last_tail_val, tail_idx);

    itr = list_itr_create(&my_cool_list);
    index = 0;

    while((data = list_itr_next(&itr)))
    {
        int data_itr = *(int*)data;
        int data_from_get_at_idx = *(int*)list_get_at_idx(&my_cool_list, index);

        if(index == 0)
        {
            assert(data_itr == head_val);
            assert(data_from_get_at_idx == head_val);
        }
        else if(index == insert_loc)
        {
            assert(data_itr == middle_val);
            assert(data_from_get_at_idx == middle_val);
        }
        else if(index == initial_list_size + 2) // for head+middle+tail
        {
            assert(data_itr == last_tail_val);
            assert(data_from_get_at_idx == last_tail_val);
        }
        else if(index == initial_list_size + 3) // for head+middle+tail+last_tail
        {
            assert(data_itr == tail_val);
            assert(data_from_get_at_idx == tail_val);
        }
        else
        {
            assert(data_itr == test_data[test_data_itr]);
            assert(data_from_get_at_idx == test_data[test_data_itr]);
            test_data_itr++;
        }
        index++;
    }

    assert(list_get_len(&my_cool_list) == initial_list_size + 4);

    // clear the list
    list_clear(&my_cool_list);

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_is_empty(&my_cool_list));
}

// Test inserting at head, middle, and tail of list
// - list_init
// - list_push_back
// - list_swap
// - list_swap_at_idx
// - list_is_empty
// - list_get_len
// - list_itr_create
// - list_itr_next
// - list_clear
// - list_get_at_idx
void test_list_swap(void)
{
    List my_cool_list = list_init();

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_get_len(&my_cool_list) == 0);
    assert(list_is_empty(&my_cool_list));

    const int initial_list_size = 5;
    int test_data[initial_list_size];

    for(int i = 0; i < initial_list_size; i++)
    {
        // 0 -> 1 -> 2 -> 3 -> 4
        test_data[i] = i;
        list_push_back(&my_cool_list, &test_data[i]);
    }

    assert(list_get_len(&my_cool_list) == initial_list_size);


    ListItr itr = list_itr_create(&my_cool_list);
    void* data;

    int index = 0;

    while((data = list_itr_next(&itr)))
    {
        // 0 -> 1 -> 2 -> 3 -> 4
        assert(*(int*)list_get_at_idx(&my_cool_list, index) == test_data[index]);
        assert(*(int*)data == test_data[index]);
        index++;
    }

    // Swap with ListNodes

    int idx_a = 1;
    int idx_b = 3;
    ListNode* node_a = list_get_at_idx(&my_cool_list, idx_a);
    ListNode* node_b = list_get_at_idx(&my_cool_list, idx_b);

    assert(!list_swap(NULL, NULL));
    assert(!list_swap(node_a, NULL));
    assert(!list_swap(NULL, node_b));

    assert(list_swap(node_a, node_b));
    void* data_a = list_get_at_idx(&my_cool_list, idx_a);
    void* data_b = list_get_at_idx(&my_cool_list, idx_b);
    assert(*(int*)data_a == test_data[idx_a]);
    assert(*(int*)data_b == test_data[idx_b]);
    assert(list_swap(node_a, node_b));
    data_a = list_get_at_idx(&my_cool_list, idx_a);
    data_b = list_get_at_idx(&my_cool_list, idx_b);
    assert(*(int*)data_a == test_data[idx_a]);
    assert(*(int*)data_b == test_data[idx_b]);

    // Swap with indices

    // swap nothing, out of range
    assert(!list_swap_at_idx(&my_cool_list, 100, 100));

    itr = list_itr_create(&my_cool_list);
    index = 0;
    while((data = list_itr_next(&itr)))
    {
        // 0 -> 1 -> 2 -> 3 -> 4
        assert(*(int*)list_get_at_idx(&my_cool_list, index) == test_data[index]);
        assert(*(int*)data == test_data[index]);
        index++;
    }

    // swap nothing, in range
    assert(list_swap_at_idx(&my_cool_list, 0, 0));

    itr = list_itr_create(&my_cool_list);
    index = 0;
    while((data = list_itr_next(&itr)))
    {
        // 0 -> 1 -> 2 -> 3 -> 4
        assert(*(int*)list_get_at_idx(&my_cool_list, index) == test_data[index]);
        assert(*(int*)data == test_data[index]);
        index++;
    }

    // swap head and "middle"
    int mid_idx = initial_list_size / 2;
    assert(list_swap_at_idx(&my_cool_list, 0, mid_idx));

    itr = list_itr_create(&my_cool_list);
    index = 0;

    while((data = list_itr_next(&itr)))
    {
        int data_itr = *(int*)data;
        int data_from_get_at_idx = *(int*)list_get_at_idx(&my_cool_list, index);

        // 2 -> 1 -> 0 -> 3 -> 4
        if(index == 0)
        {
            assert(data_itr == test_data[mid_idx]);
            assert(data_from_get_at_idx == test_data[mid_idx]);
        }
        else if(index == mid_idx)
        {
            assert(data_itr == test_data[0]);
            assert(data_from_get_at_idx == test_data[0]);
        }
        else
        {
            assert(data_itr == test_data[index]);
            assert(data_from_get_at_idx == test_data[index]);
        }
        index++;
    }

    // swap tail and "middle"
    assert(list_swap_at_idx(&my_cool_list, initial_list_size - 1, mid_idx));

    itr = list_itr_create(&my_cool_list);
    index = 0;

    while((data = list_itr_next(&itr)))
    {
        int data_itr = *(int*)data;
        int data_from_get_at_idx = *(int*)list_get_at_idx(&my_cool_list, index);

        // 2 -> 1 -> 4 -> 3 -> 0
        if(index == 0)
        {
            assert(data_itr == test_data[mid_idx]);
            assert(data_from_get_at_idx == test_data[mid_idx]);
        }
        else if(index == mid_idx)
        {
            assert(data_itr == test_data[initial_list_size - 1]);
            assert(data_from_get_at_idx == test_data[initial_list_size - 1]);
        }
        else if(index == initial_list_size - 1)
        {
            assert(data_itr == test_data[0]);
            assert(data_from_get_at_idx == test_data[0]);
        }
        else
        {
            assert(data_itr == test_data[index]);
            assert(data_from_get_at_idx == test_data[index]);
        }
        index++;
    }

    // clear the list
    list_clear(&my_cool_list);

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_is_empty(&my_cool_list));
}


// Test the "list_remove_data" function
// - list_init
// - list_is_empty
// - list_get_len
// - list_clear
void test_remove_data(void)
{
    // this value MUST be 5
    const int initial_list_size = 5;
    const int midpoint = initial_list_size / 2;
    List my_cool_list = list_init();

    // verify no data 
    assert(my_cool_list.head == NULL);
    assert(my_cool_list.tail == NULL);
    assert(list_get_len(&my_cool_list) == 0);
    assert(list_is_empty(&my_cool_list));

    int test_data[initial_list_size];

    for(int i = 0; i < initial_list_size; i++)
    {
        // 0 -> 1 -> 2 -> 3 -> 4
        test_data[i] = i;
        list_push_back(&my_cool_list, &test_data[i]);
    }

    assert(list_get_len(&my_cool_list) == initial_list_size);

    // remove from the front
    assert(list_remove_data(&my_cool_list, &test_data[0]));

    assert(my_cool_list.head->data == &test_data[1]);
    assert(list_get_len(&my_cool_list) == initial_list_size - 1);

    // remove from the back
    assert(list_remove_data(&my_cool_list, &test_data[initial_list_size - 1]));

    assert(my_cool_list.head->data == &test_data[1]);
    assert(my_cool_list.tail->data == &test_data[initial_list_size - 2]);
    assert(list_get_len(&my_cool_list) == initial_list_size - 2);

    // remove from the middle
    assert(list_remove_data(&my_cool_list, &test_data[midpoint]));

    assert(my_cool_list.head->data == &test_data[1]);
    assert(my_cool_list.tail->data == &test_data[initial_list_size - 2]);
    assert(list_get_len(&my_cool_list) == initial_list_size - 3);

    // fail on removal of pointer not in list
    assert(!list_remove_data(&my_cool_list, NULL));

    assert(my_cool_list.head->data == &test_data[1]);
    assert(my_cool_list.tail->data == &test_data[initial_list_size - 2]);
    assert(list_get_len(&my_cool_list) == initial_list_size - 3);

    // remove last two elements, make sure list can be emptied
    assert(!list_is_empty(&my_cool_list));
    assert(list_remove_data(&my_cool_list, &test_data[1]));
    assert(!list_is_empty(&my_cool_list));
    assert(list_remove_data(&my_cool_list, &test_data[3]));
    assert(list_is_empty(&my_cool_list));

    list_clear(&my_cool_list);

    assert(list_is_empty(&my_cool_list));
}

int main(void)
{
    printf("Testing List Create and Clear.\n");
    create_and_clear_list();

    printf("Testing List Push Back.\n");
    push_back_one_entry();

    printf("Testing List Push Front.\n");
    push_front_one_entry();

    printf("Testing List Complete Exercise.\n");
    push_back_three_remove_push_front_three_entries();

    printf("Testing List Insertion.\n");
    test_list_insertion();

    printf("Testing List Swap.\n");
    test_list_swap();

    printf("Testing List Remove Data.\n");
    test_remove_data();

    printf("-------------------------------------------------------------------------------\n");
    printf("List Tests Passed :)\n");
    printf("-------------------------------------------------------------------------------\n");

    return 0;
}
