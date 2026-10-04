/**
 * @file planet.c
 */
#include "planet.h"

#include "audio_utils.h"
#include "card.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "joker.h"
#include "soundbank.h"
#include "util.h"

#include <stdio.h>
#include <tonc.h>

#define MAX_PLANET_OBJECTS 4
#define PLANET_SFX_VOLUME  255

extern const u32 planet_gfxTiles[];
extern const u16 planet_gfxPal[];

#define PLANET_GFX_WORDS (TILE_SIZE * JOKER_SPRITE_OFFSET) // u32 words per planet sprite

typedef struct
{
    const char* name;
    u8 chips;
    u8 mult;
    u8 gfx_idx; // Index of the sprite in planet_gfxTiles
} PlanetInfo;

// Indexed by enum HandType. Royal Flush has no planet of its own, it levels with Straight Flush.
static const PlanetInfo PLANET_INFO[HAND_TYPE_MAX + 1] = {
    [NONE] = {NULL, 0, 0, 0},
    [HIGH_CARD] = {"Pluto", 10, 1, 0},
    [PAIR] = {"Mercury", 15, 1, 1},
    [TWO_PAIR] = {"Uranus", 20, 1, 2},
    [THREE_OF_A_KIND] = {"Venus", 20, 2, 3},
    [STRAIGHT] = {"Saturn", 30, 3, 4},
    [FLUSH] = {"Jupiter", 15, 2, 5},
    [FULL_HOUSE] = {"Earth", 25, 2, 6},
    [FOUR_OF_A_KIND] = {"Mars", 30, 3, 7},
    [STRAIGHT_FLUSH] = {"Neptune", 40, 4, 8},
    [ROYAL_FLUSH] = {"Neptune", 40, 4, 8},
    [FIVE_OF_A_KIND] = {"Planet X", 35, 3, 9},
    [FLUSH_HOUSE] = {"Ceres", 40, 4, 10},
    [FLUSH_FIVE] = {"Eris", 50, 3, 11},
};

static PlanetObject s_planet_objects[MAX_PLANET_OBJECTS];

static inline enum HandType s_level_key(enum HandType hand_type)
{
    if ((int)hand_type <= NONE || hand_type > HAND_TYPE_MAX)
        return NONE;
    return (hand_type == ROYAL_FLUSH) ? STRAIGHT_FLUSH : hand_type;
}

void hand_levels_reset(void)
{
    for (int i = 0; i <= HAND_TYPE_MAX; i++)
    {
        g_game_vars.hand_levels[i] = 1;
    }
}

int hand_get_level(enum HandType hand_type)
{
    enum HandType key = s_level_key(hand_type);
    if (key == NONE)
        return 1;

    int level = g_game_vars.hand_levels[key];
    return (level < 1) ? 1 : level;
}

void hand_level_up(enum HandType hand_type)
{
    enum HandType key = s_level_key(hand_type);
    if (key != NONE && g_game_vars.hand_levels[key] < HAND_LEVEL_MAX)
    {
        g_game_vars.hand_levels[key]++;
    }
}

u32 hand_get_level_bonus_chips(enum HandType hand_type)
{
    enum HandType key = s_level_key(hand_type);
    return (u32)(hand_get_level(key) - 1) * PLANET_INFO[key].chips;
}

u32 hand_get_level_bonus_mult(enum HandType hand_type)
{
    enum HandType key = s_level_key(hand_type);
    return (u32)(hand_get_level(key) - 1) * PLANET_INFO[key].mult;
}

const char* planet_get_name(enum HandType hand_type)
{
    enum HandType key = s_level_key(hand_type);
    return (key == NONE) ? "" : PLANET_INFO[key].name;
}

