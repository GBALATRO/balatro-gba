/**
 * @file game_variables.h
 *
 * @brief Game global game variables struct definition
 */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H

#include <tonc.h>

#include "blind.h"
#include "sprite.h"

/**
 * @brief A central location for all game variables.
 *
 * **NOTE**: This is currently WIP and will be populated with a refactor effort.
 * NOT ALL VARIABLES ARE LOCATED HERE YET
 */
typedef struct
{
    u32 timer;
    u32 rng_seed;

    int current_blind;

    enum BlindState blinds_states[BLIND_TYPE_MAX];
    Sprite* blind_select_tokens[BLIND_TYPE_MAX];

    int round;
    int ante;
} GameVariables;

#endif // GAME_VARIABLES_H
