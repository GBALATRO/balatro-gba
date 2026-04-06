#include "blind_select.h"
#include "background_blind_select_gfx.h"

#include "audio_utils.h"
#include "blind.h"
#include "common_ui.h"
#include "timer.h"
#include "game.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "util.h"

#define BLIND_SELECT_BTN_PID                 15
#define BLIND_SELECT_BTN_SELECTED_BORDER_PID 18
#define BLIND_SKIP_BTN_SELECTED_BORDER_PID   10
#define BLIND_SKIP_BTN_PID                   5
#define MENU_POP_OUT_ANIM_FRAMES 20

static const Rect SINGLE_BLIND_SELECT_RECT  = {9,       7,      13,     31 };
static const Rect BLIND_SKIP_BTN_GRAY_RECT  = {0,       24,     4,      27 };
static const Rect BLIND_SKIP_BTN_PREANIM_DEST_RECT = {9,29,     19,     31 };

static void game_blind_select_start_anim_seq(GameVariables* vars);
static void game_blind_select_handle_input(GameVariables* vars);
static void game_blind_select_selected_anim_seq(GameVariables* vars);
static void game_blind_select_display_blind_panel(GameVariables* vars);
static Rect game_blind_select_get_req_score_rect(enum BlindType blind, GameVariables* vars);
static void game_blind_select_print_blinds_reqs_and_rewards(GameVariables* vars);

// Incorrect name, but correct type now. needs the passed pointer
static const GameStateCallback blind_select_state_actions[] = {
    game_blind_select_start_anim_seq,
    game_blind_select_handle_input,
    game_blind_select_selected_anim_seq,
    game_blind_select_display_blind_panel
};

static const Rect POP_MENU_ANIM_RECT = { 9, 7, 24, 31 };
// The sprites that display the blinds when in "GAME_BLIND_SELECT" state

static int selection_x = 0;
static int selection_y = 0;

static int substate;

static enum BackgroundId background;

static void game_blind_select_start_anim_seq(GameVariables* vars)
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        sprite_position(
            vars->blind_select_tokens[i],
            vars->blind_select_tokens[i]->pos.x,
            vars->blind_select_tokens[i]->pos.y - TILE_SIZE
        );
    }

    if (vars->timer == TM_END_ANIM_SEQ)
    {
        game_blind_select_print_blinds_reqs_and_rewards(vars);
        substate = BLIND_SELECT;
        vars->timer = TM_ZERO; // Reset the timer
    }
}

static const Rect SINGLE_BLIND_SEL_REQ_SCORE_RECT = {80, 120,    104,     128  };

static inline void game_blind_select_erase_blind_reqs_and_rewards()
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

static void game_blind_select_handle_input(GameVariables* vars)
{
    if (vars->timer == TM_BLIND_SELECT_START && vars->current_blind == BLIND_TYPE_BOSS)
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
            //play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            substate = BLIND_SELECTED_ANIM_SEQ;
            vars->timer = TM_ZERO;
            ++vars->round;
            display_round();
        }
        else if (vars->current_blind != BLIND_TYPE_BOSS)
        {
            //play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
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
                    vars->blind_select_tokens[i],
                    vars->blind_select_tokens[i]->pos.x,
                    vars->blind_select_tokens[i]->pos.y - (TILE_SIZE * 12)
                );
            }

            game_blind_select_print_blinds_reqs_and_rewards(vars);

            vars->timer = TM_ZERO;
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

static void game_blind_select_selected_anim_seq(GameVariables* vars)
{
    if (vars->timer < 15)
    {
        Rect blinds_rect = POP_MENU_ANIM_RECT;
        blinds_rect.top -= 1; // Because of the raised blind
        main_bg_se_move_rect_1_tile_vert(blinds_rect, SCREEN_DOWN);

        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            sprite_position(
                vars->blind_select_tokens[i],
                vars->blind_select_tokens[i]->pos.x,
                vars->blind_select_tokens[i]->pos.y + TILE_SIZE
            );
        }
    }
    else if (vars->timer >= MENU_POP_OUT_ANIM_FRAMES)
    {
        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            obj_hide(vars->blind_select_tokens[i]->obj);
        }

        substate = DISPLAY_BLIND_PANEL; // Reset the state
        vars->timer = TM_ZERO;                                       // Reset the timer
    }
}

static const Rect ROUND_END_MENU_RECT       = {9,       7,      24,     20 }; 
static const BG_POINT TOP_LEFT_PANEL_EMPTY_3W_ROW_POS = {29, 31};
static const Rect TOP_LEFT_PANEL_ANIM_RECT  = {0,       0,      8,      4  };

