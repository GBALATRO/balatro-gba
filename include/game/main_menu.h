#ifndef GAME_MAIN_MENU_H
#define GAME_MAIN_MENU_H

// Main menu state initialization
void game_main_menu_on_init(void);

// Main menu state update
void game_main_menu_on_update(void);

// Main menu cleanup (called when transitioning to game start)
void game_main_menu_cleanup(void);

#endif // GAME_MAIN_MENU_H
