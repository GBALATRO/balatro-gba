/**
 * @file boss_blind_effects.c
 *
 * @brief Boss blind effect implementations.
 *
 * See boss_blind_effects.h for full documentation.
 */

#include "boss_blind_effects.h"

/* ── Internal state ────────────────────────────────────────────────────── */

/**
 * Tracks which hand types have been played this round (for The Eye).
 * Indexed by int hand_type value (0 = NONE, 1 = HIGH_CARD, …).
 * Reset by boss_blind_reset().
 */
static bool s_eye_played_types[BOSS_BLIND_MAX_HAND_TYPES];

/* ── Name table ─────────────────────────────────────────────────────────── */

static const char* const s_boss_names[BOSS_BLIND_ID_MAX] = {
    "The Needle",  /* BOSS_THE_NEEDLE  */
    "The Water",   /* BOSS_THE_WATER   */
    "The Hook",    /* BOSS_THE_HOOK    */
    "The Psychic", /* BOSS_THE_PSYCHIC */
    "The Tooth",   /* BOSS_THE_TOOTH   */
    "The Eye",     /* BOSS_THE_EYE     */
    "The Ox",      /* BOSS_THE_OX      */
};

/* ── Public API ─────────────────────────────────────────────────────────── */

enum BossBlindId boss_blind_get_for_ante(int ante)
{
    if (ante < 1)
        ante = 1;
    return (enum BossBlindId)((ante - 1) % (int)BOSS_BLIND_ID_MAX);
}

void boss_blind_apply_round_start(
    enum BossBlindId id,
    int*             hands,
    int*             discards,
    int*             hand_size
)
{
    (void)hand_size; /* reserved for future use (e.g. The Manacle) */

    switch (id)
    {
        case BOSS_THE_NEEDLE:
            /* Only 1 hand allowed – overwrite however many were reset. */
            *hands = 1;
            break;

        case BOSS_THE_WATER:
            /* No discards available this round. */
            *discards = 0;
            break;

        default:
            /* Other boss blinds have no round-start stat modification. */
            break;
    }
}

bool boss_blind_validate_play(
    enum BossBlindId id,
    int              hand_selections,
    int              hand_type
)
{
    switch (id)
    {
        case BOSS_THE_PSYCHIC:
            /* Must select exactly 5 cards before playing. */
            return hand_selections == 5;

        case BOSS_THE_EYE:
            /* Reject if this hand type has already been played this round. */
            if (hand_type >= 0 && hand_type < BOSS_BLIND_MAX_HAND_TYPES)
                return !s_eye_played_types[hand_type];
            return true;

        default:
            return true;
    }
}

int boss_blind_get_hook_count(enum BossBlindId id)
{
    return (id == BOSS_THE_HOOK) ? 2 : 0;
}

int boss_blind_get_tooth_penalty(enum BossBlindId id, int cards_played)
{
    if (id == BOSS_THE_TOOTH && cards_played > 0)
        return cards_played; /* $1 per card played */
    return 0;
}

bool boss_blind_is_ox_active(enum BossBlindId id)
{
    return id == BOSS_THE_OX;
}

void boss_blind_register_hand(enum BossBlindId id, int hand_type)
{
    if (id == BOSS_THE_EYE && hand_type >= 0 && hand_type < BOSS_BLIND_MAX_HAND_TYPES)
        s_eye_played_types[hand_type] = true;
}

void boss_blind_reset(void)
{
    for (int i = 0; i < BOSS_BLIND_MAX_HAND_TYPES; i++)
        s_eye_played_types[i] = false;
}

const char* boss_blind_get_name(enum BossBlindId id)
{
    if (id < 0 || id >= BOSS_BLIND_ID_MAX)
        return "Boss Blind";
    return s_boss_names[id];
}
