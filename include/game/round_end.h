#ifndef GAME_ROUND_END_H
#define GAME_ROUND_END_H

#include "game/common_ui.h"
#include "game_state_ctx.h"
#include "sprite.h"

void game_round_end_on_update(GameStateCtx* ctx);
void game_round_end_on_exit(GameStateCtx* ctx);

void game_round_end_change_background(enum BackgroundId current_background);

#endif // GAME_ROUND_END_H
