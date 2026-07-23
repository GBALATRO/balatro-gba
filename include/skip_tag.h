/**
 * @file skip_tag.h
 * @brief Data structures and functions related to the handling of Skip Tags
 */
#ifndef SKIP_TAGS_H
#define SKIP_TAGS_H

#include "game/common_ui.h"
#include "graphic_utils.h"
#include "joker.h"
#include "sprite.h"

#include <tonc.h>

/**
 * @brief Sprite IDs of the Skip Tags, as an offset relative to SKIP_TAG_STARTING_LAYER
 *
 * @sa SpriteStartingLayers
 */
enum SkipTagLayers
{
    SMALL_BLIND_SKIP_TAG_LAYER,
    BIG_BLIND_SKIP_TAG_LAYER,
    OWNED_SKIP_TAG_STARTING_LAYER
};

// Tile ID for the starting index in the tile memory
#define SKIP_TAG_TID (SKIP_TAG_STARTING_LAYER * SKIP_TAG_SPRITE_SIZE)

#define INVESTMENT_TAG_REWARD 25

// clang-format off
/**
 * @brief List of all Skip Tags, in the order they are in on their spritesheet.
 *         Not all of them are implemented (or will be) but at least they are in order.
 *
 * The Negative, Foil, Holographic and Polychrome Tags are not planned and thus absent from both
 * this enum, and the Tags spritesheet, but the sprites for them are available in the relevant
 * description should we ever need them:
 * https://github.com/GBALATRO/balatro-gba/discussions/131
 *
 * @sa NB_SKIP_TAG_TYPES, ante1_skip_tags_roll_table, all_skip_tags_roll_table
 */
enum SkipTagTypes
{
    SKIP_TAG_TYPE_UNCOMMON,    // TODO
    SKIP_TAG_TYPE_RARE,        // TODO
    SKIP_TAG_TYPE_INVESTMENT,
    SKIP_TAG_TYPE_VOUCHER,     // NEEDS VOUCHERS
    SKIP_TAG_TYPE_BOSS,
    SKIP_TAG_TYPE_STANDARD,    // NEEDS PACKS
    SKIP_TAG_TYPE_CHARM,       // NEEDS PACKS
    SKIP_TAG_TYPE_METEOR,      // NEEDS PACKS
    SKIP_TAG_TYPE_BUFFOON,     // NEEDS PACKS
    SKIP_TAG_TYPE_HANDY,
    SKIP_TAG_TYPE_GARBAGE,
    SKIP_TAG_TYPE_ETHEREAL,    // NEEDS PACKS
    SKIP_TAG_TYPE_COUPON,      // TODO
    SKIP_TAG_TYPE_DOUBLE,
    SKIP_TAG_TYPE_JUGGLE,
    SKIP_TAG_TYPE_D6,
    SKIP_TAG_TYPE_TOP_UP,
    SKIP_TAG_TYPE_SPEED,
    SKIP_TAG_TYPE_ORBITAL,     // NEEDS HANDS MENU
    SKIP_TAG_TYPE_ECONOMY,
    MAX_SKIP_TAG_TYPES
};
// clang-format on

typedef struct SkipTag
{
    SpriteObject;
    u8 type;

} SkipTag;

enum SkipTagEvent
{
    SKIP_TAG_EVENT_NONE,           // For tags not yet impelemented, does nothing
    SKIP_TAG_EVENT_IMMEDIATE,      // Triggers immediately after being bought.
                                   // (e.g. Boss, Charm, Double,...)
    SKIP_TAG_EVENT_ON_ROUND_START, // Triggers when starting a new round (Juggler)
    SKIP_TAG_EVENT_ON_ROUND_END,   // Triggers when beating a round (Investment)
    SKIP_TAG_EVENT_ON_SHOP_INIT,   // Triggers when entering the Shop (e.g. Coupon, D6,...)
    SKIP_TAG_EVENT_ON_SHOP_REROLL  // Triggers when rolling items for sale in the Shop
                                   // (Foil, Holographic, Negative, Polychrome)
};

enum SkipTagEffect
{
    SKIP_TAG_EFFECT_NONE,
    SKIP_TAG_EFFECT_TRIGGER,
    SKIP_TAG_EFFECT_END
};

// SkipTagCallbacks will attempt to either return whether the SkipTag can be activated or trigger it
typedef bool (*SkipTagCondition)(void);
typedef void (*SkipTagCallback)(void);

/**
 * @brief Data structure containing the info about a Skip Tag, namely:
 *
 *  - `event_type` the event for which the Tag will trigger (immediate, on round start, etc...)
 *  - `tag_condition_func` a function returning whether the Tag should trigger. Indeed, not all tags
 *    must trigger for their given event, e.g. the D6 Tag, only one of which can be consumed per
 *    shop.
 *  - `tag_effect_func` the function that will actually do the Tag's work, like doubling money.
 */
