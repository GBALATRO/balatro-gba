/**
 * @file round_end.h
 *
 * @brief Round end game state functions
 */
#ifndef GAME_ROUND_END_H
#define GAME_ROUND_END_H

/**
 * @brief Change to the round end background
 */
void game_round_end_change_background(void);

/**
 * @brief Round end state initialization
 */
void game_round_end_on_init(void);

/**
 * @brief Round end state update
 */
void game_round_end_on_update(void);

/**
 * @brief Round end cleanup
 */
void game_round_end_on_exit(void);

#endif // GAME_ROUND_END_H
