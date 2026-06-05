#include "object_funcs.h"
#include "joker.h"
#include "card.h"
#include "util.h"

ObjectFuncs object_func_table[] = {
    [OBJ_TYPE_JOKER] = {
        .get_buy_price = joker_object_get_buy_price,
        .add_to_inventory = joker_object_add_to_owned
    }
    // TODO: OBJ_TYPE_PLAYING_CARD... etc.
};

ObjectFuncs* get_object_type_funcs(enum ObjectType type)
{
    ObjectFuncs* ret_val = NULL;
    if (type < OBJ_NUM_TYPES && type < NUM_ELEM_IN_ARR(object_func_table))
    {
        ret_val = &object_func_table[type];
    }
    return ret_val;
}