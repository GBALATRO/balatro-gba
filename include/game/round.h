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

// Getters and Setters
void set_retrigger(bool value);
u32 get_chips(void);
void set_chips(u32 new_chips);
u32 get_mult(void);
void set_mult(u32 new_mult);

int get_scored_card_index(void);
void reset_joker_scored_itr(void);

#endif
