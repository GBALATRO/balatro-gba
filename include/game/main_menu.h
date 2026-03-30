#ifndef GAME_MAIN_MENU_H
#define GAME_MAIN_MENU_H

#include "game_variables.h"

// Change the main menu background
void game_main_menu_change_background(void);

// Main menu state initialization
void game_main_menu_on_init(GameVariables* vars);

// Main menu state update
void game_main_menu_on_update(GameVariables* vars);

// Main menu cleanup (called when transitioning to game start)
void game_main_menu_on_exit(GameVariables* vars);

#endif // GAME_MAIN_MENU_H
