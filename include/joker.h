#ifndef JOKER_H
#define JOKER_H

#include "card.h"
#include "game.h"
#include "graphic_utils.h"
#include "sprite.h"

#include <maxmod.h>

// This won't be more than the number of jokers in your current deck
// plus the amount that can fit in the shop, 8 should be fine. For now...
#define MAX_ACTIVE_JOKERS 8

#define MAX_DEFINABLE_JOKERS 150

// Tile ID for the starting index in the tile memory
#define JOKER_TID           (MAX_HAND_SIZE + MAX_SELECTION_SIZE) * JOKER_SPRITE_OFFSET
#define JOKER_SPRITE_OFFSET 16 // Offset for the joker sprites
#define JOKER_BASE_PB       4  // The starting palette index for the jokers
#define JOKER_LAST_PB       (NUM_PALETTES - 1)
// Currently allocating the rest of the palettes for the jokers.
// This number needs to be decreased once we need to allocated palettes for other sprites
// such as planet cards etc.

#define JOKER_STARTING_LAYER 27

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

#define MAX_JOKER_OBJECTS 32 // The maximum number of joker objects that can be created at once

// Jokers in the game
#define DEFAULT_JOKER_ID      0
#define GREEDY_JOKER_ID       1
#define STENCIL_JOKER_ID      16
#define SHORTCUT_JOKER_ID     26
#define PAREIDOLIA_JOKER_ID   30
#define BLUEPRINT_JOKER_ID    39
#define BRAINSTORM_JOKER_ID   40
#define FOUR_FINGERS_JOKER_ID 48

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
    Joker* joker;
    SpriteObject* sprite_object;
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

typedef struct
{
    u8 rarity;
    u8 base_value;
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
int joker_get_sell_value(const Joker* joker);

JokerObject* joker_object_new(Joker* joker);
void joker_object_destroy(JokerObject** joker_object);
void joker_object_update(JokerObject* joker_object);
// This doesn't actually score anything, it just performs an animation and plays a sound effect
void joker_object_shake(JokerObject* joker_object, mm_word sound_id);
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
