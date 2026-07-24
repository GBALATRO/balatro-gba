#include "item.h"

#include "game.h"
#include "game_variables.h"
#include "item_funcs.h"
#include "mgba_logger.h"
#include "util.h"

#include <tonc.h>

Item* item_roll_new(enum ItemType item_type, enum RngSequence key)
{
    if ((int)item_type < 0 || item_type >= ITEM_NUM_TYPES)
    {
        MGBA_FUNC_ERROR("Invalid type %d", item_type);
        return NULL;
    }

    ItemFuncs* item_funcs = get_item_type_funcs(item_type);

    GBAL_RETURN_IF_NULL_RET(item_funcs, NULL);
    if (item_funcs->roll_new == NULL)
    {
        MGBA_FUNC_ERROR("Unimplemented 'roll_new' function called for item type %d", item_type);
        return NULL;
    }

    Item* ret_item = item_funcs->roll_new(key);
    ret_item->is_owned = false;

    return ret_item;
}

int item_get_buy_price(Item* item)
{
    GBAL_RET_FUNC_RETURN_IF_NULL(item, UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, UNDEFINED);
    if (item_funcs->get_buy_price == NULL)
    {
        MGBA_FUNC_ERROR(
            "Unimplemented 'get_buy_price' function called for item type %d",
            item->type
        );
        return UNDEFINED;
    }

    return item_funcs->get_buy_price(item);
}

int item_get_sell_price(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, UNDEFINED);
    if (item_funcs->get_sell_price == NULL)
    {
        MGBA_FUNC_ERROR(
            "Unimplemented 'get_sell_price' function called for item type %d",
            item->type
        );
        return UNDEFINED;
    }

    return item_funcs->get_sell_price(item);
}

const char* item_get_name(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, ITEM_NAME_UNDEFINED);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, ITEM_NAME_UNDEFINED);
    if (item_funcs->get_name == NULL)
    {
        MGBA_FUNC_ERROR("Unimplemented 'get_name' function called for item type %d", item->type);
        return ITEM_NAME_UNDEFINED;
    }

    // Guarantees no implementation ever returns NULL
    const char* item_name = item_funcs->get_name(item);
    GBAL_RETURN_IF_NULL_RET(item_name, ITEM_NAME_UNDEFINED);

    return item_name;
}

ItemDescSubtypeInfo item_get_subtype_info(Item* item)
{
    ItemDescSubtypeInfo error_info = ITEM_SUBTYPE_INFO_DEFAULT;

    GBAL_RETURN_IF_NULL_RET(item, error_info);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, error_info);
    if (item_funcs->get_subtype_info == NULL)
    {
        MGBA_FUNC_ERROR(
            "Unimplemented 'get_subtype_info' function called for item type %d",
            item->type
        );
        return error_info;
    }

    ItemDescSubtypeInfo ret_info = item_funcs->get_subtype_info(item);
    if (ret_info.name_str == NULL)
    {
        MGBA_FUNC_ERROR("Item of type %d returned NULL as description subtype name", item->type);
        ret_info.name_str = ITEM_NAME_UNDEFINED;
    }

    return ret_info;
}

void item_acquire(Item* item)
{
    GBAL_VOID_FUNC_RETURN_IF_NULL(item);

    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_VOID(item_funcs);
    GBAL_RETURN_IF_NULL_VOID(item_funcs->acquire);
    if (item_funcs->acquire == NULL)
    {
        MGBA_FUNC_ERROR("Unimplemented 'acquire' function called for item type %d", item->type);
        return;
    }

    item_funcs->acquire(item);
    item->is_owned = true;
}

bool item_can_acquire(Item* item)
{
    GBAL_RET_FUNC_RETURN_IF_NULL(item, false);
    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, false);
    if (item_funcs->can_acquire == NULL)
    {
        MGBA_FUNC_ERROR("Unimplemented 'can_acquire' function called for item type %d", item->type);
        return false;
    }

    return item_funcs->can_acquire(item);
}

void item_dispose(Item** item)
{
    GBAL_VOID_FUNC_RETURN_IF_NULL(item);
    GBAL_VOID_FUNC_RETURN_IF_NULL(*item);
    ItemFuncs* item_funcs = get_item_type_funcs((*item)->type);
    GBAL_RETURN_IF_NULL_VOID(item_funcs);
    if (item_funcs->dispose == NULL)
    {
        MGBA_FUNC_ERROR("Unimplemented 'dispose' function called for item type %d", (*item)->type);
        return;
    }

    item_funcs->dispose(item);
}

int item_print_description(Item* item, Rect dest_rect)
{
    GBAL_RETURN_IF_NULL_RET(item, 0);
    ItemFuncs* item_funcs = get_item_type_funcs(item->type);
    GBAL_RETURN_IF_NULL_RET(item_funcs, 0);
    if (item_funcs->print_description == NULL)
    {
        MGBA_FUNC_ERROR(
            "Unimplemented 'print_description' function called for item type %d",
            item->type
        );
        return 0;
    }

    return item_funcs->print_description(item, dest_rect);
}

bool item_is_owned(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, false);
    return item->is_owned;
}

void item_sell(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);

    int sell_price = item_get_sell_price(item);

    if (sell_price == UNDEFINED)
    {
        MGBA_FUNC_ERROR("Undefined sell price for item of type %d", item->type);
        return;
    }

    g_game_vars.money += sell_price;
    display_money();
    sprite_object_erase_text_under((SpriteObject*)item);
    item_start_discard_animation(item);
}

void item_print_buy_price_under(Item* item)
{
    GBAL_VOID_FUNC_RETURN_IF_NULL(item);
    sprite_object_print_price_under((SpriteObject*)item, item_get_buy_price(item));
}
