#ifndef GAME_ROUND_END_H
#define GAME_ROUND_END_H

#include "game.h"
#include "game/common_ui.h"
#include "sprite.h"

typedef struct
{
    uint timer;
    int substate;
    int money;
    int hands;
    int max_hands;
    int discards;
    int max_discards;
    int ante;
    int current_blind;
    int score;
} RoundEndProps;

void game_round_end_on_init(void* ctx);
void game_round_end_on_update(void* ctx);
void game_round_end_on_exit(void* ctx);

void game_round_end_change_background(enum BackgroundId current_background);

#endif // GAME_ROUND_END_H
