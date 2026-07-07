#include "item_funcs.h"

#include "card.h"
#include "game.h"
#include "game/shop.h"
#include "joker.h"
#include "util.h"

static Item* item_roll_new_unimplemented();
static void item_acquire_unimplemented(Item* item);
static bool item_always_can_acquire(Item* item);

// clang-format off
ItemFuncs item_func_table[] = {
    [ITEM_TYPE_JOKER] = {
        .roll_new = joker_object_roll_new,
        .get_buy_price = joker_object_get_buy_price,
        .acquire = joker_object_add_to_owned,
        .can_acquire = joker_object_can_acquire,
        .dispose = joker_object_dispose
    },
    
    /* Currently playing cards have partial implementations since Magic Trick is not implemented
     * and playing cards can't appear in the shop.
     * If the Magic Trick voucher is implemented, these need to be completed.
     * Playing cards must stay the only exception, the rest of the item types must implement
     * all the basic functions to appear in the shop.
     */
    [ITEM_TYPE_PLAYING_CARD] = {
        .roll_new = item_roll_new_unimplemented,
        .get_buy_price = card_object_get_buy_price,
        .acquire = item_acquire_unimplemented,
        .can_acquire = item_always_can_acquire,
        .dispose = card_object_dispose
    }
};
// clang-format on

ItemFuncs* get_item_type_funcs(enum ItemType type)
{
    if ((int)type < 0 || type >= NUM_ELEM_IN_ARR(item_func_table))
    {
        MGBA_FUNC_ERROR("Invalid type %d", type);
        return NULL;
    }

    return &item_func_table[type];
}

static Item* item_roll_new_unimplemented()
{
    MGBA_FUNC_ERROR("Unimplemented roll_new function called");
    return NULL;
}

static void item_acquire_unimplemented(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    MGBA_FUNC_ERROR("Unimplemented acquire function called for item type type %d", (item)->type);
}

static bool item_always_can_acquire(Item* item)
{
    return true;
}