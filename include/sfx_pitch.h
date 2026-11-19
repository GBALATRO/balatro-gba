/**
 * @file sfx_pitch.h
 *
 * @brief Keeps the pitch rate of sound effects within what the sound mixer can handle.
 *
 * Only arithmetic lives here so it can be unit tested on the host.
 */
#ifndef SFX_PITCH_H
#define SFX_PITCH_H

/**
 * @brief Lowest pitch rate a sound effect can be played at, a quarter of the normal speed.
 */
#define SFX_PITCH_RATE_MIN 256

/**
 * @brief Highest pitch rate a sound effect can be played at, 4 times the normal speed.
 */
#define SFX_PITCH_RATE_MAX 4096

/**
 * @brief Bring a pitch rate back into the range the sound mixer can play.
 *
 * WHY: maxmod stores the rate in 16 bits and its mixer overflows on very fast playback. A rate of
 * 0, a negative one (which wraps to a huge value) or a huge one makes the mixer write far past
 * its buffer, which overwrites the game's memory and freezes the console.
 *
 * @param rate the wanted pitch rate, 1024 being the normal speed
 * @return @p rate if it is playable, the closest playable rate otherwise
 */
int sfx_pitch_rate_clamp(int rate);

#endif // SFX_PITCH_H
