/**
 * @file round.h
 * @brief API relative to the Rounds we play
 */

#ifndef GAME_ROUND_H
#define GAME_ROUND_H

#include <stdbool.h>

/**
 * @brief Checks whether the score that would result from the current Chips and Mult exceeds the
 *         current Blind's score requirement, and applies the flaming effect if needed.
 */
void toggle_flaming_score(void);

/**
 * @brief Get the index of the last played card
 *
 * @return int
 */
int get_played_top(void);

/**
 * @brief Get the number of cards played
 *
 * @return int
 */
int get_played_size(void);

/**
 * @brief Get the index of the last discarded card
 *
 * @return int
 */
int get_discard_top(void);

/**
 * @brief Get the index of the card that is currently being scored
 *
 * @return int
 */
int get_scored_card_index(void);

/**
 * @brief Set whether the current card should get triggered again
 *
 * @param new_retrigger true if the card should trigger again, false if not
 */
void set_retrigger(bool new_retrigger);

/**
 * @brief Change to the round card selection background
 */
void game_round_change_background_selecting(void);

/**
 * @brief Change to the round card playing background
 */
void game_round_change_background_playing(void);

/**
 * @brief Round state initialization
 */
void game_round_on_init(void);

/**
 * @brief Round state update
 */
void game_round_on_update(void);

#endif // GAME_ROUND_H
