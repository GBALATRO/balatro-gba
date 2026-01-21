#include "game/round_end.h"

#include "affine_background.h"
#include "affine_background_gfx.h"
#include "blind.h"
#include "game.h"
#include "game/common_ui.h"
#include "game/palette.h"
#include "game/rect.h"
#include "game/timer.h"
#include "graphic_utils.h"
#include "tonc_memdef.h"
#include "util.h"

#include <stdio.h>
#include <tonc.h>

#define ROUND_END_REWARD_AMOUNT_X            168
#define ROUND_END_REWARD_TEXT_X              88
#define ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE 12

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

// Module-local variables
static int blind_reward = 0;
static int hand_reward = 0;
static int interest_reward = 0;
static int interest_to_count = 0;
static int interest_start_time = UNDEFINED;

// Forward declarations
static void game_round_end_start(RoundEndProps* props);
static void game_round_end_start_expand_popup(RoundEndProps* props);
static void game_round_end_display_finished_blind(RoundEndProps* props);
static void game_round_end_display_score_min(RoundEndProps* props);
static void game_round_end_update_blind_reward(RoundEndProps* props);
static void game_round_end_panel_exit(RoundEndProps* props);
static void game_round_end_display_rewards(RoundEndProps* props);
static void game_round_end_display_cashout(RoundEndProps* props);
static void game_round_end_dismiss_round_end_panel(RoundEndProps* props);

typedef void (*SubStateActionFn)(RoundEndProps* props);
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

int calculate_interest_reward(int money)
{
    int reward = (money / 5) * INTEREST_PER_5;
    if (reward > MAX_INTEREST)
        reward = MAX_INTEREST;
    return reward;
}

void game_round_end_on_exit(void* ctx)
{
    // Cleanup blind tokens from this round to avoid accumulating
    // allocated blind sprites each round
    blind_reward = 0;
    hand_reward = 0;
    interest_reward = 0;
    destroy_playing_and_round_end_blind_tokens();
    // TODO: Reuse sprites for blind selection?
}

void game_round_end_on_update(void* ctx)
{
    RoundEndProps* props = (RoundEndProps*)ctx;
    int substate = props->substate;

    if (substate == ROUND_END_EXIT)
    {
        game_change_state(GAME_STATE_SHOP);
        return;
    }

    round_end_state_actions[substate](props);
}

static void game_round_end_start(RoundEndProps* props)
{
    // Reset static variables to default values upon re-entering the round end state
    if (props->timer == TM_RESET_STATIC_VARS)
    {
        change_background(BG_ROUND_END); // Change the background to the round end background
        props->substate = START_EXPAND_POPUP;
        props->timer = TM_ZERO; // Reset the timer
        blind_reward = blind_get_reward(props->current_blind);
        hand_reward = props->hands;
        interest_reward = calculate_interest_reward(props->money);
        interest_to_count = interest_reward;
        interest_start_time = UNDEFINED;
    }
}

static void game_round_end_start_expand_popup(RoundEndProps* props)
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (props->timer == TM_END_POP_MENU_ANIM)
    {
        props->substate = DISPLAY_FINISHED_BLIND;
        props->timer = TM_ZERO;
    }
}

static void game_round_end_extend_black_panel_down(int black_panel_bottom)
{
    Rect single_line_rect = ROUND_END_MENU_RECT;
    single_line_rect.bottom = black_panel_bottom;
    single_line_rect.top = single_line_rect.bottom - 1;
    main_bg_se_copy_rect_1_tile_vert(single_line_rect, SCREEN_DOWN);
}

static void game_round_end_display_finished_blind(RoundEndProps* props)
{
    unhide_round_end_blind_token();

    // Beating the boss blind increases the ante, so we need to display the previous ante value
    if (props->current_blind == BLIND_TYPE_BOSS)
        props->ante--;

    Rect blind_req_rect = ROUND_END_BLIND_REQ_RECT;
    u32 blind_req = blind_get_requirement(props->current_blind, props->ante);

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

    if (props->timer == TM_START_ROUND_END_REWARDS_ANIM)
    {
        game_round_end_extend_black_panel_down(ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE);
    }

    if (props->timer >= TM_END_DISPLAY_FIN_BLIND)
    {
        props->substate = DISPLAY_SCORE_MIN;
        props->timer = TM_ZERO;
    }
}

