#include "round_end.h"

#include "affine_background.h"
#include "affine_background_gfx.h"
#include "game.h"
#include "game_variables.h"
#include "layout.h"
#include "skip_tag.h"
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

enum RewardType
{
    REWARD_TYPE_HAND,
    REWARD_TYPE_INVESTMENT,
    REWARD_TYPE_INTEREST,
    REWARD_TYPE_MAX
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

/**
 * @brief Data struct for the different types of rewards.
 *
 * Contains both constant info (display, reward amount), and variables used during the round end
 * animation.
 */
typedef struct RewardTypeData
{
    /**
     * @brief the name of the reward (i.e. "Hands")
     */
    const char* text;

    /**
     * @brief the color the name of the reward is printed with
     */
    const u8 text_color;

    /**
     * @brief how many dollars the player recieves every time a certain type of reward is counted
     * towards the total cashout. Equal to 1 most of the time, but the Investment Tag increments
     * by a full reward amount per tag so the the multiplier is a full amount.
     *
     * @note make this not const if "To the Moon" is ever implemented, since it increases the
     * Interest multiplier from 1 to 2
     */
    const int multiplier;

    /**
     * @brief how many times the reward was earned by the player during the last round
     */
    int count;

    /**
     * @brief how many times the reward still needs to be counted for this End of Round
     */
    int remaining;
} RewardTypeData;

// clang-format off
static RewardTypeData s_reward_data[] = {
    [REWARD_TYPE_HAND] = {
        .text = "Hands",
        .text_color = TTE_BLUE_PB,
        .multiplier = 1,
        .count = 0,
        .remaining = 0
    },
    [REWARD_TYPE_INVESTMENT] = {
        .text = "Boss",
        .text_color = TTE_YELLOW_PB,
        .multiplier = INVESTMENT_TAG_REWARD,
        .count = 0,
        .remaining = 0
    },
    [REWARD_TYPE_INTEREST] = {
        .text = "Interest",
        .text_color = TTE_YELLOW_PB,
        .multiplier = 1,
        .count = 0,
        .remaining = 0
    }
};
// clang-format on

static enum RewardType s_current_reward = 0;
static u32 s_current_reward_start_time = TM_DISPLAY_REWARDS_CONT_WAIT;
static int s_reward_y_offset = 1;

static int s_blind_reward = 0;
static int s_cashout = 0;

static int calculate_interest_reward(void);

static void round_end_start(void);
static void round_end_start_expand_popup(void);
static void round_end_display_finished_blind(void);
static void round_end_display_score_min(void);
static void round_end_update_blind_reward(void);
static void round_end_panel_exit(void);
static void round_end_display_rewards(void);
static void round_end_display_cashout(void);
static void round_end_dismiss_round_end_panel(void);

static void round_end_extend_black_panel_down(int black_panel_bottom);

// clang-format off
static StateInfo state_info[] =
{
    [ROUND_END_START]           = STATE_INFO_UPDATE_FN_ONLY(round_end_start),
    [START_EXPAND_POPUP]        = STATE_INFO_UPDATE_FN_ONLY(round_end_start_expand_popup),
    [DISPLAY_FINISHED_BLIND]    = STATE_INFO_UPDATE_FN_ONLY(round_end_display_finished_blind),
    [DISPLAY_SCORE_MIN]         = STATE_INFO_UPDATE_FN_ONLY(round_end_display_score_min),
    [UPDATE_BLIND_REWARD]       = STATE_INFO_UPDATE_FN_ONLY(round_end_update_blind_reward),
    [BLIND_PANEL_EXIT]          = STATE_INFO_UPDATE_FN_ONLY(round_end_panel_exit),
    [DISPLAY_REWARDS]           = STATE_INFO_UPDATE_FN_ONLY(round_end_display_rewards),
    [DISPLAY_CASHOUT]           = STATE_INFO_UPDATE_FN_ONLY(round_end_display_cashout),
    [DISMISS_ROUND_END_PANEL]   = STATE_INFO_UPDATE_FN_ONLY(round_end_dismiss_round_end_panel),
};
// clang-format on

static StateMachine round_end_sm = STATE_MACHINE_DEFINE(state_info, ROUND_END_STATES_MAX);

static int calculate_interest_reward(void)
{
    int reward = (g_game_vars.money / 5) * INTEREST_PER_5;
    if (reward > MAX_INTEREST)
        reward = MAX_INTEREST;
    return reward;
}

static void round_end_start(void)
{
    // Reset static variables to default values upon re-entering the round end state
    if (g_game_vars.timer == TM_RESET_STATIC_VARS)
    {
        change_background(BG_ROUND_END, false); // Change the background to the round end background

        g_game_vars.timer = TM_ZERO; // Reset the timer
        state_machine_change_state(&round_end_sm, START_EXPAND_POPUP);

        s_current_reward = 0;
        s_current_reward_start_time = TM_DISPLAY_REWARDS_CONT_WAIT;
        s_reward_y_offset = 1;

        s_blind_reward = blind_get_reward(g_game_vars.current_blind);
        s_reward_data[REWARD_TYPE_HAND].count = g_game_vars.hands;
        s_reward_data[REWARD_TYPE_INVESTMENT].count = g_game_vars.current_blind >= BLIND_TYPE_BOSS
                                                        ? skip_tag_count(SKIP_TAG_TYPE_INVESTMENT)
                                                        : 0;
        s_reward_data[REWARD_TYPE_INTEREST].count = calculate_interest_reward();

        s_cashout = blind_get_reward(g_game_vars.current_blind);
        for (enum RewardType i = 0; i < REWARD_TYPE_MAX; i++)
        {
            s_reward_data[i].remaining = s_reward_data[i].count;
            s_cashout += s_reward_data[i].count * s_reward_data[i].multiplier;
        }
    }
}

static void round_end_start_expand_popup(void)
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (g_game_vars.timer == TM_END_POP_MENU_ANIM)
    {
        state_machine_change_state(&round_end_sm, DISPLAY_FINISHED_BLIND);
        g_game_vars.timer = TM_ZERO;
    }
}

