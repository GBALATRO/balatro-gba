#include <assert.h>
#include <sfx_pitch.h>
#include <stdio.h>

// Same values as in the game
#define BASE_PITCH_RATE         1024
#define PITCH_STEP_DISCARD_SFX  (-64)
#define PITCH_STEP_DRAW_SFX     24
#define MAX_HAND_SIZE           16
#define SOUND_LIB_RATE_MAX      0xFFFF

void test_playable_rates_are_kept(void)
{
    assert(sfx_pitch_rate_clamp(BASE_PITCH_RATE) == BASE_PITCH_RATE);
    assert(sfx_pitch_rate_clamp(BASE_PITCH_RATE / 2) == BASE_PITCH_RATE / 2);
    assert(sfx_pitch_rate_clamp(2 * BASE_PITCH_RATE) == 2 * BASE_PITCH_RATE);
    assert(sfx_pitch_rate_clamp(SFX_PITCH_RATE_MIN) == SFX_PITCH_RATE_MIN);
    assert(sfx_pitch_rate_clamp(SFX_PITCH_RATE_MAX) == SFX_PITCH_RATE_MAX);

    // Drawing a full hand raises the pitch a little with each card
    int full_hand_draw_rate = BASE_PITCH_RATE + MAX_HAND_SIZE * PITCH_STEP_DRAW_SFX;
    assert(sfx_pitch_rate_clamp(full_hand_draw_rate) == full_hand_draw_rate);
}

void test_unplayable_rates_are_brought_back(void)
{
    assert(sfx_pitch_rate_clamp(0) == SFX_PITCH_RATE_MIN);
    assert(sfx_pitch_rate_clamp(-1) == SFX_PITCH_RATE_MIN);
    assert(sfx_pitch_rate_clamp(SFX_PITCH_RATE_MIN - 1) == SFX_PITCH_RATE_MIN);
    assert(sfx_pitch_rate_clamp(SFX_PITCH_RATE_MAX + 1) == SFX_PITCH_RATE_MAX);
    assert(sfx_pitch_rate_clamp(SOUND_LIB_RATE_MAX) == SFX_PITCH_RATE_MAX);
}

void test_discard_pitch_never_leaves_the_playable_range(void)
{
    // Regression: 33 cards shuffled away without using a Discard froze the game, the rate
    // 1024 - 64 * 33 = -1088 reached the sound mixer as 64448
    int crash_rate = BASE_PITCH_RATE + 33 * PITCH_STEP_DISCARD_SFX;
    assert(crash_rate == -1088);
    assert(sfx_pitch_rate_clamp(crash_rate) == SFX_PITCH_RATE_MIN);

    for (int cards_discarded = 0; cards_discarded <= 1000; cards_discarded++)
    {
        int rate = sfx_pitch_rate_clamp(BASE_PITCH_RATE + cards_discarded * PITCH_STEP_DISCARD_SFX);
        assert(rate >= SFX_PITCH_RATE_MIN);
        assert(rate <= SFX_PITCH_RATE_MAX);
    }
}

int main()
{
    printf("Testing SFX pitch rate limits.\n");
    test_playable_rates_are_kept();
    test_unplayable_rates_are_brought_back();
    test_discard_pitch_never_leaves_the_playable_range();

    printf("-------------------------------------------------------------------------------\n");
    printf("SFX Pitch Tests Passed :)\n");
    printf("-------------------------------------------------------------------------------\n");

    return 0;
}
