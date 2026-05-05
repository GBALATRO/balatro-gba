#ifndef HAND_ANALYSIS_H
#define HAND_ANALYSIS_H

#include "card.h"

#include <tonc.h>

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
