#include "game/blind_select.h"

#include "audio_utils.h"
#include "background_blind_select_gfx.h"
#include "blind.h"
#include "game.h"
#include "game/common_ui.h"
#include "game/palette.h"
#include "game/rect.h"
#include "game/timer.h"
#include "graphic_utils.h"
#include "soundbank.h"
#include "sprite.h"
#include "util.h"

#include <stdint.h>
#include <string.h>

#define TILE_SIZE         8
#define BUTTON_SFX_VOLUME 154 // 60% of MM_FULL_VOLUME
#define UINT_MAX_DIGITS   10
#define OVERFLOW_RIGHT    0

static void blind_select_start_anim_seq(BlindSelectProps* props);
static void blind_select_handle_input(BlindSelectProps* props);
static void blind_select_selected_anim_seq(BlindSelectProps* props);
static void blind_select_display_blind_panel(BlindSelectProps* props);
typedef void (*SubStateActionFn)(BlindSelectProps* props);
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

static int selection_x;
static int selection_y;

void game_blind_select_on_init(void* _)
{
    change_background(BG_BLIND_SELECT);
    selection_x = 0;
    selection_y = 0;
    play_sfx(SFX_POP, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
}

void game_blind_select_change_background(void)
{
    unhide_all_blind_select_tokens();

    // Default y position for the blind select tokens. 12 is the amount of tiles the background
    // is shifted down by
    const int default_y = 89 + (TILE_SIZE * 12);
    // TODO refactor magic numbers '80/120/160' into a map to loop with
    move_blind_select_token(BLIND_TYPE_SMALL, 80, default_y);
    move_blind_select_token(BLIND_TYPE_BIG, 120, default_y);
    move_blind_select_token(BLIND_TYPE_BOSS, 160, default_y);

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
    memcpy16(&pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID], &pal_bg_mem[BLIND_SKIP_BTN_PID], 1);

    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        Rect curr_blind_rect = SINGLE_BLIND_SELECT_RECT;

        // There's no gap between them
        curr_blind_rect.left += i * rect_width(&SINGLE_BLIND_SELECT_RECT);
        curr_blind_rect.right += i * rect_width(&SINGLE_BLIND_SELECT_RECT);

        enum BlindState blinds_state = get_blinds_state((enum BlindType)i);

        if (blinds_state != BLIND_STATE_CURRENT &&
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

        switch (blinds_state)
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
                Rect gap_fill_rect =
                    {x_from, y_from, x_from + rect_width(&SINGLE_BLIND_SELECT_RECT) - 1, y_from};
                BG_POINT gap_fill_point = {x_to, y_to};
                main_bg_se_copy_rect(gap_fill_rect, gap_fill_point);

                // Move token up by a tile
                int sprite_pos_x, sprite_pos_y;
                get_blind_select_token_pos((enum BlindType)i, &sprite_pos_x, &sprite_pos_y);
                move_blind_select_token((enum BlindType)i, sprite_pos_x, sprite_pos_y - TILE_SIZE);
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

void game_blind_select_on_update(void* ctx)
{
    BlindSelectProps* props = (BlindSelectProps*)ctx;
    int substate = props->substate;

    if (substate == BLIND_SELECT_MAX)
    {
        game_change_state(GAME_STATE_PLAYING);
        return;
    }

    blind_select_state_actions[substate](props);
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

static Rect blind_select_get_req_score_rect(BlindSelectProps* props, enum BlindType blind)
{
    Rect blind_req_score_rect = SINGLE_BLIND_SEL_REQ_SCORE_RECT;

    blind_req_score_rect.left += blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;
    blind_req_score_rect.right += blind * rect_width(&SINGLE_BLIND_SELECT_RECT) * TILE_SIZE;

    if (props->blinds_states[blind] == BLIND_STATE_CURRENT)
    {
        // Current blind is raised
        blind_req_score_rect.top -= TILE_SIZE;
        blind_req_score_rect.bottom -= TILE_SIZE;
    }

    return blind_req_score_rect;
}

static inline void blind_select_print_blind_req(BlindSelectProps* props, enum BlindType blind)
{
    Rect blind_req_score_rect = blind_select_get_req_score_rect(props, blind);

    u32 blind_req = blind_get_requirement(blind, props->ante);
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

static inline void blind_select_print_blind_reward(BlindSelectProps* props, enum BlindType blind)
{
    int blind_reward = blind_get_reward(blind);
    Rect blind_reward_rect = blind_select_get_req_score_rect(props, blind);

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

static void blind_select_print_blinds_reqs_and_rewards(BlindSelectProps* props)
{
    for (enum BlindType curr_blind = 0; curr_blind < BLIND_TYPE_MAX; curr_blind++)
    {
        blind_select_print_blind_req(props, curr_blind);
        blind_select_print_blind_reward(props, curr_blind);
    }
}

// Sub-state action functions
static void blind_select_start_anim_seq(BlindSelectProps* props)
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        int sprite_pos_x, sprite_pos_y;
        get_blind_select_token_pos((enum BlindType)i, &sprite_pos_x, &sprite_pos_y);
        move_blind_select_token((enum BlindType)i, sprite_pos_x, sprite_pos_y - TILE_SIZE);
    }

    if (props->timer == TM_END_ANIM_SEQ)
    {
        blind_select_print_blinds_reqs_and_rewards(props);
        props->substate = BLIND_SELECT;
        props->timer = TM_ZERO;
    }
}

static void blind_select_handle_input(BlindSelectProps* props)
{
    if (props->timer == TM_BLIND_SELECT_START && props->current_blind == BLIND_TYPE_BOSS)
    {
        selection_y = 0;
    }

    // Blind select input logic
    if (key_hit(KEY_UP))
    {
        selection_y = 0;
    }
    else if (key_hit(KEY_DOWN) && props->current_blind != BLIND_TYPE_BOSS)
    {
        selection_y = 1;
    }
    else if (key_hit(SELECT_CARD))
    {
        blind_select_erase_blind_reqs_and_rewards();

        if (selection_y == 0) // Blind selected
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            props->substate = BLIND_SELECTED_ANIM_SEQ;
            props->timer = TM_ZERO;
            display_round(increment_round());
        }
        else if (props->current_blind != BLIND_TYPE_BOSS)
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            increment_blind(props->blinds_states, &props->current_blind, BLIND_STATE_SKIPPED);

            reset_background(); // Force refresh of the background
            update_game_state_ctx(GAME_STATE_BLIND_SELECT);
            change_background(BG_BLIND_SELECT);

            // TODO: Create a generic vertical move by any number of tiles to avoid for loops?
            for (int i = 0; i < 12; i++)
            {
                main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);
            }

            for (int i = 0; i < BLIND_TYPE_MAX; i++)
            {
                int sprite_pos_x, sprite_pos_y;
                get_blind_select_token_pos((enum BlindType)i, &sprite_pos_x, &sprite_pos_y);
                move_blind_select_token(
                    (enum BlindType)i,
                    sprite_pos_x,
                    sprite_pos_y - (TILE_SIZE * 12)
                );
            }

            blind_select_print_blinds_reqs_and_rewards(props);

            props->timer = TM_ZERO;
            selection_y = 0;
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

static void blind_select_selected_anim_seq(BlindSelectProps* props)
{
    uint timer = props->timer;
    if (timer < 15)
    {
        Rect blinds_rect = POP_MENU_ANIM_RECT;
        blinds_rect.top -= 1; // Because of the raised blind
        main_bg_se_move_rect_1_tile_vert(blinds_rect, SCREEN_DOWN);

        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            int sprite_pos_x, sprite_pos_y;
            get_blind_select_token_pos((enum BlindType)i, &sprite_pos_x, &sprite_pos_y);
            move_blind_select_token((enum BlindType)i, sprite_pos_x, sprite_pos_y + TILE_SIZE);
        }
    }
    else if (timer >= MENU_POP_OUT_ANIM_FRAMES)
    {
        hide_all_blind_select_tokens();
        props->substate = DISPLAY_BLIND_PANEL; // Reset the state
        props->timer = TM_ZERO;                // Reset the timer
    }
}

static void blind_select_display_blind_panel(BlindSelectProps* props)
{
    uint timer = props->timer;

    if (timer >= TM_DISP_BLIND_PANEL_FINISH)
    {
        props->substate = BLIND_SELECT_MAX;
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

void game_blind_select_on_exit(void* _)
{
    selection_y = 0;
    reset_background();
}
