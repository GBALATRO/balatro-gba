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
#include "random.h"

#include <maxmod.h>

/**
 * @def MAX_DEFINABLE_JOKERS
 * @brief Number of Jokers which can be registered
 */
#define MAX_DEFINABLE_JOKERS 150

// Pack the enum into a u8 so that the Joker struct uses as little memory as possible
enum JokerRarity : u8
{
    COMMON_JOKER,
    UNCOMMON_JOKER,
    RARE_JOKER,
    LEGENDARY_JOKER,
    MAX_RARITIES
};

/**
 * @def COMMON_JOKER_CHANCE
 * @brief Chances out of 100 for a Common Joker to appear in the shop
 */
#define COMMON_JOKER_CHANCE 70

/**
 * @def UNCOMMON_JOKER_CHANCE
 * @brief Chances out of 100 for an Uncommon Joker to appear in the shop
 */
#define UNCOMMON_JOKER_CHANCE 25

/**
 * @def RARE_JOKER_CHANCE
 * @brief Chances out of 100 for a Rare Joker to appear in the shop
 */
#define RARE_JOKER_CHANCE 5

/**
 * @def LEGENDARY_JOKER_CHANCE
 * @brief Chances out of 100 for a Legendary Joker to appear in the shop
 */
#define LEGENDARY_JOKER_CHANCE 0

// These are the common Joker Events. Special Joker behaviour will be checked on a
// Joker per Joker basis (see if it's there, then do something, e.g. Pareidolia, Baseball Card)
enum JokerEvent
{
    /**
     * @brief Triggers only once when the Joker is created, mainly used for data initialization
     */
    JOKER_EVENT_ON_JOKER_CREATED,

    /**
     * @brief Triggers only once when the hand is played and before the cards are scored
     */
    JOKER_EVENT_ON_HAND_PLAYED,

    /**
     * @brief Triggers when a played card scores (e.g. Walkie Talkie, Fibonnacci...)
     */
    JOKER_EVENT_ON_CARD_SCORED,

    /**
     * @brief Triggers after the card has finishd scoring (e.g. retrigger Jokers)
     */
    JOKER_EVENT_ON_CARD_SCORED_END,

    /**
     * @brief Triggers when going through held cards
     */
    JOKER_EVENT_ON_CARD_HELD,

    /**
     * @brief Joker will trigger normally, when Jokers are scored (e.g. base Joker)
     */
    JOKER_EVENT_INDEPENDENT,

    /**
     * @brief Triggers when entire hand has finished scoring (e.g. food Jokers)
     */
    JOKER_EVENT_ON_HAND_SCORED_END,

    /**
     * @brief Triggers when discarding a hand
     */
    JOKER_EVENT_ON_HAND_DISCARDED,

    /**
     * @brief Triggers at the end of the round (e.g. Rocket)
     */
    JOKER_EVENT_ON_ROUND_END,

    /**
     * @brief Triggers when selecting a blind (e.g. Dagger, Riff Raff, Madness..)
     */
    JOKER_EVENT_ON_BLIND_SELECTED
};

/**
 * @brief These flags indicate which fields of an output JokerEffect are valid when returned by
 *         JokerEffect functions.
 */
// clang-format off
enum JokerEffectFlag : u8
{
    JOKER_EFFECT_FLAG_NONE      = 0,
    JOKER_EFFECT_FLAG_CHIPS     = (1 << 0),
    JOKER_EFFECT_FLAG_MULT      = (1 << 1),
    JOKER_EFFECT_FLAG_XMULT     = (1 << 2),
    JOKER_EFFECT_FLAG_MONEY     = (1 << 3),
    JOKER_EFFECT_FLAG_RETRIGGER = (1 << 4),
    JOKER_EFFECT_FLAG_EXPIRE    = (1 << 5),
    JOKER_EFFECT_FLAG_MESSAGE   = (1 << 6)
};
// clang-format on

/**
 * @def MAX_JOKER_OBJECTS
 * @brief The maximum number of joker objects that can be created at once
 */
#define MAX_JOKER_OBJECTS 32

// Jokers in the game
#define STENCIL_JOKER_ID      15
#define SHORTCUT_JOKER_ID     48
#define BRAINSTORM_JOKER_ID   41
#define PAREIDOLIA_JOKER_ID   46
#define FOUR_FINGERS_JOKER_ID 50
#define BLUEPRINT_JOKER_ID    52

/**
 * @brief Data structure representing a Joker's inherent state and values
 */
typedef struct
{
    /**
     * @brief Unique ID for the joker, used to identify different jokers
     */
    u8 id;

    /**
     * @brief Price to pay in order to acquire this joker
     */
    u8 value;

    /**
     * @brief Rarity value of this Joker
     */
    enum JokerRarity rarity;

    // General purpose values that are interpreted differently for each Joker (scaling, last
    // retriggered card, etc...)

    /**
     * @brief Temporary value used to store various data while the Joker's effect is being
     *         processed. Interpreted differently for each Joker (e.g. retriggered card, etc...)
     */
    s32 scoring_state;

    /**
     * @brief Value storing long term data, retain during the whole lifetime of the Joker.
     *         Interpreted differently for each Joker (e.g. scaling, expiration, etc...)
     */
    s32 persistent_state;
} Joker;

