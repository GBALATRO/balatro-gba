#ifndef GAME_MAIN_MENU_H
#define GAME_MAIN_MENU_H

#include <tonc.h>

struct MainMenuProps
{
    uint timer;
    uint rng_seed;
};

// Main menu state initialization
void game_main_menu_on_init(void* ctx);

// Change the main menu background
void game_main_menu_change_background(void);

// Main menu state update
void game_main_menu_on_update(void* ctx);

// Main menu cleanup (called when transitioning to game start)
void game_main_menu_cleanup(void);

#endif // GAME_MAIN_MENU_H
