/**
 * @file boss_blind_effects.h
 *
 * @brief Boss blind effect definitions and API.
 *
 * The active boss blind is identified by the game''s current_blind variable
 * (enum BlindType from blind.h). All functions in this module take a
 * BlindType directly; callers pass current_blind instead of calling the
 * removed boss_blind_get_for_ante() helper.
 *
 * Implemented boss blinds:
 *   The Needle  : Only 1 hand allowed per round.
 *   The Water   : Start the round with 0 discards.
 *   The Hook    : After each hand scored, 2 held cards are discarded.
 *   The Psychic : Must play exactly 5 cards.
 *   The Tooth   : Lose $1 for each card played.
 *   The Eye     : Each hand type can only be played once per round.
 *   The Ox      : Playing any hand sets money to $0.
 */

#ifndef BOSS_BLIND_EFFECTS_H
#define BOSS_BLIND_EFFECTS_H

#include <stdbool.h>

#include `"blind.h`"

/** Maximum number of distinct hand types (matches enum HandType in game.h). */
#define BOSS_BLIND_MAX_HAND_TYPES 14

void boss_blind_apply_round_start(enum BlindType id, int* hands, int* discards, int* hand_size);
bool boss_blind_validate_play(enum BlindType id, int hand_selections, int hand_type);
int boss_blind_get_hook_count(enum BlindType id);
int boss_blind_get_tooth_penalty(enum BlindType id, int cards_played);
bool boss_blind_is_ox_active(enum BlindType id);
void boss_blind_register_hand(enum BlindType id, int hand_type);
void boss_blind_reset(void);

#endif // BOSS_BLIND_EFFECTS_H