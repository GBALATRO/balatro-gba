/**
 * @file game_over.h
 *
 * @brief Game Over screen state functions
 */
#ifndef GAME_OVER_H
#define GAME_OVER_H

/**
 * @brief Game Over screen state initialization when losing
 */
void game_lose_on_init(void);

/**
 * @brief Game Over screen state initialization when winning
 */
void game_win_on_init(void);

/**
 * @brief Game Over screen state update
 */
void game_over_on_update(void);

/**
 * @brief Game Over screen cleanup, common to both losing and winning
 */
void game_over_on_exit(void);

#endif // GAME_OVER_H