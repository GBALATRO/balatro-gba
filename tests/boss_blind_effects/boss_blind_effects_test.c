#include boss_blind_effects.h

#include <assert.h>
#include <stdio.h>

#define HT_NONE      0
#define HT_HIGH_CARD 1
#define HT_PAIR      2
#define HT_TWO_PAIR  3
#define HT_THREE_OAK 4
#define HT_STRAIGHT  5
#define HT_FLUSH     6

static void test_apply_round_start_needle(void)
{
    int hands = 4, discards = 4, hand_size = 8;
    boss_blind_apply_round_start(BLIND_TYPE_NEEDLE, &hands, &discards, &hand_size);
    assert(hands == 1 && discards == 4 && hand_size == 8);
    printf(test_apply_round_start_needle: PASS\n);
}

static void test_apply_round_start_water(void)
{
    int hands = 4, discards = 4, hand_size = 8;
    boss_blind_apply_round_start(BLIND_TYPE_WATER, &hands, &discards, &hand_size);
    assert(hands == 4 && discards == 0 && hand_size == 8);
    printf(test_apply_round_start_water: PASS\n);
}

static void test_apply_round_start_no_effect(void)
{
    int hands = 4, discards = 4, hand_size = 8;
    boss_blind_apply_round_start(BLIND_TYPE_HOOK,    &hands, &discards, &hand_size);
    boss_blind_apply_round_start(BLIND_TYPE_PSYCHIC, &hands, &discards, &hand_size);
    boss_blind_apply_round_start(BLIND_TYPE_TOOTH,   &hands, &discards, &hand_size);
    boss_blind_apply_round_start(BLIND_TYPE_EYE,     &hands, &discards, &hand_size);
    boss_blind_apply_round_start(BLIND_TYPE_OX,      &hands, &discards, &hand_size);
    assert(hands == 4 && discards == 4 && hand_size == 8);
    printf(test_apply_round_start_no_effect: PASS\n);
}

static void test_validate_play_psychic(void)
{
    assert(!boss_blind_validate_play(BLIND_TYPE_PSYCHIC, 4, HT_STRAIGHT));
    assert( boss_blind_validate_play(BLIND_TYPE_PSYCHIC, 5, HT_STRAIGHT));
    assert(!boss_blind_validate_play(BLIND_TYPE_PSYCHIC, 6, HT_FLUSH));
    printf(test_validate_play_psychic: PASS\n);
}

static void test_validate_play_eye(void)
{
    boss_blind_reset();
    assert( boss_blind_validate_play(BLIND_TYPE_EYE, 1, HT_HIGH_CARD));
    boss_blind_register_hand(BLIND_TYPE_EYE, HT_HIGH_CARD);
    assert(!boss_blind_validate_play(BLIND_TYPE_EYE, 1, HT_HIGH_CARD));
    assert( boss_blind_validate_play(BLIND_TYPE_EYE, 2, HT_PAIR));
    boss_blind_reset();
    assert( boss_blind_validate_play(BLIND_TYPE_EYE, 1, HT_HIGH_CARD));
    printf(test_validate_play_eye: PASS\n);
}

static void test_validate_play_others_always_true(void)
{
    assert(boss_blind_validate_play(BLIND_TYPE_NEEDLE, 3, HT_FLUSH));
    assert(boss_blind_validate_play(BLIND_TYPE_WATER,  3, HT_FLUSH));
    assert(boss_blind_validate_play(BLIND_TYPE_HOOK,   3, HT_FLUSH));
    assert(boss_blind_validate_play(BLIND_TYPE_TOOTH,  3, HT_FLUSH));
    assert(boss_blind_validate_play(BLIND_TYPE_OX,     3, HT_FLUSH));
    printf(test_validate_play_others_always_true: PASS\n);
}

static void test_hook_count(void)
{
    assert(boss_blind_get_hook_count(BLIND_TYPE_HOOK)    == 2);
    assert(boss_blind_get_hook_count(BLIND_TYPE_NEEDLE)  == 0);
    assert(boss_blind_get_hook_count(BLIND_TYPE_WATER)   == 0);
    assert(boss_blind_get_hook_count(BLIND_TYPE_PSYCHIC) == 0);
    assert(boss_blind_get_hook_count(BLIND_TYPE_TOOTH)   == 0);
    assert(boss_blind_get_hook_count(BLIND_TYPE_EYE)     == 0);
    assert(boss_blind_get_hook_count(BLIND_TYPE_OX)      == 0);
    printf(test_hook_count: PASS\n);
}

static void test_tooth_penalty(void)
{
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_TOOTH,   1) == 1);
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_TOOTH,   5) == 5);
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_TOOTH,   0) == 0);
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_NEEDLE,  3) == 0);
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_WATER,   3) == 0);
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_HOOK,    3) == 0);
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_PSYCHIC, 3) == 0);
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_EYE,     3) == 0);
    assert(boss_blind_get_tooth_penalty(BLIND_TYPE_OX,      3) == 0);
    printf(test_tooth_penalty: PASS\n);
}

static void test_ox_active(void)
{
    assert( boss_blind_is_ox_active(BLIND_TYPE_OX));
    assert(!boss_blind_is_ox_active(BLIND_TYPE_NEEDLE));
    assert(!boss_blind_is_ox_active(BLIND_TYPE_WATER));
    assert(!boss_blind_is_ox_active(BLIND_TYPE_HOOK));
    assert(!boss_blind_is_ox_active(BLIND_TYPE_PSYCHIC));
    assert(!boss_blind_is_ox_active(BLIND_TYPE_TOOTH));
    assert(!boss_blind_is_ox_active(BLIND_TYPE_EYE));
    printf(test_ox_active: PASS\n);
}

int main(void)
{
    test_apply_round_start_needle();
    test_apply_round_start_water();
    test_apply_round_start_no_effect();
    test_validate_play_psychic();
    test_validate_play_eye();
    test_validate_play_others_always_true();
    test_hook_count();
    test_tooth_penalty();
    test_ox_active();

    printf(\nAll boss_blind_effects tests passed.\n);
    return 0;
}