#include "game/game_over.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "blind.h"
#include "button.h"
#include "game.h"
#include "game/common_ui.h"
#include "game/rect.h"
#include "graphic_utils.h"
#include "hand_analysis.h"
#include "joker.h"
#include "list.h"
#include "soundbank.h"
#include "sprite.h"
#include "tonc_memdef.h"
#include "util.h"

#include <maxmod.h>
#include <tonc.h>

#define GAME_OVER_ANIM_FRAMES 15

static void game_over_init(void)
{
    // Clears the round end menu
    main_bg_se_clear_rect(POP_MENU_ANIM_RECT);
    main_bg_se_copy_expand_3x3_rect(GAME_OVER_DIALOG_DEST_RECT, GAME_OVER_SRC_RECT_3X3_POS);
    main_bg_se_copy_rect(NEW_RUN_BTN_SRC_RECT, NEW_RUN_BTN_DEST_POS);
}

void game_lose_on_init(GameStateCtx* _)
{
    game_over_init();
    // Using the text color to match the "Game Over" text
    affine_background_set_color(TEXT_CLR_RED);
}

void game_win_on_init(GameStateCtx* _)
{
    game_over_init();
    // Using the text color to match the "You Win" text
    affine_background_set_color(TEXT_CLR_BLUE);
}

static void game_over_anim_frame(void)
{
    main_bg_se_move_rect_1_tile_vert(GAME_OVER_ANIM_RECT, SCREEN_UP);
}

static inline void game_over_process_user_input()
{
    if (key_hit(SELECT_CARD))
    {
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
        game_change_state(GAME_STATE_BLIND_SELECT);
    }
}

void game_lose_on_update(GameStateCtx* ctx)
{
    GameOverProps* props = &ctx->game_over;

    if (props->timer < GAME_OVER_ANIM_FRAMES)
    {
        game_over_anim_frame();
    }
    else if (props->timer == GAME_OVER_ANIM_FRAMES)
    {
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}GAME OVER",
            GAME_LOSE_MSG_TEXT_RECT.left,
            GAME_LOSE_MSG_TEXT_RECT.top,
            TTE_RED_PB
        );
    }

    game_over_process_user_input();
}

void game_win_on_update(GameStateCtx* ctx)
{
    GameOverProps* props = &ctx->game_over;

    if (props->timer < GAME_OVER_ANIM_FRAMES)
    {
        game_over_anim_frame();
    }
    else if (props->timer == GAME_OVER_ANIM_FRAMES)
    {
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}YOU WIN",
            GAME_WIN_MSG_TEXT_RECT.left,
            GAME_WIN_MSG_TEXT_RECT.top,
            TTE_BLUE_PB
        );
    }

    game_over_process_user_input();
}

// This function isn't set in stone. This is just a placeholder
// allowing the player to restart the game. Thought it would be nice to have
// util we decide what we want to do after a game over.
void game_over_on_exit(GameStateCtx* ctx)
{
    GameOverProps* props = &ctx->game_over;

    List* jokers_list = props->owned_jokers_list;
    while (list_get_len(jokers_list) > 0)
    {
        JokerObject* joker_object = list_get_at_idx(jokers_list, 0);
        remove_owned_joker(0);
        joker_object_destroy(&joker_object);
    }

    tte_erase_screen();

    // For some reason that I haven't figured out yet,
    // if I don't destroy the blind tokens they won't
    // show up on the next run.
    destroy_playing_and_round_end_blind_tokens();
    destroy_all_blind_select_tokens();

    clear_joker_lists();

    game_init();

    display_round(props->game_round);
    display_score(props->score);
    display_chips();
    display_mult();
    display_hands();
    display_discards();
    display_money();
}
