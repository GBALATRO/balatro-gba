#include "blind.h"

#include "blind_gfx.h"
#include "graphic_utils.h"
#include "list.h"
#include "stdbool.h"
#include "util.h"

#include <stdlib.h>
#include <tonc.h>

static const unsigned int* blind_gfxTiles[] = {
#define DEF_BLIND_GFX(idx) blind_gfx##idx##Tiles,
#include "../include/def_blind_gfx_table.h"
#undef DEF_BLIND_GFX
};

static const unsigned short* blind_gfxPal[] = {
#define DEF_BLIND_GFX(idx) blind_gfx##idx##Pal,
#include "../include/def_blind_gfx_table.h"
#undef DEF_BLIND_GFX
};

// Bitfields storing blinds we have yet to beat during the current run
static List unbeaten_boss_blinds;
static List unbeaten_showdown_blinds;

// Maps the ante number to the base blind requirement for that ante.
// The game starts at ante 1 which is at index 1 for base requirement 300.
// Ante 0 is also there in case it is ever reached.
static const u32 ante_lut[] = {100, 300, 800, 2000, 5000, 11000, 20000, 35000, 50000};

static Blind _blind_type_map[BLIND_TYPE_MAX] = {
    {BLIND_TYPE_SMALL,   FIX_ONE,          3},
    {BLIND_TYPE_BIG,    (FIX_ONE * 3) / 2, 4},
    {BLIND_TYPE_HOOK,    FIX_ONE * 2,      5},
    {BLIND_TYPE_OX,      FIX_ONE * 2,      5},
    {BLIND_TYPE_HOUSE,   FIX_ONE * 2,      5},
    {BLIND_TYPE_WALL,    FIX_ONE * 2,      5}, // x4 score requirement will be part of the effect
    {BLIND_TYPE_WHEEL,   FIX_ONE * 2,      5},
    {BLIND_TYPE_ARM,     FIX_ONE * 2,      5},
    {BLIND_TYPE_CLUB,    FIX_ONE * 2,      5},
    {BLIND_TYPE_FISH,    FIX_ONE * 2,      5},
    {BLIND_TYPE_PSYCHIC, FIX_ONE * 2,      5},
    {BLIND_TYPE_GOAD,    FIX_ONE * 2,      5},
    {BLIND_TYPE_WATER,   FIX_ONE * 2,      5},
    {BLIND_TYPE_WINDOW,  FIX_ONE * 2,      5},
    {BLIND_TYPE_MANACLE, FIX_ONE * 2,      5},
    {BLIND_TYPE_EYE,     FIX_ONE * 2,      5},
    {BLIND_TYPE_MOUTH,   FIX_ONE * 2,      5},
    {BLIND_TYPE_PLANT,   FIX_ONE * 2,      5},
    {BLIND_TYPE_SERPENT, FIX_ONE * 2,      5},
    {BLIND_TYPE_PILLAR,  FIX_ONE * 2,      5},
    {BLIND_TYPE_NEEDLE,  FIX_ONE,          5},
    {BLIND_TYPE_HEAD,    FIX_ONE * 2,      5},
    {BLIND_TYPE_TOOTH,   FIX_ONE * 2,      5},
    {BLIND_TYPE_FLINT,   FIX_ONE * 2,      5},
    {BLIND_TYPE_MARK,    FIX_ONE * 2,      5},
    {BLIND_TYPE_ACORN,   FIX_ONE * 2,      8},
    {BLIND_TYPE_LEAF,    FIX_ONE * 2,      8},
    {BLIND_TYPE_VESSEL,  FIX_ONE * 2,      8}, // Save as the Wall
    {BLIND_TYPE_HEART,   FIX_ONE * 2,      8},
    {BLIND_TYPE_BELL,    FIX_ONE * 2,      8}
};
// clang-format on

void blind_init()
{
    // Set up the two palettes used by these tokens
    memcpy16(&pal_obj_bank[NORMAL_BLIND_PB], blind_gfxPal[0], NUM_ELEM_IN_ARR(blind_gfx0Pal));
    memcpy16(&pal_obj_bank[BOSS_BLIND_PB], blind_gfxPal[1], NUM_ELEM_IN_ARR(blind_gfx0Pal));

    return;
}

u32 blind_get_requirement(enum BlindType type, int ante)
{
    // Ensure ante is within valid range
    if (ante < 0 || ante > MAX_ANTE)
        ante = 0;

    return fx2int(_blind_type_map[type].score_req_multipler * ante_lut[ante]);
}

int blind_get_reward(enum BlindType type)
{
    return _blind_type_map[type].reward;
}

u16 blind_get_color(enum BlindType type, enum BlindColorIndex index)
{
    // Do a little translation of palette idx -> custom array idx
    u32 color_idx;
    switch (index)
    {
        case BLIND_TEXT_COLOR_INDEX:
            color_idx = 1;
            break;
        case BLIND_SHADOW_COLOR_INDEX:
            color_idx = 2;
            break;
        case BLIND_HIGHLIGHT_COLOR_INDEX:
            color_idx = 3;
            break;
        case BLIND_MAIN_COLOR_INDEX:
            color_idx = 4;
            break;
        case BLIND_BACKGROUND_MAIN_COLOR_INDEX:
            color_idx = 5;
            break;
        case BLIND_BACKGROUND_SECONDARY_COLOR_INDEX:
            color_idx = 6;
            break;
        case BLIND_BACKGROUND_SHADOW_COLOR_INDEX:
            color_idx = 7;
            break;
        default:
            color_idx = 0;
            break;
    }
    return blind_token_palettes[type][color_idx];
}

