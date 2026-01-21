#ifndef GAME_MAIN_MENU_H
#define GAME_MAIN_MENU_H

#include "game_state_ctx.h"

#include <tonc.h>

// Main menu state initialization
void game_main_menu_on_init(GameStateCtx* ctx);

// Change the main menu background
void game_main_menu_change_background(void);

// Main menu state update
void game_main_menu_on_update(GameStateCtx* ctx);

// Main menu cleanup (called when transitioning to game start)
void game_main_menu_cleanup(void);

#endif // GAME_MAIN_MENU_H
