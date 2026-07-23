#include "joker.h"

#include "card.h"
#include "game/round.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "layout.h"
#include "mgba_logger.h"
#include "pool.h"
#include "random.h"
#include "soundbank.h"
#include "util.h"

// Tiles and palettes
#include "card_rarity_pal_gfx.h"
#include "item.h"
#include "joker_gfx.h"

#include <maxmod.h>
#include <stdlib.h>
#include <string.h>
#include <tonc.h>

#define JOKER_SCORE_TEXT_Y     48
#define HELD_CARD_SCORE_TEXT_Y 108
#define MAX_CARD_SCORE_STR_LEN 2
// what it was before (MAX_DEFINEABLE_JOKERS / JOKERS_PER_SPRITESHEET)
#define MAX_NUM_JOKERS_SPRITESHEETS 75

static const unsigned int* joker_gfxTiles[] = {
#define DEF_JOKER_GFX(idx) joker_gfx##idx##Tiles,
#include "../include/def_joker_gfx_table.h"
#undef DEF_JOKER_GFX
};
static const unsigned short* joker_gfxPal[] = {
#define DEF_JOKER_GFX(idx) joker_gfx##idx##Pal,
#include "def_joker_gfx_table.h"
#undef DEF_JOKER_GFX
};

// TODO: Removed unplanned editions...
const static u8 EDITION_PRICE_LUT[MAX_EDITIONS] = {
    0, // BASE_EDITION
    2, // FOIL_EDITION
    3, // HOLO_EDITION
    5, // POLY_EDITION
    5, // NEGATIVE_EDITION
};

/* So for the card objects, I needed them to be properly sorted
   which is why they let you specify the layer index when creating a new card object.
   Since the cards would overlap a lot in your hand, If they weren't sorted properly, it would look
   like a mess. The joker objects are functionally identical to card objects, so they use the same
   logic. But I'm going to use a simpler approach for the joker objects since I'm lazy and sorting
   them wouldn't look good enough to warrant the effort.
*/
static bool s_used_layers[MAX_ACTIVE_JOKERS] = {false}; // Track used layers for joker sprites
// TODO: Refactor sorting into SpriteObject?

// Maps the spritesheet index to the palette bank index allocated to it.
// Spritesheets that were not allocated are
static int s_joker_spritesheet_pb_map[MAX_NUM_JOKERS_SPRITESHEETS];
static int s_joker_pb_num_sprite_users[JOKER_LAST_PB - JOKER_BASE_PB + 1] = {0};

BITSET_DEFINE(s_rollable_jokers_bitset, MAX_DEFINABLE_JOKERS)

// See linked issue for context of maps
// https://github.com/GBALATRO/balatro-gba/issues/274#issue-3685075538

// clang-format off
// Map of Joker ID -> Spritesheet idx
static const int JOKER_ID_TO_SPRITE_MAP[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1,
    2, 2,
    3, 3, 3, 3, 3,
    4, 4, 4, 4, 4,
    5, 5, 5, 5,
    6, 6, 6, 6,
    7, 7,
    8, 8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
};

// Map of Spritesheet idx -> first Joker ID in sheet
// This is used to determine the sprite index of a Joker's sprite
// within its spritesheet by substracting its ID to this starting ID.
// Notice how spritesheets with only one Joker have sequential starting IDs.
static const int SPRITESHEET_IDX_TO_STARTING_JOKER_ID[] = {
     0, 18, 20, 22, 27, 32, 36, 40, 42, 44,
    45, 46, 47, 48, 49, 50, 51, 52
};

// Lookup table of Joker Rarity strings. Used to display at the bottom of the description screen.
static const char* JOKER_RARITY_STRINGS_LUT[MAX_RARITIES] = {
    "Common", "Uncommon", "Rare", "Legendary"
};
// clang-format on

static int s_get_num_spritesheets(void);
static int s_joker_get_spritesheet_idx(u8 joker_id);
static int s_joker_get_sprite_idx_in_sheet(u8 joker_id, int spritesheet_idx);
static void s_joker_pb_add_sprite_user(int pb);
static void s_joker_pb_remove_sprite_user(int pb);
static int s_joker_pb_get_num_sprite_users(int joker_pb);
static int s_get_unused_joker_pb(void);
static int s_allocate_pb_if_needed(u8 joker_id);

