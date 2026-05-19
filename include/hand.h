/**
 * @file hand.h
 *
 * @brief Functions relative to manipulating and analyzing the contents of the Hand,
 *         a.k.a. the Cards we hold and play.
 */
#ifndef HAND_ANALYSIS_H
#define HAND_ANALYSIS_H

#include "card.h"

#include <tonc.h>

#define MAX_HAND_SIZE 16

enum HandState
{
    HAND_DRAW,
    HAND_SELECT,
    // This is actually a misnomer because it's used for the deck
    // but it mechanically makes sense to be a state of the hand
    HAND_SHUFFLING,
    HAND_DISCARD,
    HAND_PLAY,
    HAND_PLAYING
};

enum HandType
{
    NONE,
    HIGH_CARD,
    PAIR,
    TWO_PAIR,
    THREE_OF_A_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HOUSE,
    FOUR_OF_A_KIND,
    STRAIGHT_FLUSH,
    ROYAL_FLUSH,
    FIVE_OF_A_KIND,
    FLUSH_HOUSE,
    FLUSH_FIVE
};

// clang-format off
// Store all contained hands to optimize "whole hand condition" Jokers
typedef struct ContainedHandTypes
{
    union
    {
        struct
        {
            u16 HIGH_CARD : 1;
            u16 PAIR : 1;
            u16 TWO_PAIR : 1;
            u16 THREE_OF_A_KIND : 1;
            u16 STRAIGHT : 1;
            u16 FLUSH : 1;
            u16 FULL_HOUSE : 1;
            u16 FOUR_OF_A_KIND : 1;
            u16 STRAIGHT_FLUSH : 1;
            u16 ROYAL_FLUSH : 1;
            u16 FIVE_OF_A_KIND : 1;
            u16 FLUSH_HOUSE : 1;
            u16 FLUSH_FIVE : 1;
            u16 : 3;
        };
        u16 value;
    };
} ContainedHandTypes;
// clang-format on

// Hand Structure Manipulation

/**
 * @brief Set the hand state. Primarily used by the GAME_PLAYING game state.
 *
 * @sa HandState
 */
void set_hand_state(enum HandState);

/**
 * @brief Get the hand state
 *
 * @return enum HandState
 *
 * @sa set_hand_state
 */
enum HandState get_hand_state(void);

/**
 * @brief Determine the HandType and ContainedHandTypes of the currently selected Cards,
 *         then print the Hand's name, chips, and mult on screen.
 */
void compute_hand_value_info(void);

/**
 * @brief Get the current hand type
 *
 * @return enum HandType
 *
 * @sa compute_hand_value_info
 */
enum HandType get_hand_type(void);

/**
 * @brief Get the contained hands within the selected hand
 *
 * @return ContainedHandTypes*
 *
 * @sa compute_hand_value_info
 */
ContainedHandTypes* get_contained_hands(void);

/**
 * @brief Get the hand array of Cards currently held in hand
 *
 * @return CardObject**
 */
CardObject** get_hand_array(void);

/**
 * @brief Get the position in hand array of the last card obtained
 *
 * @return int
 *
 * @sa get_hand_array
 */
int get_hand_top(void);

/**
 * @brief Get the current number of Cards in hand.
 *
 * @return `hand_top + 1`
 *
 * @sa get_hand_top
 */
int hand_nb_held_cards(void);

/**
 * @brief Set the card at a given index in Hand as selected
 *
 * @param index Index of card to select in hand
 */
void hand_select_card(int index);

/**
 * @brief Switch to the given sort method. Can sort playing cards in two ways: by rank and suit.
 *         The order of suits is as follows:
 * ```
 * SPADES > HEARTS > CLUBS > DIAMONDS
 * ```
 *
 * @param to_sort_by_suit
 */
void hand_change_sort(bool to_sort_by_suit);

/**
 * @brief Deselect all cards in hand.
 */
void hand_deselect_all_cards(void);

/**
 * @brief Swaps the order of two cards in hand.
 *
 * @param idx_a index of the first card
 * @param idx_b index of the second card
 */
void swap_cards_in_hand(int idx_a, int idx_b);

/**
 * @brief Destroy the sprites of the Cards held in hand and recreate then in the same
 *         order as the Cards in the hand array. This allows Cards to render properly
 *         when help in hand during the round or a Tarot/Spectral booster pack.
 */
void reorder_card_sprites_layers(void);

/**
 * @brief Sort the hand array according to the selected method to do that.
 *
 * @sa hand_change_sort
 */
void sort_cards(void);

// Hand Contents Analysis

/**
 * Finds the largest flush (set of cards with the same suit) in the given array of played cards.
 * Marks the cards belonging to the best flush in the out_selection array.
 *
 * @param played        Array of pointers to CardObject representing played cards.
 * @param top           Index of the top of the played stack.
 * @param min_len       Minimum number of cards required for a flush.
 * @param out_selection Output array of bools; set to true for cards in the best flush, false
 * otherwise.
 * @return              The number of cards in the best flush found, or 0 if no flush meets min_len.
 */
int find_flush_in_played_cards(CardObject** played, int top, int min_len, bool* out_selection);
int find_straight_in_played_cards(
    CardObject** played,
    int top,
    bool shortcut_active,
    int min_len,
    bool* out_selection
);
void select_paired_cards_in_hand(CardObject** played, int top, bool* selection);

#endif
