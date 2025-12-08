#include "blind.h"

#include "blind_gfx.h"
#include "graphic_utils.h"
#include "list.h"
#include "util.h"
#include "stdbool.h"

#include <stdlib.h>
#include <tonc.h>

static const unsigned int *blind_gfxTiles[] =
{
#define DEF_BLIND_GFX(idx) blind_gfx##idx##Tiles,
#include "../include/def_blind_gfx_table.h"
#undef DEF_BLIND_GFX
};
/*
static const unsigned short *blind_gfxPal[] = 
{
#define DEF_BLIND_GFX(idx) blind_gfx##idx##Pal,
#include "../include/def_blind_gfx_table.h"
#undef DEF_BLIND_GFX
};
*/

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
    {0x0000, 0x088B, 0x0CD0, 0x29B7, 0x1115, 0x0CD1, 0x0491, 0x006E},
    {0x0000, 0x00AB, 0x0112, 0x1DF9, 0x0577, 0x15FA, 0x090F, 0x00CE},
    {0x0000, 0x2D25, 0x3D87, 0x5E8F, 0x560A, 0x666A, 0x3966, 0x24E2},
    {0x0000, 0x2CC9, 0x3D0D, 0x5E14, 0x5171, 0x4D0F, 0x2CC8, 0x3089},
    {0x0000, 0x21A5, 0x2E47, 0x4F2F, 0x3EEA, 0x3B08, 0x25C5, 0x1D83},
    {0x0000, 0x7EB5, 0x5929, 0x7A31, 0x798D, 0x7673, 0x654B, 0x54C7},
    {0x0000, 0x29CC, 0x3A72, 0x5759, 0x4B37, 0x5379, 0x320F, 0x1E0D},
    {0x0000, 0x6AD2, 0x4585, 0x668D, 0x5E07, 0x7734, 0x45A7, 0x4564},
    {0x0000, 0x11B0, 0x1656, 0x333E, 0x1EFD, 0, 0, 0},
    {0x0000, 0x28CC, 0x3911, 0x5619, 0x4977, 0, 0, 0},
    {0x0000, 0x41ED, 0x5AB2, 0x779A, 0x7798, 0, 0, 0},
    {0x0000, 0x296B, 0x39F0, 0x56F7, 0x4A95, 0, 0, 0},
    {0x0000, 0x5294, 0x2108, 0x4210, 0x294A, 0, 0, 0},
    {0x0000, 0x3CE5, 0x5967, 0x764E, 0x71C9, 0, 0, 0},
    {0x0000, 0x24EC, 0x3550, 0x5258, 0x45D5, 0, 0, 0},
    {0x0000, 0x2968, 0x31AA, 0x52B2, 0x3E2D, 0, 0, 0},
    {0x0000, 0x1544, 0x1DC6, 0x3ACD, 0x2668, 0, 0, 0},
    {0x0000, 0x14E8, 0x1D2B, 0x3E33, 0x298F, 0, 0, 0},
    {0x0000, 0x0CE6, 0x1148, 0x2E30, 0x19AB, 0, 0, 0},
    {0x0000, 0x314C, 0x45D0, 0x62D7, 0x5A75, 0, 0, 0},
    {0x0000, 0x0C6C, 0x1091, 0x2D78, 0x14B6, 0, 0, 0},
    {0x0000, 0x0CEF, 0x1156, 0x2E3D, 0x15BC, 0, 0, 0},
    {0x0000, 0x1467, 0x18AA, 0x39B1, 0x20ED, 0, 0, 0},
    // Showdown Blinds
    {0x0000, 0x01F7, 0x01F7, 0x1EFF, 0x029F, 0x2B1F, 0x01F7, 0x0193},
    {0x0000, 0x31E8, 0x31E8, 0x52EF, 0x428A, 0x5710, 0x31E8, 0x25C2},
    {0x0000, 0x554D, 0x554D, 0x7654, 0x71D1, 0x7654, 0x552C, 0x50C9},
    {0x0000, 0x1090, 0x1090, 0x3198, 0x18D5, 0x35B9, 0x1090, 0x000F},
    {0x0000, 0x65E0, 0x65E0, 0x7EC7, 0x7E60, 0x7F0D, 0x59C0, 0x4960},
};

static Blind _blind_type_map[BLIND_TYPE_MAX] = {
#define BLIND_INFO(NAME, multi, _reward) \
    {                                    \
        .type = BLIND_TYPE_##NAME,       \
        .score_req_multipler = multi,    \
        .reward = _reward,               \
},
    BLIND_TYPE_INFO_TABLE
#undef BLIND_INFO
};
// clang-format on

static void s_blind_gfx_init(enum BlindType type);

void blind_init()
{
    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        s_blind_gfx_init(i);
    }

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
    return blind_token_palettes[type][index];
}

enum BlindType roll_blind_type(bool showdown)
{
    static bool init = false;
    if (!init)
    {
        init = true;
        unbeaten_showdown_blinds = list_create();
        unbeaten_boss_blinds = list_create();
    }
    List* p_unbeaten_blinds = showdown ? &unbeaten_showdown_blinds : &unbeaten_boss_blinds;
    int lower_blind = showdown ? BLIND_TYPE_ACORN : BLIND_TYPE_HOOK;
    int upper_blind = showdown ? BLIND_TYPE_BELL : BLIND_TYPE_MARK;

    // Fill the list with all blinds if it is empty
    // (happens on startup or if we have beaten all blinds)
    if (list_is_empty(p_unbeaten_blinds))
    {
        for (int i = lower_blind; i <= upper_blind; i++)
        {
            list_push_back(p_unbeaten_blinds, &_blind_type_map[i]);
        }
    }

    // roll a random blind among the unbeaten ones
    int random_blind_idx = rand() % list_get_len(p_unbeaten_blinds);
    return ((Blind*)list_get_at_idx(p_unbeaten_blinds, random_blind_idx))->type;
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

static u32 get_blind_tid(enum BlindType type)
{
    return BLIND_BASE_TID + type * BLIND_SPRITE_OFFSET;
}

static u32 get_blind_pb(enum BlindType type)
{
    return (type <= BLIND_TYPE_BIG) ? NORMAL_BLIND_PB : BOSS_BLIND_PB;
}

Sprite* blind_token_new(enum BlindType type, int x, int y, int sprite_index)
{
    u16 a0 = ATTR0_SQUARE | ATTR0_4BPP;
    u16 a1 = ATTR1_SIZE_32x32;
    // All Blind sprites are store sequentially and correspond to their IDs
    u32 tid = get_blind_tid(type);
    u32 pb = get_blind_pb(type);

    Sprite* sprite = sprite_new(a0, a1, tid, pb, sprite_index);

    sprite_position(sprite, x, y);

    return sprite;
}

static void s_blind_gfx_init(enum BlindType type)
{
    // TODO: Re-add grit copy. You need to decouple the blind graphics first.
    // This will allow this function to change the boss graphics info
    // GRIT_CPY(&tile_mem[4][_blind_type_map[type].pal_info.tid], tiles);
    memcpy32(&tile_mem[4][get_blind_tid(type)], &blind_gfxTiles[get_blind_pb(type)-1][type * TILE_SIZE * BLIND_SPRITE_OFFSET], BLIND_SPRITE_COPY_SIZE);

    // Small and Big Blinds are already colored as they should, only need to
    // swap the palette for Boss and Showdown Blinds
    if (type > BLIND_TYPE_BIG)
    {
        memcpy16(&pal_obj_bank[get_blind_pb(type)], blind_token_palettes[type], PAL_ROW_LEN);
    }
    
}
