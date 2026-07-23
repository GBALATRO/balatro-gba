/**
 * @file joker.h
 *
 * @brief Functions relative to the handling of Jokers
 */
#ifndef JOKER_H
#define JOKER_H

#include "card.h"
#include "game.h"
#include "graphic_utils.h"
#include "item.h"

#include <maxmod.h>

#define MAX_DEFINABLE_JOKERS 150

#define BASE_EDITION     0
#define FOIL_EDITION     1
#define HOLO_EDITION     2
#define POLY_EDITION     3
#define NEGATIVE_EDITION 4

#define MAX_EDITIONS 5

#define COMMON_JOKER    0
#define UNCOMMON_JOKER  1
#define RARE_JOKER      2
#define LEGENDARY_JOKER 3

#define MAX_RARITIES (LEGENDARY_JOKER + 1)

// Percent chance to get a joker of each rarity
// Note that this deviates slightly from the Balatro wiki to allow legendary
// jokers to appear without spectral cards implemented
#define COMMON_JOKER_CHANCE    70
#define UNCOMMON_JOKER_CHANCE  25
#define RARE_JOKER_CHANCE      5
#define LEGENDARY_JOKER_CHANCE 0

// These are the common Joker Events. Special Joker behaviour will be checked on a
// Joker per Joker basis (see if it's there, then do something, e.g. Pareidolia, Baseball Card)
enum JokerEvent
{
    JOKER_EVENT_ON_JOKER_CREATED, // Triggers only once when the Joker is created, mainly used for
                                  // data initialization
    JOKER_EVENT_ON_HAND_PLAYED,   // Triggers only once when the hand is played and before the cards
                                  // are scored
    JOKER_EVENT_ON_CARD_SCORED,   // Triggers when a played card scores (e.g. Walkie Talkie,
                                  // Fibonnacci...)
    JOKER_EVENT_ON_CARD_SCORED_END, // Triggers after the card has finishd scoring (e.g. retrigger
                                    // Jokers)
    JOKER_EVENT_ON_CARD_HELD,       // Triggers when going through held cards
    JOKER_EVENT_INDEPENDENT, // Joker will trigger normally, when Jokers are scored (e.g. base
                             // Joker)
    JOKER_EVENT_ON_HAND_SCORED_END, // Triggers when entire hand has finished scoring (e.g. food
                                    // Jokers)
    JOKER_EVENT_ON_HAND_DISCARDED,  // Triggers when discarding a hand
    JOKER_EVENT_ON_ROUND_END,       // Triggers at the end of the round (e.g. Rocket)
    JOKER_EVENT_ON_BLIND_SELECTED,  // Triggers when selecting a blind (e.g. Dagger, Riff Raff,
                                    // Madness..)
};

// These are flags that can be combined into a single u32 and returned by
// JokerEffect functions to indicate which fields of the output JokerEffect are valid

#define JOKER_EFFECT_FLAG_NONE      0
#define JOKER_EFFECT_FLAG_CHIPS     (1 << 0)
#define JOKER_EFFECT_FLAG_MULT      (1 << 1)
#define JOKER_EFFECT_FLAG_XMULT     (1 << 2)
#define JOKER_EFFECT_FLAG_MONEY     (1 << 3)
#define JOKER_EFFECT_FLAG_RETRIGGER (1 << 4)
#define JOKER_EFFECT_FLAG_EXPIRE    (1 << 5)
#define JOKER_EFFECT_FLAG_MESSAGE   (1 << 6)

// Jokers in the game
#define STENCIL_JOKER_ID      15
#define SHORTCUT_JOKER_ID     48
#define BRAINSTORM_JOKER_ID   41
#define PAREIDOLIA_JOKER_ID   46
#define FOUR_FINGERS_JOKER_ID 50
#define BLUEPRINT_JOKER_ID    52

typedef struct
{
    u8 id;       // Unique ID for the joker, used to identify different jokers
    u8 modifier; // base, foil, holo, poly, negative
    u8 value;
    u8 rarity;

    // General purpose values that are interpreted differently for each Joker (scaling, last
    // retriggered card, etc...)
    s32 scoring_state;
    s32 persistent_state;
} Joker;

typedef struct JokerObject
{
    Item; // First member struct inheritance
    Joker* joker;
} JokerObject;

typedef struct // These jokers are triggered after the played hand has finished scoring.
{
    u32 chips;
    u32 mult;
    u32 xmult;
    int money;
    bool retrigger; // Retrigger played hand (e.g. "Dusk" joker, even though on the wiki it says "On
                    // Scored" it makes more sense to have it here)
    bool expire;    // Will make the Joker expire/destry itself if true (i.e. Bananas and fully
                    // consumed Food Jokers)
    char* message;  // Used to send custom messages e.g. "Extinct!" or "Again!"
} JokerEffect;

