#include "game/blind_select.h"

#include "audio_utils.h"
#include "blind.h"
#include "game.h"
#include "game/palette.h"
#include "game/rect.h"
#include "game/timer.h"
#include "graphic_utils.h"
#include "soundbank.h"
#include "sprite.h"
#include "util.h"

#include <stdint.h>
#include <string.h>

// Forward declarations - extern variables (game.h and other included headers provide these)
extern int ante;
extern int game_round;
extern uint timer;
extern int game_speed;
extern enum BackgroundId background;
extern int selection_y;
extern enum GameState game_state;

extern Sprite* blind_select_tokens[BLIND_TYPE_MAX];
extern enum BlindState blinds_states[BLIND_TYPE_MAX];
extern int current_blind;

extern StateInfo state_info[];

// Forward declarations - extern functions from game.c
extern void game_change_state(enum GameState new_game_state);
extern void change_background(enum BackgroundId id);
extern void increment_blind(enum BlindState increment_reason);
extern void reset_top_left_panel_bottom_row(void);
extern void display_round(int round);

#define TILE_SIZE         8
#define BUTTON_SFX_VOLUME 154 // 60% of MM_FULL_VOLUME
#define UINT_MAX_DIGITS   10
#define OVERFLOW_RIGHT    0

static void blind_select_start_anim_seq();
static void blind_select_handle_input();
static void blind_select_selected_anim_seq();
static void blind_select_display_blind_panel();
typedef void (*SubStateActionFn)(void);
static const SubStateActionFn blind_select_state_actions[] = {
    blind_select_start_anim_seq,
    blind_select_handle_input,
    blind_select_selected_anim_seq,
    blind_select_display_blind_panel
};

// Blind select sub-states enum
enum BlindSelectStates
{
    START_ANIM_SEQ,
    BLIND_SELECT,
    BLIND_SELECTED_ANIM_SEQ,
    DISPLAY_BLIND_PANEL,
    BLIND_SELECT_MAX
};

// Internal helper functions
static inline int blind_select_rect_width(const Rect* rect)
{
    return rect->right - rect->left;
}

void game_blind_select_on_init(void)
{
    change_background(BG_BLIND_SELECT);
    play_sfx(SFX_POP, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
}

void game_blind_select_on_update(void)
{
    if (state_info[game_state].substate == BLIND_SELECT_MAX)
    {
        game_change_state(GAME_STATE_PLAYING);
        return;
    }

    int substate = state_info[game_state].substate;
    blind_select_state_actions[substate]();
}

static inline void blind_select_erase_blind_reqs_and_rewards()
{
    for (enum BlindType curr_blind = 0; curr_blind < BLIND_TYPE_MAX; curr_blind++)
    {
        Rect blind_req_and_reward_rect = SINGLE_BLIND_SEL_REQ_SCORE_RECT;

        // To account for both raised blind and reward
        blind_req_and_reward_rect.top -= TILE_SIZE;
        blind_req_and_reward_rect.bottom += TILE_SIZE;

        // To account for overflow
        blind_req_and_reward_rect.right += TILE_SIZE;

        blind_req_and_reward_rect.left +=
            curr_blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;
        blind_req_and_reward_rect.right +=
            curr_blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;

        tte_erase_rect_wrapper(blind_req_and_reward_rect);
    }
}

static Rect blind_select_get_req_score_rect(enum BlindType blind)
{
    Rect blind_req_score_rect = SINGLE_BLIND_SEL_REQ_SCORE_RECT;

    blind_req_score_rect.left += blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;
    blind_req_score_rect.right += blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;

    if (blinds_states[blind] == BLIND_STATE_CURRENT)
    {
        // Current blind is raised
        blind_req_score_rect.top -= TILE_SIZE;
        blind_req_score_rect.bottom -= TILE_SIZE;
    }

    return blind_req_score_rect;
}

static inline void blind_select_print_blind_req(enum BlindType blind)
{
    Rect blind_req_score_rect = blind_select_get_req_score_rect(blind);

    u32 blind_req = blind_get_requirement(blind, ante);

    char blind_req_str_buff[UINT_MAX_DIGITS + 1];
    truncate_uint_to_suffixed_str(
        blind_req,
        rect_width(&blind_req_score_rect) / TTE_CHAR_SIZE,
        blind_req_str_buff
    );

    update_text_rect_to_right_align_str(&blind_req_score_rect, blind_req_str_buff, OVERFLOW_RIGHT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        blind_req_score_rect.left,
        blind_req_score_rect.top,
        TTE_RED_PB,
        blind_req_str_buff
    );
}

static inline void blind_select_print_blind_reward(enum BlindType blind)
{
    int blind_reward = blind_get_reward(blind);
    Rect blind_reward_rect = blind_select_get_req_score_rect(blind);

    // The reward is right below the score.
    blind_reward_rect.top += TILE_SIZE;
    blind_reward_rect.bottom += TILE_SIZE;

    char blind_reward_str_buff[UINT_MAX_DIGITS + 2]; // +2 for null terminator and "$"
    snprintf(blind_reward_str_buff, sizeof(blind_reward_str_buff), "$%d", blind_reward);

    update_text_rect_to_right_align_str(&blind_reward_rect, blind_reward_str_buff, OVERFLOW_RIGHT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        blind_reward_rect.left,
        blind_reward_rect.top,
        TTE_YELLOW_PB,
        blind_reward_str_buff
    );
}

static void blind_select_print_blinds_reqs_and_rewards(void)
{
    for (enum BlindType curr_blind = 0; curr_blind < BLIND_TYPE_MAX; curr_blind++)
    {
        blind_select_print_blind_req(curr_blind);
        blind_select_print_blind_reward(curr_blind);
    }
}

// Sub-state action functions
static void blind_select_start_anim_seq()
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        sprite_position(
            blind_select_tokens[i],
            blind_select_tokens[i]->pos.x,
            blind_select_tokens[i]->pos.y - TILE_SIZE
        );
    }

    if (timer == TM_END_ANIM_SEQ)
    {
        blind_select_print_blinds_reqs_and_rewards();
        state_info[game_state].substate = BLIND_SELECT;
        timer = TM_ZERO; // Reset the timer
    }
}

