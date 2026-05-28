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
    s32 target;
    s32 stride;
} AudioParamReq;

typedef struct
{
    s32 current;
    AudioParamReq req;
} AudioParam;

// clang-format off
#define AUDIO_PARAM_REQ_DEFAULT { .target = 0, .stride = 0 }
#define AUDIO_PARAM_DEFINE(init_val) { .current = init_val, .req = AUDIO_PARAM_REQ_DEFAULT }
// clang-format on

typedef struct
{
    AudioParam pitch;
    AudioParam tempo;
    AudioParam volume;
} MusicPlayerState;

static const u32 DEFAULT_PITCH = 0x400;
static const u32 DEFAULT_TEMPO = 0x400;
static const u32 DEFAULT_VOLUME = MM_MODULE_FULL_VOLUME;
static const u32 MUSIC_CHANGE_FRAMES = 75;
static const u32 VOLUME_CHANGE_FRAMES = MUSIC_CHANGE_FRAMES / 3;

static void set_audio_param_req(AudioParam* param, s32 target, s32 steps);
static void speed_change_update(void);

static MusicPlayerState music_player = {
    .pitch = AUDIO_PARAM_DEFINE(DEFAULT_PITCH),
    .tempo = AUDIO_PARAM_DEFINE(DEFAULT_TEMPO),
    .volume = AUDIO_PARAM_DEFINE(DEFAULT_VOLUME),
};

static StateInfo state_info[] = {
    STATE_INFO_UPDATE_FN_ONLY(speed_change_update),
};

static StateMachine song_speed_sm = STATE_MACHINE_DEFINE(state_info, 1);

static void set_audio_param_req(AudioParam* param, s32 target, s32 steps)
{
    int offset = target - param->current;

    // if '0' set to '1'
    steps |= !steps;
    param->req.stride = offset / steps;
    param->req.stride = !param->req.stride ? SIGN(offset) : param->req.stride;
    param->req.target = target;
}

/**
 * @brief Update the @ref AudioParam for the music transition state machine for audio transitions
 *
 * @return true if target is reached, false otherwise
 */
static inline bool audio_param_update(AudioParam* param)
{
    if (abs(param->current - param->req.target) <= abs(param->req.stride))
    {
        param->current = param->req.target;
        return true;
    }
    param->current += param->req.stride;
    return false;
}

static void speed_change_update(void)
{
    bool tempo_reached = audio_param_update(&music_player.tempo);
    bool pitch_reached = audio_param_update(&music_player.pitch);
    bool volume_reached = audio_param_update(&music_player.volume);

    mmSetModuleTempo(music_player.tempo.current);
    mmSetModulePitch(music_player.pitch.current);
    set_volume(music_player.volume.current);

    if (tempo_reached && pitch_reached && volume_reached)
        state_machine_remove(&song_speed_sm);
}

void play_sfx(mm_word id, mm_word rate, mm_byte volume)
{
    int adj_volume = volume * g_game_vars.sound_volume / VOLUME_OPTION_MAX;
    mm_sound_effect sfx = {
        {id},
        rate,
        0,
        adj_volume,
        SFX_DEFAULT_PAN,
    };
    mmEffectEx(&sfx);
}

void play_lose_music(void)
{
    const u32 slow_music_speed = 0x200;
    // Don't adjust the volume if already on the lowest setting, otherwise it's not audible
    u32 vol = g_game_vars.music_volume;
    // Don't divide the audio by half if it's one, just sounds bad
    // This works too, not for negative numbers. It's neat.
    // vol |= !((vol - 1) > 0);
    vol = (vol == 1) ? 1 : vol / 2;
    u32 target_vol = volume_module_step_to_val(vol);

    set_audio_param_req(&music_player.pitch, slow_music_speed, MUSIC_CHANGE_FRAMES);
    set_audio_param_req(&music_player.tempo, slow_music_speed, MUSIC_CHANGE_FRAMES);
    set_audio_param_req(&music_player.volume, target_vol, VOLUME_CHANGE_FRAMES);

    state_machine_register(&song_speed_sm);
    state_machine_change_state(&song_speed_sm, 0);
}

void play_regular_music(void)
{
    u32 target_vol = volume_module_step_to_val(g_game_vars.music_volume);
    set_audio_param_req(&music_player.pitch, DEFAULT_PITCH, MUSIC_CHANGE_FRAMES);
    set_audio_param_req(&music_player.tempo, DEFAULT_TEMPO, MUSIC_CHANGE_FRAMES);
    set_audio_param_req(&music_player.volume, target_vol, VOLUME_CHANGE_FRAMES);

    state_machine_register(&song_speed_sm);
    state_machine_change_state(&song_speed_sm, 0);
}

void set_volume(int volume)
{
    music_player.volume.current = volume;
    mmSetModuleVolume(volume);
}
