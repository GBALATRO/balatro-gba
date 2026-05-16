#include "blind_select.h"

#include "audio_utils.h"
#include "background_blind_select_gfx.h"
#include "blind.h"
#include "button.h"
#include "game.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "layout.h"
#include "soundbank.h"
#include "sprite.h"
#include "timer.h"
#include "util.h"

#include <maxmod.h>

#define BLIND_SELECT_BTN_PID                 15
#define TM_DISP_BLIND_PANEL_FINISH           7
#define TM_DISP_BLIND_PANEL_START            1
#define BLIND_SKIP_BTN_PID                   5
#define BLIND_SKIP_BTN_SELECTED_BORDER_PID   10
#define BLIND_SELECT_BTN_SELECTED_BORDER_PID 18

static int timer;

static void game_blind_select_start_anim_seq(void);
static void game_blind_select_handle_input(void);
static void game_blind_select_selected_anim_seq(void);
static void game_blind_select_display_blind_panel(void);
static Rect game_blind_select_get_req_score_rect(enum BlindTokens blind);
static void game_blind_select_print_blinds_reqs_and_rewards(void);
static enum BlindType get_blind_type_from_token(enum BlindTokens blind);
static void blind_tokens_init(void);

enum BlindSelectState
{
    START_ANIM_SEQ,
    BLIND_SELECT,
    BLIND_SELECTED_ANIM_SEQ,
    DISPLAY_BLIND_PANEL,
    BLIND_SELECT_MAX
};

// TODO: this will be refactored into common state machine
static const SubStateActionFn blind_select_state_actions[] = {
    game_blind_select_start_anim_seq,
    game_blind_select_handle_input,
    game_blind_select_selected_anim_seq,
    game_blind_select_display_blind_panel
};

// clang-format off
// Points                                                x        y
static const BG_POINT TOP_LEFT_PANEL_EMPTY_3W_ROW_POS = {29,      31};
// Rects                                                 left     top     right   bottom
static const Rect BLIND_SKIP_BTN_GRAY_RECT            = {0,       24,     4,      27};
static const Rect BLIND_SKIP_BTN_PREANIM_DEST_RECT    = {9,       29,     19,     31};
static const Rect SINGLE_BLIND_SEL_REQ_SCORE_RECT     = {80,      120,    104,    128};
static const Rect SINGLE_BLIND_SELECT_RECT            = {9,       7,      13,     31};
// clang-format on

static const u32 TM_END_ANIM_SEQ = 12;
static const u32 TM_BLIND_SELECT_START = 1;

static const u32 BLIND_LEFT_X = 80;
static const u32 BLIND_CENTER_X = 120;
static const u32 BLIND_RIGHT_X = 160;

static const u32 BLIND_ROW = 0;
static const u32 SKIP_ROW = 1;

static int selection_x = 0;
static int selection_y = 0;

static enum BlindSelectState substate;

static Sprite* blind_select_tokens[NUM_BLINDS_PER_ANTE] = {NULL};

static void game_blind_select_start_anim_seq()
{
    // main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    for (int i = 0; i < NUM_BLINDS_PER_ANTE; i++)
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

static inline void game_blind_select_erase_blind_reqs_and_rewards()
{
    for (enum BlindTokens curr_blind = SMALL_BLIND; curr_blind < NUM_BLINDS_PER_ANTE; curr_blind++)
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

void increment_blind(enum BlindState increment_reason)
{
    switch (g_game_vars.current_blind)
    {
        // defeated small blind: go to big
        case BLIND_TYPE_SMALL:
            g_game_vars.current_blind = BLIND_TYPE_BIG;
            g_game_vars.blinds_states[SMALL_BLIND] = increment_reason;
            g_game_vars.blinds_states[BIG_BLIND] = BLIND_STATE_CURRENT;
            break;
        // defeated big blind: go to next boss
        case BLIND_TYPE_BIG:
            g_game_vars.current_blind = g_game_vars.next_boss_blind;
            g_game_vars.blinds_states[BIG_BLIND] = increment_reason;
            g_game_vars.blinds_states[BOSS_BLIND] = BLIND_STATE_CURRENT;
            break;
        // defeated a boss: reset everything
        default:
            g_game_vars.current_blind = BLIND_TYPE_SMALL;
            g_game_vars.blinds_states[SMALL_BLIND] = BLIND_STATE_CURRENT;
            g_game_vars.blinds_states[BIG_BLIND] = BLIND_STATE_UPCOMING;
            g_game_vars.blinds_states[BOSS_BLIND] = BLIND_STATE_UPCOMING;
            break;
    }
}

// TODO: convert these to proper buttons.
static inline void highlight_select_button(void)
{
    memset16(&pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID], 0xFFFF, 1);
    memcpy16(&pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID], &pal_bg_mem[BLIND_SKIP_BTN_PID], 1);
}

static inline void highlight_skip_button(void)
{
    memcpy16(
        &pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID],
        &pal_bg_mem[BLIND_SELECT_BTN_PID],
        1
    );
    memset16(&pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID], 0xFFFF, 1);
}

