/**
 * @file game_variables.h
 *
 * @brief Game global game variables struct definition
 */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H

#include "blind.h"
#include "hand.h"
#include "random.h"

#include <tonc.h>

#define GAME_SPEED_MIN 1
#define GAME_SPEED_MAX 4

// Volume is stored from 0 to 5 but is an increment of 20 so 0 to 100 will be displayed
#define VOLUME_OPTION_MIN       0
#define VOLUME_OPTION_MAX       5
#define VOLUME_OPTION_INCREMENT 20

#define DEFAULT_GAME_SPEED   1
#define DEFAULT_MUSIC_VOLUME VOLUME_OPTION_MAX
#define DEFAULT_SOUND_VOLUME VOLUME_OPTION_MAX

#define MAX_HANDS    4
#define MAX_DISCARDS 4

#define DEFAULT_HAND_SIZE 8

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
    RngInfo rng_info;

    // Variables visible by the player

    u32 score;
    u32 chips;
    u32 mult;
    s32 hands;
    s32 discards;
    s32 money;
    s32 hand_size;
    s32 ante;
    s32 round;
    s32 deck;
    u32 nb_played_hands[HAND_TYPE_MAX];

    // Hidden run variables

    u32 best_hand_score;
    u32 nb_skipped_rounds;
    u32 nb_unused_discards;

    // Blind variables

    enum BlindType current_blind;
    enum BlindType next_boss_blind;
    enum BlindState blinds_states[NUM_BLINDS_PER_ANTE];

    Sprite* playing_blind_token;
    Sprite* round_end_blind_token;

    // Options variables

    // BY DEFAULT IS SET TO 1, but if changed to 2 or more, should speed up all (or most) of the
    // game aspects that should be sped up by speed, as in the original game.
    u8 game_speed;
    u8 music_volume;
    u8 sound_volume;
} GameVariables;

extern GameVariables g_game_vars;

#endif // GAME_VARIABLES_H
