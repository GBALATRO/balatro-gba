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

enum HandState get_hand_state(void);
void set_hand_state(enum HandState);
CardObject** get_hand_array(void);
int get_hand_top(void);
int hand_get_size(void);
enum HandType get_hand_type(void);
void set_hand(void);
ContainedHandTypes* get_contained_hands(void);
void hand_select_card(int index);
void hand_change_sort(bool to_sort_by_suit);
void hand_deselect_all_cards(void);
void swap_cards_in_hand(int idx_a, int idx_b);
void reorder_card_sprites_layers(void);
void sort_cards(void);

// Hand Contents Analysis

/**
 * @brief Outputs the distribution of ranks and suits in the hand
 * @param ranks_out output - updated such as ranks_out[rank] is the number of cards of rank in the
 *                  hand. Must be of size NUM_RANKS.
 * @param suits_out output - updated such as suits_out[suit] is the number of cards if suit in the
 *                  hand Must be of size NUM_SUITS
 */
void get_hand_distribution(u8 ranks_out[NUM_RANKS], u8 suits_out[NUM_SUITS]);

/**
 * @brief Outputs the distribution of ranks and suits in the played stack
 * @param ranks_out output - updated such as ranks_out[rank] is the number of cards of rank in the
 *                  played stack. Must be of size NUM_RANKS.
 * @param suits_out output - updated such as suits_out[suit] is the number of cards if suit in the
 *                  played stack. Must be of size NUM_SUITS
 */
void get_played_distribution(u8 ranks_out[NUM_RANKS], u8 suits_out[NUM_SUITS]);

u8 hand_contains_n_of_a_kind(u8* ranks);
bool hand_contains_two_pair(u8* ranks);
bool hand_contains_full_house(u8* ranks);
bool hand_contains_straight(u8* ranks);
bool hand_contains_flush(u8* suits);

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