typedef struct
{
    enum SkipTagEvent event_type;
    SkipTagCondition tag_condition_func;
    SkipTagCallback tag_effect_func;
} SkipTagInfo;

/**
 * @brief Get the registry entry info for the given Skip Tag
 *
 * @param tag_id ID of the Skip Tag we need the info of
 *
 * @return a pointer to the info structure for the given Skip Tag
 * @sa SkipTagInfo
 */
const SkipTagInfo* get_skip_tag_registry_entry(int tag_id);

/**
 * @brief Get the owned skip tags list
 *
 * @return List*
 */
List* get_owned_skip_tags(void);

/**
 * @brief Create a new Skip Tag object for the given Tag ID and allocates space for it in the
 *        corresponding memory pool.
 *
 * @param tag_type the ID of the Tag we want
 *
 * @return a pointer to a new SkipTag object
 * @sa SkipTagTypes, SkipTag
 */
SkipTag* skip_tag_new(u8 tag_type);

/**
 * @brief Set the sprite for the given SkipTag, according to its internal ID.
 *
 * @param tag pointer to the SkipTag object
 * @param pos position the new Sprite will immediately be shown at. Cannot be **UNDEFINED**
 * @param layer sprite layer for the new Sprite, as an offset to the starting layer for the
 *              SKIP_TAG sprite type
 */
void skip_tag_set_sprite(SkipTag* tag, BG_POINT pos, int layer);

/**
 * @brief Destroy the given SkipTag, invalidate it in the corresponding memory pool and set the
 *        pointer provided to NULL
 *
 * @param tag pointer of pointer to the SkipTag we want to destroy
 */
void skip_tag_destroy(SkipTag** tag);

/**
 * @brief Show or hide the owned SkipTag sprites by moving them offscreen to the right.
 *
 * @param hidden whether the sprites should be hidden or not
 */
void move_owned_skip_tags_offscreen(bool hidden);

/**
 * @brief Create a new SkipTag with a random ID, picked by taking the Ante into account, as some
 *        Tags only appear from Ante 2 onwards.
 *
 * @return SkipTag pointer to the new SkipTag object
 * @sa skip_tag_new
 */
SkipTag* roll_skip_tag(void);

/**
 * @brief Checks if the given tag type is present in owned Tags list
 *
 * @param tag_type SkipTagType to check
 * @return true if tag type is present, false otherwise
 */
bool skip_tag_is_owned(u8 tag_type);

/**
 * @brief Counts how many tags of a certain type we own.
 *
 * @param tag_type SkipTagType to count
 * @return int
 */
int skip_tag_count(u8 tag_type);

/**
 * @brief Adds the SkipTag to the owned list, snaps it into position and sets the original pointer
 *         to NULL
 *
 * @param blind_tag
 */
void add_skip_tag(SkipTag** blind_tag);

/**
 * @brief Pops the Tag at index in owned list, destroys it and rearranges the remaining Tags'
 *         sprites
 *
 * @param tag_idx index of the Tag to remove in owned list
 */
void remove_skip_tag(int tag_idx);

/**
 * @brief Initialize the Skip Tag processing structure and state, then launches the processing.
 *
 * Skip Tags will be fully processed over 61 frames (1 + 2 * TM_SKIP_TAG_ANIM_DURATION) so we have
 * time to process what's happening:
 *
 *  - Detect triggered Tag, executed instantly but will take a full frame given how state machines
 *    work
 *  - Starting the little bouncy animation as the Tag triggers
 *  - Delete triggered Tag, serves as a little pause
 *
 * @param checked_tag_event the event for which the owned Tags will be evaluated
 *
 * @sa SkipTagEvent
 */
void skip_tag_process_init(enum SkipTagEvent checked_tag_event);

/**
 * @brief Recover the effect of the Tag being processed, if any.
 *
 * This function must be called every frame after `skip_tag_process_init` so that we stay up to date
 * with the Tags processing.
 *
 * @return enum SkipTagEffect
 *
 * @sa skip_tag_process_init
 */
enum SkipTagEffect skip_tag_process_get_effect(void);

/**
 * @brief Suspend the processing of Skip Tags.
 *
 * This is called automatically when Tags have finished processing for the current SkipTagEvent, but
 * it can be invoked by a game state to temporarily pause the processing while doing some other
 * thing, like showing an animation. Call `skip_tag_process_resume` to pick up processing where it
 * was left.
 *
 * @sa skip_tag_process_resume
 */
void skip_tag_process_pause(void);

/**
 * @brief Resume the processing of Skip Tags.
 *
 * After `skip_tag_process_pause` was called, use this function some time afterwards to resume Tag
 * processing.
 *
 * @sa skip_tag_process_pause
 */
void skip_tag_process_resume(void);

#endif // SKIP_TAGS_H
