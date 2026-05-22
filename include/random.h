/**
 * @file random.h
 *
 * @brief Common functions to handle RNG manipulation. Using this interface has three goals:
 * 1. Make RNG repeatable so that people can share seeds and saves, and end up with the same run
 * 2. Prevent the players from engaging in save-scumming since the pseudo-RNG is deterministic
 * 3. Accomodate for base-36 seeds, so that we can freely chose one with alpha-numeric characters in
 * the seed screen input
 */
#ifndef RANDOM_H
#define RANDOM_H

#include <tonc.h>

/**
 * @brief Hex value of "ZZZZZZ" in base 36
 */
#define MAX_SEED 0x81BF0FFF

/**
 * @brief Information to track and restore RNG state
 */
typedef struct
{
    /** Initial seed */
    u32 seed;
    /** Position in the rng sequence. */
    u32 step;
} RngInfo;

/**
 * @brief Starts counting CPU cycles, this will be used by rng_shuffle_seed to
 *         generate a more random seed. To be called once on game start.
 */
void rng_init(void);

/**
 * @brief Update the CPU cycles counter and accumulate it into a bigger u32 `timer_acc` variable.
 */
void rng_update(void);

/**
 * @brief Set the rng seed to the chosen value, and reset the step counter to 0.
 *         The seed will be capped at `MAX_SEED` for compatibility with the Seed Input screen
 *         used to choose a seed for seeded runs.
 *
 * @param seed the new RNG seed
 */
void rng_set_seed(u32 seed);

/**
 * @brief Uses the CPU cycles counter to randomize the RNG seed as much as possible.
 *         This will be called by the main menu and the game over screens so that
 *         the next run's seed isn't the same as the last's.
 *         rng_start_sampling needs to have been called, and will stop the profiling.
 */
void rng_shuffle_seed(void);

/**
 * @brief Get the next "randomly" generated number in the sequence from the current seed.
 *
 * @return u32
 */
u32 rng_get_u32(void);

/**
 * @brief Restore RNG info struct in the GameVariables. Sets the `seed` and seeks the
 *         position `step` in the rng sequence.
 *
 * @param info RngInfo struct applied
 *
 * @note Consistency of the RNG after reloading the RngInfo struct has yet to be tested properly
 */
void rng_restore(RngInfo info);

#endif // RANDOM_H
