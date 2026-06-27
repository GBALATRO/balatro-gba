#include "item_funcs.h"

#include "card.h"
#include "joker.h"
#include "util.h"
#include "game.h"

// clang-format off
ItemFuncs item_func_table[] = {
    [ITEM_TYPE_JOKER] = {
        .get_buy_price = joker_object_get_buy_price,
        .on_acquired = joker_object_add_to_owned,
        .can_acquire = joker_object_can_acquire
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