int planet_object_print_desc(PlanetObject* planet, Rect dest_rect)
{
    GBAL_RETURN_IF_NULL_RET(planet, 0);

    enum HandType hand_type = s_level_key(planet->hand_type);
    char desc[160];
    snprintf(
        desc,
        sizeof(desc),
        TTE_BLACK_TAG "Level up " TTE_DARK_BLUE_TAG "%s " TTE_BLACK_TAG "to lvl " TTE_DARK_BLUE_TAG
                      "%d " TTE_BLUE_TAG "+%d " TTE_BLACK_TAG "Chips " TTE_RED_TAG "+%d "
                      TTE_BLACK_TAG "Mult",
        get_hand_type_name(hand_type),
        hand_get_level(hand_type) + 1,
        PLANET_INFO[hand_type].chips,
        PLANET_INFO[hand_type].mult
    );
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static PlanetObject* s_planet_object_new(enum HandType hand_type)
{
    PlanetObject* planet = NULL;
    for (int i = 0; i < MAX_PLANET_OBJECTS; i++)
    {
        if (!s_planet_objects[i].in_use)
        {
            planet = &s_planet_objects[i];
            break;
        }
    }
    if (planet == NULL)
        return NULL;

    // Planets borrow a sprite slot (OAM entry + 16 tiles) from the Joker sprite slots
    int layer = joker_sprite_layer_alloc();
    if (layer == UNDEFINED)
        return NULL;

    sprite_object_init((SpriteObject*)planet);
    planet->type = ITEM_TYPE_PLANET;
    planet->hand_type = (u8)hand_type;
    planet->layer = (s8)layer;
    planet->in_use = true;

    int tile_index = JOKER_TID + layer * JOKER_SPRITE_OFFSET;
    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index],
        &planet_gfxTiles[PLANET_INFO[s_level_key(hand_type)].gfx_idx * PLANET_GFX_WORDS],
        PLANET_GFX_WORDS
    );
    memcpy16(pal_obj_bank[PLANET_PB], planet_gfxPal, NUM_ELEM_IN_ARR(pal_obj_bank[PLANET_PB]));

    sprite_object_set_sprite(
        (SpriteObject*)planet,
        sprite_new(
            ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF,
            ATTR1_SIZE_32,
            tile_index,
            PLANET_PB,
            JOKER_STARTING_LAYER + layer
        )
    );

    return planet;
}

static void s_planet_object_destroy(PlanetObject* planet)
{
    if (planet == NULL || !planet->in_use)
        return;

    sprite_object_destroy((SpriteObject*)planet);
    joker_sprite_layer_free(planet->layer);
    planet->in_use = false;
}

Item* planet_object_roll_new(enum RngSequence key)
{
    // Hands the player can currently get a planet for. The three "secret" hands only
    // show up once they have been played at least once during the run.
    enum HandType candidates[HAND_TYPE_MAX];
    int num_candidates = 0;

    for (int hand_type = HIGH_CARD; hand_type <= HAND_TYPE_MAX; hand_type++)
    {
        if (hand_type == ROYAL_FLUSH)
            continue;
        if (hand_type >= FIVE_OF_A_KIND && g_game_vars.nb_played_hands[hand_type - 1] == 0)
            continue;
        candidates[num_candidates++] = (enum HandType)hand_type;
    }

    enum HandType rolled = candidates[rng_get_u32(key) % num_candidates];
    return (Item*)s_planet_object_new(rolled);
}

int planet_object_get_buy_price(Item* item)
{
    GBAL_RETURN_IF_NULL_RET(item, UNDEFINED);
    return PLANET_BUY_PRICE;
}

bool planet_object_can_acquire(Item* item)
{
    return item != NULL;
}

void planet_object_acquire(Item* item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID(item, ITEM_TYPE_PLANET);

    PlanetObject* planet = (PlanetObject*)item;
    hand_level_up(planet->hand_type);

    play_sfx(SFX_MULT, MM_BASE_PITCH_RATE, PLANET_SFX_VOLUME);

    // Planets are used immediately, they never go to the inventory
    s_planet_object_destroy(planet);
}

void planet_object_dispose(Item** item)
{
    GBAL_RETURN_IF_NULL_VOID(item);
    GBAL_RETURN_IF_NULL_VOID(*item);
    ITEM_RETURN_IF_UNEXPECTED_TYPE_VOID(*item, ITEM_TYPE_PLANET);

    s_planet_object_destroy((PlanetObject*)(*item));
    *item = NULL;
}
