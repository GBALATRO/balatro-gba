/**
 * @file deck_types.h
 *
 * @brief Functions related to the different types of Decks.
 */
#ifndef DECK_TYPES_H
#define DECK_TYPES_H

#include "graphic_utils.h"

#define DECK_NAME_LENGTH 14 // 13 chars + '\0'
#define DECK_DESC_WIDTH  DECK_NAME_LENGTH
#define DECK_DESC_HEIGHT 5

enum DeckType
{
    DECK_TYPE_RED,
    DECK_TYPE_BLUE,
    DECK_TYPE_YELLOW,
    DECK_TYPE_GREEN,
    DECK_TYPE_BLACK,
    DECK_TYPE_PAINTED,
    DECK_TYPE_MAX
};

/**
 * @brief Gameplay rules a Deck applies to a run.
 */
typedef struct DeckRules
{
    s8 hands;                       // Hands available every Round
    s8 discards;                    // Discards available every Round
    s8 hand_size;                   // Cards held in hand
    s8 joker_slots;                 // Jokers the player can own
    s8 starting_money;              // Money at the start of the run
    s8 money_per_remaining_hand;    // End of Round reward
    s8 money_per_remaining_discard; // End of Round reward
    bool earns_interest;            // Whether held money pays interest at the End of Round
} DeckRules;

/**
 * @brief Get the gameplay rules of a Deck.
 *
 * @param deck DeckType we want the rules of.
 * @return the rules of @p deck, or the Red Deck's if @p deck is not a valid DeckType
 */
const DeckRules* deck_get_rules(enum DeckType deck);

/**
 * @brief Prints the Deck name string to the screen at the given position.
 *
 * @param deck Decktype we want the name of.
 * @param pos screen position in pixels to print the name at.
 */
void print_deck_name(enum DeckType deck, BG_POINT pos);

/**
 * @brief Prints the Deck description string to the screen at the given position.
 *
 * @param deck DeckType we want the description of.
 * @param pos screen position in pixels of the top-left corner of the rectangle
 *             we'll print the Deck description into
 */
void print_deck_description(enum DeckType deck, BG_POINT pos);

#endif // DECK_TYPES_H
