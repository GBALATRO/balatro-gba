#include "audio_utils.h"

#include "game_variables.h"
#include "state_machine.h"

#include <maxmod.h>
#include <stdlib.h>

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

static const s32 DEFAULT_PITCH = 0x400;
static const s32 DEFAULT_TEMPO = 0x400;
static const u32 MUSIC_CHANGE_FRAMES = 30;

static MusicPlayerState music_player = {.pitch = DEFAULT_PITCH, .tempo = DEFAULT_TEMPO};
static MusicSpeedChangeReq current_req;

static void speed_change_update(void);

static StateInfo state_info[] = {
    STATE_INFO_UPDATE_FN_ONLY(speed_change_update),
};

static StateMachine song_speed_sm = {
    .state_infos = &state_info[0],
    .num_infos = 1,
};

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
    req.tempo_itr = (target_tempo - music_player.tempo) / num_steps;
    req.pitch_itr = (target_pitch - music_player.pitch) / num_steps;
    req.target_pitch = target_pitch;
    req.target_tempo = target_tempo;
    req.num_steps = num_steps;
    return req;
}

void shop_music(void)
{
    const u32 shop_pitch = 0x600;

    MusicSpeedChangeReq req =
        make_music_speed_change_req(shop_pitch, DEFAULT_TEMPO, MUSIC_CHANGE_FRAMES);
    request_music_speed_change(req);
}

void fast_music(void)
{
    const u32 fast_speed = 0x800;

    MusicSpeedChangeReq req =
        make_music_speed_change_req(fast_speed, fast_speed, MUSIC_CHANGE_FRAMES);
    request_music_speed_change(req);
}

void slow_music(void)
{
    const u32 slow_speed = 0x200;

    MusicSpeedChangeReq req =
        make_music_speed_change_req(slow_speed, slow_speed, MUSIC_CHANGE_FRAMES);
    request_music_speed_change(req);
}

void normal_music(void)
{
    MusicSpeedChangeReq req =
        make_music_speed_change_req(DEFAULT_PITCH, DEFAULT_TEMPO, MUSIC_CHANGE_FRAMES);
    request_music_speed_change(req);
}

static inline bool tempo_update(void)
{
    if (abs(music_player.tempo - current_req.target_tempo) < abs(current_req.tempo_itr))
    {
        music_player.tempo = current_req.target_tempo;
        return true;
    }
    music_player.tempo += current_req.tempo_itr;
    return false;
}

static inline bool pitch_update(void)
{
    if (abs(music_player.pitch - current_req.target_pitch) < abs(current_req.pitch_itr))
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
