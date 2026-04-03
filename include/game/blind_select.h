/**
 * @file blind_select.h
 *
 * @brief Blind selection screen
 */
#ifndef BLIND_SELECT_H
#define BLIND_SELECT_H

#include "game_variables.h"

/**
 * @brief Blind select screen state initialization
 *
 * @param vars passed @ref GameVariables struct
 */
void game_blind_select_on_init(GameVariables* vars);

/**
 * @brief Blind select screen state update
 * 
 * @param vars passed @ref GameVariables struct
 */
void game_blind_select_on_update(GameVariables* vars);

/**
 * @brief Blind select screen cleanup (called when transitioning to game start)
 * 
 * @param vars passed @ref GameVariables struct
 */
void game_blind_select_on_exit(GameVariables* vars);

#endif // BLIND_SELECT_H
