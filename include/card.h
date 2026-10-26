/**
 * @file card.h
 *
 * @brief Functions relative to the handling of playing Cards
 */

#ifndef CARD_H
#define CARD_H

#include "deck_types.h"
#include "game/common_ui.h"
#include "item.h"

#include <maxmod.h>
#include <tonc.h>

/**
 * @def MAX_CARDS
 * @brief Number of Cards that can be allocated in memory. Arbitrary high value so we don't have
 *         issues
 */
#define MAX_CARDS 128

/**
 * @def MAX_CARDS_ON_SCREEN
 * @brief How many Cards can be shown on screen at any given moment through a Sprite owned by an
 *         associated CardObject
 *
 * Equal to plus one to @p MAX_HAND_SIZE account for the Deck sprite
 */
#define MAX_CARDS_ON_SCREEN (MAX_HAND_SIZE + 1)

/**
 * @brief Card suits values
 */
enum CardSuit : u8
{
    DIAMONDS = 0,
    CLUBS,
    HEARTS,
    SPADES,
    NUM_SUITS
};

/**
 * @brief Card ranks as stored in the Card structures
 * @sa RANK_OFFSET
 */
enum CardRank : u8
{
    TWO = 0,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    TEN,
    JACK,
    QUEEN,
    KING,
    ACE,
    NUM_RANKS
};

/**
 * @def RANK_OFFSET
 * @brief Offset of the rank to get the chips actually scored by simple cards to account for the
 *         lowest rank being 2 when rank values start at 0
 */
#define RANK_OFFSET 2

/**
 * @def IMPOSSIBLY_HIGH_CARD_VALUE
 * @brief Arbitrary high value used as default when searching for the lowest Card value scored
 */
#define IMPOSSIBLY_HIGH_CARD_VALUE 100

/**
 * @def DEFAULT_HIGH_CONTRAST
 * @brief Default value for the High Contrast Sprites accessibility option
 */
#define DEFAULT_HIGH_CONTRAST false

/**
 * @def DEFAULT_MORE_READABLE
 * @brief Default value for the Big Sprites accessibility option
 */
#define DEFAULT_MORE_READABLE false

/**
 * @brief Data structure containing values inherent to a playing card
 */
typedef struct Card
{
    /**
     * @brief Suit of the Card (e.g. Spade, Diamond, etc...)
     */
    enum CardSuit suit;

    /**
     * @brief Rank of the Card (e.g. 3, King, Ace, etc...)
     */
    enum CardRank rank;
} Card;

/**
 * @brief Data structure holding a Card's data and inheriting the Item API for the standardization
 *         of its management and visuals
 * @sa Item
 */
typedef struct CardObject
{
    /**
     * @brief First member struct inheritance
     */
    Item;

    /**
     * @brief Card structure holding the CardObject data
     */
    Card* card;

    /**
     * @brief Determines whether the Card should be displayed as selected in some way
     */
    bool selected;
} CardObject;

/**
 * @brief Initializes the data necessary to the handling and rendering of Cards and their Sprites
 */
void card_init(void);

// Card sprites accessibility functions

/**
 * @brief Toggle the High Contrast card sprites on or off
 *
 * @param enable if true, high contrast will be enabled, disabled otherwise
 */
void set_cards_high_contrast(bool enable);

/**
 * @brief Get whether High Contrast card sprites are currently enabled
 *
 * @return true if high contrast is enabled, false otherwise
 */
bool get_cards_high_contrast(void);

/**
 * @brief Toggle the more readable card sprites on or off
 *
 * @param enable if true, the more readable sprites will be enabled, disabled otherwise
 */
void set_cards_more_readable(bool enable);

/**
 * @brief Get whether the more readable card sprites are currently enabled
 *
 * @return true if the more readable sprites are enabled, false otherwise
 */
bool get_cards_more_readable(void);

// Card methods

/**
 * @brief Create a new Card structure
 *
 * @param suit of the new Card
 * @param rank of the new Card
 * @return pointer to the new Card structure in the corresponding memory pool
 */
Card* card_new(enum CardSuit suit, enum CardRank rank);

/**
 * @brief Destroy the given Card struct and free its slot in the corresponding memory pool
 *
 * @param card pointer of pointer to the Card struct to destroy and free
 */
void card_destroy(Card** card);

/**
 * @brief Get the amount of Chips a Card will give when scored
 *
 * @param card pointer to the Card to get the Chips value of
 * @return the actual Chips amount the Card will score
 */
u8 card_get_value(Card* card);

// CardObject methods

/**
 * @brief Create a new CardObject structure
 *
 * @param card pointer to the Card structure to base the CardObject on
 * @return pointer to the new CardObject structure in the corresponding memory pool
 */
CardObject* card_object_new(Card* card);

/**
 * @brief Destroy the given CardObject struct and free its slot in the corresponding memory pool
 *
 * @param card pointer of pointer to the CardObject struct to destroy and free; set to NULL.
 */
void card_object_destroy(CardObject** card_object);

/**
 * @brief Allocate the Sprite of a CardObject with the visuals corresponding to its Suit and Rank
 *
 * @param card_object pointer to the CardObject to set the Sprite of
 * @param layer       index of the Sprite in the OAM memory, expressed as an offset relative to the
 *                    base layer for Sprite type `CARD_SPRITE`
 *
 * @sa SpriteType, sprite_get_starting_layer
 */
void card_object_set_sprite(CardObject* card_object, s16 layer);

/**
 * @brief Allocate the Sprite of a CardObject with the visuals corresponding to a Deck
 *
 * @param card_object pointer to the CardObject to set the Sprite of
 * @param deck        type of deck to show with the CardObject's Sprite
 * @param layer       index of the Sprite in the OAM memory, expressed as an offset relative to the
 *                    base layer for Sprite type `CARD_SPRITE`
 *
 * @sa card_object_set_sprite
 */
void card_object_set_sprite_face_down(CardObject* card_object, enum DeckType deck, s16 layer);

/**
 * @brief Performs an animation and plays a sound effect. Doesn't actually score anything.
 *
 * @param card_object whose Sprite to shake
 * @param sound_id    SFX to play upon shaking the Sprite
 */
void card_object_shake(CardObject* card_object, mm_word sound_id);

/**
 * @brief Set whether the CardObject should be shown as selected in some way
 *
 * @param card_object pointer to the CardObject to select/deselect
 * @param selected    true if @p card_object should be selected, false otherwise
 */
void card_object_set_selected(CardObject* card_object, bool selected);

/**
 * @brief Get whether the CardObject should be shown as selected in some way
 *
 * @param card_object pointer to a CardObject
 */
bool card_object_is_selected(CardObject* card_object);

/**
 * @brief Get the Sprite showing the visuals of a CardObject
 *
 * @param card_object pointer to a CardObject to get the Sprite of
 * @return pointer to @p card_object 's Sprite
 */
Sprite* card_object_get_sprite(CardObject* card_object);

/**
 * @brief Get the price to pay to acquire a certain CardObject
 *
 * Matches @ref ItemFuncs.get_buy_price()
 *
 * @param card_object Pointer to the card - if NULL returns UNDEFINED
 * @return UNDEFINED if error occurred, the card's purchase value otherwise
 */
int card_object_get_buy_price(Item* card_object);

/**
 * @brief Destroy a CardObject item and free its resources in the associated memory pool.
 *
 * Matches @ref ItemFuncs.dispose()
 *
 * @param joker_object Pointer to the CardObject Item* to destroy; set to NULL.
 */
void card_object_dispose(Item** card_object);

#endif // CARD_H
