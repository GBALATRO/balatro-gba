#ifndef GAME_BLIND_SELECT_H
#define GAME_BLIND_SELECT_H

#include "blind.h"
#include "graphic_utils.h"

#include <tonc.h>

// State callbacks for state machine
void game_blind_select_on_init(void);
void game_blind_select_on_update(void);
void game_blind_select_on_exit(void);

// Background change function
void game_blind_select_change_background(void);

// Internal functions
void game_blind_select_init(void);
void blind_select_update(void);
void game_blind_select_exit(void);
void blind_select_setup_tokens(void);

#endif // GAME_BLIND_SELECT_H
