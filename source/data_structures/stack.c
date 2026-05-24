#include "stack.h"

#include "stddef.h"

void stack_reset(Stack* stack)
{
    stack->top = -1;
}

bool stack_empty(Stack* stack)
{
    return stack->top < 0;
}

int stack_len(Stack* stack)
{
    return stack->top + 1;
}

void stack_push(Stack* stack, void* data)
{
    if (stack->top < stack->max)
        stack->data_array[++stack->top] = data;
}

void* stack_pop(Stack* stack)
{
    return (stack->top < 0) ? NULL : stack->data_array[stack->top--];
}

void* stack_at(Stack* stack, unsigned int idx)
{
    return (idx <= stack->top) ? stack->data_array[idx] : NULL;
}
