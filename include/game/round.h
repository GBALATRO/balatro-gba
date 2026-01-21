#ifndef __INCLUDE_GAME_ROUND_H__
#define __INCLUDE_GAME_ROUND_H__

#include "card.h"
#include "game/common_ui.h"
#include "game_state_ctx.h"
#include "list.h"

#include <tonc.h>

enum PlayState
{
    PLAY_STARTING,
    PLAY_BEFORE_SCORING,
    PLAY_SCORING_CARDS,
    PLAY_SCORING_CARD_JOKERS,
    PLAY_SCORING_HELD_CARDS,
    PLAY_SCORING_INDEPENDENT_JOKERS,
    PLAY_SCORING_HAND_SCORED_END,
    PLAY_ENDING,
    PLAY_ENDED
};

// Hand types
enum HandType
{
    NONE,
    HIGH_CARD,
    PAIR,
    TWO_PAIR,
    THREE_OF_A_KIND,
    FOUR_OF_A_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HOUSE,
    STRAIGHT_FLUSH,
    ROYAL_FLUSH,
    FIVE_OF_A_KIND,
    FLUSH_HOUSE,
    FLUSH_FIVE
};

// Main round state functions
void game_round_on_init(GameStateCtx* ctx);
void game_playing_on_update(GameStateCtx* ctx);

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
void reset_joker_scored_itr(List* jokers_list);

#endif
