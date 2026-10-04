/**
 * @file pack.c
 */
#include "pack.h"

#include "card.h"
#include "game.h"
#include "graphic_utils.h"
#include "joker.h"
#include "list.h"
#include "util.h"

#include <tonc.h>

extern const u32 pack_gfxTiles[];
extern const u32 planet_pack_gfxTiles[];

// Only one pack of each kind is on sale at a time
static PackObject s_joker_pack_object;
static PackObject s_planet_pack_object;

bool item_is_pack(const Item* item)
{
    return item != NULL && (item->type == ITEM_TYPE_PACK || item->type == ITEM_TYPE_PLANET_PACK);
}

enum ItemType pack_get_content_type(enum ItemType pack_type)
{
    return (pack_type == ITEM_TYPE_PLANET_PACK) ? ITEM_TYPE_PLANET : ITEM_TYPE_JOKER;
}

const char* pack_get_name(enum ItemType pack_type)
{
    return (pack_type == ITEM_TYPE_PLANET_PACK) ? "Planet Pack" : "Joker Pack";
}

int pack_object_print_desc(enum ItemType pack_type, Rect dest_rect)
{
    static const char joker_desc[] =
        TTE_BLACK_TAG "Choose " TTE_DARK_BLUE_TAG "1 " TTE_BLACK_TAG "of " TTE_DARK_BLUE_TAG "2 "
                      TTE_BLACK_TAG "Jokers for free";
    static const char planet_desc[] =
        TTE_BLACK_TAG "Choose " TTE_DARK_BLUE_TAG "1 " TTE_BLACK_TAG "of " TTE_DARK_BLUE_TAG "2 "
                      TTE_BLACK_TAG "Planets for free";
    return tte_printf_justified_in_rect(
        (pack_type == ITEM_TYPE_PLANET_PACK) ? planet_desc : joker_desc,
        dest_rect,
        JUSTIFY_CENTER,
        SCREEN_LEFT,
        true
    );
}

static void s_pack_object_destroy(PackObject* pack)
{
    if (pack == NULL || !pack->in_use)
        return;

    sprite_object_destroy((SpriteObject*)pack);
    joker_sprite_layer_free(pack->layer);
    pack->in_use = false;
}

static Item* s_pack_object_new(PackObject* pack, enum ItemType pack_type, const u32* tiles)
{
    if (pack->in_use)
        return NULL;

    // Packs borrow a sprite slot (OAM entry + 16 tiles) from the Joker sprite slots
    int layer = joker_sprite_layer_alloc();
    if (layer == UNDEFINED)
        return NULL;

    sprite_object_init((SpriteObject*)pack);
    pack->type = pack_type;
    pack->layer = (s8)layer;
    pack->in_use = true;

    int tile_index = JOKER_TID + layer * JOKER_SPRITE_OFFSET;
    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index],
        tiles,
        TILE_SIZE * JOKER_SPRITE_OFFSET
    );

    sprite_object_set_sprite(
        (SpriteObject*)pack,
        sprite_new(
            ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF,
            ATTR1_SIZE_32,
            tile_index,
            CARD_PB,
            JOKER_STARTING_LAYER + layer
        )
    );

    return (Item*)pack;
}

Item* pack_object_roll_new(enum RngSequence key)
{
    return s_pack_object_new(&s_joker_pack_object, ITEM_TYPE_PACK, pack_gfxTiles);
}

Item* planet_pack_object_roll_new(enum RngSequence key)
{
    return s_pack_object_new(&s_planet_pack_object, ITEM_TYPE_PLANET_PACK, planet_pack_gfxTiles);
}

int pack_object_get_buy_price(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, UNDEFINED);
    return PACK_BUY_PRICE;
}

bool pack_object_can_acquire(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, false);

    if (item->type == ITEM_TYPE_PLANET_PACK)
        return true;

    // No point opening a Joker Pack with no free Joker slot
    return (list_get_len(get_jokers_list()) < MAX_JOKERS_HELD_SIZE);
}

void pack_object_acquire(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    if (!item_is_pack(item))
        return;

    // The pack is consumed, the shop takes care of offering its content
    s_pack_object_destroy((PackObject*)item);
}

void pack_object_dispose(Item** item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    GBAL_RETURN_IF_NULL_VOID(*item);
    if (!item_is_pack(*item))
        return;

    s_pack_object_destroy((PackObject*)(*item));
    *item = NULL;
}