static void blind_select_handle_input()
{
    if (timer == TM_BLIND_SELECT_START && current_blind == BLIND_TYPE_BOSS)
    {
        selection_y = 0;
    }

    // Blind select input logic
    if (key_hit(KEY_UP))
    {
        selection_y = 0;
    }
    else if (key_hit(KEY_DOWN) && current_blind != BLIND_TYPE_BOSS)
    {
        selection_y = 1;
    }
    else if (key_hit(SELECT_CARD))
    {
        blind_select_erase_blind_reqs_and_rewards();

        if (selection_y == 0) // Blind selected
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            state_info[game_state].substate = BLIND_SELECTED_ANIM_SEQ;
            timer = TM_ZERO;
            display_round(++game_round);
        }
        else if (current_blind != BLIND_TYPE_BOSS)
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            increment_blind(BLIND_STATE_SKIPPED);

            background = UNDEFINED; // Force refresh of the background
            change_background(BG_BLIND_SELECT);

            // TODO: Create a generic vertical move by any number of tiles to avoid for loops?
            for (int i = 0; i < 12; i++)
            {
                main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);
            }

            for (int i = 0; i < BLIND_TYPE_MAX; i++)
            {
                sprite_position(
                    blind_select_tokens[i],
                    blind_select_tokens[i]->pos.x,
                    blind_select_tokens[i]->pos.y - (TILE_SIZE * 12)
                );
            }

            blind_select_print_blinds_reqs_and_rewards();

            timer = TM_ZERO;
        }
    }

    if (selection_y == 0)
    {
        // 5 is the multiplier palette color and the skip button color
        memset16(&pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID], 0xFFFF, 1);
        memcpy16(
            &pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[BLIND_SKIP_BTN_PID],
            1
        );
    }
    else
    {
        // 15 is the select button color
        memcpy16(
            &pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[BLIND_SELECT_BTN_PID],
            1
        );
        memset16(&pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID], 0xFFFF, 1);
    }
}

static void blind_select_selected_anim_seq()
{
    if (timer < 15)
    {
        Rect blinds_rect = POP_MENU_ANIM_RECT;
        blinds_rect.top -= 1; // Because of the raised blind
        main_bg_se_move_rect_1_tile_vert(blinds_rect, SCREEN_DOWN);

        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            sprite_position(
                blind_select_tokens[i],
                blind_select_tokens[i]->pos.x,
                blind_select_tokens[i]->pos.y + TILE_SIZE
            );
        }
    }
    else if (timer >= MENU_POP_OUT_ANIM_FRAMES)
    {
        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            obj_hide(blind_select_tokens[i]->obj);
        }

        state_info[game_state].substate = DISPLAY_BLIND_PANEL; // Reset the state
        timer = TM_ZERO;                                       // Reset the timer
    }
}

static void blind_select_display_blind_panel()
{
    if (timer >= TM_DISP_BLIND_PANEL_FINISH)
    {
        state_info[game_state].substate = BLIND_SELECT_MAX;
        return;
    }

    // Switches to the selecting background and clears the blind panel area
    if (timer == TM_DISP_BLIND_PANEL_START)
    {
        change_background(BG_CARD_SELECTING);

        main_bg_se_clear_rect(ROUND_END_MENU_RECT);

        for (int y = 0; y < 5; y++)
        {
            int y_from = 28;
            int y_to = 0 + y;

            Rect from = {0, y_from, 8, y_from + 1};
            BG_POINT to = {0, y_to};

            main_bg_se_copy_rect(from, to);
        }

        reset_top_left_panel_bottom_row();
    }

    // Shift the blind panel down onto screen
    for (int y = 0; y < timer; y++)
    {
        int y_from = 26 + y - timer;
        int y_to = 0 + y;

        Rect from = {0, y_from, 8, y_from};
        BG_POINT to = {0, y_to};

        main_bg_se_copy_rect(from, to);
    }
}

void game_blind_select_on_exit(void)
{
    selection_y = 0;
    background = UNDEFINED;
}
