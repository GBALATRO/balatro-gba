#ifndef GAME_STATE_CTX_H
#define GAME_STATE_CTX_H

#include "blind.h"
#include "card.h"
#include "list.h"

typedef struct
{
    uint timer;
    int substate;
    int game_round;
    int current_blind;
    enum BlindState blinds_states[BLIND_TYPE_MAX];
    int ante;
} BlindSelectProps;

typedef struct
{
    uint timer;
    int game_round;
    int score;
    List* owned_jokers_list;
} GameOverProps;

typedef struct
{
    uint timer;
    uint rng_seed;
} MainMenuProps;

typedef struct
{
    uint timer;
    int substate;
    int money;
    int ante;
    int current_blind;
    u32 score;
    u32 temp_score;
    FIXED lerped_score;
    FIXED lerped_temp_score;
    int hands;
    int discards;
    List* owned_jokers_list;
    CardObject** played;
    int played_top;
    CardObject** hand;
    int hand_top;
    Card** deck;
    int deck_top;
    Card** discard_pile;
    int discard_top;
} RoundProps;

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

typedef struct
{
    uint timer;
    int substate;
    int money;
    enum BlindState blinds_states[BLIND_TYPE_MAX];
    int current_blind;
    int shortcut_joker_count;
    int four_fingers_joker_count;
    List* owned_jokers_list;
    List* discarded_jokers_list;
} ShopProps;

typedef union
{
    MainMenuProps main_menu;
    BlindSelectProps blind_select;
    RoundProps round;
    RoundEndProps round_end;
    ShopProps shop;
    GameOverProps game_over;
} GameStateCtx;

#endif // GAME_STATE_CTX_H