void joker_init()
{
    // This should init once only so no need to free
    int num_spritesheets = s_get_num_spritesheets();

    for (int i = 0; i < num_spritesheets; i++)
    {
        s_joker_spritesheet_pb_map[i] = UNDEFINED;
    }
}

Joker* joker_new(u8 id)
{
    if (id >= get_joker_registry_size())
        return NULL;

    Joker* joker = POOL_GET(Joker);
    const JokerInfo* jinfo = get_joker_registry_entry(id);

    joker->id = id;
    joker->modifier = BASE_EDITION; // TODO: Make this a parameter
    joker->value = jinfo->base_value + EDITION_PRICE_LUT[joker->modifier];
    joker->rarity = jinfo->rarity;
    joker->scoring_state = 0;
    joker->persistent_state = 0;

    // initialize persistent Joker data if needed
    JokerEffect* joker_effect = NULL;
    jinfo->joker_effect_func(joker, NULL, JOKER_EVENT_ON_JOKER_CREATED, &joker_effect);

    return joker;
}

void joker_destroy(Joker** joker)
{
    POOL_FREE(Joker, *joker);
    *joker = NULL;
}

u32 joker_get_score_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    const JokerInfo* jinfo = get_joker_registry_entry(joker->id);
    if (!jinfo)
        return JOKER_EFFECT_FLAG_NONE;

    return jinfo->joker_effect_func(joker, scored_card, joker_event, joker_effect);
}

const char* joker_get_rarity_string(u8 rarity)
{
    if (rarity >= MAX_RARITIES)
        return NULL;

    return JOKER_RARITY_STRINGS_LUT[rarity];
}

u16 joker_get_rarity_color(u8 rarity, bool main_color)
{
    if (rarity >= MAX_RARITIES)
        return 0x0;

    // +1 to account for the transparency
    // odd indices are the main colors, even ones are the shadows
    return card_rarity_pal_gfxPal[1 + 2 * rarity + (main_color ? 0 : 1)];
}

int joker_get_buy_price(const Joker* joker)
{
    GBAL_RETURN_IF_NULL_RET(joker, UNDEFINED);

    return joker->value;
}

int joker_get_sell_value(const Joker* joker)
{
    GBAL_RETURN_IF_NULL_RET(joker, UNDEFINED);

    return joker->value / 2;
}

// JokerObject methods
JokerObject* joker_object_new(Joker* joker)
{
    JokerObject* joker_object = POOL_GET(JokerObject);

    sprite_object_init((SpriteObject*)joker_object);

    int layer = 0;
    for (int i = MAX_ACTIVE_JOKERS - 1; i >= 0; i--)
    {
        if (!s_used_layers[i])
        {
            layer = i;
            s_used_layers[i] = true; // Mark this layer as used
            break;
        }
    }

    joker_object->joker = joker;

    joker_object->type = ITEM_TYPE_JOKER;

    int tile_index = sprite_type_get_avail_tid(JOKER_SPRITE);
    int joker_spritesheet_idx = s_joker_get_spritesheet_idx(joker->id);
    int joker_idx = s_joker_get_sprite_idx_in_sheet(joker->id, joker_spritesheet_idx);
    int joker_pb = s_allocate_pb_if_needed(joker->id);
    s_joker_pb_add_sprite_user(joker_pb);

    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index],
        &joker_gfxTiles[joker_spritesheet_idx][joker_idx * TILE_SIZE * JOKER_SPRITE_SIZE],
        TILE_SIZE * JOKER_SPRITE_SIZE
    );

    sprite_object_set_sprite(
        (SpriteObject*)joker_object,
        sprite_new(
            JOKER_SPRITE,
            ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF,
            ATTR1_SIZE_32,
            tile_index,
            joker_pb,
            layer
        )
    );

    return joker_object;
}

void joker_object_destroy(JokerObject** joker_object)
{
    if (joker_object == NULL || *joker_object == NULL)
        return;

    int layer = sprite_get_layer(joker_object_get_sprite(*joker_object)) -
                sprite_type_get_starting_layer(JOKER_SPRITE);
    s_used_layers[layer] = false;
    s_joker_pb_remove_sprite_user(sprite_get_pb(joker_object_get_sprite(*joker_object)));
    if (s_joker_pb_get_num_sprite_users((sprite_get_pb(joker_object_get_sprite(*joker_object)))) ==
        0)
    {
        s_joker_spritesheet_pb_map[s_joker_get_spritesheet_idx((*joker_object)->joker->id)] =
            UNDEFINED;
    }

    sprite_object_destroy((SpriteObject*)(*joker_object));
    joker_destroy(&(*joker_object)->joker);
    POOL_FREE(JokerObject, *joker_object);
    *joker_object = NULL;
}

