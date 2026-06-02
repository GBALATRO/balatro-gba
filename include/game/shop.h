/**
 * @file shop.h
 *
 * @brief Shop state functions.
 */
#ifndef GAME_SHOP_H
#define GAME_SHOP_H

#include <stdbool.h>

/**
 * @brief Initialize the shop for a run.
 * Resets all the shop data for the run, needs to be called once per run.
 */
void game_shop_reset(void);

/**
 * @brief Set whether a Joker can appear in the shop.
 *
 * @param avail - true to make it available to the shop to appear in
 *                false to make it unavailable.
 */
void game_shop_set_joker_avail(int joker_id, bool avail);

/**
 * @brief Change to the shop background
 */
void game_shop_change_background(void);

/**
 * @brief Shop state initialization
 */
void game_shop_on_init(void);

/**
 * @brief Shop state update
 */
void game_shop_on_update(void);

/**
 * @brief Shop cleanup
 */
void game_shop_on_exit(void);

#endif // GAME_SHOP_H