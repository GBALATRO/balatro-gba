#include "round_end.h"

#include "game_variables.h"
#include "timer.h"
#include "util.h"

enum GameRoundEndStates
{
    ROUND_END_START,
    START_EXPAND_POPUP,
    DISPLAY_FINISHED_BLIND,
    DISPLAY_SCORE_MIN,
    UPDATE_BLIND_REWARD,
    BLIND_PANEL_EXIT,
    DISPLAY_REWARDS,
    DISPLAY_CASHOUT,
    DISMISS_ROUND_END_PANEL,
    ROUND_END_EXIT
};

static const u32 TM_RESET_STATIC_VARS = 30;

static int substate;
static int blind_reward = 0;
static int hand_reward = 0;
static int interest_reward = 0;
static int interest_to_count = 0;
static int interest_start_time = UNDEFINED;

static int calculate_interest_reward(void);
static void game_round_end_start(void);
static void game_round_end_start_expand_popup(void);
static void game_round_end_display_finished_blind(void);
static void game_round_end_display_score_min(void);
static void game_round_end_update_blind_reward(void);
static void game_round_end_panel_exit(void);
static void game_round_end_display_rewards(void);
static void game_round_end_display_cashout(void);
static void game_round_end_dismiss_round_end_panel(void);

static const SubStateActionFn round_end_state_actions[] = {
    game_round_end_start,
    game_round_end_start_expand_popup,
    game_round_end_display_finished_blind,
    game_round_end_display_score_min,
    game_round_end_update_blind_reward,
    game_round_end_panel_exit,
    game_round_end_display_rewards,
    game_round_end_display_cashout,
    game_round_end_dismiss_round_end_panel
};

static int calculate_interest_reward(void)
{
    int reward = (g_game_vars.money / 5) * INTEREST_PER_5;
    if (reward > MAX_INTEREST)
        reward = MAX_INTEREST;
    return reward;
}

static void game_round_end_start(void)
{
    // Reset static variables to default values upon re-entering the round end state
    if (g_game_vars.timer == TM_RESET_STATIC_VARS)
    {
        change_background(BG_ROUND_END, false); // Change the background to the round end background
        substate = START_EXPAND_POPUP; // Change the state to the next one
        g_game_vars.timer = TM_ZERO;                          // Reset the timer
        blind_reward = blind_get_reward(g_game_vars.current_blind);
        hand_reward = g_game_vars.hands;
        interest_reward = calculate_interest_reward();
        interest_to_count = interest_reward;
        interest_start_time = UNDEFINED;
    }
}

