/**
 * @file game/game_over.h
 * @brief Game lose/win/game over state handlers
 */
#ifndef GAME_GAME_OVER_H
#define GAME_GAME_OVER_H

#include "game_state_ctx.h"
#include "list.h"

#include <tonc.h>

void game_lose_on_init(GameStateCtx* ctx);
void game_lose_on_update(GameStateCtx* ctx);
void game_win_on_init(GameStateCtx* ctx);
void game_win_on_update(GameStateCtx* ctx);
void game_over_on_exit(GameStateCtx* ctx);

#endif // GAME_GAME_OVER_H
