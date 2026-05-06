/**
 * @file game_variables.h
 *
 * @brief Game global game variables struct definition
 */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H

#include "blind.h"

#include <tonc.h>

#define GAME_SPEED_MIN 1
#define GAME_SPEED_MAX 4

// Volume is stored from 0 to 5 but is an increment of 20 so 0 to 100 will be displayed
#define VOLUME_OPTION_MIN       0
#define VOLUME_OPTION_MAX       5
#define VOLUME_OPTION_INCREMENT 20

#define DEFAULT_GAME_SPEED    1
#define DEFAULT_HIGH_CONTRAST false
#define DEFAULT_MUSIC_VOLUME  VOLUME_OPTION_MAX
#define DEFAULT_SOUND_VOLUME  VOLUME_OPTION_MAX

/**
 * @brief A central location for all game variables.
 *
 * **NOTE**: This is currently WIP and will be populated with a refactor effort.
 * NOT ALL VARIABLES ARE LOCATED HERE YET
 */
typedef struct
{
    // Internal variables

    s32 timer; // This might already exist in libtonc but idk so i'm just making my own
    u32 rng_seed;
    u32 rng_step; // Position in the rng sequence.

    // Variables visible by the player

    int round;
    int ante;
    int money;

    // Blind variables

    enum BlindType current_blind;
    enum BlindType next_boss_blind;
    enum BlindState blinds_states[NUM_BLINDS_PER_ANTE];

    // Options variables

    // BY DEFAULT IS SET TO 1, but if changed to 2 or more, should speed up all (or most) of the
    // game aspects that should be sped up by speed, as in the original game.
    u8 game_speed;
    bool high_contrast;
    u8 music_volume;
    u8 sound_volume;
} GameVariables;

extern GameVariables g_game_vars;

#endif // GAME_VARIABLES_H
