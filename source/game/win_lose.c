#include "affine_background.h"
#include "audio_utils.h"
#include "blind.h"
#include "button.h"
#include "game.h"
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

// Extern declarations from game.c
extern int timer;
extern List _owned_jokers_list;
extern List _discarded_jokers_list;
extern List _expired_jokers_list;
extern int hands;
extern int discards;
extern int game_round;
extern int score;
extern void remove_owned_joker(int owned_joker_idx);
extern void joker_object_destroy(JokerObject** joker_object);
extern void tte_erase_screen(void);
extern void tte_erase_rect_wrapper(Rect rect);
extern void game_init(void);
extern void display_round(int round);
extern void display_score(int score_val);
extern void display_chips(void);
extern void display_mult(void);
extern void display_hands(int hands_val);
extern void display_discards(int discards_val);
extern void display_money(void);
extern void game_change_state(enum GameState new_game_state);
extern Sprite* playing_blind_token;
extern Sprite* round_end_blind_token;
extern Sprite** blind_select_tokens;
extern void main_bg_se_clear_rect(Rect rect);
extern void affine_background_set_color(u16 color);

static void game_over_init(void)
{
    // Clears the round end menu
    main_bg_se_clear_rect(POP_MENU_ANIM_RECT);
    main_bg_se_copy_expand_3x3_rect(GAME_OVER_DIALOG_DEST_RECT, GAME_OVER_SRC_RECT_3X3_POS);
    main_bg_se_copy_rect(NEW_RUN_BTN_SRC_RECT, NEW_RUN_BTN_DEST_POS);
}

void game_lose_on_init()
{
    game_over_init();
    // Using the text color to match the "Game Over" text
    affine_background_set_color(TEXT_CLR_RED);
}

void game_win_on_init()
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

void game_lose_on_update()
{
    if (timer < GAME_OVER_ANIM_FRAMES)
    {
        game_over_anim_frame();
    }
    else if (timer == GAME_OVER_ANIM_FRAMES)
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

void game_win_on_update()
{
    if (timer < GAME_OVER_ANIM_FRAMES)
    {
        game_over_anim_frame();
    }
    else if (timer == GAME_OVER_ANIM_FRAMES)
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
void game_over_on_exit()
{
    while (list_get_len(&_owned_jokers_list) > 0)
    {
        JokerObject* joker_object = list_get_at_idx(&_owned_jokers_list, 0);
        remove_owned_joker(0);
        joker_object_destroy(&joker_object);
    }

    tte_erase_screen();

    // For some reason that I haven't figured out yet,
    // if I don't destroy the blind tokens they won't
    // show up on the next run.
    sprite_destroy(&playing_blind_token);
    sprite_destroy(&round_end_blind_token);
    sprite_destroy(&blind_select_tokens[BLIND_TYPE_SMALL]);
    sprite_destroy(&blind_select_tokens[BLIND_TYPE_BIG]);
    sprite_destroy(&blind_select_tokens[BLIND_TYPE_BOSS]);

    list_clear(&_owned_jokers_list);
    list_clear(&_discarded_jokers_list);
    list_clear(&_expired_jokers_list);

    game_init();

    display_round(game_round);
    display_score(score);
    display_chips();
    display_mult();
    display_hands(hands);
    display_discards(discards);
    display_money();
}