void joker_object_dispose(Item** joker_object_item)
{
    GBAL_RETURN_IF_NULL_VOID(joker_object_item);
    GBAL_RETURN_IF_NULL_VOID(*joker_object_item);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID(*joker_object_item, ITEM_TYPE_JOKER);

    JokerObject* joker_object = (JokerObject*)(*joker_object_item);
    GBAL_RETURN_IF_NULL_VOID(joker_object->joker);

    joker_set_rollable(joker_object->joker->id, true);

    joker_object_destroy(&joker_object);
    *joker_object_item = NULL;
}

void joker_object_shake(JokerObject* joker_object, mm_word sound_id)
{
    sprite_object_shake((SpriteObject*)joker_object, sound_id);
}

int joker_object_get_buy_price(Item* joker_object)
{
    GBAL_RETURN_IF_NULL_RET(joker_object, UNDEFINED);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_RET(joker_object, ITEM_TYPE_JOKER, UNDEFINED);

    return ((JokerObject*)joker_object)->joker->value;
}

void joker_object_add_to_owned(Item* joker_object)
{
    GBAL_RETURN_IF_NULL_VOID(joker_object);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID(joker_object, ITEM_TYPE_JOKER);

    joker_object->ty = int2fx(HELD_JOKERS_POS.y);
    add_joker((JokerObject*)joker_object);
    sprite_object_sort_list((void*)get_jokers_list(), true);
}

void joker_set_rollable(int joker_id, bool rollable)
{
    bitset_set_idx(&s_rollable_jokers_bitset, joker_id, rollable);
}

/**
 * @brief Computes the number of Jokers we can currently roll in the Shop.
 *         The Jokers we own is taken into account and can't be rolled again.
 */
static inline int get_num_rollable_jokers(void)
{
    return bitset_num_set_bits(&s_rollable_jokers_bitset);
}

/**
 * @brief Returns true if we can't roll any Joker
 */
static inline bool no_rollable_jokers(void)
{
    return bitset_is_empty(&s_rollable_jokers_bitset);
}

GBAL_UNUSED
static inline bool joker_is_rollable(int joker_id)
{
    return bitset_get_idx(&s_rollable_jokers_bitset, joker_id);
}

void joker_reset_rollable_jokers(void)
{
    int num_jokers = get_joker_registry_size();

    bitset_clear(&s_rollable_jokers_bitset);
    for (int i = 0; i < num_jokers; i++)
    {
        bitset_set_idx(&s_rollable_jokers_bitset, i, true);
    }
}

/**
 * @brief Rolls a random Joker among the available ones
 */
static int joker_roll_id(void)
{
    // Roll for what rarity the joker will be
    int joker_rarity = joker_get_random_rarity();
    return joker_roll_id_with_rarity(joker_rarity);
}

int joker_roll_id_with_rarity(int joker_rarity)
{
    // Now determine how many jokers are available based on the rarity
    int jokers_avail_size = get_num_rollable_jokers();

    if (jokers_avail_size == 0)
        return UNDEFINED;

    int matching_joker_ids[jokers_avail_size];
    int fallback_random_idx = rng_get_u32() % jokers_avail_size;
    int fallback_random_joker_id = UNDEFINED;
    int match_count = 0;

    BitsetItr itr = bitset_itr_create(&s_rollable_jokers_bitset);

    int i = 0;
    int joker_id = UNDEFINED;
    while ((joker_id = bitset_itr_next(&itr)) != UNDEFINED)
    {
        if (i++ == fallback_random_idx)
            fallback_random_joker_id = joker_id;
        const JokerInfo* info = get_joker_registry_entry(joker_id);
        if (info->rarity == joker_rarity)
        {
            matching_joker_ids[match_count++] = joker_id;
        }
    }

    int selected_joker_id = (match_count > 0) ? matching_joker_ids[rng_get_u32() % match_count]
                                              : fallback_random_joker_id;

    return selected_joker_id;
}