// JokerEffectFuncs take in a joker that will be scored, a scored_card that is not NULL when related
// to the given joker_event, and output a joker_effect storing the effects of the scored joker They
// return a set of flags indicating what fields of the joker_effect are valid to access
typedef u32 (*JokerEffectFunc)(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

typedef int (*JokerDescFunc)(Joker* joker, Rect dest_rect);

typedef struct
{
    const char* name;
    u8 rarity;
    u8 base_value;
    bool is_desc_dynamic; // Is the little variable description at the bottom dynamic?
                          // Only used by the Misprint joker for now
    JokerDescFunc joker_print_desc;
    JokerEffectFunc joker_effect_func;
} JokerInfo;
const JokerInfo* get_joker_registry_entry(int joker_id);
size_t get_joker_registry_size(void);

void joker_init();

Joker* joker_new(u8 id);
void joker_destroy(Joker** joker);

// Unique effects like "Four Fingers" or "Credit Card" will be hard coded into game.c with a
// conditional check for the joker ID from the players owned jokers game.c should probably be
// restructured so most of the variables in it are moved to some sort of global variable header file
// so they can be easily accessed and modified for the jokers
u32 joker_get_score_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

const char* joker_get_rarity_string(u8 rarity);

/**
 * @brief Get Joker rarity panel color.
 *
 * The colors are organized in the `card_rarity_pal_gfx.png` file which is organized like this:
 *  - 0     -> transparency
 *  - 1,2   -> Common Joker
 *  - 3,4   -> Uncommon Joker
 *  - 5,6   -> Rare Joker
 *  - 7,8   -> Legendary Joker / Tarot Card
 *  - 9,10  -> Planet Card
 *  - 11,12 -> Spectral Card
 *  - 13,14 -> Voucher
 *
 * @param rarity Value of the rarity (Common, Rare...)
 * @param main_color Whether we want the main or shadow color
 * @return u16 value of the color, not a pointer
 */
u16 joker_get_rarity_color(u8 rarity, bool main_color);

int joker_get_sell_value(const Joker* joker);

JokerObject* joker_object_new(Joker* joker);
void joker_object_destroy(JokerObject** joker_object);
// This doesn't actually score anything, it just performs an animation and plays a sound effect
void joker_object_shake(JokerObject* joker_object, mm_word sound_id);

/**
 * @brief Returns the buy price of the joker object.
 *
 * @param joker_object the joker object whose price to return.
 *
 * @return UNDEFINED in case of error, the buy price of the joker otherwise.
 */
int joker_object_get_buy_price(Item* joker_object);

// TODO: Move to an owned_jokers.c/.h file?
/**
 * @brief Add a Joker to the list of owned Jokers and place it in the joker row.
 *
 * @param joker_object The JokerObject to add cast to Item*
 */
void joker_object_add_to_owned(Item* joker_object);

/**
 * @brief Destroy a JokerObject item, free its resources, and make it available to be rolled.
 *
 * @param joker_object Pointer to the JokerObject Item* to destroy; set to NULL.
 */
void joker_object_dispose(Item** joker_object);

/**
 * @brief Rolls a random Joker among the available ones of a certain rarity
 *
 * @param joker_rarity the rarity of the joker we want to roll
 *
 * @return rolled joker ID
 */
int joker_roll_id_with_rarity(int joker_rarity);

/**
 * @brief Set whether a Joker is available to be rolled for the shop, packs, etc.
 *
 * @param joker_id The ID of the joker whose availability to set.
 * @param rollable true to make it rollable, false otherwise.
 */
void joker_set_rollable(int joker_id, bool rollable);

/**
 * @brief Reset rollable jokers to include all jokers in the registry.
 */
void joker_reset_rollable_jokers(void);

/**
 * @brief Roll and create a new JokerObject item.
 *
 * @return Newly created `Item*` (JokerObject) or NULL if none available.
 */
Item* joker_object_roll_new(void);

// This scores the joker and returns true if it was scored successfully
// card_object = NULL means the joker_event does not concern a particular Card, i.e. Independend or
// On_Blind_Selected as opposed to events that concern a particular card, i.e. On_Card_Scored or
// On_Card_Held
bool joker_object_score(
    JokerObject* joker_object,
    CardObject* card_object,
    enum JokerEvent joker_event
);

Sprite* joker_object_get_sprite(JokerObject* joker_object);
int joker_get_random_rarity();

#endif // JOKER_H
