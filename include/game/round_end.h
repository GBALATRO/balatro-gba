#ifndef GAME_ROUND_END_H
#define GAME_ROUND_END_H

#include "game.h"
#include "game/common_ui.h"

void game_round_end_on_init(void* ctx);
void game_round_end_on_update(void* ctx);
void game_round_end_on_exit(void* ctx);
int calculate_interest_reward(void);

void game_round_end_change_background(enum BackgroundId current_background);

#endif // GAME_ROUND_END_H