Item* joker_object_roll_new(void)
{
    if (no_rollable_jokers())
        return NULL;

    int joker_id = 0;
#ifdef TEST_JOKER_ID0 // Allow defining an ID for a joker to always appear in shop and be tested
    if (joker_is_rollable(TEST_JOKER_ID0))
    {
        joker_id = TEST_JOKER_ID0;
    }
    else
#endif
#ifdef TEST_JOKER_ID1
        if (joker_is_rollable(TEST_JOKER_ID1))
    {
        joker_id = TEST_JOKER_ID1;
    }
    else
#endif
    {
        joker_id = joker_roll_id();
    }

    // If for some reason only no joker is left, don't make another
    if (joker_id == UNDEFINED)
        return NULL;

    joker_set_rollable(joker_id, false);

    return (Item*)joker_object_new(joker_new(joker_id));
}

static void set_and_shift_text(char* str, int* cursor_pos_x, int* cursor_pos_y, int color_pb)
{
    tte_set_pos(*cursor_pos_x, *cursor_pos_y);
    tte_set_special(color_pb * TTE_SPECIAL_PB_MULT_OFFSET);
    tte_write(str);

    // + 1 For space
    const int joker_score_display_offset_px = (MAX_CARD_SCORE_STR_LEN + 1) * TTE_CHAR_SIZE;
    *cursor_pos_x += joker_score_display_offset_px;
}

bool joker_object_score(
    JokerObject* joker_object,
    CardObject* card_object,
    enum JokerEvent joker_event
)
{
    if (joker_object == NULL)
    {
        return false;
    }

    JokerEffect* joker_effect = NULL;
    u32 effect_flags_ret =
        joker_get_score_effect(joker_object->joker, card_object->card, joker_event, &joker_effect);

    if (effect_flags_ret == JOKER_EFFECT_FLAG_NONE)
    {
        return false;
    }

    if (effect_flags_ret & JOKER_EFFECT_FLAG_RETRIGGER)
    {
        set_retrigger(joker_effect->retrigger);
    }

    // joker_effect.message will have been set if the Joker had anything custom to say

    int cursorPosX = TILE_SIZE; // Offset of one tile to better center the text on the card
    int cursorPosY = 0;
    if (joker_event == JOKER_EVENT_ON_CARD_HELD)
    {
        // display the text on top of the card instead of below the Joker for Held Cards effects
        // scored_card cannot be NULL here because of the joker event
        cursorPosX += fx2int(card_object->x);
        cursorPosY = HELD_CARD_SCORE_TEXT_Y;
    }
    else
    {
        cursorPosX += fx2int(joker_object->x);
        cursorPosY = JOKER_SCORE_TEXT_Y;
    }

    mm_word sfx_id;
    if (effect_flags_ret & JOKER_EFFECT_FLAG_CHIPS)
    {
        g_game_vars.chips = u32_protected_add(g_game_vars.chips, joker_effect->chips);
        char score_buffer[INT_MAX_DIGITS + 2]; // For '+' and null terminator
        snprintf(score_buffer, sizeof(score_buffer), "+%lu", joker_effect->chips);
        set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_BLUE_PB);
        sfx_id = SFX_CHIPS_GENERIC; // The joker chips effect is "generic"
    }
    if (effect_flags_ret & JOKER_EFFECT_FLAG_MULT)
    {
        g_game_vars.mult = u32_protected_add(g_game_vars.mult, joker_effect->mult);
        char score_buffer[INT_MAX_DIGITS + 2];
        snprintf(score_buffer, sizeof(score_buffer), "+%lu", joker_effect->mult);
        set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_RED_PB);
        sfx_id = SFX_MULT;
    }
    // if xmult is zero, DO NOT multiply by it
    if (effect_flags_ret & JOKER_EFFECT_FLAG_XMULT && joker_effect->xmult > 0)
    {
        g_game_vars.mult = u32_protected_mult(g_game_vars.mult, joker_effect->xmult);
        char score_buffer[INT_MAX_DIGITS + 2];
        snprintf(score_buffer, sizeof(score_buffer), "X%lu", joker_effect->xmult);
        set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_RED_PB);
        sfx_id = SFX_XMULT;
    }
    if (effect_flags_ret & JOKER_EFFECT_FLAG_MONEY)
    {
        g_game_vars.money += joker_effect->money;
        char score_buffer[INT_MAX_DIGITS + 2];
        snprintf(score_buffer, sizeof(score_buffer), "%d$", joker_effect->money);
        set_and_shift_text(score_buffer, &cursorPosX, &cursorPosY, TTE_YELLOW_PB);
        // TODO: Money sound effect
    }
    // custom message for Jokers (including retriggers where Jokers will say "Again!")
    // joker_effect->message will have been set if the Joker had anything custom to say
    if (effect_flags_ret & JOKER_EFFECT_FLAG_MESSAGE)
    {
        set_and_shift_text(joker_effect->message, &cursorPosX, &cursorPosY, TTE_WHITE_PB);
    }
    // this will start the Joker expire animation
    if (effect_flags_ret & JOKER_EFFECT_FLAG_EXPIRE && joker_effect->expire)
    {
        joker_object_shake(joker_object, UNDEFINED);
        list_push_back(get_expired_jokers_list(), joker_object);
    }

    // Update displays
    display_chips();
    display_mult();
    display_money();

    joker_object_shake(joker_object, sfx_id);

    return true;
}

