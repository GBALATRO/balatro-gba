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

#include "graphic_utils.h"
#include "mgba_logger.h"
#include "random.h"
#include "sprite.h"
#include "util.h"

#include <stdint.h>

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
 * @brief Default Item name, for the sake of consistency
 */
#define ITEM_NAME_UNDEFINED "UNDEFINED"

/**
 * @brief Default subtype info struct declaration
 */
// clang-format off
#define ITEM_SUBTYPE_INFO_DEFAULT {.main_color = 0, .shadow_color = 0, .name_str = ITEM_NAME_UNDEFINED}
// clang-format on

/**
 * @brief Structure containing the main and shadow colors, and the name string associated with an
 *         Item's subtype
 *
 * Shadow colors are always a darker tone of the main color.
 * The colors are organized in the `card_rarity_pal_gfx.png` file which is organized like this:
 *  - 0     -> transparency
 *  - 1,2   -> Common Joker (blue)
 *  - 3,4   -> Uncommon Joker (green)
 *  - 5,6   -> Rare Joker (red)
 *  - 7,8   -> Legendary Joker / Tarot Card (purple)
 *  - 9,10  -> Planet Card (blue with a tint of green)
 *  - 11,12 -> Spectral Card (deep blue)
 *  - 13,14 -> Voucher (red with a tint of orange)
 *
 * @sa get_subtype_info
 */
typedef struct ItemDescSubtypeInfo
{
    u16 main_color;
    u16 shadow_color;
    const char* name_str;
} ItemDescSubtypeInfo;

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

    /**
     * @brief Whether the item is currently held - used in cases where a behaviour needs to be
     * different for Items still in the Shop and the ones we have bought.
     */
    bool is_owned;
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
    Item* (*roll_new)(enum RngSequence key);
    int (*get_buy_price)(Item* item);
    const char* (*get_name)(Item* item);
    ItemDescSubtypeInfo (*get_subtype_info)(Item* item);
    bool (*can_acquire)(Item* item);
    void (*acquire)(Item* item);
    void (*dispose)(Item** item);
    int (*print_description)(Item* item, Rect dest_rect);

    // Optional implementation functions - not required to appear in the shop
    int (*get_sell_price)(Item* item);
} ItemFuncs;

/**
 * @brief Rolls a random item of type @p item_type and returns a newly created one.
 * Manages rollable items set if necessary (i.e. not rolling items already in inventory)
 * To be used when rolling new items for the shop or packs.
 *
 * Matches @ref ItemFuncs.roll_new()
 *
 * @param item_type The type of the item to roll
 * @param key to the RNG sequence used to roll the Item
 *
 * @return The newly created randomly rolled item
 */
Item* item_roll_new(enum ItemType item_type, enum RngSequence key);

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
 * @brief Returns the sell price of the item.
 *
 * Matches @ref ItemFuncs.get_sell_price()
 *
 * @param item The item whose price to return.
 *
 * @return UNDEFINED in case of error, the item's sell price otherwise.
 */
int item_get_sell_price(Item* item);

/**
 * @brief Returns the name of the Item
 *
 * Matches @ref ItemFuncs.get_name()
 *
 * @param item The item whose name to return.
 *
 * @return The item name. In case of error ITEM_NAME_UNDEFINED, will not be NULL
 */
const char* item_get_name(Item* item);

/**
 * @brief Returns the colors and name of the Item's subtype
 *
 * Matches @ref ItemFuncs.get_subtype_info()
 *
 * @param item The item whose subtype's color and name to return.
 *
 * @return Struct containing values of main and shadow colors, as well as the name of the subtype.
 *          In case of an error, all colors will be 0 and the name "UNDEFINED"
 *
 * @sa ItemDescSubtypeInfo
 */
ItemDescSubtypeInfo item_get_subtype_info(Item* item);

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
 * Matches @ref ItemFuncs.dispose()
 *
 * @param item A pointer to an item for destruction.
 */
void item_dispose(Item** item);

/**
 * @brief Prints the item's description inside the given rectangle
 *
 * @param item The item to print the description of
 * @param dest_rect the target rectangle the description needs to fit in
 *
 * Matches @ref ItemFuncs.print_description()
 *
 * @return the number of lines used by the description
 */
int item_print_description(Item* item, Rect dest_rect);

/**
 * @brief Returns whether or not the given Item is in the player's possession
 *
 * @param item the Item to test
 *
 * @return true if the item is owned, false otherwise
 */
bool item_is_owned(Item* item);

/**
 * @brief Performs the item sell transaction, gaining its sell value and discarding it
 * @param item The sold item
 */
void item_sell(Item* item);

/**
 * @brief Prints the buy price under the item
 * Relies on the fact item is a SpriteObject
 *
 * @param item The item to print under
 */
void item_print_buy_price_under(Item* item);

#endif // ITEM_H