// Fills the unbeaten boss blinds lists
// This must be called at the beginning of a run
void init_unbeaten_blinds_list(bool showdown)
{
    // create the lists when calling for the first time
    static bool init = false;
    if (!init)
    {
        init = true;
        unbeaten_showdown_blinds = list_create();
        unbeaten_boss_blinds = list_create();
    }

    List* p_unbeaten_blinds = showdown ? &unbeaten_showdown_blinds : &unbeaten_boss_blinds;

    // empty the list just to be sure
    list_clear(p_unbeaten_blinds);

    int lower_blind = showdown ? BLIND_TYPE_ACORN : BLIND_TYPE_HOOK;
    int upper_blind = showdown ? BLIND_TYPE_BELL : BLIND_TYPE_MARK;

    for (int i = lower_blind; i <= upper_blind; i++)
    {
        list_push_back(p_unbeaten_blinds, &_blind_type_map[i]);
    }
}

enum BlindType roll_blind_type(bool showdown)
{
    List* p_unbeaten_blinds = showdown ? &unbeaten_showdown_blinds : &unbeaten_boss_blinds;

    // Fill the list with all blinds if it is empty
    // (happens on startup or if we have beaten all blinds)
    if (list_is_empty(p_unbeaten_blinds))
    {
        init_unbeaten_blinds_list(showdown);
    }

    // roll a random blind among the unbeaten ones
    int random_blind_idx = rand() % list_get_len(p_unbeaten_blinds);
    Blind* random_blind = list_get_at_idx(p_unbeaten_blinds, random_blind_idx);

    return random_blind->type;
}

void set_blind_beaten(enum BlindType type)
{
    bool showdown = (type >= BLIND_TYPE_ACORN);
    List* p_unbeaten_blinds = showdown ? &unbeaten_showdown_blinds : &unbeaten_boss_blinds;

    // find the beaten blind idx in the list
    int beaten_idx = 0;
    Blind* beaten_blind = NULL;
    ListItr itr = list_itr_create(p_unbeaten_blinds);

    while ((beaten_blind = list_itr_next(&itr)))
    {
        if (beaten_blind->type == type)
        {
            break;
        }
        beaten_idx++;
    }

    if (beaten_idx < list_get_len(p_unbeaten_blinds))
    {
        list_remove_at_idx(p_unbeaten_blinds, beaten_idx);
    }
}

static u32 get_blind_pb(enum BlindType type)
{
    return (type <= BLIND_TYPE_BIG) ? NORMAL_BLIND_PB : BOSS_BLIND_PB;
}

void apply_blind_colors(enum BlindType type)
{
    u32 pb = get_blind_pb(type);

    // swap the sprite's palette only if it's a boss blind
    if (type >= BLIND_TYPE_HOOK)
    {
        memcpy16(
            &pal_obj_mem[PAL_ROW_LEN * pb + BLIND_TEXT_COLOR_INDEX],
            &blind_token_palettes[type][1],
            1
        );
        memcpy16(
            &pal_obj_mem[PAL_ROW_LEN * pb + BLIND_SHADOW_COLOR_INDEX],
            &blind_token_palettes[type][2],
            1
        );
        memcpy16(
            &pal_obj_mem[PAL_ROW_LEN * pb + BLIND_HIGHLIGHT_COLOR_INDEX],
            &blind_token_palettes[type][3],
            1
        );
        memcpy16(
            &pal_obj_mem[PAL_ROW_LEN * pb + BLIND_MAIN_COLOR_INDEX],
            &blind_token_palettes[type][4],
            1
        );
    }

    // but always refresh the background colors
    memcpy16(
        &pal_obj_mem[PAL_ROW_LEN * pb + BLIND_BACKGROUND_MAIN_COLOR_INDEX],
        &blind_token_palettes[type][5],
        1
    );
    memcpy16(
        &pal_obj_mem[PAL_ROW_LEN * pb + BLIND_BACKGROUND_SECONDARY_COLOR_INDEX],
        &blind_token_palettes[type][6],
        1
    );
    memcpy16(
        &pal_obj_mem[PAL_ROW_LEN * pb + BLIND_BACKGROUND_SHADOW_COLOR_INDEX],
        &blind_token_palettes[type][7],
        1
    );
}

static u32 get_layer_tile_index(int layer)
{
    // All Blind sprites are stored sequentially and correspond to their IDs
    return (BLIND_BASE_LAYER + layer) * BLIND_SPRITE_OFFSET;
}

void apply_blind_tiles(enum BlindType type, int layer)
{
    u32 tile_offset = (type >= BLIND_TYPE_HOOK) ? type - BLIND_TYPE_HOOK : type;
    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][get_layer_tile_index(layer)],
        &blind_gfxTiles[get_blind_pb(type) - 1][tile_offset * BLIND_SPRITE_COPY_SIZE],
        BLIND_SPRITE_COPY_SIZE
    );

    apply_blind_colors(type);
}

Sprite* blind_token_new(enum BlindType type, int x, int y, int layer)
{
    apply_blind_tiles(type, layer);

    Sprite* sprite = sprite_new(
        ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF,
        ATTR1_SIZE_32x32,
        get_layer_tile_index(layer),
        get_blind_pb(type),
        BLIND_BASE_LAYER + layer
    );
    sprite_position(sprite, x, y);

    return sprite;
}