Sprite* joker_object_get_sprite(JokerObject* joker_object)
{
    if (joker_object == NULL)
        return NULL;
    return sprite_object_get_sprite((SpriteObject*)joker_object);
}

int joker_get_random_rarity()
{
    int joker_rarity = 0;
    int rarity_roll = rng_get_u32() % 100;
    if (rarity_roll < COMMON_JOKER_CHANCE)
    {
        joker_rarity = COMMON_JOKER;
    }
    else if (rarity_roll < COMMON_JOKER_CHANCE + UNCOMMON_JOKER_CHANCE)
    {
        joker_rarity = UNCOMMON_JOKER;
    }
    else if (rarity_roll < COMMON_JOKER_CHANCE + UNCOMMON_JOKER_CHANCE + RARE_JOKER_CHANCE)
    {
        joker_rarity = RARE_JOKER;
    }
    else if (rarity_roll < COMMON_JOKER_CHANCE + UNCOMMON_JOKER_CHANCE + RARE_JOKER_CHANCE +
                               LEGENDARY_JOKER_CHANCE)
    {
        joker_rarity = LEGENDARY_JOKER;
    }

    return joker_rarity;
}

static int s_get_num_spritesheets()
{
    return MAX_NUM_JOKERS_SPRITESHEETS;
}

static int s_joker_get_spritesheet_idx(u8 joker_id)
{
    return JOKER_ID_TO_SPRITE_MAP[joker_id];
}

static int s_joker_get_sprite_idx_in_sheet(u8 joker_id, int spritesheet_idx)
{
    return joker_id - SPRITESHEET_IDX_TO_STARTING_JOKER_ID[JOKER_ID_TO_SPRITE_MAP[joker_id]];
}

static void s_joker_pb_add_sprite_user(int pb)
{
    s_joker_pb_num_sprite_users[pb - JOKER_BASE_PB]++;
}

static void s_joker_pb_remove_sprite_user(int pb)
{
    int num_sprite_users = s_joker_pb_num_sprite_users[pb - JOKER_BASE_PB];
    s_joker_pb_num_sprite_users[pb - JOKER_BASE_PB] = max(0, num_sprite_users - 1);
}

static int s_joker_pb_get_num_sprite_users(int joker_pb)
{
    return s_joker_pb_num_sprite_users[joker_pb - JOKER_BASE_PB];
}

static int s_get_unused_joker_pb()
{
    for (int i = 0; i < NUM_ELEM_IN_ARR(s_joker_pb_num_sprite_users); i++)
    {
        if (s_joker_pb_num_sprite_users[i] == 0)
        {
            return (i + JOKER_BASE_PB);
        }
    }

    return UNDEFINED;
}

static int s_allocate_pb_if_needed(u8 joker_id)
{
    int joker_spritesheet_idx = s_joker_get_spritesheet_idx(joker_id);
    int joker_pb = s_joker_spritesheet_pb_map[joker_spritesheet_idx];
    if (joker_pb != UNDEFINED)
    {
        // Already allocated
        return joker_pb;
    }

    // Allocate a new palette
    joker_pb = s_get_unused_joker_pb();

    if (joker_pb == UNDEFINED)
    {
        // Ran out of palettes, default to base and pray
        joker_pb = JOKER_BASE_PB;
    }
    else
    {
        s_joker_spritesheet_pb_map[joker_spritesheet_idx] = joker_pb;
        memcpy16(
            &pal_obj_mem[PAL_ROW_LEN * joker_pb],
            joker_gfxPal[joker_spritesheet_idx],
            NUM_ELEM_IN_ARR(joker_gfx0Pal)
        );
    }

    return joker_pb;
}
