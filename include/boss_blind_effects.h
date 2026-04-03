/**
 * @file boss_blind_effects.h
 *
 * @brief Boss blind effect definitions and API.
 *
 * Each ante (1–7) has a distinct boss blind with a unique gameplay modifier.
 * Antes beyond 7 cycle back to the first boss blind (The Needle).
 *
 * Implemented boss blinds:
 *   Ante 1 – The Needle  : Only 1 hand allowed per round.
 *   Ante 2 – The Water   : Start the round with 0 discards.
 *   Ante 3 – The Hook    : After each hand scored, 2 held cards are discarded.
 *   Ante 4 – The Psychic : Must play exactly 5 cards.
 *   Ante 5 – The Tooth   : Lose $1 for each card played.
 *   Ante 6 – The Eye     : Each hand type can only be played once per round.
 *   Ante 7 – The Ox      : Playing any hand sets money to $0.
 */

#ifndef BOSS_BLIND_EFFECTS_H
#define BOSS_BLIND_EFFECTS_H

#include <stdbool.h>

/** Maximum number of distinct hand types (matches enum HandType in game.h). */
#define BOSS_BLIND_MAX_HAND_TYPES 14

/**
 * @brief Identifies which boss blind effect is active.
 * Assigned deterministically: boss_blind_get_for_ante(ante) returns
 * (ante - 1) % BOSS_BLIND_ID_MAX.
 */
enum BossBlindId
{
    BOSS_THE_NEEDLE = 0,  /**< Only 1 hand allowed.                          */
    BOSS_THE_WATER = 1,   /**< Start with 0 discards.                        */
    BOSS_THE_HOOK = 2,    /**< Discard 2 held cards after each hand scored.  */
    BOSS_THE_PSYCHIC = 3, /**< Must play exactly 5 cards.                    */
    BOSS_THE_TOOTH = 4,   /**< Lose $1 per card played.                      */
    BOSS_THE_EYE = 5,     /**< Each hand type playable only once per round.  */
    BOSS_THE_OX = 6,      /**< Playing any hand sets money to $0.            */
    BOSS_BLIND_ID_MAX     /**< Sentinel – do not use as a boss blind ID.     */
};

/**
 * @brief Returns the boss blind active for the given ante (1-based).
 *        Cycles when ante exceeds BOSS_BLIND_ID_MAX.
 *
 * @param ante  Current ante number (>= 1).
 * @return      Active boss blind ID.
 */
enum BossBlindId boss_blind_get_for_ante(int ante);

/**
 * @brief Applies round-start stat modifications for the active boss blind.
 *        Call once at the start of a boss blind round, before displaying
 *        hands/discards counts.
 *
 * @param id        Active boss blind ID.
 * @param hands     Pointer to the current hands-remaining counter.
 * @param discards  Pointer to the current discards-remaining counter.
 * @param hand_size Pointer to the current hand size.
 */
void boss_blind_apply_round_start(enum BossBlindId id, int* hands, int* discards, int* hand_size);

/**
 * @brief Validates whether playing the current selection is allowed.
 *        Returns false if the boss blind forbids the action.
 *
 * @param id              Active boss blind ID.
 * @param hand_selections Number of currently selected cards.
 * @param hand_type       Current hand type (cast from enum HandType).
 * @return                true if the play is allowed.
 */
bool boss_blind_validate_play(enum BossBlindId id, int hand_selections, int hand_type);

/**
 * @brief Returns how many random held cards should be discarded after scoring.
 *        Non-zero only for The Hook (returns 2).
 *
 * @param id  Active boss blind ID.
 * @return    Number of cards to discard (0 or 2).
 */
int boss_blind_get_hook_count(enum BossBlindId id);

/**
 * @brief Returns the money penalty after playing a hand.
 *        Non-zero only for The Tooth ($1 per card played).
 *
 * @param id           Active boss blind ID.
 * @param cards_played Number of cards played this hand.
 * @return             Money to deduct (>= 0).
 */
int boss_blind_get_tooth_penalty(enum BossBlindId id, int cards_played);

/**
 * @brief Returns true if playing any hand should drain money to $0 (The Ox).
 *
 * @param id  Active boss blind ID.
 * @return    true for The Ox, false otherwise.
 */
bool boss_blind_is_ox_active(enum BossBlindId id);

/**
 * @brief Records that a hand type has been played this round.
 *        Used by The Eye to track which types are exhausted.
 *
 * @param id         Active boss blind ID.
 * @param hand_type  Hand type that was just played (cast from enum HandType).
 */
void boss_blind_register_hand(enum BossBlindId id, int hand_type);

/**
 * @brief Resets all per-round boss blind state (e.g. The Eye's played-types
 *        bitfield). Call at the start of each boss blind round.
 */
void boss_blind_reset(void);

/**
 * @brief Returns the display name for a boss blind.
 *
 * @param id  Boss blind ID.
 * @return    Null-terminated display string, never NULL.
 */
const char* boss_blind_get_name(enum BossBlindId id);

#endif // BOSS_BLIND_EFFECTS_H
