/**
 * @file sfx_pitch.c
 *
 * @brief Sound effects pitch rate limits implementation.
 */
#include "sfx_pitch.h"

int sfx_pitch_rate_clamp(int rate)
{
    if (rate < SFX_PITCH_RATE_MIN)
        return SFX_PITCH_RATE_MIN;

    if (rate > SFX_PITCH_RATE_MAX)
        return SFX_PITCH_RATE_MAX;

    return rate;
}
