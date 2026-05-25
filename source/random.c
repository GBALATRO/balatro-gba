#include "random.h"

#include "game_variables.h"
#include "list.h"

#include <stdlib.h>
#include <tonc.h>

// Accumulate timer 1 into a bigger variable so we can generate more diverse seeds
static u32 timer_acc = 0;

// Timers usage docs: https://gbadev.net/tonc/timers.html
void rng_init(void)
{
    REG_TM1D = 0;
    REG_TM1CNT = TM_FREQ_1 | TM_ENABLE; // using timer with x1 prescale
}

void rng_update(void)
{
    timer_acc += (u32)REG_TM1D;
}

void rng_set_seed(u32 seed)
{
    g_game_vars.rng_info.seed = seed % (MAX_SEED + 1);
    g_game_vars.rng_info.step = 0;
    srand(g_game_vars.rng_info.seed);
}

void rng_shuffle_seed(void)
{
    rng_set_seed(timer_acc);
}

u32 rng_get_u32(void)
{
    g_game_vars.rng_info.step++;
    return rand();
}

void rng_restore(RngInfo info)
{
    g_game_vars.rng_info = info;

    srand(g_game_vars.rng_info.seed);
    for (u32 i = 0; i < g_game_vars.rng_info.step; i++)
    {
        (void)rng_get_u32();
    }
}

void rng_shuffle_array(void** array, int len)
{
    for (int i = len - 1; i >= 0; i--)
    {
        int j = rng_get_u32() % (i + 1);
        void* temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

void rng_shuffle_list(List* list)
{
    // Allow shuffling up to 256 items in a list at a time
    static const int MAX_ELEMENTS = 256;
    void* shuffle_array[MAX_ELEMENTS];

    ListItr list_itr = list_itr_create(list);

    void* data;
    int arr_idx = 0;
    while ((data = list_itr_next(&list_itr)))
    {
        shuffle_array[arr_idx++] = data;
    }

    rng_shuffle_array(shuffle_array, arr_idx);

    list_itr = list_itr_create(list);

    arr_idx = 0;
    while ((data = list_itr_next(&list_itr)))
    {
        list_itr.current_node->data = shuffle_array[arr_idx++];
    }
}
