#ifndef BLIND_H
#define BLIND_H

#include "sprite.h"

// The GBA's max uint value is around 4 billion, so we're going to not add endless mode for
// simplicity's sake
#define MAX_ANTE 8

// Sprite IDs of the various Blind Tokens used in the game, expressed as an offset
// relative to `BLIND_BASE_LAYER`
#define PLAYING_BLIND_TOKEN_LAYER   0
#define ROUND_END_BLIND_TOKEN_LAYER 1
#define SMALL_BLIND_TOKEN_LAYER     2
#define BIG_BLIND_TOKEN_LAYER       3
#define BOSS_BLIND_TOKEN_LAYER      4

// The sprites that display the blinds when in "GAME_BLIND_SELECT" state
// There are only 3 blinds per Ante, so we don't need more sprites than that
enum BlindTokens
{
    SMALL_BLIND,
    BIG_BLIND,
    BOSS_BLIND,
    NUM_BLINDS_PER_ANTE
};

// Order of the Blind sprites' colors as encoded in the files' palettes with Aseprite
enum BlindColorIndex
{
    BLIND_TEXT_COLOR_INDEX = 1,
    BLIND_SHADOW_COLOR_INDEX,
    BLIND_HIGHLIGHT_COLOR_INDEX,
    BLIND_MAIN_COLOR_INDEX,
    BLIND_BACKGROUND_MAIN_COLOR_INDEX,
    BLIND_BACKGROUND_SECONDARY_COLOR_INDEX,
    BLIND_BACKGROUND_SHADOW_COLOR_INDEX,
};

// clang-format off
enum BlindType
{
    // Normal Blinds
    BLIND_TYPE_SMALL,
    BLIND_TYPE_BIG,

    // Boss Blinds
    BLIND_TYPE_BOSS,

    BLIND_TYPE_HOOK = BLIND_TYPE_BOSS,
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
    BLIND_TYPE_SHOWDOWN,

    BLIND_TYPE_ACORN = BLIND_TYPE_SHOWDOWN,
    BLIND_TYPE_LEAF,
    BLIND_TYPE_VESSEL,
    BLIND_TYPE_HEART,
    BLIND_TYPE_BELL,

    // End of Blinds' list
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

u32 blind_get_requirement(enum BlindType type, int ante);
int blind_get_reward(enum BlindType type);
u16 blind_get_color(enum BlindType type, enum BlindColorIndex index);

void init_unbeaten_blinds_list(bool showdown);
enum BlindType roll_blind_type(bool showdown);
void set_blind_beaten(enum BlindType type);

void apply_blind_colors(enum BlindType type);
void apply_blind_tiles(enum BlindType type, int layer);
Sprite* blind_token_new(enum BlindType type, int x, int y, int sprite_index);

#endif // BLIND_H