static void game_round_end_display_score_min(RoundEndProps* props)
{
    uint timer = props->timer;
    const int timer_offset = timer - 1;
    const int x_from = 0;
    const int y_from = 29;
    const int x_to = 13;
    const int y_to = 11;

    memcpy16(
        &se_mem[MAIN_BG_SBB][x_to + timer_offset + 32 * y_to],
        &se_mem[MAIN_BG_SBB][x_from + timer_offset + 32 * y_from],
        1
    );

    if (timer >= TM_END_DISPLAY_SCORE_MIN)
    {
        props->substate = UPDATE_BLIND_REWARD;
        props->timer = TM_ZERO;
    }
}

static void game_round_end_update_blind_reward(RoundEndProps* props)
{
    uint timer = props->timer;

    if (timer % logical_frames_to_real(20) != 0)
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
            blind_get_reward(props->current_blind) - blind_reward
        );
    }
    else if (timer > logical_frames_to_real(20))
    {
        tte_erase_rect_wrapper(BLIND_REWARD_RECT);
        tte_erase_rect_wrapper(BLIND_REQ_TEXT_RECT);
        hide_playing_blind_token();
        affine_background_load_palette(affine_background_gfxPal);
        props->substate = BLIND_PANEL_EXIT;
        props->timer = TM_ZERO;
    }
}

static void game_round_end_panel_exit(RoundEndProps* props)
{
    // TODO: make heads or tails of what's going on here and replace
    // magic numbers.
    uint timer = props->timer;
    if (timer < 8)
    {
        main_bg_se_copy_rect_1_tile_vert(TOP_LEFT_PANEL_ANIM_RECT, SCREEN_UP);

        if (timer == 1) // Copied from shop. Feels slightly too niche of a function for me
                        // personally to make one.
        {
            reset_top_left_panel_bottom_row();
        }
        else if (timer == 2)
        {
            int y = 5;
            memset16(&se_mem[MAIN_BG_SBB][32 * (y - 1)], 0x0001, 1);
            memset16(&se_mem[MAIN_BG_SBB][1 + 32 * (y - 1)], 0x0002, 7);
            memset16(&se_mem[MAIN_BG_SBB][8 + 32 * (y - 1)], 0x0401, 1);
        }
    }
    else if (timer > logical_frames_to_real(20))
    {
        memset16(&pal_bg_mem[REWARD_PANEL_BORDER_PID], 0x1483, 1);
        props->substate = DISPLAY_REWARDS;
        props->timer = TM_ZERO;
    }
}

static void game_round_end_print_separator_ellipsis(uint timer)
{
    int x =
        (ROUND_END_REWARDS_ELLIPSIS_POS.x + timer - TM_REWARDS_ELLIPSIS_PRINT_START) * TILE_SIZE;
    int y = (ROUND_END_REWARDS_ELLIPSIS_POS.y) * TILE_SIZE;

    tte_printf("#{P:%d,%d; cx:0x%X000}.", x, y, TTE_WHITE_PB);
}

// TODO: Allow for more generic rewards and consolidate with game_round_end_print_interest_reward()
static void game_round_end_print_hand_reward(RoundEndProps* props, int hand_y_offset)
{
    uint timer = props->timer;
    int hand_y = ROUND_END_REWARDS_ELLIPSIS_POS.y + hand_y_offset;
    if (timer == TM_DISPLAY_REWARDS_CONT_WAIT)
    {
        game_round_end_extend_black_panel_down(hand_y);

        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%d #{cx:0x%X000}Hands",
            ROUND_END_REWARD_TEXT_X,
            hand_y * TILE_SIZE,
            TTE_BLUE_PB,
            hand_reward,
            TTE_WHITE_PB
        );
    }
    // Increment the hand reward text until the hand reward variable is depleted
    else if (timer > TM_HAND_REWARD_INCR_WAIT &&
             timer % logical_frames_to_real(TM_REWARD_INCREMENT_INTERVAL) == 0)
    {
        hand_reward--;
        tte_printf(
            "#{P:%d, %d; cx:0x%X000}$%d",
            ROUND_END_REWARD_AMOUNT_X,
            hand_y * TILE_SIZE,
            TTE_YELLOW_PB,
            props->hands - hand_reward
        );
        if (hand_reward == 0)
        {
            interest_start_time = timer + TM_REWARD_DISPLAY_INTERVAL;
        }
    }
}

