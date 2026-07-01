#include "item_funcs.h"

#include "card.h"
#include "game.h"
#include "game/shop.h"
#include "joker.h"
#include "util.h"

// clang-format off
ItemFuncs item_func_table[] = {
    [ITEM_TYPE_JOKER] = {
        .roll_new = joker_object_roll_new,
        .get_buy_price = joker_object_get_buy_price,
        .acquire = joker_object_add_to_owned,
        .can_acquire = joker_object_can_acquire,
        .dispose = joker_object_dispose
    }
    // TODO: implement for ITEM_TYPE_PLAYING_CARD... etc.
    // Currently unimplemented functions are handled by get_item_type_funcs()
};
// clang-format on

ItemFuncs* get_item_type_funcs(enum ItemType type)
{
    ItemFuncs* ret_val = NULL;
    if (type < ITEM_NUM_TYPES && type < NUM_ELEM_IN_ARR(item_func_table))
    {
        ret_val = &item_func_table[type];
    }
    return ret_val;
}