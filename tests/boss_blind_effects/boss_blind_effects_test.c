#include "boss_blind_effects.h"
#include <assert.h>
#include <stdio.h>

/* ── Helpers ──────────────────────────────────────────────────────────────── */

/* Hand type constants mirrored from game.h (kept local to avoid GBA deps). */
#define HT_NONE          0
#define HT_HIGH_CARD     1
#define HT_PAIR          2
#define HT_TWO_PAIR      3
#define HT_THREE_OAK     4
#define HT_STRAIGHT      5
#define HT_FLUSH         6

/* ── boss_blind_get_for_ante ──────────────────────────────────────────────── */

static void test_get_for_ante(void)
{
    /* Ante 1 → The Needle (index 0) */
    assert(boss_blind_get_for_ante(1) == BOSS_THE_NEEDLE);
    /* Ante 2 → The Water  (index 1) */
    assert(boss_blind_get_for_ante(2) == BOSS_THE_WATER);
    /* Ante 7 → The Ox     (index 6) */
    assert(boss_blind_get_for_ante(7) == BOSS_THE_OX);
    /* Ante 8 cycles back to The Needle */
    assert(boss_blind_get_for_ante(8) == BOSS_THE_NEEDLE);
    /* Ante 0 (clamped to 1) → The Needle */
    assert(boss_blind_get_for_ante(0) == BOSS_THE_NEEDLE);

    printf("test_get_for_ante: PASS\n");
}

/* ── boss_blind_apply_round_start ─────────────────────────────────────────── */

static void test_apply_round_start_needle(void)
{
    int hands = 4, discards = 4, hand_size = 8;
    boss_blind_apply_round_start(BOSS_THE_NEEDLE, &hands, &discards, &hand_size);
    assert(hands     == 1);  /* The Needle caps hands to 1 */
    assert(discards  == 4);  /* Discards untouched          */
    assert(hand_size == 8);  /* Hand size untouched          */
    printf("test_apply_round_start_needle: PASS\n");
}

static void test_apply_round_start_water(void)
{
    int hands = 4, discards = 4, hand_size = 8;
    boss_blind_apply_round_start(BOSS_THE_WATER, &hands, &discards, &hand_size);
    assert(hands     == 4);  /* Hands untouched              */
    assert(discards  == 0);  /* The Water removes discards   */
    assert(hand_size == 8);  /* Hand size untouched          */
    printf("test_apply_round_start_water: PASS\n");
}

static void test_apply_round_start_no_effect(void)
{
    int hands = 4, discards = 4, hand_size = 8;
    /* Blinds with no round-start stat change should leave everything alone. */
    boss_blind_apply_round_start(BOSS_THE_HOOK,    &hands, &discards, &hand_size);
    boss_blind_apply_round_start(BOSS_THE_PSYCHIC, &hands, &discards, &hand_size);
    boss_blind_apply_round_start(BOSS_THE_TOOTH,   &hands, &discards, &hand_size);
    boss_blind_apply_round_start(BOSS_THE_EYE,     &hands, &discards, &hand_size);
    boss_blind_apply_round_start(BOSS_THE_OX,      &hands, &discards, &hand_size);
    assert(hands == 4 && discards == 4 && hand_size == 8);
    printf("test_apply_round_start_no_effect: PASS\n");
}

/* ── boss_blind_validate_play ─────────────────────────────────────────────── */

static void test_validate_play_psychic(void)
{
    /* The Psychic: must play exactly 5 cards. */
    assert(!boss_blind_validate_play(BOSS_THE_PSYCHIC, 4, HT_STRAIGHT));
    assert( boss_blind_validate_play(BOSS_THE_PSYCHIC, 5, HT_STRAIGHT));
    assert(!boss_blind_validate_play(BOSS_THE_PSYCHIC, 6, HT_FLUSH));
    printf("test_validate_play_psychic: PASS\n");
}

static void test_validate_play_eye(void)
{
    boss_blind_reset();

    /* First time playing HIGH_CARD – allowed. */
    assert(boss_blind_validate_play(BOSS_THE_EYE, 1, HT_HIGH_CARD));
    boss_blind_register_hand(BOSS_THE_EYE, HT_HIGH_CARD);

    /* Second time playing HIGH_CARD – rejected. */
    assert(!boss_blind_validate_play(BOSS_THE_EYE, 1, HT_HIGH_CARD));

    /* Different type (PAIR) – still allowed. */
    assert(boss_blind_validate_play(BOSS_THE_EYE, 2, HT_PAIR));

    /* After reset, HIGH_CARD is allowed again. */
    boss_blind_reset();
    assert(boss_blind_validate_play(BOSS_THE_EYE, 1, HT_HIGH_CARD));

    printf("test_validate_play_eye: PASS\n");
}

