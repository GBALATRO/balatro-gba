/**
 * @file pack.h
 *
 * @brief Packs: shop items that, once bought, let the player pick one of their content for free.
 * The pick itself is handled by the shop, the pack object is only the purchasable item.
 * Two kinds exist: Joker Packs (ITEM_TYPE_PACK) and Planet Packs (ITEM_TYPE_PLANET_PACK).
 */
#ifndef PACK_H
#define PACK_H

#include "graphic_utils.h"
#include "item.h"

#define PACK_BUY_PRICE   4
#define PACK_NUM_CHOICES 2

typedef struct PackObject
{
    Item; // First member struct inheritance
    s8 layer;
    bool in_use;
} PackObject;

/** @brief true if @p item is a pack of any kind */
bool item_is_pack(const Item* item);

/** @brief The type of items a pack of type @p pack_type offers */
enum ItemType pack_get_content_type(enum ItemType pack_type);

const char* pack_get_name(enum ItemType pack_type);

/** @brief Print the pack description in @p dest_rect, returns the number of lines used. */
int pack_object_print_desc(enum ItemType pack_type, Rect dest_rect);

// ItemFuncs implementations
Item* pack_object_roll_new(enum RngSequence key);
Item* planet_pack_object_roll_new(enum RngSequence key);
int pack_object_get_buy_price(Item* item);
bool pack_object_can_acquire(Item* item);
void pack_object_acquire(Item* item);
void pack_object_dispose(Item** item);

#endif // PACK_H
