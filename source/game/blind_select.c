#include "blind_select.h"

#include "audio_utils.h"
#include "blind.h"
#include "common_ui.h"
#include "game.h"
#include "game_variables.h"
#include "graphic_utils.h"

enum BlindSelectStates
{
    START_ANIM_SEQ,
    BLIND_SELECT,
    BLIND_SELECTED_ANIM_SEQ,
    DISPLAY_BLIND_PANEL,
    BLIND_SELECT_MAX
};

static void game_blind_select_start_anim_seq(void);
static void game_blind_select_handle_input(GameVariables* vars);
static void game_blind_select_selected_anim_seq(void);
static void game_blind_select_display_blind_panel(void);
static Rect game_blind_select_get_req_score_rect(enum BlindType blind);
static void game_blind_select_print_blinds_reqs_and_rewards(void);

static const SubStateActionFn blind_select_state_actions[] = {
    game_blind_select_start_anim_seq,
    game_blind_select_handle_input,
    game_blind_select_selected_anim_seq,
    game_blind_select_display_blind_panel
};

static const Rect POP_MENU_ANIM_RECT = { 9, 7, 24, 31 };
// The sprites that display the blinds when in "GAME_BLIND_SELECT" state
static Sprite* blind_select_tokens[BLIND_TYPE_MAX] = {NULL};

static int selection_x = 0;
static int selection_y = 0;

static int substate;

static void game_blind_select_start_anim_seq()
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
        game_blind_select_print_blinds_reqs_and_rewards();
        substate = BLIND_SELECT;
        timer = TM_ZERO; // Reset the timer
    }
}

static void game_blind_select_handle_input(GameVariables* vars)
{
    if (timer == TM_BLIND_SELECT_START && vars->current_blind == BLIND_TYPE_BOSS)
    {
        selection_y = 0;
    }

    // Blind select input logic
    if (key_hit(KEY_UP))
    {
        selection_y = 0;
    }
    else if (key_hit(KEY_DOWN) && vars->current_blind != BLIND_TYPE_BOSS)
    {
        selection_y = 1;
    }
    else if (key_hit(SELECT_CARD))
    {
        game_blind_select_erase_blind_reqs_and_rewards();

        if (selection_y == 0) // Blind selected
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            substate = BLIND_SELECTED_ANIM_SEQ;
            timer = TM_ZERO;
            ++round;
            display_round();
        }
        else if (current_blind != BLIND_TYPE_BOSS)
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            increment_blind(BLIND_STATE_SKIPPED);

            selection_y = 0; // Reset selection to first option

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

            game_blind_select_print_blinds_reqs_and_rewards();

            timer = TM_ZERO;
        }
    }

    if (selection_y == 0)
    {
        memset16(&pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID], 0xFFFF, 1);
        memcpy16(
            &pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[BLIND_SKIP_BTN_PID],
            1
        );
    }
    else
    {
        memcpy16(
            &pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[BLIND_SELECT_BTN_PID],
            1
        );
        memset16(&pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID], 0xFFFF, 1);
    }
}

static void game_blind_select_selected_anim_seq()
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

static void game_blind_select_display_blind_panel()
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

        // Need to clear the top left panel as a side effect of change_background()
        main_bg_se_copy_expand_3w_row(TOP_LEFT_PANEL_ANIM_RECT, TOP_LEFT_PANEL_EMPTY_3W_ROW_POS);

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

static Rect game_blind_select_get_req_score_rect(enum BlindType blind)
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

static void game_blind_select_print_blinds_reqs_and_rewards(void)
{
    for (enum BlindType curr_blind = 0; curr_blind < BLIND_TYPE_MAX; curr_blind++)
    {
        game_blind_select_print_blind_req(curr_blind);
        game_blind_select_print_blind_reward(curr_blind);
    }
}

void game_blind_select_on_init(GameVariables* vars)
{
    change_background(BG_BLIND_SELECT);
    selection_x = 0;
    selection_y = 0;

    //play_sfx(SFX_POP, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
}

void game_blind_select_on_update(GameVariables* vars)
{
    if (substate == BLIND_SELECT_MAX)
    {
        game_change_state(GAME_STATE_PLAYING);
        return;
    }

    blind_select_state_actions[substate]();
}

void game_blind_select_on_exit(GameVariables* vars)
{
}

