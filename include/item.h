#ifndef ITEM_H
#define ITEM_H

#include "sprite.h"

enum ItemType
{
    ITEM_TYPE_JOKER,
    ITEM_TYPE_PLAYING_CARD,
    ITEM_TYPE_CONSUMABLE, // TODO: Expand to PLANET, TAROT, and SPECTRAL?

    ITEM_NUM_TYPES
};

typedef struct Item
{
    SpriteObject;
    enum ItemType type;
} Item;

typedef struct item_funcs
{
    /*
     * @brief Returns the buy price of the item
     */
    int (*get_buy_price)(struct Item* item);

    /**
     * @brief Adds the object to the inventory.
     * Called when it is purchased from the shop.
     * Note that for packs this could just be to open the pack.
     */
    void (*add_to_inventory)(struct Item* item);
} ItemFuncs;

int item_get_buy_price(Item* item);
void item_add_to_inventory(Item* item);

#endif // ITEM_H