static void game_blind_select_handle_input()
{
    if (timer == TM_BLIND_SELECT_START && g_game_vars.current_blind == BLIND_TYPE_BOSS)
    {
        selection_y = BLIND_ROW;
    }

    // Blind select input logic
    if (key_hit(KEY_UP))
    {
        selection_y = BLIND_ROW;
        highlight_select_button();
    }
    else if (key_hit(KEY_DOWN) && g_game_vars.current_blind <= BLIND_TYPE_BIG)
    {
        selection_y = SKIP_ROW;
        highlight_skip_button();
    }
    else if (key_hit(SELECT_CARD))
    {
        game_blind_select_erase_blind_reqs_and_rewards();

        switch (selection_y)
        {
            case BLIND_ROW:
                play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
                substate = BLIND_SELECTED_ANIM_SEQ;
                timer = TM_ZERO;
                ++g_game_vars.round;
                display_round();
                break;
            case SKIP_ROW:
                if (g_game_vars.current_blind <= BLIND_TYPE_BIG)
                {
                    play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
                    increment_blind(BLIND_STATE_SKIPPED);

                    selection_y = BLIND_ROW; // Reset selection to first option

                    change_background(BG_BLIND_SELECT, true);

                    // TODO: Create a generic vertical move by any number of tiles to avoid for
                    // loops?
                    for (int i = 0; i < 12; i++)
                    {
                        main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);
                    }

                    for (int i = 0; i < NUM_BLINDS_PER_ANTE; i++)
                    {
                        sprite_position(
                            blind_select_tokens[i],
                            blind_select_tokens[i]->pos.x,
                            blind_select_tokens[i]->pos.y - (TILE_SIZE * 12)
                        );
                    }

                    game_blind_select_print_blinds_reqs_and_rewards();
                    highlight_select_button();

                    timer = TM_ZERO;
                }
                break;
            default:
                break;
        }
    }
}

static void game_blind_select_selected_anim_seq()
{
    if (timer < 15)
    {
        Rect blinds_rect = POP_MENU_ANIM_RECT;
        blinds_rect.top -= 1; // Because of the raised blind
        main_bg_se_move_rect_1_tile_vert(blinds_rect, SCREEN_DOWN);

        for (int i = 0; i < NUM_BLINDS_PER_ANTE; i++)
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
        for (int i = 0; i < NUM_BLINDS_PER_ANTE; i++)
        {
            obj_hide(blind_select_tokens[i]->obj);
        }

        substate = DISPLAY_BLIND_PANEL; // Reset the state
        timer = TM_ZERO;                // Reset the timer
    }
}

