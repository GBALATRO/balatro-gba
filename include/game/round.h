/**
 * @file round.h
 *
 * @brief Round game state functions
 */
#ifndef GAME_ROUND_H
#define GAME_ROUND_H

/**
 * @brief Change to the round background
 */
void game_round_selecting_change_background(void);
void game_round_playing_change_background(void);

/**
 * @brief Round state initialization
 */
void game_round_on_init(void);

/**
 * @brief Round state update
 */
void game_round_on_update(void);

#endif // GAME_ROUND_H
