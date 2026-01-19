#ifndef GAME_BLIND_SELECT_H
#define GAME_BLIND_SELECT_H

#include "blind.h"
#include "graphic_utils.h"

#include <tonc.h>

typedef struct
{
    uint timer;
    int substate;
    int game_round;
    int current_blind;
    enum BlindState blinds_states[BLIND_TYPE_MAX];
    int ante;
} BlindSelectProps;

// State callbacks for state machine
void game_blind_select_on_init(void* ctx);
void game_blind_select_on_update(void* ctx);
void game_blind_select_on_exit(void* ctx);

// Background change function
void game_blind_select_change_background(void);

// Internal functions
void game_blind_select_init(void);
void blind_select_update(void);
void game_blind_select_exit(void);
void blind_select_setup_tokens(void);

#endif // GAME_BLIND_SELECT_H
