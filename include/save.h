/**
 * @file save.h
 *
 * @brief Utils functions to save/load data structures from/to the SRAM.
 *
 * Here is an overwiew of the contents of a valid save file.
 *
 * ```
 * ┌─────────────┐ <-- SaveHeader
 * │   Header    │
 * ├─────────────┤ <-- SaveOptions
 * │   Options   │
 * ├─────────────┤ <-- SaveGame
 * │ Engine vars │
 * │   Jokers    │
 * └─────────────┘
 * ```
 *
 * SaveHeader indicates save validity by its presence.
 * SaveOptions contains options values set in the corresponding menu and apply to the game itself.
 * SaveGame contains values tied to a run with round, ante, money, owned Jokers etc...
 *
 * @sa SaveHeader, SaveOptions, SaveGame
 *
 * I strongly recommend using `xxd -l 512 -e -g 4 <gbalatro.sav>` to view the contents of the save
 */
#ifndef SAVE_H
#define SAVE_H

#include "blind_select.h"
#include "game.h"
#include "game_variables.h"
#include "skip_tag.h"

#include <tonc.h>

#define SAVE_LABEL_SIZE 16

/**
 * @brief JokerObjectSaveData will hold the minimal amount of data necessary to reconstruct a Joker.
 *         The `id` is a u8 in the base Joker struct, but I made it a u32 here to keep
 *         a better aligment when looking at the save file in a hex viewer.
 */
typedef struct JokerObjectSaveData
{
    u32 id;
    u32 persistent_state;
} JokerObjectSaveData;

// clang-format off
/**
 * @brief SaveGame will contain the data about the current run to be saved to SRAM.
 *
 * GameVariables was used for this purpose at first, but some data needed to be shared but not
 * saved, so it couldn't be dumped "as is" anymore and this struct had to be created.
 *
 * This data has been organized to be viewed with the following command:
 * `xxd -l 512 -e -g 4 build/balatro-gba.sav`
 *
 * Byte 0 | Byte 1 | Byte 2 | Byte 3 | name         | purpose
 * -------|--------|--------|--------|--------------|------------------------------------------------------------------
 * '-'    | 'I'    | 'N'    | 'T'    | TAG          | Spells "-INTERNAL DATA -"
 * 'E'    | 'R'    | 'N'    | 'A'    | -            | -
 * 'L'    | ' '    | 'D'    | 'A'    | -            | -
 * 'T'    | 'A'    | ' '    | '-'    | -            | -
 * STT[0] | STT[1] | STT[2] | STT[3] | GAME STATE   | The game state, i.e. on what screen was the save made
 * T[0]   | T[1]   | T[2]   | T[3]   | GLOB TIMER   | The global timer used for animations thoughout the game
 * RNG[0] | RNG[1] | RNG[2] | RNG[3] | RNG INFO     | RNG Info struct, containing the seed used for RNG, either randomly shuffled or chosen by the player
 * RNG[4] | RNG[5] | RNG[6] | RNG[7] | -            | at game start, and the current position in the RNG sequence for the given seed, since the start of the run
 * GAM[0] | GAM[1] | GAM[2] | GAM[3] | GAME VARS    | Various data taken from Game Variables
 * ...    | ...    | ...    | ...    | ...          | ...
 * '-'    | 'P'    | 'L'    | 'A'    | TAG          | Spells "-PLAYING CARDS -"
 * 'Y'    | 'I'    | 'N'    | 'G'    | -            | -
 * ' '    | 'C'    | 'A'    | 'R'    | -            | -
 * 'D'    | 'S'    | ' '    | '-'    | -            | -
 * SUIT 0 | RANK 0 | SUIT 1 | RANK 1 | CARD DATA    | Minimal necessary data to reconstruct a Card. Contains the Card's `suit` and `rank`
 * ...    | ...    | ...    | ...    | ...          | ...
 * ...    | ...    | ...    | ...    | ...          | ...
 * '-'    | ' '    | 'O'    | 'W'    | TAG          | Spells "- OWNED JOKERS -"
 * 'N'    | 'E'    | 'D'    | ' '    | -            | -
 * 'J'    | 'O'    | 'K'    | 'E'    | -            | -
 * 'R'    | 'S'    | ' '    | '-'    | -            | -
 * ID[0]  | ID[1]  | ID[2]  | ID[3]  | JOKER DATA 0 | Minimal necessary data to reconstruct a JokerObject
 * STT[0] | STT[1] | STT[2] | STT[3] | -            | Contains the Joker's `id` and `persistent_state`
 * ...    | ...    | ...    | ...    | ...          | ...
 * ...    | ...    | ...    | ...    | ...          | ...
 * '_'    | 'E'    | 'N'    | 'D'    | END_TAG      | Spells "_END", marks the end of the savefile
 */
// clang-format on
typedef struct SaveGame
{
    char tag_internal[SAVE_LABEL_SIZE];

    int game_state;

    s32 timer;
    RngInfo rng_info;

    s32 money;
    s32 hand_size;
    s32 ante;
    s32 round;
    s32 deck;
    u32 nb_played_hands[HAND_TYPE_MAX];

    u32 best_hand_score;
    u32 nb_skipped_rounds;
    u32 nb_unused_discards;

    enum BlindType current_blind;
    enum BlindType next_boss_blind;
    enum BlindState blinds_states[NUM_BLINDS_PER_ANTE];

    s32 padding0[1];

    char tag_cards[SAVE_LABEL_SIZE];
    int nb_playing_cards;
    Card playing_cards[MAX_DECK_SIZE];

    s32 padding1[1];

    char tag_jokers[SAVE_LABEL_SIZE];
    int nb_jokers;
    JokerObjectSaveData jokers_data[MAX_JOKERS_HELD_SIZE];

    s32 padding2[1];

    char tag_skip_tags[SAVE_LABEL_SIZE];
    enum SkipTagType blind_skip_tags[NB_SKIPPABLE_BLINDS];
    int nb_owned_skip_tags;
    enum SkipTagType owned_skip_tags[MAX_SKIP_TAGS - NB_SKIPPABLE_BLINDS];

    char tag_end[4];
} SaveGame;

/**
 * @brief Checks whether the SaveGame section is present and valid.
 *
 * @returns true if it is, false if not.
 */
bool is_game_data_valid(void);

/**
 * @brief Sets the valid save section flag for game data to false.
 *
 * We don't really need to set all bits to zero in the SRAM because we don't read it to determine if
 * there is a valid save file.
 */
void erase_save_data(void);

/**
 * @brief Save current run data to SRAM.
 *
 * @param state of the game, easier to pass it than recovering it given how state machines work
 */
void save_game(enum GameState state);

/**
 * @brief Load previous run data from SRAM.
 *
 * @param game_data pointer to a SaveGame data struct to fill
 *
 * @sa save_game
 */
void get_game_saved_data(SaveGame* game_data);

/**
 * @brief Apply previous run data from a SaveGame struct.
 *
 * @param game_data pointer to a SaveGame data struct to reload
 *
 * @sa get_game_saved_data
 */
void load_game(SaveGame* game_data);

/**
 * @brief Save options values to SRAM.
 */
void save_options(void);

/**
 * @brief Load options values from SRAM.
 *
 * @sa save_options
 */
void load_options(void);

#endif // SAVE_H
