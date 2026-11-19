/**
 * @file pause_menu.h
 *
 * @brief Pause overlay that freezes the game and offers to resume or quit the run.
 */
#ifndef GAME_PAUSE_MENU_H
#define GAME_PAUSE_MENU_H

#include <stdbool.h>

/**
 * @brief Callback run after the pause menu closed because "Quit run" was chosen.
 */
typedef void (*PauseMenuQuitFunc)(void);

/**
 * @brief Open the pause menu: dims the screen and replaces the on-screen text by the menu.
 *
 * Does nothing if the menu is already open.
 *
 * @param on_quit_run called once the menu is closed if the player chose to quit the run
 */
void pause_menu_open(PauseMenuQuitFunc on_quit_run);

/**
 * @brief Tell whether the pause menu is open, including while it waits to close.
 *
 * @return true if the game must stay frozen, false otherwise
 */
bool pause_menu_is_open(void);

/**
 * @brief Handle the pause menu inputs. Must be called every frame while the menu is open.
 */
void pause_menu_update(void);

#endif // GAME_PAUSE_MENU_H
