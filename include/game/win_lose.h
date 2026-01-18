/**
 * @file game/win_lose.h
 * @brief Game lose/win/game over state handlers
 */
#ifndef GAME_WIN_LOSE_H
#define GAME_WIN_LOSE_H

void game_lose_on_init(void* ctx);
void game_lose_on_update(void* ctx);
void game_win_on_init(void* ctx);
void game_win_on_update(void* ctx);
void game_over_on_exit(void* ctx);
#endif // GAME_WIN_LOSE_H