static void game_blind_select_display_blind_panel(GameVariables* vars)
{
    if (vars->timer >= TM_DISP_BLIND_PANEL_FINISH)
    {
        substate = BLIND_SELECT_MAX;
        return;
    }

    // Switches to the selecting background and clears the blind panel area
    if (vars->timer == TM_DISP_BLIND_PANEL_START)
    {
        change_background(BG_CARD_SELECTING);

        main_bg_se_clear_rect(ROUND_END_MENU_RECT);

        // Need to clear the top left panel as a side effect of change_background()
        main_bg_se_copy_expand_3w_row(TOP_LEFT_PANEL_ANIM_RECT, TOP_LEFT_PANEL_EMPTY_3W_ROW_POS);

        reset_top_left_panel_bottom_row();
    }

    // Shift the blind panel down onto screen
    for (int y = 0; y < vars->timer; y++)
    {
        int y_from = 26 + y - vars->timer;
        int y_to = 0 + y;

        Rect from = {0, y_from, 8, y_from};
        BG_POINT to = {0, y_to};

        main_bg_se_copy_rect(from, to);
    }
}

static Rect game_blind_select_get_req_score_rect(enum BlindType blind, GameVariables* vars)
{
    Rect blind_req_score_rect = SINGLE_BLIND_SEL_REQ_SCORE_RECT;

    blind_req_score_rect.left += blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;
    blind_req_score_rect.right += blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;

    if (vars->blinds_states[blind] == BLIND_STATE_CURRENT)
    {
        // Current blind is raised
        blind_req_score_rect.top -= TILE_SIZE;
        blind_req_score_rect.bottom -= TILE_SIZE;
    }

    return blind_req_score_rect;
}

