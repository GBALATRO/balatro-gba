#include "random.h"

#include "game_variables.h"
#include "util.h"

#include <stdlib.h>
#include <tonc.h>

static void s_init_rng_states(void);
static void s_xorshift32(u32* state);

// Accumulate timer 1 into a bigger variable so we can generate more diverse seeds
static u32 s_timer_acc = 0;

// Timers usage docs: https://gbadev.net/tonc/timers.html
void rng_init(void)
{
    REG_TM1D = 0;
    REG_TM1CNT = TM_FREQ_1 | TM_ENABLE; // using timer with x1 prescale
}

void rng_update(void)
{
    s_timer_acc += (u32)REG_TM1D;
}

void rng_shuffle_seed(void)
{
    srand(s_timer_acc);
    rng_set_seed(rand());
}

void rng_set_seed(u32 seed)
{
    // We store the seed to display it at the end of the run, but here it's only used to generate
    // the independent rng sequences' initial states. We also avoid the seed 0 as the Xorshift32
    // method used will stay stuck.
    u32 capped_seed = seed % (MAX_BASE36 + 1);
    g_game_vars.rng_info.seed = (capped_seed == 0) ? MAX_BASE36 : capped_seed;
    s_init_rng_states();
}

/**
 * @brief Reset all independent RNG sequences to their initial states using the
 *         global custom seed.
 */
static inline void s_init_rng_states(void)
{
    srand(g_game_vars.rng_info.seed);
    for (enum RngSequence key = 0; key < RNG_SEQ_MAX; key++)
    {
        g_game_vars.rng_info.states[key] = rand();
    }
}

u32 rng_get_u32(enum RngSequence key)
{
    s_xorshift32(&g_game_vars.rng_info.states[key]);
    return g_game_vars.rng_info.states[key];
}

/**
 * @brief Transforms a given RNG state according to the Xorshift32 algorithm.
 *
 * Custom RNG had to be implemented to be able to manage several independent sequences, since
 * `initstate` and `setstate` are POSIX and not available on GBA via devkitpro.
 *
 * @param state pointer to a 32-bit RNG state
 */
static inline void s_xorshift32(u32* state)
{
    *(state) ^= *(state) << 13;
    *(state) ^= *(state) >> 17;
    *(state) ^= *(state) << 5;
}

void rng_restore(RngInfo info)
{
    g_game_vars.rng_info = info;
}
