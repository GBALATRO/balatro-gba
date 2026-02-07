#ifndef BLIND_H
#define BLIND_H

#include "sprite.h"
// The GBA's max uint value is around 4 billion, so we're going to not add endless mode for
// simplicity's sake
#define MAX_ANTE 8

#define SMALL_BLIND_PB 1
#define BIG_BLIND_PB   2
#define BOSS_BLIND_PB  3

#define BLIND_SPRITE_OFFSET    16
#define BLIND_SPRITE_COPY_SIZE BLIND_SPRITE_OFFSET * 8 // 8 ints per tile
#define SMALL_BLIND_TID        960
#define BIG_BLIND_TID          (BLIND_SPRITE_OFFSET + SMALL_BLIND_TID)
#define BOSS_BLIND_TID         (BLIND_SPRITE_OFFSET + BIG_BLIND_TID)

enum BlindColorIndex
{
    BLIND_TEXT_COLOR_INDEX = 1,
    BLIND_SHADOW_COLOR_INDEX = 2,
    BLIND_HIGHLIGHT_COLOR_INDEX = 3,
    BLIND_MAIN_COLOR_INDEX = 4,
    BLIND_BACKGROUND_MAIN_COLOR_INDEX = 5,
    BLIND_BACKGROUND_SECONDARY_COLOR_INDEX = 6,
    BLIND_BACKGROUND_SHADOW_COLOR_INDEX = 7,
};

#define BLIND_TYPE_INFO_TABLE                  \
    BLIND_INFO(SMALL, small, FIX_ONE, 3)       \
    BLIND_INFO(BIG, big, (FIX_ONE * 3) / 2, 4) \
    BLIND_INFO(BOSS, boss, FIX_ONE * 2, 5)

// clang-format off
enum BlindType
{
#define BLIND_INFO(NAME, name, multi, reward) \
    BLIND_TYPE_##NAME,
    BLIND_TYPE_INFO_TABLE
#undef BLIND_INFO
    BLIND_TYPE_MAX,
};
// clang-format on

enum BlindState
{
    BLIND_STATE_CURRENT,
    BLIND_STATE_UPCOMING,
    BLIND_STATE_DEFEATED,
    BLIND_STATE_SKIPPED,
    BLIND_STATE_MAX,
};

// TODO: Move this to a common interface for other palettes
typedef struct
{
    const unsigned int* tiles;
    const u16* palette;
    u32 tid;
    u32 pb;
} BlindGfxInfo;

typedef struct
{
    enum BlindType type;
    BlindGfxInfo gfx_info;
    FIXED score_req_multipler;
    s32 reward;
} Blind;

void blind_init();

void blind_set_boss_graphics(const unsigned int* tiles, const u16* palette);

u32 blind_get_requirement(enum BlindType type, int ante);
int blind_get_reward(enum BlindType type);
u16 blind_get_color(enum BlindType type, enum BlindColorIndex index);

Sprite* blind_token_new(enum BlindType type, int x, int y, int sprite_index);

#endif // BLIND_H
