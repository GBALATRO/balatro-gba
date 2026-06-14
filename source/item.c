#include "item.h"

#include "item_funcs.h"
#include "mgba_logger.h"
#include "util.h"

#include <tonc.h>

int item_get_buy_price(Item* item)
{
    CHECK_NULL_ARG_RET(item, UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    if (item_funcs == NULL || item_funcs->get_buy_price == NULL)
    {
        MGBA_ERROR(__func__ ": object function not implemented");
        return UNDEFINED;
    }

    return item_funcs->get_buy_price(item);
}

void item_add_to_inventory(Item* item)
{
    CHECK_NULL_ARG_VOID(item);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    if (item_funcs == NULL || item_funcs->add_to_inventory == NULL)
    {
        MGBA_FUNC_ERROR("object function not implemented");
        return;
    }

    item_funcs->add_to_inventory(item);
}
