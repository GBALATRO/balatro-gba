/**
 * @file options_menu.h
 *
 * @brief Options menu state functions.
 */
#ifndef GAME_OPTIONS_MENU_H
#define GAME_OPTIONS_MENU_H

#include "game_variables.h"

/**
 * @brief Change the options menu background
 */
void game_options_menu_change_background(void);

/**
 * @brief Options menu state initialization
 */
void game_options_menu_on_init(void);

/**
 * @brief Options menu state update
 */
void game_options_menu_on_update(void);

/**
 * @brief Options menu cleanup (called when going back to main menu)
 */
void game_options_menu_on_exit(void);

#endif // GAME_OPTIONS_MENU_H