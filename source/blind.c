/**
 * @file blind.c
 * @brief Implementation of functions relative to the behaviour and graphics of Blinds.
 */

#include "blind.h"

#include "blind_gfx.h"
#include "game.h"
#include "game/common_ui.h"
#include "graphic_utils.h"
#include "hand.h"
#include "list.h"
#include "random.h"
#include "sprite.h"
#include "util.h"

#include <stdbool.h>
#include <stdlib.h>
#include <tonc.h>

#define BLIND_SPRITE_COPY_SIZE (BLIND_SPRITE_SIZE * TILE_SIZE)

#define BLIND_TOKENS_PER_SPRITESHEET 2
#define BLIND_TOKEN_PALETTE_SIZE     8

#define SMALL_BLIND_REWARD    3
#define BIG_BLIND_REWARD      4
#define BOSS_BLIND_REWARD     5
#define SHOWDOWN_BLIND_REWARD 8

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
static List s_unbeaten_boss_blinds = LIST_DEFAULT;
static List s_unbeaten_showdown_blinds = LIST_DEFAULT;

// Maps the ante number to the base blind requirement for that ante.
// The game starts at ante 1 which is at index 1 for base requirement 300.
// Ante 0 is also there in case it is ever reached.
static const u32 ANTE_LUT[] = {100, 300, 800, 2000, 5000, 11000, 20000, 35000, 50000};

// clang-format off
/**
 * @brief Stores an instance of the Blind struct for each BlindType value, ordered in the same way.
 *         Acts the same way the Joker registry does, and may need to go in its own file depending
 *         on how Blind effects are implemented.
 */
static Blind s_blind_type_map[BLIND_TYPE_MAX] = {
    {BLIND_TYPE_SMALL,   FIX_ONE         },
    {BLIND_TYPE_BIG,    (FIX_ONE * 3) / 2},
    {BLIND_TYPE_HOOK,    FIX_ONE * 2     },
    {BLIND_TYPE_OX,      FIX_ONE * 2     },
    {BLIND_TYPE_HOUSE,   FIX_ONE * 2     },
    {BLIND_TYPE_WHEEL,   FIX_ONE * 2     },
    {BLIND_TYPE_WALL,    FIX_ONE * 2     }, // x4 score requirement will be part of the effect
    {BLIND_TYPE_ARM,     FIX_ONE * 2     },
    {BLIND_TYPE_CLUB,    FIX_ONE * 2     },
    {BLIND_TYPE_FISH,    FIX_ONE * 2     },
    {BLIND_TYPE_PSYCHIC, FIX_ONE * 2     },
    {BLIND_TYPE_GOAD,    FIX_ONE * 2     },
    {BLIND_TYPE_WATER,   FIX_ONE * 2     },
    {BLIND_TYPE_WINDOW,  FIX_ONE * 2     },
    {BLIND_TYPE_MANACLE, FIX_ONE * 2     },
    {BLIND_TYPE_EYE,     FIX_ONE * 2     },
    {BLIND_TYPE_MOUTH,   FIX_ONE * 2     },
    {BLIND_TYPE_PLANT,   FIX_ONE * 2     },
    {BLIND_TYPE_SERPENT, FIX_ONE * 2     },
    {BLIND_TYPE_PILLAR,  FIX_ONE * 2     },
    {BLIND_TYPE_NEEDLE,  FIX_ONE * 2     }, // Same as the Wall with normal requirement
    {BLIND_TYPE_HEAD,    FIX_ONE * 2     },
    {BLIND_TYPE_TOOTH,   FIX_ONE * 2     },
    {BLIND_TYPE_FLINT,   FIX_ONE * 2     },
    {BLIND_TYPE_MARK,    FIX_ONE * 2     },
    {BLIND_TYPE_ACORN,   FIX_ONE * 2     },
    {BLIND_TYPE_LEAF,    FIX_ONE * 2     },
    {BLIND_TYPE_VESSEL,  FIX_ONE * 2     }, // Same as the Wall with x6 requirement
    {BLIND_TYPE_HEART,   FIX_ONE * 2     },
    {BLIND_TYPE_BELL,    FIX_ONE * 2     }
};
// clang-format on

void blind_init()
{
    // Set up the palette used by normal blind tokens.
    // We will never need to edit it ever again.
    memcpy16(&pal_obj_bank[NORMAL_BLIND_PB], blind_gfxPal[0], NUM_ELEM_IN_ARR(blind_gfx0Pal));

    return;
}

u32 blind_get_requirement(enum BlindType type, int ante)
{
    // Ensure ante is within valid range
    if (ante < 0 || ante > MAX_ANTE)
        ante = 0;

    return fx2int(s_blind_type_map[type].score_req_multiplier * ANTE_LUT[ante]);
}

int blind_get_reward(enum BlindType type)
{
    switch (type)
    {
        case BLIND_TYPE_SMALL:
            return SMALL_BLIND_REWARD;
        case BLIND_TYPE_BIG:
            return BIG_BLIND_REWARD;
        case BLIND_TYPE_BOSS ...(BLIND_TYPE_SHOWDOWN - 1):
            return BOSS_BLIND_REWARD;
        default:
            return SHOWDOWN_BLIND_REWARD;
    }
}

