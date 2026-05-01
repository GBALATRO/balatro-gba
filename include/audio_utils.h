/**
 * @file audio_utils.h
 *
 * @brief Utilities for using maxmod to play sound effects
 */
#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <mm_types.h>

/**
 * @def MM_MODULE_FULL_VOLUME
 * @brief The maximum volume for maxmod module volume (main theme)
 */
#define MM_MODULE_FULL_VOLUME 1024

/**
 * @def MM_SFX_FULL_VOLUME
 * @brief The maximum volume for maxmod mm_sound_effect.volume
 */
#define MM_SFX_FULL_VOLUME 255

/**
 * @def MM_PAN_CENTER
 * @brief The center pan of stereo audio for maxmod
 */
#define MM_PAN_CENTER 128

/**
 * @def MM_BASE_PITCH_RATE
 * @brief The default pitch rate for the sound effect played.
 *
 * Increasing the rate increases the pitch, while decreasing lowers the pitch.
 */
#define MM_BASE_PITCH_RATE 1024

/**
 * @def SFX_DEFAULT_VOLUME
 * @brief Default volume for sound effects
 */
#define SFX_DEFAULT_VOLUME MM_SFX_FULL_VOLUME

/**
 * @def SFX_DEFAULT_PAN
 * @brief The default stereo pan, will always be center pan.
 */
#define SFX_DEFAULT_PAN MM_PAN_CENTER

/**
 * @def GBAL_MM_NUM_CHANNELS
 * The number of audio channels to allocate for maxmod,
 * passed to mmInitDefault()
 */
#define GBAL_MM_NUM_CHANNELS 14

/**
 * @brief Play a sound effect, wrapper for mmEffectEx()
 * See https://maxmod.org/ref/functions/mm_sound_effect.html
 * and https://maxmod.org/ref/functions/mmEffectEx.html
 *
 * @param id the sound id to play, from maxmod compiled soundbank.h header
 * @param rate the pitch rate, the default value is @ref MM_BASE_PTCH_RATE
 * @param volume the volume ranging from 0 (silent) to 255 (loudest)
 */
void play_sfx(mm_word id, mm_word rate, mm_byte volume);

#endif
