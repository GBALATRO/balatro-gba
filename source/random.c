#include "random.h"

#include "game_variables.h"

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
