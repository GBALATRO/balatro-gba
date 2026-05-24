/**
 * @file stack.h
 *
 * @brief Stack implementation, no frills
 */
#ifndef STACK_H
#define STACK_H

#define STACK_DEFINE(name, size)                                               \
    static void* name##_data_array[size];                                             \
    static Stack name  = { .data_array = name##_data_array, .top = -1, .max = size }; 

typedef struct
{
    void** data_array;
    int top;
    int max;
} Stack;

void stack_reset(Stack* stack);
bool stack_empty(Stack* stack);
int stack_len(Stack* stack);
void stack_push(Stack* stack, void* data);
void* stack_pop(Stack* stack);
void* stack_at(Stack* stack, unsigned int idx);

#endif // STACK_H
