/**
 * @file shop.h
 *
 * @brief Shop state functions.
 */
#ifndef GAME_SHOP_H
#define GAME_SHOP_H

#include "item.h"

#include <stdbool.h>

/**
 * @brief Initialize the shop for a run.
 * Resets all the shop data for the run, needs to be called once per run.
 */
void shop_reset(void);

/**
 * @brief Get the current cost to reroll the items sold in the shop
 *
 * @return int
 */
int shop_get_reroll_cost(void);

/**
 * @brief Set the current cost to reroll the items sold in the shop to the given value
 *
 * @param cost new reroll price for the shop
 */
void shop_set_reroll_cost(int cost);

/**
 * @brief Get a pointer to the Card we are currently showing the description of.
 *
 * @return Item pointer, NULL if no description is currently being shown
 */
Item* shop_get_description_item(void);

/**
 * @brief Change to the shop background
 */
void shop_change_background(void);

/**
 * @brief Shop state initialization
 */
void shop_on_init(void);

/**
 * @brief Shop state update
 */
void shop_on_update(void);

/**
 * @brief Shop cleanup
 */
void shop_on_exit(void);

#endif // GAME_SHOP_H