static void round_end_display_finished_blind(void)
{
    sprite_unhide(g_game_vars.round_end_blind_token);

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
        round_end_extend_black_panel_down(ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE);
    }

    if (g_game_vars.timer >= TM_END_DISPLAY_FIN_BLIND)
    {
        state_machine_change_state(&round_end_sm, DISPLAY_SCORE_MIN);
        g_game_vars.timer = TM_ZERO;
    }
}

static void round_end_display_score_min(void)
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

static void round_end_update_blind_reward(void)
{
    if (g_game_vars.timer % FRAMES(20) != 0)
        return;

    // TODO: Add sound effect here

    if (s_blind_reward > 0)
    {
        s_blind_reward--;
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            BLIND_REWARD_RECT.left,
            BLIND_REWARD_RECT.top,
            TTE_YELLOW_PB,
            s_blind_reward
        );
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            ROUND_END_BLIND_REWARD_RECT.left,
            ROUND_END_BLIND_REWARD_RECT.top,
            TTE_YELLOW_PB,
            blind_get_reward(g_game_vars.current_blind) - s_blind_reward
        );
    }
    else if (g_game_vars.timer > FRAMES(20))
    {
        tte_erase_rect_wrapper(BLIND_REWARD_RECT);
        tte_erase_rect_wrapper(BLIND_REQ_TEXT_RECT);
        sprite_hide(g_game_vars.playing_blind_token);
        affine_background_load_palette(affine_background_gfxPal);
        state_machine_change_state(&round_end_sm, BLIND_PANEL_EXIT);
        g_game_vars.timer = TM_ZERO;
    }
}

static void round_end_panel_exit(void)
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

static inline void round_end_print_separator_ellipsis(void)
{
    int x =
        (ROUND_END_REWARDS_ELLIPSIS_POS.x + g_game_vars.timer - TM_REWARDS_ELLIPSIS_PRINT_START) *
        TILE_SIZE;
    int y = (ROUND_END_REWARDS_ELLIPSIS_POS.y) * TILE_SIZE;

    tte_printf("#{P:%d,%d; cx:0x%X000}.", x, y, TTE_WHITE_PB);
}

static inline bool s_should_increment_curr_reward(void)
{
    switch (s_current_reward)
    {
        // For Hands and Interest rewards, updating the values on screen
        // depends only on the timer
        case REWARD_TYPE_HAND:
        case REWARD_TYPE_INTEREST:
            return g_game_vars.timer % FRAMES(TM_REWARD_INCREMENT_INTERVAL) == 0;

        // For the Investment Tag reward, updating the values on screen will
        // rely on the Tag processing stage
        case REWARD_TYPE_INVESTMENT:
            return skip_tag_process_get_proc_stage() == SKIP_TAG_PROCESS_STAGE_TRIGGER;

        default:
            return true;
    }
}

static inline void round_end_print_current_reward(void)
{
    int reward_y = ROUND_END_REWARDS_ELLIPSIS_POS.y + s_reward_y_offset;
    if (g_game_vars.timer == s_current_reward_start_time)
    {
        round_end_extend_black_panel_down(reward_y);

        tte_printf(
            "#{P:%lu,%d; cx:0x%X000}%d #{cx:0x%X000}%s",
            ROUND_END_REWARD_TEXT_X,
            reward_y * TILE_SIZE,
            s_reward_data[s_current_reward].text_color,
            s_reward_data[s_current_reward].count,
            TTE_WHITE_PB,
            s_reward_data[s_current_reward].text
        );

        if (s_current_reward == REWARD_TYPE_INVESTMENT)
            skip_tag_process_start(SKIP_TAG_EVENT_ON_ROUND_END);
    }

    // Increment the reward text until the reward variable is depleted
    else if (g_game_vars.timer > s_current_reward_start_time + TM_REWARD_DISPLAY_INTERVAL)
    {
        if (!s_should_increment_curr_reward())
            return;

        s_reward_data[s_current_reward].remaining--;
        int accumulated_reward =
            s_reward_data[s_current_reward].count - s_reward_data[s_current_reward].remaining;

        tte_printf(
            "#{P:%lu, %d; cx:0x%X000}$%d",
            ROUND_END_REWARD_AMOUNT_X,
            reward_y * TILE_SIZE,
            TTE_YELLOW_PB,
            accumulated_reward * s_reward_data[s_current_reward].multiplier
        );

        if (s_reward_data[s_current_reward].remaining <= 0)
        {
            s_current_reward++;
            s_reward_y_offset++;
            s_current_reward_start_time = g_game_vars.timer + TM_REWARD_INCREMENT_INTERVAL;
        }
    }
}

