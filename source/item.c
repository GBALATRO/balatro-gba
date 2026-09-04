#include "item.h"

#include "item_funcs.h"
#include "mgba_logger.h"
#include "util.h"

#include <tonc.h>

Item* item_roll_new(enum ItemType item_type, enum RngSequence key)
{
    if ((int)item_type < 0 || item_type >= ITEM_NUM_TYPES)
    {
        MGBA_FUNC_ERROR("Undefined type %d", item_type);
        return NULL;
    }

    ItemFuncs* item_funcs = get_item_type_funcs(item_type);
    GBAL_RETURN_IF_NULL(item_funcs, NULL);
    GBAL_RETURN_IF_NULL(item_funcs->roll_new, NULL);

    return item_funcs->roll_new(key);
}

int item_get_buy_price(Item* item)
{
    GBAL_RETURN_IF_NULL(item, UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL(item_funcs, UNDEFINED);
    GBAL_RETURN_IF_NULL(item_funcs->get_buy_price, UNDEFINED);

    return item_funcs->get_buy_price(item);
}

void item_acquire(Item* item)
{
    GBAL_RETURN_IF_NULL(item, RET_NONE);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL(item_funcs, RET_NONE);
    GBAL_RETURN_IF_NULL(item_funcs->acquire, RET_NONE);

    item_funcs->acquire(item);
}

bool item_can_acquire(Item* item)
{
    GBAL_RETURN_IF_NULL(item, false);
    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL(item_funcs, false);
    GBAL_RETURN_IF_NULL(item_funcs->can_acquire, false);

    return item_funcs->can_acquire(item);
}

void item_dispose(Item** item)
{
    GBAL_RETURN_IF_NULL(item, RET_NONE);
    GBAL_RETURN_IF_NULL(*item, RET_NONE);
    ItemFuncs* item_funcs = get_item_type_funcs((*item)->type);
    GBAL_RETURN_IF_NULL(item_funcs, RET_NONE);
    GBAL_RETURN_IF_NULL(item_funcs->dispose, RET_NONE);

    item_funcs->dispose(item);
}

void item_print_buy_price_under(Item* item)
{
    GBAL_RETURN_IF_NULL(item, RET_NONE);
    sprite_object_print_price_under((SpriteObject*)item, item_get_buy_price(item));
}