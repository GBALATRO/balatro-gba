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

typedef struct Item
{
    SpriteObject;
    enum ItemType type;
} Item;

typedef struct item_funcs
{
    int (*get_buy_price)(struct Item* item);
    void (*acquire)(struct Item* item);
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

#endif // ITEM_H
