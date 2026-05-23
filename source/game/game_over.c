#include "game/game_over.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "button.h"
#include "game.h"
#include "game/main_menu.h"
#include "graphic_utils.h"
#include "layout.h"
#include "random.h"
#include "soundbank.h"
#include "timer.h"
#include "util.h"

#include <tonc.h>

enum EndCondition
{
    END_CONDITION_NONE,
    END_CONDITION_WIN,
    END_CONDITION_LOSS
};

// clang-format off
static const BG_POINT GAME_OVER_SRC_RECT_3X3_POS = { 25,  29};
static const Rect     GAME_OVER_DIALOG_DEST_RECT = { 11,  21,  23,  28};
static const Rect     GAME_OVER_ANIM_RECT        = { 11,   8,  23,  28};
static const Rect     GAME_LOSE_MSG_TEXT_RECT    = {104,  72, UNDEFINED, UNDEFINED};
// 1 character to the right of GAME_LOSE
static const Rect     GAME_WIN_MSG_TEXT_RECT     = {112,  72, UNDEFINED, UNDEFINED};
static const BG_POINT NEW_RUN_BTN_DEST_POS       = { 15,  26};
static const Rect     NEW_RUN_BTN_SRC_RECT       = {  0,  30,   4,  31};
// clang-format on

static const u32 GAME_OVER_ANIM_FRAMES = 15;

static enum EndCondition condition = END_CONDITION_NONE;
static u32 timer = TM_ZERO;

static void game_over_init(enum EndCondition init_condition)
{
    condition = init_condition;
    // Clears the round end menu
    main_bg_se_clear_rect(POP_MENU_ANIM_RECT);
    main_bg_se_copy_expand_3x3_rect(GAME_OVER_DIALOG_DEST_RECT, GAME_OVER_SRC_RECT_3X3_POS);
    main_bg_se_copy_rect(NEW_RUN_BTN_SRC_RECT, NEW_RUN_BTN_DEST_POS);

    // Using the text color to match the Win/Loss condition
    switch (condition)
    {
        case END_CONDITION_WIN:
            affine_background_set_color(TEXT_CLR_BLUE);
            break;
        case END_CONDITION_LOSS:
            affine_background_set_color(TEXT_CLR_RED);
            break;
        default:
            break;
    }

    timer = TM_ZERO;
}

void game_win_on_init(void)
{
    game_over_init(END_CONDITION_WIN);
}

void game_lose_on_init(void)
{
    game_over_init(END_CONDITION_LOSS);
}

/**
 * @brief Makes the message background frame move 1 tile up, called until it is in its place.
 */
static inline void game_over_anim_frame(void)
{
    main_bg_se_move_rect_1_tile_vert(GAME_OVER_ANIM_RECT, SCREEN_UP);
}

/**
 * @brief Polls user input to detect "Retry" button press.
 */
static inline void game_over_process_user_input()
{
    if (key_hit(SELECT_CARD))
    {
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
        game_change_state(GAME_STATE_BLIND_SELECT);
    }
}

void game_over_on_update(void)
{
    timer++;
    if (timer < GAME_OVER_ANIM_FRAMES)
    {
        game_over_anim_frame();
    }
    else if (timer == GAME_OVER_ANIM_FRAMES)
    {
        switch (condition)
        {
            case END_CONDITION_WIN:
                tte_printf(
                    "#{P:%d,%d; cx:0x%X000}YOU WIN",
                    GAME_WIN_MSG_TEXT_RECT.left,
                    GAME_WIN_MSG_TEXT_RECT.top,
                    TTE_BLUE_PB
                );
                break;
            case END_CONDITION_LOSS:
                tte_printf(
                    "#{P:%d,%d; cx:0x%X000}GAME OVER",
                    GAME_LOSE_MSG_TEXT_RECT.left,
                    GAME_LOSE_MSG_TEXT_RECT.top,
                    TTE_RED_PB
                );
                break;
            default:
                break;
        }
    }
    game_over_process_user_input();
}

void game_over_on_exit(void)
{
    condition = END_CONDITION_NONE;
    game_reset();
    rng_shuffle_seed();
}
