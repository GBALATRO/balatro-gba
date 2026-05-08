#include "round_end.h"

#include "game_variables.h"
#include "timer.h"

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

void game_round_end_start(void)
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

void game_round_end_on_update(void)
{
    if (state_info[game_state].substate == ROUND_END_EXIT)
    {
        game_change_state(GAME_STATE_SHOP);
        return;
    }

    int substate = state_info[game_state].substate;
    round_end_state_actions[substate]();
}

void game_round_end_on_exit(void)
{
    // Cleanup blind tokens from this round to avoid accumulating
    // allocated blind sprites each round
    blind_reward = 0;
    hand_reward = 0;
    interest_reward = 0;
    sprite_destroy(&playing_blind_token);
    sprite_destroy(&round_end_blind_token);
    // TODO: Reuse sprites for blind selection?
}
