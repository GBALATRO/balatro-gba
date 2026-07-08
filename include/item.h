/**
 * @file item.h
 *
 * @brief The core structure for items in the shop and inventory.
 * Provides a common API for the shop and inventory to handle all types of items.
 * Uses struct inheritance so all inherited items can implement an is-a relationship with Item.
 * This means that pointers to structs that inherit Item using first member struct inheritance
 * can and should be cast to Item* so code that expects an Item* can use them.
 */

#ifndef ITEM_H
#define ITEM_H

#include "mgba_logger.h"
#include "sprite.h"

// TODO: Merge these with GBAL_RETURN_IF_NULL macros?
/**
 * @brief Checks if @p item is of type @p expected_type - logs error message and returns otherwise
 *
 *  This version is for a function that returns a value, while
 *  @ref ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID is for a void function.
 */
#define ITEM_RETURN_IF_UNEXPECTED_TYPE_RET(item, expected_type, ret_val)         \
    do                                                                           \
    {                                                                            \
        if ((item)->type != expected_type)                                       \
        {                                                                        \
            MGBA_FUNC_ERROR("Unexpected %s->type != %s", #item, #expected_type); \
            return (ret_val);                                                    \
        }                                                                        \
    } while (0)

/**
 * @brief Checks if @p item is of type @p expected_type - logs error message and returns otherwise
 *
 * This version is for a void function, while @ref ITEM_RETURN_IF_UNEXPECTED_TYPE_RET is for one
 * with a return value.
 */
#define ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID(item, expected_type)                 \
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

    // Future planned item types
    // ITEM_TYPE_CONSUMABLE, // Expand to PLANET, TAROT, and SPECTRAL?
    // ITEM_TYPE_VOUCHER,
    // ITEM_TYPE_PACK

    ITEM_NUM_TYPES
};

/**
 * @brief A generic interface for all items that can appear in the shop or be in the inventory.
 * This uses first member struct inheritance - other structs are meant to inherit it by
 * making their first member field Item.
 * Then casts from inheriting structs to Item* are allowed and intentional and this allows for
 * generic code that uses polymorphism.
 * The -fms-extensions compile flag allows for anonymous members making it behave fully
 * as inheritance. It makes all member fields be fully inherited so any struct
 * that inherits Item for example will have all its fields accessible directly,
 * e.g. `JokerObject joker_object; joker_object.type = ITEM_TYPE_JOKER`
 */
typedef struct Item
{
    /**
     * @brief First member struct inheritance
     * all items that can appear in the shop are SpriteObjects.
     * Note that this is an anonymous member.
     */
    SpriteObject;

    /**
     * @brief The item type - used to dispatch the function implementations for inheriting types.
     */
    enum ItemType type;
} Item;

/**
 * @brief The set of functions that each item type implements.
 */
typedef struct ItemFuncs
{
    /**
     * All items must implement the following since they are called by the shop and all items
     * must be capable of appearing in the shop.
     */
    Item* (*roll_new)(void);
    int (*get_buy_price)(Item* item);
    bool (*can_acquire)(Item* item);
    void (*acquire)(Item* item);
    void (*dispose)(Item** item);
    // TODO: void (*print_description)(Item* item); // or something of the form

    // Optional implementation functions will be added here
} ItemFuncs;

/**
 * @brief Rolls a random item of type @p item_type and returns a newly created one.
 * Manages rollable items set if necessary (i.e. not rolling items already in inventory)
 * To be used when rolling new items for the shop or packs.
 *
 * Matches @ref ItemFuncs.roll_new()
 *
 * @param item_type The type of the item to roll
 *
 * @return The newly created randomly rolled item
 */
Item* item_roll_new(enum ItemType item_type);

/**
 * @brief Returns the buy price of the item.
 *
 * Matches @ref ItemFuncs.get_buy_price()
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
 * Matches @ref ItemFuncs.acquire()
 *
 * @param item The item to acquire
 */
void item_acquire(Item* item);

/**
 * @brief Returns true if the item can be acquired, i.e. added to inventory.
 * Does not check if the player has enough money to buy the item, that is the shop's job,
 * as this will be used both when purchasing and when selecting in a pack.
 *
 * Matches @ref ItemFuncs.can_acquire()
 *
 * @param item The item to check
 */
bool item_can_acquire(Item* item);

/**
 * @brief Destroys an item, freeing underlying resources, and manages rollable items sets if needed.
 * To be used when destroying items from the inventory, shop, or packs.
 *
 * @param item A pointer to an item for destruction.
 */
void item_dispose(Item** item);

/**
 * @brief Prints the buy price under the item
 * Relies on the fact item is a SpriteObject
 *
 * @param item The item to print under
 */
void item_print_buy_price_under(Item* item);

#endif // ITEM_H
