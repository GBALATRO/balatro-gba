#include "item.h"

#include "item_funcs.h"
#include "mgba_logger.h"
#include "util.h"

#include <tonc.h>

int item_get_buy_price(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, UNDEFINED);
    GBAL_RETURN_IF_NULL_RET(item_funcs->get_buy_price, UNDEFINED);

    return item_funcs->get_buy_price(item);
}

void item_on_acquired(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_VOID(item_funcs);
    GBAL_RETURN_IF_NULL_VOID(item_funcs->on_acquired);

    item_funcs->on_acquired(item);
}

bool item_can_acquire(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, false);
    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, false);
    GBAL_RETURN_IF_NULL_RET(item_funcs->can_acquire, false);

    return item_funcs->can_acquire(item);
}

void item_destroy(Item** item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    GBAL_RETURN_IF_NULL_VOID(*item);
    ItemFuncs* item_funcs = get_item_type_funcs((*item)->type);
    GBAL_RETURN_IF_NULL_VOID(item_funcs);
    GBAL_RETURN_IF_NULL_VOID(item_funcs->destroy);

    return item_funcs->destroy(item);
}

void item_set_available_to_shop(Item* item, bool available)
{
    GBAL_RETURN_IF_NULL_VOID(item);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_VOID(item_funcs);
    GBAL_RETURN_IF_NULL_VOID(item_funcs->set_available_to_shop);

    item_funcs->set_available_to_shop(item, available);
}

void item_print_buy_price_under(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    sprite_object_print_price_under((SpriteObject*)item, item_get_buy_price(item));
}