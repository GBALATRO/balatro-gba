#ifndef GAME_ROUND_H
#define GAME_ROUND_H

#include "card.h"

void check_flaming_score(void);
CardObject** get_played_array(void);
int get_played_top(void);
int get_discard_top(void);
int get_scored_card_index(void);
void set_retrigger(bool new_retrigger);

void game_round_change_background_selecting(void);
void game_round_change_background_playing(void);
void game_round_on_init(void);
void game_round_on_update(void);

#endif // GAME_ROUND_H
