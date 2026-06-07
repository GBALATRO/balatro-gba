#include "item_funcs.h"
#include "joker.h"
#include "card.h"
#include "util.h"

ItemFuncs item_func_table[] = {
    [ITEM_TYPE_JOKER] = {
        .get_buy_price = joker_object_get_buy_price,
        .add_to_inventory = joker_object_add_to_owned
    }
    // TODO: ITEM_TYPE_PLAYING_CARD... etc.
};

ItemFuncs* get_item_type_funcs(enum ItemType type)
{
    ItemFuncs* ret_val = NULL;
    if (type < ITEM_NUM_TYPES && type < NUM_ELEM_IN_ARR(item_func_table))
    {
        ret_val = &item_func_table[type];
    }
    return ret_val;
}