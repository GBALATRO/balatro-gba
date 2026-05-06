/**
 * @file blind_select.h
 *
 * @brief Blind select state functions
 */
#ifndef GAME_BLIND_SELECT_H
#define GAME_BLIND_SELECT_H

#include "blind.h"

/**
 * @brief Change to the blind select background
 */
void game_blind_select_change_background(void);

/**
 * @brief Blind select state initialization
 */
void game_blind_select_on_init(void);

/**
 * @brief Blind select state update
 */
void game_blind_select_on_update(void);

/**
 * @brief Blind select cleanup
 */
void game_blind_select_on_exit(void);

void increment_blind(enum BlindState increment_reason);

#endif // GAME_BLIND_SELECT_H