static void game_round_end_print_interest_reward(uint timer, int interest_y_offset)
{
    int interest_y = ROUND_END_REWARDS_ELLIPSIS_POS.y + interest_y_offset;

    if (timer == interest_start_time)
    {
        game_round_end_extend_black_panel_down(interest_y);

        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%d #{cx:0x%X000}Interest",
            ROUND_END_REWARD_TEXT_X,
            interest_y * TILE_SIZE,
            TTE_YELLOW_PB,
            interest_reward,
            TTE_WHITE_PB
        );
    }
    // Increment the interest reward text until the interest reward variable is depleted
    else if (timer > interest_start_time + TM_REWARD_DISPLAY_INTERVAL &&
             timer % logical_frames_to_real(TM_REWARD_INCREMENT_INTERVAL) == 0)
    {
        interest_to_count--;
        tte_printf(
            "#{P:%d, %d; cx:0x%X000}$%d",
            ROUND_END_REWARD_AMOUNT_X,
            interest_y * TILE_SIZE,
            TTE_YELLOW_PB,
            interest_reward - interest_to_count
        );
    }
}

static void game_round_end_display_rewards(RoundEndProps* props)
{
    uint timer = props->timer;
    int hand_y_offset = 0;
    int interest_y_offset = 0;

    if (props->hands > 0)
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
        props->timer = TM_ZERO;
        props->substate = DISPLAY_CASHOUT;
    }
    else if (timer == TM_START_ROUND_END_REWARDS_ANIM)
    {
        game_round_end_extend_black_panel_down(ROUND_END_REWARDS_ELLIPSIS_POS.y);
    }
    else if (timer < TM_REWARDS_ELLIPSIS_PRINT_END)
    {
        game_round_end_print_separator_ellipsis(props->timer);
    }
    else if (timer >= TM_DISPLAY_REWARDS_CONT_WAIT && hand_reward > 0)
    {
        game_round_end_print_hand_reward(props, hand_y_offset);
    }
    else if (interest_start_time != UNDEFINED && timer >= interest_start_time &&
             interest_to_count > 0)
    {
        game_round_end_print_interest_reward(props->timer, interest_y_offset);
    }
}

static void game_round_end_cashout(RoundEndProps* props)
{
    // Reward the player
    props->money += props->hands + blind_get_reward(props->current_blind) +
                    calculate_interest_reward(props->money);
    update_game_state_ctx(GAME_STATE_ROUND_END); // Sync the global state with the updated context
    display_money();

    props->hands = props->max_hands;       // Reset the hands to the maximum
    props->discards = props->max_discards; // Reset the discards to the maximum
    display_hands();                       // Set the hands display
    display_discards();                    // Set the discards display

    props->score = 0;            // Reset the score to 0
    display_score(props->score); // Set the score display
}

static void game_round_end_display_cashout(RoundEndProps* props)
{
    uint timer = props->timer;
    if (timer == logical_frames_to_real(40))
    {
        // Put the "cash out" button onto the round end panel
        main_bg_se_copy_expand_3x3_rect(CASHOUT_DEST_RECT, CASHOUT_SRC_3X3_RECT_POS);

        int cashout_amount = props->hands + blind_get_reward(props->current_blind) +
                             calculate_interest_reward(props->money);

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
    else if (timer > logical_frames_to_real(40) && key_hit(SELECT_CARD))
    {
        game_round_end_cashout(props);

        props->substate = DISMISS_ROUND_END_PANEL; // Go to the next state
        props->timer = TM_ZERO;                    // Reset the timer

        hide_round_end_blind_token();                  // Hide the blind token object
        tte_erase_rect_wrapper(BLIND_TOKEN_TEXT_RECT); // Erase the blind token text
    }
}

static void game_round_end_dismiss_round_end_panel(RoundEndProps* props)
{
    Rect round_end_down = ROUND_END_MENU_RECT;
    round_end_down.top--;
    main_bg_se_copy_rect_1_tile_vert(round_end_down, SCREEN_DOWN);

    if (props->timer >= TM_DISMISS_ROUND_END_TM)
    {
        props->timer = TM_ZERO;
        props->substate = ROUND_END_EXIT;
    }
}

void game_round_end_change_background(enum BackgroundId current_background)
{
    if (current_background != BG_CARD_SELECTING && current_background != BG_CARD_PLAYING)
    {
        change_background(BG_CARD_SELECTING);
    }

    // Disable window 0 so it doesn't make the cashout menu transparent
    toggle_windows(false, true);

    main_bg_se_clear_rect(ROUND_END_MENU_RECT);
    tte_erase_rect_wrapper(HAND_SIZE_RECT);
}