static void game_round_end_start_expand_popup()
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (g_game_vars.timer == TM_END_POP_MENU_ANIM)
    {
        state_info[game_state].substate = DISPLAY_FINISHED_BLIND;
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_display_finished_blind()
{
    obj_unhide(g_game_vars.round_end_blind_token->obj, 0);

    int current_ante = g_game_vars.ante;

    // Beating the boss blind increases the ante, so we need to display the previous ante value
    if (g_game_vars.current_blind > BLIND_TYPE_BIG)
        current_ante--;

    Rect blind_req_rect = ROUND_END_BLIND_REQ_RECT;
    u32 blind_req = blind_get_requirement(g_game_vars.current_blind, current_ante);

    /* Not bothering to truncate here because there are 8 tiles
     * and the blind requirement will not increase past ante 8
     * so there's enough room for sure.
     */
    char blind_req_str_buff[UINT_MAX_DIGITS + 1];
    snprintf(blind_req_str_buff, sizeof(blind_req_str_buff), "%lu", blind_req);

    update_text_rect_to_right_align_str(&blind_req_rect, blind_req_str_buff, OVERFLOW_RIGHT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        blind_req_rect.left,
        blind_req_rect.top,
        TTE_RED_PB,
        blind_req_str_buff
    );

    if (g_game_vars.timer == TM_START_ROUND_END_REWARDS_ANIM)
    {
        game_round_end_extend_black_panel_down(ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE);
    }

    if (g_game_vars.timer >= TM_END_DISPLAY_FIN_BLIND)
    {
        state_info[game_state].substate = DISPLAY_SCORE_MIN;
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_display_score_min()
{
    const int timer_offset = g_game_vars.timer - 1;
    const int x_from = 0;
    const int y_from = 29;
    const int x_to = 13;
    const int y_to = 11;

    memcpy16(
        &se_mem[MAIN_BG_SBB][x_to + timer_offset + 32 * y_to],
        &se_mem[MAIN_BG_SBB][x_from + timer_offset + 32 * y_from],
        1
    );

    if (g_game_vars.timer >= TM_END_DISPLAY_SCORE_MIN)
    {
        state_info[game_state].substate = UPDATE_BLIND_REWARD;
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_update_blind_reward()
{
    if (g_game_vars.timer % FRAMES(20) != 0)
        return;

    // TODO: Add sound effect here

    if (blind_reward > 0)
    {
        blind_reward--;
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            BLIND_REWARD_RECT.left,
            BLIND_REWARD_RECT.top,
            TTE_YELLOW_PB,
            blind_reward
        );
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            ROUND_END_BLIND_REWARD_RECT.left,
            ROUND_END_BLIND_REWARD_RECT.top,
            TTE_YELLOW_PB,
            blind_get_reward(g_game_vars.current_blind) - blind_reward
        );
    }
    else if (g_game_vars.timer > FRAMES(20))
    {
        tte_erase_rect_wrapper(BLIND_REWARD_RECT);
        tte_erase_rect_wrapper(BLIND_REQ_TEXT_RECT);
        obj_hide(g_game_vars.playing_blind_token->obj);
        affine_background_load_palette(affine_background_gfxPal);
        state_info[game_state].substate = BLIND_PANEL_EXIT;
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_panel_exit()
{
    // TODO: make heads or tails of what's going on here and replace
    // magic numbers.
    if (g_game_vars.timer < 8)
    {
        main_bg_se_copy_rect_1_tile_vert(TOP_LEFT_PANEL_ANIM_RECT, SCREEN_UP);

        if (g_game_vars.timer == 1)
        {
            reset_top_left_panel_bottom_row();
        }
        else if (g_game_vars.timer == 2)
        {
            int y = 5;
            memset16(&se_mem[MAIN_BG_SBB][32 * (y - 1)], 0x0001, 1);
            memset16(&se_mem[MAIN_BG_SBB][1 + 32 * (y - 1)], 0x0002, 7);
            memset16(&se_mem[MAIN_BG_SBB][8 + 32 * (y - 1)], 0x0401, 1);
        }
    }
    else if (g_game_vars.timer > FRAMES(20))
    {
        memset16(&pal_bg_mem[REWARD_PANEL_BORDER_PID], 0x1483, 1);
        state_info[game_state].substate = DISPLAY_REWARDS;
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_display_rewards()
{
    int hand_y_offset = 0;
    int interest_y_offset = 0;

    if (g_game_vars.hands > 0)
    {
        hand_y_offset = 1;
    }
    else
    {
        interest_start_time = TM_DISPLAY_REWARDS_CONT_WAIT;
    }

    if (interest_reward > 0)
    {
        interest_y_offset = hand_y_offset + 1;
    }

    // Once all rewards are accounted for go to the next state
    if (hand_reward <= 0 && interest_to_count <= 0)
    {
        g_game_vars.timer = TM_ZERO;
        state_info[game_state].substate = DISPLAY_CASHOUT;
    }
    else if (g_game_vars.timer == TM_START_ROUND_END_REWARDS_ANIM)
    {
        game_round_end_extend_black_panel_down(ROUND_END_REWARDS_ELLIPSIS_POS.y);
    }
    else if (g_game_vars.timer < TM_REWARDS_ELLIPSIS_PRINT_END)
    {
        game_round_end_print_separator_ellipsis();
    }
    else if (g_game_vars.timer >= TM_DISPLAY_REWARDS_CONT_WAIT && hand_reward > 0)
    {
        game_round_end_print_hand_reward(hand_y_offset);
    }
    else if (interest_start_time != UNDEFINED && g_game_vars.timer >= interest_start_time &&
             interest_to_count > 0)
    {
        game_round_end_print_interest_reward(interest_y_offset);
    }
}

static void game_round_end_display_cashout()
{
    if (g_game_vars.timer == FRAMES(40))
    {
        // Put the "cash out" button onto the round end panel
        main_bg_se_copy_expand_3x3_rect(CASHOUT_DEST_RECT, CASHOUT_SRC_3X3_RECT_POS);

        int cashout_amount =
            g_game_vars.hands + blind_get_reward(g_game_vars.current_blind) + calculate_interest_reward();

        bool omit_space = cashout_amount >= 10;
        tte_printf(
            "#{P:%d, %d; cx:0x%X000}Cash Out:%s$%d",
            CASHOUT_TEXT_RECT.left,
            CASHOUT_TEXT_RECT.top,
            TTE_WHITE_PB,
            omit_space ? "" : " ",
            cashout_amount
        );
    }

    // Wait until the player presses A to cash out
    else if (g_game_vars.timer > FRAMES(40) && key_hit(SELECT_CARD))
    {
        game_round_end_cashout();

        state_info[game_state].substate = DISMISS_ROUND_END_PANEL; // Go to the next state
        g_game_vars.timer = TM_ZERO;

        obj_hide(g_game_vars.round_end_blind_token->obj);          // Hide the blind token object
        tte_erase_rect_wrapper(BLIND_TOKEN_TEXT_RECT); // Erase the blind token text
    }
}

static void game_round_end_dismiss_round_end_panel()
{
    Rect round_end_down = ROUND_END_MENU_RECT;
    round_end_down.top--;
    main_bg_se_copy_rect_1_tile_vert(round_end_down, SCREEN_DOWN);

    if (g_game_vars.timer >= TM_DISMISS_ROUND_END_TM)
    {
        g_game_vars.timer = TM_ZERO;
        state_info[game_state].substate = ROUND_END_EXIT;
    }
}

void game_round_end_on_update(void)
{
    if (substate == ROUND_END_EXIT)
    {
        game_change_state(GAME_STATE_SHOP);
        return;
    }

    round_end_state_actions[substate]();
}

void game_round_end_on_exit(void)
{
    // Cleanup blind tokens from this round to avoid accumulating
    // allocated blind sprites each round
    blind_reward = 0;
    hand_reward = 0;
    interest_reward = 0;
    sprite_destroy(&g_game_vars.playing_blind_token);
    sprite_destroy(&g_game_vars.round_end_blind_token);
    // TODO: Reuse sprites for blind selection?
}
