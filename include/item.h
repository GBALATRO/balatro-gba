/**
 * @file item.h
 *
 * @brief The core structure for items in the shop and inventory.
 * Provides a common API for the shop and inventory to handle all types of items.
 * Uses struct inheritance so all items can implement an is-a relationship with Item.
 *
 * This means that pointers to structs that inherit Item using first member struct inhertiance
 * can and should be cast to Item* so code that expects an Item* can use them.
 */

#ifndef ITEM_H
#define ITEM_H

#include "sprite.h"

enum ItemType
{
    ITEM_TYPE_JOKER,
    ITEM_TYPE_CONSUMABLE, // TODO: Expand to PLANET, TAROT, and SPECTRAL?
    ITEM_TYPE_PLAYING_CARD,

    ITEM_NUM_TYPES
};

typedef struct
{
    SpriteObject; // First member struct inheritance
    // All items in the shop and inventory are SpriteObjects.
    // Casts from Item* (or inheriting structs) to SpriteObject* are allowed and intentional.

    enum ItemType type;
} Item;

typedef struct item_funcs
{
    int (*get_buy_price)(Item* item);
    void (*acquire)(Item* item);
} ItemFuncs;

/*
 * @brief Returns the buy price of the item
 *
 * Matches ItemFuncs.get_buy_price()
 *
 * @param item The item whose price to return.
 *
 * @return UNDEFINED in case of error, the item's buy price otherwise.
 */
int item_get_buy_price(Item* item);

/**
 * @brief Acquires the item, adding to inventory if applicable.
 * Called when it is purchased from the shop, note that it does not
 * perform the pruchase operation of decrementing the player's money,
 * that should be handled by the shop code.
 * For packs this can be to just open the pack,
 * for vouchers, this will apply their effect.
 *
 * Matches ItemFuncs.acquire()
 *
 * @param item The item to acquire
 */
void item_acquire(Item* item);

/*
 * @brief Prints the buy price under the item
 * Uses the fact item is a SpriteObject
 */
void item_print_buy_price_under(Item* item);

#endif // ITEM_H
