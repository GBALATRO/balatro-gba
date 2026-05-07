/**
 * @file save.h
 *
 * @brief Utils functions to save/load data structures from/to the SRAM.
 */
#ifndef SAVE_H
#define SAVE_H

#include "game_variables.h"

#include <tonc.h>

/**
 * @brief Save current run data to SRAM.
 */
void save_game();

/**
 * @brief Load previous run data from SRAM.
 *
 * @sa save_game
 */
void load_game();

/**
 * @brief Save options values to SRAM.
 */
void save_options();

/**
 * @brief Load options values from SRAM.
 *
 * @sa save_options
 */
void load_options();

#endif // SAVE_H