/**
 * @brief Data structure holding a Joker's data and inheriting the Item API for the standardization
 *         of its management and visuals
 * @sa Item
 */
typedef struct JokerObject
{
    /**
     * @brief First member struct inheritance
     */
    Item;

    Joker* joker;
} JokerObject;

/**
 * @brief Structure containing the results of a Joker triggering
 */
typedef struct
{
    u32 chips;
    u32 mult;
    u32 xmult;
    int money;

    /**
     * @brief Retrigger played hand (e.g. "Dusk" joker, even though on the wiki it says "On Scored"
     *         it makes more sense to have it here)
     */
    bool retrigger;

    /**
     * @brief Will make the Joker expire/destroy itself if true (i.e. Bananas and fully consumed
     *         Food Jokers)
     */
    bool expire;

    /**
     * @brief Used to send custom messages e.g. "Extinct!" or "Again!"
     */
    char* message;
} JokerEffect;

/**
 * @brief JokerEffectFunc functions are called to apply Joker effects when they trigger
 *
 * @param[in]  joker        pointer to the Joker whose effect is being processed
 * @param[in]  scored_card  pointer to the Card the effect of the Joker applies to. Cannot be
 *                          **NULL** if related to the given @p joker_event
 * @param[in]  joker_event  event for which the effect is being processed
 * @param[out] joker_effect pointer of pointer to the single JokerEffect structure
 * @return a set of JokerEffectFlag to indicate what fields of the @p joker_effect are valid to
 *         access
 */