static void test_validate_play_others_always_true(void)
{
    /* Blinds other than The Psychic and The Eye never block play. */
    assert(boss_blind_validate_play(BOSS_THE_NEEDLE,  3, HT_FLUSH));
    assert(boss_blind_validate_play(BOSS_THE_WATER,   3, HT_FLUSH));
    assert(boss_blind_validate_play(BOSS_THE_HOOK,    3, HT_FLUSH));
    assert(boss_blind_validate_play(BOSS_THE_TOOTH,   3, HT_FLUSH));
    assert(boss_blind_validate_play(BOSS_THE_OX,      3, HT_FLUSH));
    printf("test_validate_play_others_always_true: PASS\n");
}

/* ── boss_blind_get_hook_count ─────────────────────────────────────────────── */

static void test_hook_count(void)
{
    assert(boss_blind_get_hook_count(BOSS_THE_HOOK)    == 2);
    assert(boss_blind_get_hook_count(BOSS_THE_NEEDLE)  == 0);
    assert(boss_blind_get_hook_count(BOSS_THE_WATER)   == 0);
    assert(boss_blind_get_hook_count(BOSS_THE_PSYCHIC) == 0);
    assert(boss_blind_get_hook_count(BOSS_THE_TOOTH)   == 0);
    assert(boss_blind_get_hook_count(BOSS_THE_EYE)     == 0);
    assert(boss_blind_get_hook_count(BOSS_THE_OX)      == 0);
    printf("test_hook_count: PASS\n");
}

/* ── boss_blind_get_tooth_penalty ─────────────────────────────────────────── */

static void test_tooth_penalty(void)
{
    /* The Tooth: $1 per card played. */
    assert(boss_blind_get_tooth_penalty(BOSS_THE_TOOTH, 1) == 1);
    assert(boss_blind_get_tooth_penalty(BOSS_THE_TOOTH, 5) == 5);
    assert(boss_blind_get_tooth_penalty(BOSS_THE_TOOTH, 0) == 0);

    /* All other blinds: no penalty. */
    assert(boss_blind_get_tooth_penalty(BOSS_THE_NEEDLE,  3) == 0);
    assert(boss_blind_get_tooth_penalty(BOSS_THE_WATER,   3) == 0);
    assert(boss_blind_get_tooth_penalty(BOSS_THE_HOOK,    3) == 0);
    assert(boss_blind_get_tooth_penalty(BOSS_THE_PSYCHIC, 3) == 0);
    assert(boss_blind_get_tooth_penalty(BOSS_THE_EYE,     3) == 0);
    assert(boss_blind_get_tooth_penalty(BOSS_THE_OX,      3) == 0);

    printf("test_tooth_penalty: PASS\n");
}

/* ── boss_blind_is_ox_active ──────────────────────────────────────────────── */

static void test_ox_active(void)
{
    assert( boss_blind_is_ox_active(BOSS_THE_OX));
    assert(!boss_blind_is_ox_active(BOSS_THE_NEEDLE));
    assert(!boss_blind_is_ox_active(BOSS_THE_WATER));
    assert(!boss_blind_is_ox_active(BOSS_THE_HOOK));
    assert(!boss_blind_is_ox_active(BOSS_THE_PSYCHIC));
    assert(!boss_blind_is_ox_active(BOSS_THE_TOOTH));
    assert(!boss_blind_is_ox_active(BOSS_THE_EYE));
    printf("test_ox_active: PASS\n");
}

/* ── boss_blind_get_name ──────────────────────────────────────────────────── */

static void test_get_name(void)
{
    /* Spot-check a few names (exact strings must match boss_blind_effects.c). */
    assert(boss_blind_get_name(BOSS_THE_NEEDLE)  != 0);
    assert(boss_blind_get_name(BOSS_THE_OX)      != 0);
    /* Out-of-range ID must not crash and must return a non-NULL string. */
    assert(boss_blind_get_name((enum BossBlindId)-1) != 0);
    assert(boss_blind_get_name(BOSS_BLIND_ID_MAX)    != 0);
    printf("test_get_name: PASS\n");
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    test_get_for_ante();
    test_apply_round_start_needle();
    test_apply_round_start_water();
    test_apply_round_start_no_effect();
    test_validate_play_psychic();
    test_validate_play_eye();
    test_validate_play_others_always_true();
    test_hook_count();
    test_tooth_penalty();
    test_ox_active();
    test_get_name();

    printf("\nAll boss_blind_effects tests passed.\n");
    return 0;
}
