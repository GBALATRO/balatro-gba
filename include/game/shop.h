/**
 * @file shop.h
 *
 * @brief Shop state functions.
 */
#ifndef GAME_SHOP_H
#define GAME_SHOP_H

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