static void round_end_display_rewards(void)
{
    // Once all rewards are accounted for go to the next state
    if (s_current_reward >= REWARD_TYPE_MAX)
    {
        g_game_vars.timer = TM_ZERO;
        state_machine_change_state(&round_end_sm, DISPLAY_CASHOUT);
        return;
    }

    if (g_game_vars.timer == TM_START_ROUND_END_REWARDS_ANIM)
    {
        round_end_extend_black_panel_down(ROUND_END_REWARDS_ELLIPSIS_POS.y);
    }
    else if (g_game_vars.timer < TM_REWARDS_ELLIPSIS_PRINT_END)
    {
        round_end_print_separator_ellipsis();
    }
    else if (g_game_vars.timer >= TM_DISPLAY_REWARDS_CONT_WAIT)
    {
        // Go to the next reward if the current one has no remaining money to give
        while (s_current_reward < REWARD_TYPE_MAX && s_reward_data[s_current_reward].remaining <= 0)
            s_current_reward++;

        if (s_current_reward < REWARD_TYPE_MAX)
            round_end_print_current_reward();
    }
}

static inline void round_end_cashout(void)
{
    // Reward the player
    g_game_vars.money += s_cashout;
    display_money();

    g_game_vars.hands = MAX_HANDS;       // Reset the hands to the maximum
    g_game_vars.discards = MAX_DISCARDS; // Reset the discards to the maximum
    // TODO: these can just be in one spot, passing global to global
    display_hands();    // Set the hands display
    display_discards(); // Set the discards display

    g_game_vars.score = 0;
    display_score(g_game_vars.score); // Set the score display
}

static void round_end_display_cashout()
{
    if (g_game_vars.timer == FRAMES(40))
    {
        // Put the "cash out" button onto the round end panel
        main_bg_se_copy_expand_3x3_rect(CASHOUT_DEST_RECT, CASHOUT_SRC_3X3_RECT_POS);

        bool omit_space = s_cashout >= 10;
        tte_printf(
            "#{P:%d, %d; cx:0x%X000}Cash Out:%s$%d",
            CASHOUT_TEXT_RECT.left,
            CASHOUT_TEXT_RECT.top,
            TTE_WHITE_PB,
            omit_space ? "" : " ",
            s_cashout
        );
    }

    // Wait until the player presses A to cash out
    else if (g_game_vars.timer > FRAMES(40) && key_hit(KEY_CONFIRM))
    {
        round_end_cashout();

        state_machine_change_state(&round_end_sm, DISMISS_ROUND_END_PANEL);
        g_game_vars.timer = TM_ZERO;

        sprite_hide(g_game_vars.round_end_blind_token); // Hide the blind token object
        tte_erase_rect_wrapper(BLIND_TOKEN_TEXT_RECT);  // Erase the blind token text
    }
}

static void round_end_dismiss_round_end_panel(void)
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

static void round_end_extend_black_panel_down(int black_panel_bottom)
{
    Rect single_line_rect = ROUND_END_MENU_RECT;
    single_line_rect.bottom = black_panel_bottom;
    single_line_rect.top = single_line_rect.bottom - 1;
    main_bg_se_copy_rect_1_tile_vert(single_line_rect, SCREEN_DOWN);
}

void round_end_change_background(void)
{
    // Disable window 0 so it doesn't make the cashout menu transparent
    toggle_windows(false, true);

    main_bg_se_clear_rect(ROUND_END_MENU_RECT);
    tte_erase_rect_wrapper(HAND_SIZE_RECT);
}

void round_end_on_init(void)
{
    // Reset hand size in case it was increased by the Juggle Tag
    g_game_vars.hand_size = DEFAULT_HAND_SIZE;
    g_game_vars.timer = 0;
    g_game_vars.nb_unused_discards += g_game_vars.discards;
    state_machine_register(&round_end_sm);
    state_machine_change_state(&round_end_sm, ROUND_END_START);
}

void round_end_on_update(void)
{
    // Substate logic only
}

void round_end_on_exit(void)
{
    for (enum RewardType i = 0; i < REWARD_TYPE_MAX; i++)
    {
        s_reward_data[i].count = 0;
        s_reward_data[i].remaining = 0;
    }
    s_blind_reward = 0;
    s_cashout = 0;

    // Cleanup blind tokens from this round to avoid accumulating
    // allocated blind sprites each round
    sprite_destroy(&g_game_vars.playing_blind_token);
    sprite_destroy(&g_game_vars.round_end_blind_token);
    state_machine_remove(&round_end_sm);
    // TODO: Reuse sprites for blind selection?
}