typedef u32 (*JokerEffectFunc)(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

/**
 * @brief Prints the description of a given Joker on the screen
 *
 * @param joker     pointer to a Joker whose description needs printing
 * @param dest_rect rectangular region within which the Joker's description must fit, size values in
 *                  pixels.
 * @return number of lines printed
 */
typedef int (*JokerDescFunc)(Joker* joker, Rect dest_rect);

/**
 * @brief Structure containing various pieces of information and callbacks about a Joker, but never
 *        change from one instance to the other.
 *
 * A static array of these structures is owned by the `joker_effects.c` and organized in such a way
 * that using a Joker's id as an index gives us the right information about it.
 *
 * Using such a registry allows for minimal memory footprint by duplicating as little data as
 * possible.
 *
 * @sa get_joker_registry_entry
 */
typedef struct
{
    /**
     * @brief Name string of a Joker
     */
    const char* name;

    /**
     * @brief Determines how often a Joker appears in the Shop
     */
    enum JokerRarity rarity;

    /**
     * @brief Base price to pay to acquire a Joker
     */
    u8 base_value;

    /**
     * @brief Description printing callback
     */
    JokerDescFunc joker_print_desc;

    /**
     * @brief Joker effect processing callback
     */
    JokerEffectFunc joker_effect_func;
} JokerInfo;

/**
 * @brief Get the registry entry for a given Joker ID
 *
 * @param joker_id of the Joker we need info on
 * @return pointer to a constant JokerInfo struct
 */
const JokerInfo* get_joker_registry_entry(int joker_id);

/**
 * @brief Get the number of Jokers with an entry in the registry
 *
 * @return size_t
 */
size_t get_joker_registry_size(void);

/**
 * @brief Initializes the necessary internal data to manage the Jokers
 */
void joker_init();

/**
 * @brief Create a Joker with a certain type
 *
 * @param id of the Joker to create
 * @return pointer to the new Joker structure in the corresponding memory pool
 */
Joker* joker_new(u8 id);

/**
 * @brief Destroy the given Joker struct and free its slot in the corresponding memory pool
 *
 * @param joker pointer of pointer to the Joker struct to destroy and free
 */
void joker_destroy(Joker** joker);

// Unique effects like "Four Fingers" or "Credit Card" will be hard coded into game.c with a
// conditional check for the joker ID from the players owned jokers game.c should probably be
// restructured so most of the variables in it are moved to some sort of global variable header file
// so they can be easily accessed and modified for the jokers
/**
 * @brief Process a Joker's effect when scored.
 *
 * Matches @ref JokerEffectFunc()
 *
 * @note Unique effects like "Four Fingers" or "Credit Card" will be hard coded into game.c with a
 *        conditional check for the joker ID from the players owned jokers.
 *        game.c should probably be restructured so most of the variables in it are moved to some
 *        sort of global variable header file so they can be easily accessed and modified for the
 *        jokers.
 *
 * @param[in]  joker        pointer to the Joker to score
 * @param[in]  scored_card  pointer to the Card the effect of the Joker applies to. Cannot be
 *                          **NULL** if related to the given @p joker_event
 * @param[in]  joker_event  event to score the Joker for
 * @param[out] joker_effect pointer of pointer to the JokerEffect structure
 * @return a set of JokerEffectFlag to indicate what fields of the @p joker_effect are valid to
 *         access
 */
u32 joker_get_score_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

/**
 * @brief Returns the sell value of the joker
 *
 * @param joker Pointer to the joker - if NULL returns UNDEFINED
 * @return UNDEFINED if error occurred, the joker's sell value otherwise
 */
int joker_get_sell_value(const Joker* joker);

/**
 * @brief Get the price to pay to acquire a certain Joker
 *
 * @param joker Pointer to the joker - if NULL returns UNDEFINED
 * @return UNDEFINED if error occurred, the joker's purchase value otherwise
 */
int joker_get_buy_price(const Joker* joker);

/**
 * @brief Get the name of a certain Joker
 *
 * @param joker_object the Joker we want to get the name of
 * @return const string containing the name of the Joker, NULL if joker data is invalid
 */
const char* joker_object_get_name(Item* joker_object);

/**
 * @brief Get Joker rarity name and associated panel color.
 *
 * @param joker_object pointer to a JokerObject we need the rarity's info of
 * @return struct containing values of all info on this Joker's rarity
 *
 * @sa item_get_subtype_info
 */
ItemDescSubtypeInfo joker_object_get_rarity_info(Item* joker_object);

/**
 * @brief Create a new JokerObject from a Joker struct
 *
 * @param joker to create the JokerObject from
 * @return pointer to the new JokerObject structure in the corresponding memory pool
 */
JokerObject* joker_object_new(Joker* joker);

/**
 * @brief Destroy the given JokerObject struct and free its slot in the corresponding memory pool
 *
 * @param joker_object pointer of pointer to the JokerObject struct to destroy and free; set to
 *                     NULL.
 */
void joker_object_destroy(JokerObject** joker_object);

/**
 * @brief Performs an animation and plays a sound effect. Doesn't actually score anything.
 *
 * @param joker_object whose Sprite to shake
 * @param sound_id     SFX to play upon shaking the Sprite
 */
void joker_object_shake(JokerObject* joker_object, mm_word sound_id);

/**
 * @brief Returns the buy price of the joker object.
 *
 * Matches @ref ItemFuncs.get_buy_price()
 *
 * @param joker_object the joker object whose price to return.
 *
 * @return UNDEFINED in case of error, the buy price of the joker otherwise.
 */
int joker_object_get_buy_price(Item* joker_object);

/**
 * @brief Returns the sell price of the joker object.
 *
 * Matches @ref ItemFuncs.get_sell_price()
 *
 * @param joker_object the joker object whose price to return.
 *
 * @return UNDEFINED in case of error, the sell price of the joker otherwise.
 */
int joker_object_get_sell_price(Item* joker_object);

// TODO: Move to an owned_jokers.c/.h file?
/**
 * @brief Add a Joker to the list of owned Jokers and place it in the joker row.
 *
 * Matches @ref ItemFuncs.acquire()
 *
 * @param joker_object The JokerObject to add cast to Item*
 */
void joker_object_add_to_owned(Item* joker_object);

/**
 * @brief Destroy a JokerObject item, free its resources, and make it available to be rolled.
 *
 * Matches @ref ItemFuncs.dispose()
 *
 * @param joker_object Pointer to the JokerObject Item* to destroy; set to NULL.
 */
void joker_object_dispose(Item** joker_object);

/**
 * @brief Print the description of the requested Joker centered within the specified Rect
 *
 * @param joker_object Joker to print the description of
 * @param dest_rect Rectangular region the description must fit in. In case the text is too big, it
 *                   will overflow downwards while still respecting left and right boundaries.
 *
 * @return the number of lines the description was actually printed on
 */
int joker_object_print_description(Item* joker_object, Rect dest_rect);

/**
 * @brief Rolls a random Joker among the available ones of a certain rarity
 *
 * @param joker_rarity the rarity of the joker we want to roll
 *
 * @return rolled joker ID
 */
int joker_roll_id_with_rarity(int joker_rarity);

/**
 * @brief Rolls a random Joker among the available ones of a certain rarity
 *
 * @param joker_rarity the rarity of the joker we want to roll
 * @param key to the RNG sequence used
 *
 * @return rolled Joker ID
 */
int joker_roll_id(enum JokerRarity joker_rarity, enum RngSequence key);

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
 * @param key to the RNG sequence that will be used to roll the Joker ID.
 *
 * @return Newly created `Item*` (JokerObject) or NULL if none available.
 */
Item* joker_object_roll_new(enum RngSequence key);

/**
 * @brief Get the Sprite of a JokerObject
 *
 * @param joker_object to get the Sprite of
 * @return pointer to the joker's Sprite. Can be **NULL** if not yet initialized
 */
Sprite* joker_object_get_sprite(JokerObject* joker_object);

#endif // JOKER_H
