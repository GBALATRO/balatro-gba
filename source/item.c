#include "item.h"

#include "item_funcs.h"
#include "mgba_logger.h"
#include "util.h"

#include <tonc.h>

int item_get_buy_price(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    if (item_funcs == NULL || item_funcs->get_buy_price == NULL)
    {
        MGBA_FUNC_ERROR("Item function not implemented");
        return UNDEFINED;
    }

    return item_funcs->get_buy_price(item);
}

void item_on_acquired(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    if (item_funcs == NULL || item_funcs->on_acquired == NULL)
    {
        MGBA_FUNC_ERROR("object function not implemented");
        return;
    }

    item_funcs->on_acquired(item);
}

void item_print_buy_price_under(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    sprite_object_print_price_under((SpriteObject*)item, item_get_buy_price(item));
}