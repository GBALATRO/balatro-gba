#include "stack.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define STACK_SIZE 64

STACK_DEFINE(coolstack, STACK_SIZE)

void test_stack(void)
{
    int int_arr[STACK_SIZE];

    for(int i = 0; i < STACK_SIZE; i++)
    {
        int_arr[i] = i;
    }

    for(int i = 0; i < STACK_SIZE; i++)
    {
        stack_push(&coolstack, &int_arr[i]);
    }

    for(int i = STACK_SIZE - 1; i >= 0; i--)
    {
        assert(*(int*)stack_pop(&coolstack) == i);
    }
}

int main(void)
{
    printf("Testing Stack.\n");
    test_stack();

    printf("-------------------------------------------------------------------------------\n");
    printf("Stack Tests Passed :)\n");
    printf("-------------------------------------------------------------------------------\n");

    return 0;
}
