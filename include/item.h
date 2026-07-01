/**
 * @file item.h
 *
 * @brief The core structure for items in the shop and inventory.
 * Provides a common API for the shop and inventory to handle all types of items.
 * Uses struct inheritance so all items can implement an is-a relationship with Item.
 *
 * This means that pointers to structs that inherit Item using first member struct inheritance
 * can and should be cast to Item* so code that expects an Item* can use them.
 */

#ifndef ITEM_H
#define ITEM_H

#include "sprite.h"
#include "mgba_logger.h"

// TODO: Document
#define CHECK_ITEM_TYPE_RET(item, expected_type, ret_val)                        \
    do                                                                           \
    {                                                                            \
        if ((item)->type != expected_type)                                       \
        {                                                                        \
            MGBA_FUNC_ERROR("Unexpected %s->type != %s", #item, #expected_type); \
            return (ret_val);                                                    \
        }                                                                        \
    } while (0)

// TODO: Document
#define CHECK_ITEM_TYPE_VOID(item, expected_type)                                \
    do                                                                           \
    {                                                                            \
        if ((item)->type != expected_type)                                       \
        {                                                                        \
            MGBA_FUNC_ERROR("Unexpected %s->type != %s", #item, #expected_type); \
            return;                                                              \
        }                                                                        \
    } while (0)

enum ItemType
{
    ITEM_TYPE_JOKER,
    ITEM_TYPE_PLAYING_CARD,

    // Future planned item implementations
    // ITEM_TYPE_CONSUMABLE, // Expand to PLANET, TAROT, and SPECTRAL?
    // ITEM_TYPE_VOUCHER,
    // ITEM_TYPE_PACK

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
    /**
     * All items must implement the following since they are called by the shop.
     * For some of them they can be no-op or return true implementations.
     */
    Item* (*roll_new)(void);
    int (*get_buy_price)(Item* item);
    void (*acquire)(Item* item);
    void (*dispose)(Item** item);
    bool (*can_acquire)(Item* item);
} ItemFuncs;

// TODO: Document
Item* item_roll_new(enum ItemType item_type);

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
 * perform the purchase operation of decrementing the player's money,
 * that should be handled by the shop code.
 * For packs this can be to just open the pack,
 * for vouchers, this will apply their effect.
 *
 * Matches ItemFuncs.on_acquired()
 *
 * @param item The item to on_acquired
 */
void item_acquire(Item* item);

/**
 * @brief Returns true if the item can be acquired, i.e. added to inventory
 * Does not check if the player has enough money to buy the item, that is the shop's job,
 * as this will be used both when purchasing and when selecting in a pack.
 */
bool item_can_acquire(Item* item);

// TODO: Document
void item_dispose(Item** item);

/*
 * @brief Prints the buy price under the item
 * Uses the fact item is a SpriteObject
 */
void item_print_buy_price_under(Item* item);

#endif // ITEM_H