static void game_blind_select_display_blind_panel()
{
    if (timer >= TM_DISP_BLIND_PANEL_FINISH)
    {
        substate = BLIND_SELECT_MAX;
        return;
    }

    // Switches to the selecting background and clears the blind panel area
    if (timer == TM_DISP_BLIND_PANEL_START)
    {
        change_background(BG_CARD_SELECTING, false);

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

static Rect game_blind_select_get_req_score_rect(enum BlindTokens blind)
{
    Rect blind_req_score_rect = SINGLE_BLIND_SEL_REQ_SCORE_RECT;

    blind_req_score_rect.left += blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;
    blind_req_score_rect.right += blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;

    if (g_game_vars.blinds_states[blind] == BLIND_STATE_CURRENT)
    {
        // Current blind is raised
        blind_req_score_rect.top -= TILE_SIZE;
        blind_req_score_rect.bottom -= TILE_SIZE;
    }

    return blind_req_score_rect;
}

static enum BlindType get_blind_type_from_token(enum BlindTokens blind)
{
    enum BlindType blind_type;
    switch (blind)
    {
        case SMALL_BLIND:
            blind_type = BLIND_TYPE_SMALL;
            break;
        case BIG_BLIND:
            blind_type = BLIND_TYPE_BIG;
            break;
        default:
            blind_type = g_game_vars.next_boss_blind;
            break;
    }
    return blind_type;
}

static inline void game_blind_select_print_blind_req(enum BlindTokens blind)
{
    Rect blind_req_score_rect = game_blind_select_get_req_score_rect(blind);

    u32 blind_req = blind_get_requirement(get_blind_type_from_token(blind), g_game_vars.ante);

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

static inline void game_blind_select_print_blind_reward(enum BlindTokens blind)
{
    int blind_reward = blind_get_reward(get_blind_type_from_token(blind));
    Rect blind_reward_rect = game_blind_select_get_req_score_rect(blind);

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

static void game_blind_select_print_blinds_reqs_and_rewards(void)
{
    for (enum BlindTokens curr_blind = 0; curr_blind < NUM_BLINDS_PER_ANTE; curr_blind++)
    {
        game_blind_select_print_blind_req(curr_blind);
        game_blind_select_print_blind_reward(curr_blind);
    }
}

static inline void reroll_boss_blind(bool no_tiles)
{
    // Showdown blinds only show up on ante 8, 16, etc...
    g_game_vars.next_boss_blind =
        roll_blind_type((g_game_vars.ante % 8 == 0) && (g_game_vars.ante > 0));
    if (!no_tiles)
    {
        apply_blind_tiles(g_game_vars.next_boss_blind, BOSS_BLIND_TOKEN_LAYER);
    }
}

static void blind_tokens_init()
{
    if (g_game_vars.current_blind == BLIND_TYPE_SMALL)
        reroll_boss_blind(true);

    sprite_destroy(&blind_select_tokens[SMALL_BLIND]);
    sprite_destroy(&blind_select_tokens[BIG_BLIND]);
    sprite_destroy(&blind_select_tokens[BOSS_BLIND]);

    blind_select_tokens[SMALL_BLIND] = blind_token_new(
        BLIND_TYPE_SMALL,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        SMALL_BLIND_TOKEN_LAYER
    );
    blind_select_tokens[BIG_BLIND] = blind_token_new(
        BLIND_TYPE_BIG,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        BIG_BLIND_TOKEN_LAYER
    );
    blind_select_tokens[BOSS_BLIND] = blind_token_new(
        g_game_vars.next_boss_blind,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        BOSS_BLIND_TOKEN_LAYER
    );

    for (int i = 0; i < NUM_BLINDS_PER_ANTE; i++)
    {
        obj_hide(blind_select_tokens[i]->obj);
    }
}

void game_blind_select_on_init(void)
{
    timer = TM_ZERO;
    substate = START_ANIM_SEQ;

    selection_x = 0;
    selection_y = 0;

    blind_tokens_init();

    // TODO: silly bug rn, the sprite tokens are unhidden on a background change.
    // this probably shouldn't be here. also need to force redraw or the callback
    // doesn't run that moves the tokens. that should probably happen here.
    change_background(BG_BLIND_SELECT, true);

    highlight_select_button();

    play_sfx(SFX_POP, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
}

void game_blind_select_on_update(void)
{
    timer++;
    if (substate == BLIND_SELECT_MAX)
    {
        reset_background();
        game_change_state(GAME_STATE_PLAYING);
        return;
    }

    blind_select_state_actions[substate]();
}

void game_blind_select_on_exit(void)
{
    // For some reason that I haven't figured out yet,
    // if I don't destroy the blind tokens they won't
    // show up on the next run.
    sprite_destroy(&blind_select_tokens[SMALL_BLIND]);
    sprite_destroy(&blind_select_tokens[BIG_BLIND]);
    sprite_destroy(&blind_select_tokens[BOSS_BLIND]);

    reset_background();
    selection_y = 0;

    g_game_vars.timer = TM_ZERO;
}

void game_blind_select_change_background(void)
{
    for (int i = 0; i < NUM_BLINDS_PER_ANTE; i++)
    {
        obj_unhide(blind_select_tokens[i]->obj, 0);
    }

    // Default y position for the blind select tokens. 12 is the amount of tiles the background
    // is shifted down by
    const int default_y = 89 + (TILE_SIZE * 12);
    // TODO refactor magic numbers '80/120/160' into a map to loop with
    sprite_position(blind_select_tokens[SMALL_BLIND], BLIND_LEFT_X, default_y);
    sprite_position(blind_select_tokens[BIG_BLIND], BLIND_CENTER_X, default_y);
    sprite_position(blind_select_tokens[BOSS_BLIND], BLIND_RIGHT_X, default_y);

    toggle_windows(false, true);

    GRIT_CPY(pal_bg_mem, background_blind_select_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_blind_select_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_blind_select_gfxMap);

    // Copy boss blind colors to blind select palette
    memset16(
        &pal_bg_mem[1],
        blind_get_color(g_game_vars.next_boss_blind, BLIND_BACKGROUND_MAIN_COLOR_INDEX),
        1
    );
    memset16(
        &pal_bg_mem[7],
        blind_get_color(g_game_vars.next_boss_blind, BLIND_BACKGROUND_SHADOW_COLOR_INDEX),
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
    memcpy16(&pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID], &pal_bg_mem[BLIND_SKIP_BTN_PID], 1);

    for (int i = 0; i < NUM_BLINDS_PER_ANTE; i++)
    {
        Rect curr_blind_rect = SINGLE_BLIND_SELECT_RECT;

        // There's no gap between them
        curr_blind_rect.left += i * rect_width(&SINGLE_BLIND_SELECT_RECT);
        curr_blind_rect.right += i * rect_width(&SINGLE_BLIND_SELECT_RECT);

        if (g_game_vars.blinds_states[i] != BLIND_STATE_CURRENT &&
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

        switch (g_game_vars.blinds_states[i])
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
                else if (i > BLIND_TYPE_BIG)
                {
                    x_from = x_to;
                    y_from = 30;
                }

                // Copy plain tiles onto the bottom of the raised blind panel to fill the gap
                // created by the raise
                Rect gap_fill_rect =
                    {x_from, y_from, x_from + rect_width(&SINGLE_BLIND_SELECT_RECT) - 1, y_from};
                BG_POINT gap_fill_point = {x_to, y_to};
                main_bg_se_copy_rect(gap_fill_rect, gap_fill_point);

                // Move token up by a tile
                sprite_position(
                    blind_select_tokens[i],
                    blind_select_tokens[i]->pos.x,
                    blind_select_tokens[i]->pos.y - TILE_SIZE
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
