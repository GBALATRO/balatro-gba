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
static void game_round_end_start(void);
static void game_round_end_start_expand_popup(void);
static void game_round_end_display_finished_blind(void);
static void game_round_end_display_score_min(void);
static void game_round_end_update_blind_reward(void);
static void game_round_end_panel_exit(void);
static void game_round_end_display_rewards(void);
static void game_round_end_display_cashout(void);
static void game_round_end_dismiss_round_end_panel(void);

typedef void (*SubStateActionFn)(void);
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

int calculate_interest_reward(void)
{
    int reward = (get_money() / 5) * INTEREST_PER_5;
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
    int substate = get_substate();
    if (substate == ROUND_END_EXIT)
    {
        game_change_state(GAME_STATE_SHOP);
        return;
    }

    substate = get_substate();
    round_end_state_actions[substate]();
}

static void game_round_end_start()
{
    // Reset static variables to default values upon re-entering the round end state
    if (get_timer() == TM_RESET_STATIC_VARS)
    {
        change_background(BG_ROUND_END);  // Change the background to the round end background
        set_substate(START_EXPAND_POPUP); // Change the state to the next one
        reset_timer();                    // Reset the timer
        blind_reward = blind_get_reward(get_current_blind());
        hand_reward = get_hands();
        interest_reward = calculate_interest_reward();
        interest_to_count = interest_reward;
        interest_start_time = UNDEFINED;
    }
}

static void game_round_end_start_expand_popup()
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (get_timer() == TM_END_POP_MENU_ANIM)
    {
        set_substate(DISPLAY_FINISHED_BLIND);
        reset_timer();
    }
}

static void game_round_end_extend_black_panel_down(int black_panel_bottom)
{
    Rect single_line_rect = ROUND_END_MENU_RECT;
    single_line_rect.bottom = black_panel_bottom;
    single_line_rect.top = single_line_rect.bottom - 1;
    main_bg_se_copy_rect_1_tile_vert(single_line_rect, SCREEN_DOWN);
}

static void game_round_end_display_finished_blind()
{
    Sprite* round_end_blind_token = get_round_end_blind_token();
    obj_unhide(round_end_blind_token->obj, 0);

    int current_ante = get_ante();
    int current_blind = get_current_blind();

    // Beating the boss blind increases the ante, so we need to display the previous ante value
    if (current_blind == BLIND_TYPE_BOSS)
        current_ante--;

    Rect blind_req_rect = ROUND_END_BLIND_REQ_RECT;
    u32 blind_req = blind_get_requirement(current_blind, current_ante);

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

    if (get_timer() == TM_START_ROUND_END_REWARDS_ANIM)
    {
        game_round_end_extend_black_panel_down(ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE);
    }

    if (get_timer() >= TM_END_DISPLAY_FIN_BLIND)
    {
        set_substate(DISPLAY_SCORE_MIN);
        reset_timer();
    }
}

static void game_round_end_display_score_min()
{
    uint timer = get_timer();
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
        set_substate(UPDATE_BLIND_REWARD);
        reset_timer();
    }
}

static void game_round_end_update_blind_reward()
{
    uint timer = get_timer();

    if (timer % FRAMES(20) != 0)
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
            blind_get_reward(get_current_blind()) - blind_reward
        );
    }
    else if (timer > FRAMES(20))
    {
        tte_erase_rect_wrapper(BLIND_REWARD_RECT);
        tte_erase_rect_wrapper(BLIND_REQ_TEXT_RECT);
        Sprite* playing_blind_token = get_playing_blind_token();
        obj_hide(playing_blind_token->obj);
        affine_background_load_palette(affine_background_gfxPal);
        set_substate(BLIND_PANEL_EXIT);
        reset_timer();
    }
}

static void game_round_end_panel_exit()
{
    // TODO: make heads or tails of what's going on here and replace
    // magic numbers.
    uint timer = get_timer();
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
    else if (timer > FRAMES(20))
    {
        memset16(&pal_bg_mem[REWARD_PANEL_BORDER_PID], 0x1483, 1);
        set_substate(DISPLAY_REWARDS);
        reset_timer();
    }
}

