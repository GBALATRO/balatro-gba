#include "round_end.h"

#include "affine_background.h"
#include "affine_background_gfx.h"
#include "game.h"
#include "game_variables.h"
#include "layout.h"
#include "state_machine.h"
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
    ROUND_END_STATES_MAX
};

static const u32 TM_RESET_STATIC_VARS = 30;
static const u32 TM_END_POP_MENU_ANIM = 13;
static const u32 TM_START_ROUND_END_REWARDS_ANIM = 1;
static const u32 TM_END_DISPLAY_FIN_BLIND = 30;
static const u32 TM_END_DISPLAY_SCORE_MIN = 4;
static const u32 TM_DISMISS_ROUND_END_TM = 20;

static const u32 REWARD_PANEL_BORDER_PID = 19;

static const u32 ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE = 12;
static const u32 ROUND_END_REWARD_TEXT_X = 88;
static const u32 ROUND_END_REWARD_AMOUNT_X = 168;

// clang-format off
static const Rect ROUND_END_BLIND_REQ_RECT    = {104,     96,     136,       UNDEFINED};
static const Rect ROUND_END_BLIND_REWARD_RECT = {168,     96,     UNDEFINED, UNDEFINED};
static const Rect CASHOUT_DEST_RECT           = {10,      8,      23,        10       };
static const Rect CASHOUT_TEXT_RECT           = {88,      72,     UNDEFINED, UNDEFINED};
static const Rect BLIND_TOKEN_TEXT_RECT       = {80,      72,     200,       160      };
static const Rect ROUND_END_MENU_RECT         = {9,       7,      24,        20       }; 

static const BG_POINT CASHOUT_SRC_3X3_RECT_POS =   {5,  29};
// clang-format on

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

static void game_round_end_extend_black_panel_down(int black_panel_bottom);

static StateInfo state_info[] = {
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_start),
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_start_expand_popup),
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_display_finished_blind),
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_display_score_min),
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_update_blind_reward),
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_panel_exit),
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_display_rewards),
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_display_cashout),
    STATE_INFO_UPDATE_FN_ONLY(game_round_end_dismiss_round_end_panel),
};

static StateMachine round_end_sm = STATE_MACHINE_DEFINE(state_info, ROUND_END_STATES_MAX);

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
        state_machine_change_state(&round_end_sm, START_EXPAND_POPUP);
        g_game_vars.timer = TM_ZERO; // Reset the timer
        blind_reward = blind_get_reward(g_game_vars.current_blind);
        hand_reward = g_game_vars.hands;
        interest_reward = calculate_interest_reward();
        interest_to_count = interest_reward;
        interest_start_time = UNDEFINED;
    }
}

static void game_round_end_start_expand_popup(void)
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (g_game_vars.timer == TM_END_POP_MENU_ANIM)
    {
        state_machine_change_state(&round_end_sm, DISPLAY_FINISHED_BLIND);
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_display_finished_blind(void)
{
    obj_unhide(g_game_vars.round_end_blind_token->obj, ATTR0_REG);

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
        state_machine_change_state(&round_end_sm, DISPLAY_SCORE_MIN);
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_display_score_min(void)
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
        state_machine_change_state(&round_end_sm, UPDATE_BLIND_REWARD);
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_update_blind_reward(void)
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
        state_machine_change_state(&round_end_sm, BLIND_PANEL_EXIT);
        g_game_vars.timer = TM_ZERO;
    }
}

static void game_round_end_panel_exit(void)
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
        state_machine_change_state(&round_end_sm, DISPLAY_REWARDS);
        g_game_vars.timer = TM_ZERO;
    }
}

static inline void game_round_end_print_separator_ellipsis(void)
{
    int x =
        (ROUND_END_REWARDS_ELLIPSIS_POS.x + g_game_vars.timer - TM_REWARDS_ELLIPSIS_PRINT_START) *
        TILE_SIZE;
    int y = (ROUND_END_REWARDS_ELLIPSIS_POS.y) * TILE_SIZE;

    tte_printf("#{P:%d,%d; cx:0x%X000}.", x, y, TTE_WHITE_PB);
}

// TODO: Allow for more generic rewards and consolidate with game_round_end_print_interest_reward()
static inline void game_round_end_print_hand_reward(int hand_y_offset)
{
    int hand_y = ROUND_END_REWARDS_ELLIPSIS_POS.y + hand_y_offset;
    if (g_game_vars.timer == TM_DISPLAY_REWARDS_CONT_WAIT)
    {
        game_round_end_extend_black_panel_down(hand_y);

        tte_printf(
            "#{P:%lu,%d; cx:0x%X000}%d #{cx:0x%X000}Hands",
            ROUND_END_REWARD_TEXT_X,
            hand_y * TILE_SIZE,
            TTE_BLUE_PB,
            hand_reward,
            TTE_WHITE_PB
        );
    }
    // Increment the hand reward text until the hand reward variable is depleted
    else if (g_game_vars.timer > TM_HAND_REWARD_INCR_WAIT &&
             g_game_vars.timer % FRAMES(TM_REWARD_INCREMENT_INTERVAL) == 0)
    {
        hand_reward--;
        tte_printf(
            "#{P:%lu, %d; cx:0x%X000}$%ld",
            ROUND_END_REWARD_AMOUNT_X,
            hand_y * TILE_SIZE,
            TTE_YELLOW_PB,
            g_game_vars.hands - hand_reward
        );
        if (hand_reward == 0)
        {
            interest_start_time = g_game_vars.timer + TM_REWARD_DISPLAY_INTERVAL;
        }
    }
}

