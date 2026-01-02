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

// Palettes for the blinds (Transparency, Text Color, Shadow, Highlight, Main Color) Use this:
// http://www.budmelvin.com/dev/15bitconverter.html
// clang-format off
static const u16 blind_token_palettes[BLIND_TYPE_MAX][PAL_ROW_LEN] = {
    // Normal Blinds
    {0x0000, 0x7FFF, 0x34A1, 0x5DCB, 0x5104, 0x55A0, 0x2D01, 0x34E0},
    {0x0000, 0x2527, 0x15F5, 0x36FC, 0x1E9C, 0x01B4, 0x0D0A, 0x010E},
    // Boss Blinds
    {0x0000, 0x088B, 0x0CD0, 0x29B7, 0x1115, 0x0492, 0x0C89, 0x044A}, // Hook
    {0x0000, 0x00AB, 0x0112, 0x1DF9, 0x0577, 0x0115, 0x08CA, 0x008B}, // Ox
    {0x0000, 0x2D25, 0x3D87, 0x5E8F, 0x560A, 0x4DC8, 0x2905, 0x28E2}, // House
    {0x0000, 0x2CC9, 0x3D0D, 0x5E14, 0x5171, 0x490E, 0x28C8, 0x34AA}, // Wall
    {0x0000, 0x21A5, 0x2E47, 0x4F2F, 0x3EEA, 0x36C7, 0x2184, 0x1D83}, // Wheel
    {0x0000, 0x7EB5, 0x5929, 0x7A31, 0x798D, 0x712A, 0x38E6, 0x3C85}, // Arm
    {0x0000, 0x29CC, 0x3A72, 0x5759, 0x4B37, 0x4715, 0x258A, 0x2E2F}, // Club
    {0x0000, 0x6AD2, 0x4585, 0x668D, 0x5E07, 0x55C4, 0x3103, 0x2D03}, // Fish
    {0x0000, 0x11B0, 0x1656, 0x333E, 0x1EFD, 0x12DC, 0x156D, 0x05B1}, // Psychic
    {0x0000, 0x28CC, 0x3911, 0x5619, 0x4977, 0x4115, 0x24CA, 0x248C}, // Goad
    {0x0000, 0x41ED, 0x5AB2, 0x779A, 0x7798, 0x6F57, 0x39AB, 0x3DCD}, // Water
    {0x0000, 0x296B, 0x39F0, 0x56F7, 0x4A95, 0x4653, 0x2549, 0x254A}, // Window
    {0x0000, 0x5294, 0x2108, 0x4210, 0x294A, 0x2107, 0x18C5, 0x0C63}, // Manacle
    {0x0000, 0x3CE5, 0x5967, 0x764E, 0x71C9, 0x6967, 0x38E4, 0x2840}, // Eye
    {0x0000, 0x24EC, 0x3550, 0x5258, 0x45D5, 0x3973, 0x24EA, 0x24CC}, // Mouth
    {0x0000, 0x2968, 0x31AA, 0x52B2, 0x3E2D, 0x3A0B, 0x2126, 0x1CE5}, // Plant
    {0x0000, 0x1544, 0x1DC6, 0x3ACD, 0x2668, 0x1A25, 0x1524, 0x0D02}, // Serpent
    {0x0000, 0x14E8, 0x1D2B, 0x3E33, 0x298F, 0x1D4D, 0x18E7, 0x0C86}, // Pillar
    {0x0000, 0x0CE6, 0x1148, 0x2E30, 0x19AB, 0x0968, 0x10E5, 0x0482}, // Needle
    {0x0000, 0x314C, 0x45D0, 0x62D7, 0x5A75, 0x5233, 0x2D4A, 0x314B}, // Head
    {0x0000, 0x0C6C, 0x1091, 0x2D78, 0x14B6, 0x0C54, 0x0C6A, 0x0008}, // Tooth
    {0x0000, 0x0CEF, 0x1156, 0x2E3D, 0x15BC, 0x095B, 0x10EC, 0x04B0}, // Flint
    {0x0000, 0x1467, 0x18AA, 0x39B1, 0x20ED, 0x146A, 0x1486, 0x0404}, // Mark
    // Showdown Blinds
    {0x0000, 0x01F7, 0x01F7, 0x1EFF, 0x029F, 0x025E, 0x054E, 0x0D4F}, // Acorn
    {0x0000, 0x31E8, 0x31E8, 0x52EF, 0x428A, 0x3A48, 0x2545, 0x1D23}, // Leaf
    {0x0000, 0x554D, 0x554D, 0x7654, 0x71D1, 0x696E, 0x38E8, 0x2CA6}, // Vessel
    {0x0000, 0x1090, 0x1090, 0x3198, 0x18D5, 0x0C53, 0x1089, 0x0427}, // Heart
    {0x0000, 0x65E0, 0x65E0, 0x7EC7, 0x7E60, 0x7A20, 0x4140, 0x3922}, // Bell
};

static Blind _blind_type_map[BLIND_TYPE_MAX] = {
    {BLIND_TYPE_SMALL,   FIX_ONE,          3},
    {BLIND_TYPE_BIG,    (FIX_ONE * 3) / 2, 4},
    {BLIND_TYPE_HOOK,    FIX_ONE * 2,      5},
    {BLIND_TYPE_OX,      FIX_ONE * 2,      5},
    {BLIND_TYPE_HOUSE,   FIX_ONE * 2,      5},
    {BLIND_TYPE_WALL,    FIX_ONE * 4,      5},
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
    {BLIND_TYPE_VESSEL,  FIX_ONE * 6,      8},
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
        if (beaten_blind != NULL && beaten_blind->type == type)
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
        &tile_mem[4][get_layer_tile_index(layer)],
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