static void game_round_end_print_separator_ellipsis(void)
{
    int x = (ROUND_END_REWARDS_ELLIPSIS_POS.x + get_timer() - TM_REWARDS_ELLIPSIS_PRINT_START) *
            TILE_SIZE;
    int y = (ROUND_END_REWARDS_ELLIPSIS_POS.y) * TILE_SIZE;

    tte_printf("#{P:%d,%d; cx:0x%X000}.", x, y, TTE_WHITE_PB);
}

// TODO: Allow for more generic rewards and consolidate with game_round_end_print_interest_reward()
static void game_round_end_print_hand_reward(int hand_y_offset)
{
    uint timer = get_timer();
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
    else if (timer > TM_HAND_REWARD_INCR_WAIT && timer % FRAMES(TM_REWARD_INCREMENT_INTERVAL) == 0)
    {
        hand_reward--;
        tte_printf(
            "#{P:%d, %d; cx:0x%X000}$%d",
            ROUND_END_REWARD_AMOUNT_X,
            hand_y * TILE_SIZE,
            TTE_YELLOW_PB,
            get_hands() - hand_reward
        );
        if (hand_reward == 0)
        {
            interest_start_time = timer + TM_REWARD_DISPLAY_INTERVAL;
        }
    }
}

static void game_round_end_print_interest_reward(int interest_y_offset)
{
    uint timer = get_timer();
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
             timer % FRAMES(TM_REWARD_INCREMENT_INTERVAL) == 0)
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

static void game_round_end_display_rewards()
{
    uint timer = get_timer();
    int hand_y_offset = 0;
    int interest_y_offset = 0;

    if (get_hands() > 0)
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
        reset_timer();
        set_substate(DISPLAY_CASHOUT);
    }
    else if (timer == TM_START_ROUND_END_REWARDS_ANIM)
    {
        game_round_end_extend_black_panel_down(ROUND_END_REWARDS_ELLIPSIS_POS.y);
    }
    else if (timer < TM_REWARDS_ELLIPSIS_PRINT_END)
    {
        game_round_end_print_separator_ellipsis();
    }
    else if (timer >= TM_DISPLAY_REWARDS_CONT_WAIT && hand_reward > 0)
    {
        game_round_end_print_hand_reward(hand_y_offset);
    }
    else if (interest_start_time != UNDEFINED && timer >= interest_start_time &&
             interest_to_count > 0)
    {
        game_round_end_print_interest_reward(interest_y_offset);
    }
}

static void game_round_end_cashout(void)
{
    int hands = get_hands();
    int current_blind = get_current_blind();

    // Reward the player
    increase_money(hands + blind_get_reward(current_blind) + calculate_interest_reward());
    display_money();

    reset_hands();      // Reset the hands to the maximum
    reset_discards();   // Reset the discards to the maximum
    display_hands();    // Set the hands display
    display_discards(); // Set the discards display

    reset_score();              // Reset the score to 0
    display_score(get_score()); // Set the score display
}

static void game_round_end_display_cashout()
{
    uint timer = get_timer();
    if (timer == FRAMES(40))
    {
        // Put the "cash out" button onto the round end panel
        main_bg_se_copy_expand_3x3_rect(CASHOUT_DEST_RECT, CASHOUT_SRC_3X3_RECT_POS);

        int cashout_amount =
            get_hands() + blind_get_reward(get_current_blind()) + calculate_interest_reward();

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
    else if (timer > FRAMES(40) && key_hit(SELECT_CARD))
    {
        game_round_end_cashout();

        set_substate(DISMISS_ROUND_END_PANEL); // Go to the next state
        reset_timer();                         // Reset the timer

        Sprite* round_end_blind_token = get_round_end_blind_token();
        obj_hide(round_end_blind_token->obj);          // Hide the blind token object
        tte_erase_rect_wrapper(BLIND_TOKEN_TEXT_RECT); // Erase the blind token text
    }
}

static void game_round_end_dismiss_round_end_panel()
{
    Rect round_end_down = ROUND_END_MENU_RECT;
    round_end_down.top--;
    main_bg_se_copy_rect_1_tile_vert(round_end_down, SCREEN_DOWN);

    if (get_timer() >= TM_DISMISS_ROUND_END_TM)
    {
        reset_timer();
        set_substate(ROUND_END_EXIT);
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