static inline void game_round_end_print_interest_reward(int interest_y_offset)
{
    int interest_y = ROUND_END_REWARDS_ELLIPSIS_POS.y + interest_y_offset;

    if (g_game_vars.timer == interest_start_time)
    {
        game_round_end_extend_black_panel_down(interest_y);

        tte_printf(
            "#{P:%lu,%d; cx:0x%X000}%d #{cx:0x%X000}Interest",
            ROUND_END_REWARD_TEXT_X,
            interest_y * TILE_SIZE,
            TTE_YELLOW_PB,
            interest_reward,
            TTE_WHITE_PB
        );
    }
    // Increment the interest reward text until the interest reward variable is depleted
    else if (g_game_vars.timer > interest_start_time + TM_REWARD_DISPLAY_INTERVAL &&
             g_game_vars.timer % FRAMES(TM_REWARD_INCREMENT_INTERVAL) == 0)
    {
        interest_to_count--;
        tte_printf(
            "#{P:%lu, %d; cx:0x%X000}$%d",
            ROUND_END_REWARD_AMOUNT_X,
            interest_y * TILE_SIZE,
            TTE_YELLOW_PB,
            interest_reward - interest_to_count
        );
    }
}

static void game_round_end_display_rewards(void)
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
        state_machine_change_state(&round_end_sm, DISPLAY_CASHOUT);
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

static inline void game_round_end_cashout(void)
{
    // Reward the player
    g_game_vars.money += g_game_vars.hands + blind_get_reward(g_game_vars.current_blind) +
                         calculate_interest_reward();
    display_money();

    g_game_vars.hands = MAX_HANDS;       // Reset the hands to the maximum
    g_game_vars.discards = MAX_DISCARDS; // Reset the discards to the maximum
    // TODO: these can just be in one spot, passing global to global
    display_hands();    // Set the hands display
    display_discards(); // Set the discards display

    g_game_vars.score = 0;
    display_score(g_game_vars.score); // Set the score display
}

static void game_round_end_display_cashout()
{
    if (g_game_vars.timer == FRAMES(40))
    {
        // Put the "cash out" button onto the round end panel
        main_bg_se_copy_expand_3x3_rect(CASHOUT_DEST_RECT, CASHOUT_SRC_3X3_RECT_POS);

        int cashout_amount = g_game_vars.hands + blind_get_reward(g_game_vars.current_blind) +
                             calculate_interest_reward();

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

        state_machine_change_state(&round_end_sm, DISMISS_ROUND_END_PANEL);
        g_game_vars.timer = TM_ZERO;

        obj_hide(g_game_vars.round_end_blind_token->obj); // Hide the blind token object
        tte_erase_rect_wrapper(BLIND_TOKEN_TEXT_RECT);    // Erase the blind token text
    }
}

static void game_round_end_dismiss_round_end_panel(void)
{
    Rect round_end_down = ROUND_END_MENU_RECT;
    round_end_down.top--;
    main_bg_se_copy_rect_1_tile_vert(round_end_down, SCREEN_DOWN);

    if (g_game_vars.timer >= TM_DISMISS_ROUND_END_TM)
    {
        g_game_vars.timer = TM_ZERO;
        game_change_state(GAME_STATE_SHOP);
    }
}

static void game_round_end_extend_black_panel_down(int black_panel_bottom)
{
    Rect single_line_rect = ROUND_END_MENU_RECT;
    single_line_rect.bottom = black_panel_bottom;
    single_line_rect.top = single_line_rect.bottom - 1;
    main_bg_se_copy_rect_1_tile_vert(single_line_rect, SCREEN_DOWN);
}

void game_round_end_change_background(void)
{
    // Disable window 0 so it doesn't make the cashout menu transparent
    toggle_windows(false, true);

    main_bg_se_clear_rect(ROUND_END_MENU_RECT);
    tte_erase_rect_wrapper(HAND_SIZE_RECT);
}

void game_round_end_on_init(void)
{
    g_game_vars.timer = 0;
    state_machine_register(&round_end_sm);
    state_machine_change_state(&round_end_sm, ROUND_END_START);
}

void game_round_end_on_update(void)
{
    // Substate logic only
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
    state_machine_remove(&round_end_sm);
    // TODO: Reuse sprites for blind selection?
}
