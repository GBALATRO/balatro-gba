#ifndef __INCLUDE_GAME_ROUND_H__
#define __INCLUDE_GAME_ROUND_H__

#include "game.h"
#include "game/common_ui.h"

#include <tonc.h>

// Main round state functions
void game_round_on_init(void);
void game_playing_on_update(void);

// Background change functions
void game_playing_change_background(enum BackgroundId current_background);
void game_selecting_change_background(enum BackgroundId current_background);

#endif
