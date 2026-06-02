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

#include "game_variables.h"

#include <tonc.h>

/**
 * @brief Checks whether the SaveGame section is present and valid.
 *
 * @returns true if it is, false if not.
 */
bool is_game_data_valid(void);

/**
 * @brief Save current run data to SRAM.
 */
void save_game(void);

/**
 * @brief Load previous run data from SRAM.
 *
 * @sa save_game
 */
void load_game(void);

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
