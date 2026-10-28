/**
 * @file blind_select.h
 *
 * @brief Blind select state functions
 */
#ifndef GAME_BLIND_SELECT_H
#define GAME_BLIND_SELECT_H

#include "blind.h"
#include "skip_tag.h"

#define NB_SKIPPABLE_BLINDS 2

/**
 * @brief Reroll the Boss Blind from inside the menu.
 *         This function is made to be used for both Vouchers and the Boss Skip Tag
 */
void blind_select_reroll_boss_from_menu(void);

/**
 * @brief Get the type of one of the Skip Tags obtainable by skipping Blinds
 *
 * @param blind_tag index of the Skip Tag to get the type of, either BLIND_TYPE_SMALL or
 *                  BLIND_TYPE_BIG
 *
 * @return type of the requested tag, or UNDEFINED if it doesn't exist
 */
enum SkipTagType get_blind_skip_tag_type(int blind_tag_idx);

/**
 * @brief Set the Skip Tag for a certain skippable Blind
 *
 * @param blind_tag_idx index of the Skip Tag to set the type of, either BLIND_TYPE_SMALL or
 *                      BLIND_TYPE_BIG
 * @param tag_type type of the Skip Tag to add
 */
void set_blind_skip_tag(int blind_tag_idx, enum SkipTagType tag_type);

/**
 * @brief Change to the blind select background
 */
void blind_select_change_background(void);

/**
 * @brief Blind select state initialization
 */
void blind_select_on_init(void);

/**
 * @brief Blind select state update
 */
void blind_select_on_update(void);

/**
 * @brief Blind select cleanup
 */
void blind_select_on_exit(void);

void increment_blind(enum BlindState increment_reason);

#endif // GAME_BLIND_SELECT_H