static inline void game_blind_select_print_blind_req(enum BlindType blind, GameVariables* vars)
{
    Rect blind_req_score_rect = game_blind_select_get_req_score_rect(blind, vars);

    u32 blind_req = blind_get_requirement(blind, vars->ante);

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

static inline void game_blind_select_print_blind_reward(enum BlindType blind, GameVariables* vars)
{
    int blind_reward = blind_get_reward(blind);
    Rect blind_reward_rect = game_blind_select_get_req_score_rect(blind, vars);

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

static void game_blind_select_print_blinds_reqs_and_rewards(GameVariables* vars)
{
    for (enum BlindType curr_blind = 0; curr_blind < BLIND_TYPE_MAX; curr_blind++)
    {
        game_blind_select_print_blind_req(curr_blind, vars);
        game_blind_select_print_blind_reward(curr_blind, vars);
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

    blind_select_state_actions[substate](vars);
}

void game_blind_select_on_exit(GameVariables* vars)
{
    sprite_destroy(&vars->blind_select_tokens[BLIND_TYPE_SMALL]);
    sprite_destroy(&vars->blind_select_tokens[BLIND_TYPE_BIG]);
    sprite_destroy(&vars->blind_select_tokens[BLIND_TYPE_BOSS]);
}

void game_blind_select_change_background(void)
{
    auto vars = get_game_vars();

    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        obj_unhide(vars->blind_select_tokens[i]->obj, 0);
    }

    // Default y position for the blind select tokens. 12 is the amount of tiles the background
    // is shifted down by
    const int default_y = 89 + (TILE_SIZE * 12);
    // TODO refactor magic numbers '80/120/160' into a map to loop with
    sprite_position(vars->blind_select_tokens[BLIND_TYPE_SMALL], 80, default_y);
    sprite_position(vars->blind_select_tokens[BLIND_TYPE_BIG], 120, default_y);
    sprite_position(vars->blind_select_tokens[BLIND_TYPE_BOSS], 160, default_y);

    toggle_windows(false, true);

    GRIT_CPY(pal_bg_mem, background_blind_select_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_blind_select_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_blind_select_gfxMap);

    // Copy boss blind colors to blind select palette
    memset16(
        &pal_bg_mem[1],
        blind_get_color(BLIND_TYPE_BOSS, BLIND_BACKGROUND_MAIN_COLOR_INDEX),
        1
    );
    memset16(
        &pal_bg_mem[7],
        blind_get_color(BLIND_TYPE_BOSS, BLIND_BACKGROUND_SHADOW_COLOR_INDEX),
        1
    );

    // Disable the button highlight colors
    // Select button PID is 15 and the outline is 18
    memcpy16(
        &pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID],
        &pal_bg_mem[BLIND_SELECT_BTN_PID],
        1
    );
    // It seems the skip button (and score multiplier and deck) PB idx is
    // actually 5, not 10. 10 is the selected border color
    // Setting this palette value though doesn't seem to have an
    // effect.
    memcpy16(
        &pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID],
        &pal_bg_mem[BLIND_SKIP_BTN_PID],
        1
    );

    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        Rect curr_blind_rect = SINGLE_BLIND_SELECT_RECT;

        // There's no gap between them
        curr_blind_rect.left += i * rect_width(&SINGLE_BLIND_SELECT_RECT);
        curr_blind_rect.right += i * rect_width(&SINGLE_BLIND_SELECT_RECT);

        if (vars->blinds_states[i] != BLIND_STATE_CURRENT &&
            (i == BLIND_TYPE_SMALL || i == BLIND_TYPE_BIG)) // Make the skip button gray
        {
            BG_POINT skip_blind_btn_pos_dest = {
                BLIND_SKIP_BTN_PREANIM_DEST_RECT.left,
                BLIND_SKIP_BTN_PREANIM_DEST_RECT.top
            };
            skip_blind_btn_pos_dest.x = curr_blind_rect.left;

            Rect skip_blind_btn_rect_src = BLIND_SKIP_BTN_GRAY_RECT;
            skip_blind_btn_rect_src.top += i * rect_height(&BLIND_SKIP_BTN_GRAY_RECT);
            skip_blind_btn_rect_src.bottom += i * rect_height(&BLIND_SKIP_BTN_GRAY_RECT);

            main_bg_se_copy_rect(skip_blind_btn_rect_src, skip_blind_btn_pos_dest);
        }

        switch (vars->blinds_states[i])
        {
            case BLIND_STATE_CURRENT: // Raise the blind panel up a bit
            {
                // TODO: Replace copies with main_bg_se_copy_rect() of named rects
                int x_from = 0;
                int y_from = 27;

                main_bg_se_copy_rect_1_tile_vert(curr_blind_rect, SCREEN_UP);

                int x_to = curr_blind_rect.left;
                int y_to = 31;

                if (i == BLIND_TYPE_BIG)
                {
                    y_from = 31;
                }
                else if (i == BLIND_TYPE_BOSS)
                {
                    x_from = x_to;
                    y_from = 30;
                }

                // Copy plain tiles onto the bottom of the raised blind panel to fill the gap
                // created by the raise
                Rect gap_fill_rect = {
                    x_from,
                    y_from,
                    x_from + rect_width(&SINGLE_BLIND_SELECT_RECT) - 1,
                    y_from
                };
                BG_POINT gap_fill_point = {x_to, y_to};
                main_bg_se_copy_rect(gap_fill_rect, gap_fill_point);

                // Move token up by a tile
                sprite_position(
                    vars->blind_select_tokens[i],
                    vars->blind_select_tokens[i]->pos.x,
                    vars->blind_select_tokens[i]->pos.y - TILE_SIZE
                );
                break;
            }
            case BLIND_STATE_UPCOMING: // Change the select icon to "NEXT"
            {
                int x_from = 0;
                int y_from = 20;

                int x_to = 10 + (i * rect_width(&SINGLE_BLIND_SELECT_RECT));
                int y_to = 20;

                memcpy16(
                    &se_mem[MAIN_BG_SBB][x_to + 32 * y_to],
                    &se_mem[MAIN_BG_SBB][x_from + 32 * y_from],
                    3
                );
                break;
            }
            case BLIND_STATE_SKIPPED: // Change the select icon to "SKIP"
            {
                int x_from = 3;
                int y_from = 20;

                int x_to = 10 + (i * 5);
                int y_to = 20;

                memcpy16(
                    &se_mem[MAIN_BG_SBB][x_to + 32 * y_to],
                    &se_mem[MAIN_BG_SBB][x_from + 32 * y_from],
                    3
                );
                break;
            }
            case BLIND_STATE_DEFEATED: // Change the select icon to "DEFEATED"
            {
                int x_from = 6;
                int y_from = 20;

                int x_to = 10 + (i * 5);
                int y_to = 20;

                memcpy16(
                    &se_mem[MAIN_BG_SBB][x_to + 32 * y_to],
                    &se_mem[MAIN_BG_SBB][x_from + 32 * y_from],
                    3
                );
                break;
            }
            default:
                break;
        }
    }
}

void increment_blind(enum BlindState increment_reason)
{
    /*
    current_blind++;
    if (game_vars.current_blind >= BLIND_TYPE_MAX)
    {
        current_blind = 0;
        blinds_states[0] = BLIND_STATE_CURRENT;  // Reset the blinds to the first one
        blinds_states[1] = BLIND_STATE_UPCOMING; // Set the next blind to upcoming
        blinds_states[2] = BLIND_STATE_UPCOMING; // Set the next blind to upcoming
    }
    else
    {
        blinds_states[game_vars.current_blind] = BLIND_STATE_CURRENT;
        blinds_states[game_vars.current_blind - 1] = increment_reason;
    }
    */
}
