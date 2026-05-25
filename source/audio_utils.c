/**
 * @file audio_utils.c
 *
 * @brief Audio utility functions implementation.
 */

#include "audio_utils.h"

#include "game_variables.h"
#include "mgba_logger.h"
#include "state_machine.h"
#include "util.h"

#include <maxmod.h>
#include <stdlib.h>

typedef struct
{
    s32 target_tempo;
    s32 target_pitch;
    s32 num_steps;
    s32 tempo_itr;
    s32 pitch_itr;
} MusicSpeedChangeReq;

typedef struct
{
    s32 pitch;
    s32 tempo;
} MusicPlayerState;

static MusicSpeedChangeReq make_music_speed_change_req(
    s32 target_pitch,
    s32 target_tempo,
    s32 num_steps
);
static void request_music_speed_change(const MusicSpeedChangeReq req);
static void speed_change_update(void);

static const u32 DEFAULT_PITCH = 0x400;
static const u32 DEFAULT_TEMPO = 0x400;
static const u32 MUSIC_CHANGE_FRAMES = 60;
static MusicPlayerState music_player = {.pitch = DEFAULT_PITCH, .tempo = DEFAULT_TEMPO};
static MusicSpeedChangeReq current_req;

static StateInfo state_info[] = {
    STATE_INFO_UPDATE_FN_ONLY(speed_change_update),
};

static StateMachine song_speed_sm = STATE_MACHINE_DEFINE(state_info, 1);

static void request_music_speed_change(const MusicSpeedChangeReq req)
{
    current_req = req;
    state_machine_register(&song_speed_sm);
    state_machine_change_state(&song_speed_sm, 0);
}

static MusicSpeedChangeReq make_music_speed_change_req(
    s32 target_pitch,
    s32 target_tempo,
    s32 num_steps
)
{
    MusicSpeedChangeReq req;

    int tempo_offset = target_tempo - music_player.tempo;
    int pitch_offset = target_pitch - music_player.pitch;

    num_steps |= !num_steps; // always ensure it's at least 1 for division

    req.tempo_itr = tempo_offset / num_steps;
    req.pitch_itr = pitch_offset / num_steps;
    // This needs to always be at least 1. Integer division above can lead to
    // to some infinite loops.
    req.tempo_itr = !req.tempo_itr ? SIGN(tempo_offset) : req.tempo_itr;
    req.pitch_itr = !req.pitch_itr ? SIGN(pitch_offset) : req.pitch_itr;
    req.target_pitch = target_pitch;
    req.target_tempo = target_tempo;
    req.num_steps = num_steps;

    return req;
}

void play_lose_music(void)
{
    const u32 slow_music_speed = 0x200;

    MusicSpeedChangeReq req =
        make_music_speed_change_req(slow_music_speed, slow_music_speed, MUSIC_CHANGE_FRAMES);
    request_music_speed_change(req);
}

void play_regular_music(void)
{
    MusicSpeedChangeReq req =
        make_music_speed_change_req(DEFAULT_PITCH, DEFAULT_TEMPO, MUSIC_CHANGE_FRAMES);
    request_music_speed_change(req);
}

/**
 * @brief Update the tempo for the music speed state machine for audio transitions
 *
 * @return true if target tempo is reached, false otherwise
 */
static inline bool tempo_update(void)
{
    if (abs(music_player.tempo - current_req.target_tempo) <= abs(current_req.tempo_itr))
    {
        music_player.tempo = current_req.target_tempo;
        return true;
    }
    music_player.tempo += current_req.tempo_itr;
    return false;
}

/**
 * @brief Update the pitch for the music speed state machine for audio transitions
 *
 * @return true if target pitch is reached, false otherwise
 */
static inline bool pitch_update(void)
{
    if (abs(music_player.pitch - current_req.target_pitch) <= abs(current_req.pitch_itr))
    {
        music_player.pitch = current_req.target_pitch;
        return true;
    }
    music_player.pitch += current_req.pitch_itr;
    return false;
}

static void speed_change_update(void)
{
    bool tempo_reached = tempo_update();
    bool pitch_reached = pitch_update();

    mmSetModuleTempo(music_player.tempo);
    mmSetModulePitch(music_player.pitch);

    if (tempo_reached && pitch_reached)
        state_machine_remove(&song_speed_sm);
}

void play_sfx(mm_word id, mm_word rate, mm_byte volume)
{
    mm_sound_effect sfx = {
        {id},
        rate,
        0,
        (volume * g_game_vars.sound_volume) / VOLUME_OPTION_MAX,
        SFX_DEFAULT_PAN,
    };
    mmEffectEx(&sfx);
}
