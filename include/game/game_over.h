/**
 * @file game/game_over.h
 * @brief Game lose/win/game over state handlers
 */
#ifndef GAME_GAME_OVER_H
#define GAME_GAME_OVER_H

#include "list.h"

#include <tonc.h>

typedef struct
{
    uint timer;
    int game_round;
    int score;
    List* owned_jokers_list;
} GameOverProps;

void game_lose_on_init(void* ctx);
void game_lose_on_update(void* ctx);
void game_win_on_init(void* ctx);
void game_win_on_update(void* ctx);
void game_over_on_exit(void* ctx);

#endif // GAME_GAME_OVER_H
