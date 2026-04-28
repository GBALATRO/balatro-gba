/**
 * @file game_variables.h
 *
 * @brief Game global game variables struct definition
 */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H

#include <tonc.h>

/**
 * @brief A central location for all game variables.
 *
 * **NOTE**: This is currently WIP and will be populated with a refactor effort.
 * NOT ALL VARIABLES ARE LOCATED HERE YET
 */
typedef struct
{
    s32 timer;
    u32 rng_seed;
} GameVariables;

extern GameVariables g_game_vars;

#endif // GAME_VARIABLES_H
