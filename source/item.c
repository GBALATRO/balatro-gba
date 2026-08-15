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
    GBAL_RETURN_IF_NULL_RET(item_funcs, NULL);
    GBAL_RETURN_IF_NULL_RET(item_funcs->roll_new, NULL);

    return item_funcs->roll_new(key);
}

int item_get_buy_price(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, UNDEFINED);
    GBAL_RETURN_IF_NULL_RET(item_funcs->get_buy_price, UNDEFINED);

    return item_funcs->get_buy_price(item);
}

int item_get_sell_price(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, UNDEFINED);
    GBAL_RETURN_IF_NULL_RET(item_funcs->get_sell_price, UNDEFINED);

    return item_funcs->get_sell_price(item);
}

const char* item_get_name(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, NULL);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, NULL);
    GBAL_RETURN_IF_NULL_RET(item_funcs->get_name, NULL);

    return item_funcs->get_name(item);
}

const char* item_get_subtype_string(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, NULL);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, NULL);
    GBAL_RETURN_IF_NULL_RET(item_funcs->get_subtype_str, NULL);

    return item_funcs->get_subtype_str(item);
}

uint32_t item_get_subtype_colors(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, 0);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, 0);
    GBAL_RETURN_IF_NULL_RET(item_funcs->get_subtype_colors, 0);

    return item_funcs->get_subtype_colors(item);
}

void item_acquire(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_VOID(item_funcs);
    GBAL_RETURN_IF_NULL_VOID(item_funcs->acquire);

    item_funcs->acquire(item);
}

bool item_can_acquire(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, false);
    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, false);
    GBAL_RETURN_IF_NULL_RET(item_funcs->can_acquire, false);

    return item_funcs->can_acquire(item);
}

void item_dispose(Item** item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    GBAL_RETURN_IF_NULL_VOID(*item);
    ItemFuncs* item_funcs = get_item_type_funcs((*item)->type);
    GBAL_RETURN_IF_NULL_VOID(item_funcs);
    GBAL_RETURN_IF_NULL_VOID(item_funcs->dispose);

    item_funcs->dispose(item);
}

void item_print_buy_price_under(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    sprite_object_print_price_under((SpriteObject*)item, item_get_buy_price(item));
}

int item_print_description(Item* item, Rect dest_rect)
{
    GBAL_RETURN_IF_NULL_RET(item, 0);
    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, 0);
    GBAL_RETURN_IF_NULL_RET(item_funcs->print_desc, 0);

    return item_funcs->print_desc(item, dest_rect);
}