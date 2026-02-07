#include "blind.h"

#include "big_blind_gfx.h"
#include "boss_blind_gfx.h"
#include "graphic_utils.h"
#include "small_blind_gfx.h"
#include "util.h"

#include <tonc.h>

// Maps the ante number to the base blind requirement for that ante.
// The game starts at ante 1 which is at index 1 for base requirement 300.
// Ante 0 is also there in case it is ever reached.
static const u32 ante_lut[] = {100, 300, 800, 2000, 5000, 11000, 20000, 35000, 50000};

// Palettes for the blinds (Transparency, Text Color, Shadow, Highlight, Main Color) Use this:
// http://www.budmelvin.com/dev/15bitconverter.html
static const u16 small_blind_token_palette[PAL_ROW_LEN] =
    {0x0000, 0x7FFF, 0x34A1, 0x5DCB, 0x5104, 0x55A0, 0x2D01, 0x34E0};
static const u16 big_blind_token_palette[PAL_ROW_LEN] =
    {0x0000, 0x2527, 0x15F5, 0x36FC, 0x1E9C, 0x01B4, 0x0D0A, 0x010E};
// This variable is temporary, each boss blind will have its own unique palette
static const u16 boss_blind_token_palette[PAL_ROW_LEN] =
    {0x0000, 0x2CC9, 0x3D0D, 0x5E14, 0x5171, 0x4D0F, 0x2CC8, 0x3089};

// clang-format off
static Blind _blind_type_map[BLIND_TYPE_MAX] = {
#define BLIND_INFO(NAME, name, multi, _reward)         \
    {                                                  \
        .type = BLIND_TYPE_##NAME,                     \
        .gfx_info =                                    \
        {                                              \
                .tiles = name##_blind_gfxTiles,        \
                .palette = name##_blind_token_palette, \
                .tid = NAME##_BLIND_TID,               \
                .pb = NAME##_BLIND_PB,                 \
        },                                             \
        .score_req_multipler = multi,                  \
        .reward = _reward,                             \
},
    BLIND_TYPE_INFO_TABLE
#undef BLIND_INFO
};
// clang-format on

static void s_blind_gfx_init(enum BlindType type);

GBAL_UNUSED
void blind_set_boss_graphics(const unsigned int* tiles, const u16* palette)
{
    // TODO: This function is unused and not fully fleshed out.
    // We need to support more boss blind graphics in the future.
    // The idea here is that we can call this function to set the boss up to
    // render new tiles
    //
    // This will eventually be in it's own map mapping graphic data to
    // boss types.

    _blind_type_map[BLIND_TYPE_BOSS].gfx_info.tiles = tiles;
    _blind_type_map[BLIND_TYPE_BOSS].gfx_info.palette = palette;
    s_blind_gfx_init(BLIND_TYPE_BOSS);
}

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
    return _blind_type_map[type].gfx_info.palette[index];
}

Sprite* blind_token_new(enum BlindType type, int x, int y, int sprite_index)
{
    u16 a0 = ATTR0_SQUARE | ATTR0_4BPP;
    u16 a1 = ATTR1_SIZE_32x32;
    u32 tid = _blind_type_map[type].gfx_info.tid, pb = _blind_type_map[type].gfx_info.pb;

    Sprite* sprite = sprite_new(a0, a1, tid, pb, sprite_index);

    sprite_position(sprite, x, y);

    return sprite;
}

static void s_blind_gfx_init(enum BlindType type)
{
    // TODO: Re-add grit copy. You need to decouple the blind graphics first.
    // This will allow this function to change the boss graphics info
    // GRIT_CPY(&tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][_blind_type_map[type].pal_info.tid], tiles);
    BlindGfxInfo* p_gfx = &_blind_type_map[type].gfx_info;
    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][p_gfx->tid],
        p_gfx->tiles,
        BLIND_SPRITE_COPY_SIZE
    );
    memcpy16(&pal_obj_bank[p_gfx->pb], p_gfx->palette, PAL_ROW_LEN);
}
