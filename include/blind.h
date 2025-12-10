#ifndef BLIND_H
#define BLIND_H

#include "sprite.h"
// The GBA's max uint value is around 4 billion, so we're going to not add endless mode for
// simplicity's sake
#define MAX_ANTE 8

#define NORMAL_BLIND_PB 1
#define BOSS_BLIND_PB   2

#define BLIND_BASE_TID         960
#define BLIND_SPRITE_OFFSET    16
#define BLIND_SPRITE_COPY_SIZE BLIND_SPRITE_OFFSET * 8 // 8 ints per tile

// Only applies to Boss/Showdown Blinds.
// Normal Blinds do not need to be palette swapped and we don't need to get their colors
enum BlindColorIndex
{
    BLIND_TEXT_COLOR_INDEX = 9,
    BLIND_SHADOW_COLOR_INDEX = 2,
    BLIND_HIGHLIGHT_COLOR_INDEX = 7,
    BLIND_MAIN_COLOR_INDEX = 8,
    // not in the file's palette, it's free space we'll use for the Boss blinds' background
    BLIND_BACKGROUND_MAIN_COLOR_INDEX = 11,
    BLIND_BACKGROUND_SECONDARY_COLOR_INDEX = 12,
    BLIND_BACKGROUND_SHADOW_COLOR_INDEX = 13,
};

// clang-format off
enum BlindType
{
    // Normal Blinds
    BLIND_TYPE_SMALL,
    BLIND_TYPE_BIG,
    // Boss Blinds
    BLIND_TYPE_HOOK,
    BLIND_TYPE_OX,
    BLIND_TYPE_HOUSE,
    BLIND_TYPE_WALL,
    BLIND_TYPE_WHEEL,
    BLIND_TYPE_ARM,
    BLIND_TYPE_CLUB,
    BLIND_TYPE_FISH,
    BLIND_TYPE_PSYCHIC,
    BLIND_TYPE_GOAD,
    BLIND_TYPE_WATER,
    BLIND_TYPE_WINDOW,
    BLIND_TYPE_MANACLE,
    BLIND_TYPE_EYE,
    BLIND_TYPE_MOUTH,
    BLIND_TYPE_PLANT,
    BLIND_TYPE_SERPENT,
    BLIND_TYPE_PILLAR,
    BLIND_TYPE_NEEDLE,
    BLIND_TYPE_HEAD,
    BLIND_TYPE_TOOTH,
    BLIND_TYPE_FLINT,
    BLIND_TYPE_MARK,
    // Showdown Blinds
    BLIND_TYPE_ACORN,
    BLIND_TYPE_LEAF,
    BLIND_TYPE_VESSEL,
    BLIND_TYPE_HEART,
    BLIND_TYPE_BELL,
    BLIND_TYPE_MAX
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

typedef struct
{
    enum BlindType type;
    FIXED score_req_multipler;
    s32 reward;
} Blind;

void blind_init();

void blind_set_boss_graphics(const unsigned int* tiles, const u16* palette);

u32 blind_get_requirement(enum BlindType type, int ante);
int blind_get_reward(enum BlindType type);
u16 blind_get_color(enum BlindType type, enum BlindColorIndex index);

enum BlindType roll_blind_type(bool showdown);
void set_blind_beaten(enum BlindType type);

Sprite* blind_token_new(enum BlindType type, int x, int y, int sprite_index);

#endif // BLIND_H