/**
 * @brief Fill Lists of unbeaten Boss and Showdown Blinds
 *
 *         By keeping track of what Blind we have beaten or not, we can ensure that until we've
 *         beaten all the Blinds in a single Run, we won't encounter the same one twice.
 *
 * This must be called at the beginning of a run.
 *
 * @param showdown toggle between the List for Showdown and regular Boss Blinds
 */
static void init_unbeaten_blinds_list(bool showdown)
{
    List* p_unbeaten_blinds = showdown ? &s_unbeaten_showdown_blinds : &s_unbeaten_boss_blinds;

    // empty the list just to be sure
    list_clear(p_unbeaten_blinds);

    int lower_blind = showdown ? BLIND_TYPE_SHOWDOWN : BLIND_TYPE_BOSS;
    int upper_blind = showdown ? BLIND_TYPE_MAX - 1 : BLIND_TYPE_SHOWDOWN - 1;

    for (int i = lower_blind; i <= upper_blind; i++)
    {
        list_push_back(p_unbeaten_blinds, &s_blind_type_map[i]);
    }
}

void init_unbeaten_blinds_lists(void)
{
    init_unbeaten_blinds_list(false);
    init_unbeaten_blinds_list(true);
}

enum BlindType roll_blind_type(bool showdown)
{
    List* p_unbeaten_blinds = showdown ? &s_unbeaten_showdown_blinds : &s_unbeaten_boss_blinds;

    // Fill the list with all blinds if it is empty
    // (happens on startup or if we have beaten all blinds)
    if (list_is_empty(p_unbeaten_blinds))
    {
        init_unbeaten_blinds_list(showdown);
    }

    // roll a random blind among the unbeaten ones
    int random_blind_idx = rng_get_u32() % list_get_len(p_unbeaten_blinds);
    Blind* random_blind = list_get_at_idx(p_unbeaten_blinds, random_blind_idx);

    return random_blind->type;
}

void set_blind_beaten(enum BlindType type)
{
    bool showdown = (type >= BLIND_TYPE_SHOWDOWN);
    List* p_unbeaten_blinds = showdown ? &s_unbeaten_showdown_blinds : &s_unbeaten_boss_blinds;

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

static u32 get_blind_spritesheet_idx(enum BlindType type)
{
    // See comment below in blind_get_color why we differenciate before/after the Mark
    return (type < BLIND_TYPE_MARK)
             ? type / BLIND_TOKENS_PER_SPRITESHEET
             : BLIND_TYPE_MARK / BLIND_TOKENS_PER_SPRITESHEET + type - BLIND_TYPE_MARK;
}

u16 blind_get_color(enum BlindType type, enum BlindColorIndex index)
{
    // Do a little translation of palette idx -> custom array idx
    // All blinds before the Mark are arranged in pairs with their palettte split in two
    // | XX |  1 |  2 |  3 |  4 |  5 |  6 |  7 | -> first sprite
    // | XX |  9 | 10 | 11 | 12 | 13 | 14 | 15 | -> second sprite
    // so we need to offset the color indices by 8 for blinds with an odd type
    // From the Mark onwards, all sprites are alone in their spritesheet, so no need to offset
    u32 color_idx = index + ((type < BLIND_TYPE_MARK)
                                 ? BLIND_TOKEN_PALETTE_SIZE * (type % BLIND_TOKENS_PER_SPRITESHEET)
                                 : 0);
    return blind_gfxPal[get_blind_spritesheet_idx(type)][color_idx];
}

void apply_blind_colors(enum BlindType type)
{
    // keep track of active boss blind spritesheet to copy colors only when changing
    static u32 active_boss_spritesheet = BLIND_TYPE_MAX;
    u32 new_spritesheet = get_blind_spritesheet_idx(type);

    // No need to update normal blind palette
    if (type < BLIND_TYPE_BOSS || active_boss_spritesheet == new_spritesheet)
    {
        return;
    }

    active_boss_spritesheet = new_spritesheet;

    // Just copy the palette as is to the sprite palette bank
    memcpy16(
        &pal_obj_bank[BOSS_BLIND_PB],
        blind_gfxPal[active_boss_spritesheet],
        NUM_ELEM_IN_ARR(blind_gfx0Pal)
    );
}

Sprite* blind_token_new(enum BlindType type, int x, int y, enum BlindTokenLayers layer)
{
    u32 tile_index = sprite_type_get_avail_tid(BLIND_TOKEN_SPRITE);
    u32 spritesheet_idx = get_blind_spritesheet_idx(type);
    u32 sprite_idx = (type < BLIND_TYPE_MARK) ? type % BLIND_TOKENS_PER_SPRITESHEET : 0;
    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index],
        &blind_gfxTiles[spritesheet_idx][sprite_idx * BLIND_SPRITE_COPY_SIZE],
        BLIND_SPRITE_COPY_SIZE
    );
    apply_blind_colors(type);

    Sprite* sprite = sprite_new(
        BLIND_TOKEN_SPRITE,
        ATTR0_SQUARE | ATTR0_4BPP,
        ATTR1_SIZE_32x32,
        tile_index,
        get_blind_pb(type),
        layer
    );
    sprite_position(sprite, x, y);

    return sprite;
}
