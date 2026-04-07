/**
 * @file main_menu.h
 *
 * @brief Common functions to render UI elements.
 */
#ifndef GAME_MAIN_MENU_H
#define GAME_MAIN_MENU_H

#include "game_variables.h"

/**
 * @brief Change the main menu background
 */
void game_main_menu_change_background(void);

/**
 * @brief Main menu state initialization
 *
 * @param vars passed @ref GameVariables struct
 */
void game_main_menu_on_init(GameVariables* vars);

/**
 * @brief Main menu state update
 *
 * @param vars passed @ref GameVariables struct
 */
void game_main_menu_on_update(GameVariables* vars);

/**
 * @brief Main menu cleanup (called when transitioning to game start)
 *
 * @param vars passed @ref GameVariables struct
 */
void game_main_menu_on_exit(GameVariables* vars);

#endif // GAME_MAIN_MENU_H
