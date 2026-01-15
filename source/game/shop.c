#include "game/shop.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "background_shop_gfx.h"
#include "bitset.h"
#include "blind.h"
#include "button.h"
#include "game.h"
#include "game/common_ui.h"
#include "game/palette.h"
#include "game/point.h"
#include "game/rect.h"
#include "game/timer.h"
#include "graphic_utils.h"
#include "joker.h"
#include "list.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "sprite.h"
#include "tonc_memdef.h"
#include "util.h"

#include <maxmod.h>
#include <stdio.h>
#include <stdlib.h>
#include <tonc.h>

#define ITEM_SHOP_Y 71

#define SHOP_LIGHTS_1_CLR 0xFFFF
#define SHOP_LIGHTS_2_CLR 0x32BE
#define SHOP_LIGHTS_3_CLR 0x4B5F
#define SHOP_LIGHTS_4_CLR 0x5F9F

#define REROLL_BASE_COST     5
#define NEXT_ROUND_BTN_SEL_X 0

enum GameShopStates
{
    GAME_SHOP_INTRO,
    GAME_SHOP_ACTIVE,
    GAME_SHOP_EXIT,
    GAME_SHOP_MAX
};

typedef void (*SubStateActionFn)(void);

// Forward declarations
void game_shop_intro(void);
static void game_shop_process_user_input(void);
static void game_shop_outro(void);
void game_shop_create_items(void);
static void game_shop_lights_anim_frame(void);
int jokers_sel_row_get_size(void);
bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static int shop_top_row_get_size(void);
static bool shop_top_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void shop_top_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static int shop_reroll_row_get_size(void);
static bool shop_reroll_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void shop_reroll_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static inline void game_shop_reroll(int* reroll_cost);
static inline void game_shop_buy_joker(int shop_joker_idx);
static inline void game_sell_joker(int joker_idx);

// Extern declarations from game.c
extern enum GameState game_state;
extern unsigned int timer;
extern int ante;
extern int money;
extern int game_speed;
extern int shortcut_joker_count;
extern int four_fingers_joker_count;
extern List _owned_jokers_list;
extern List _discarded_jokers_list;
extern bool is_shop_joker_avail(int joker_id);
extern int get_num_shop_jokers_avail(void);
extern void reset_shop_jokers(void);
extern bool no_avail_jokers(void);
extern int game_shop_get_rand_available_joker_id(void);
extern void game_change_state(enum GameState new_game_state);
extern void change_background(enum BackgroundId id);
extern void display_money(void);
extern void increment_blind(enum BlindState increment_reason);
extern void reset_top_left_panel_bottom_row(void);
extern Rect get_text_rect_under_sprite_object(SpriteObject* sprite_object);
extern void remove_owned_joker(int joker_idx);
extern void set_shop_joker_avail(int joker_id, bool avail);
extern StateInfo state_info[];

// Module-local variables
static int reroll_cost = REROLL_BASE_COST;
static List _shop_jokers_list;

static SelectionGridRow shop_selection_rows[] = {
    {0, jokers_sel_row_get_size,  jokers_sel_row_on_selection_changed,  jokers_sel_row_on_key_transit,  {.wrap = false}},
    {1, shop_top_row_get_size,    shop_top_row_on_selection_changed,    shop_top_row_on_key_transit,    {.wrap = false}},
    {2, shop_reroll_row_get_size, shop_reroll_row_on_selection_changed, shop_reroll_row_on_key_transit, {.wrap = false}}
};

static const Selection SHOP_INIT_SEL = {-1, 1};

SelectionGrid shop_selection_grid = {
    shop_selection_rows,
    NUM_ELEM_IN_ARR(shop_selection_rows),
    SHOP_INIT_SEL
};

static const SubStateActionFn shop_state_actions[] = {
    game_shop_intro,
    game_shop_process_user_input,
    game_shop_outro
};

static void erase_price_under_sprite_object(SpriteObject* sprite_object)
{
    Rect price_rect = get_text_rect_under_sprite_object(sprite_object);

    // Add SPRITE_FOCUS_RAISE_PX to cover the focused case
    price_rect.bottom = price_rect.bottom + SPRITE_FOCUS_RAISE_PX;

    tte_erase_rect_wrapper(price_rect);
}

static void print_price_under_sprite_object(SpriteObject* sprite_object, int price)
{
    Rect price_rect = get_text_rect_under_sprite_object(sprite_object);

    char price_str_buff[INT_MAX_DIGITS + 2]; // + 2 for null-terminator and "$"

    snprintf(price_str_buff, sizeof(price_str_buff), "$%d", price);

    update_text_rect_to_center_str(&price_rect, price_str_buff, SCREEN_LEFT);

    tte_printf("#{P:%d,%d; cx:0x%X000}$%d", price_rect.left, price_rect.top, TTE_YELLOW_PB, price);
}

static void add_joker(JokerObject* joker_object)
{
    list_push_back(&_owned_jokers_list, joker_object);

    // TODO: Extract to on_joker_added() callback
    // In case the player gets multiple Four Fingers Jokers,
    // only change size when the first one is added
    if (joker_object->joker->id == FOUR_FINGERS_JOKER_ID)
    {
        four_fingers_joker_count++;
    }

    if (joker_object->joker->id == SHORTCUT_JOKER_ID)
    {
        shortcut_joker_count++;
    }
}

void game_shop_create_items(void)
{
    tte_erase_rect_wrapper(SHOP_PRICES_TEXT_RECT);

    if (no_avail_jokers())
        return;

    list_clear(&_shop_jokers_list);
    _shop_jokers_list = list_create();

    for (int i = 0; i < MAX_SHOP_JOKERS; i++)
    {
        int joker_id = 0;
#ifdef TEST_JOKER_ID0 // Allow defining an ID for a joker to always appear in shop and be tested
        if (is_shop_joker_avail(TEST_JOKER_ID0))
        {
            joker_id = TEST_JOKER_ID0;
        }
        else
#endif
#ifdef TEST_JOKER_ID1
            if (is_shop_joker_avail(TEST_JOKER_ID1))
        {
            joker_id = TEST_JOKER_ID1;
        }
        else
#endif
        {
            joker_id = game_shop_get_rand_available_joker_id();
        }

        // If for some reason only no joker is left, don't make another
        if (joker_id == UNDEFINED)
            break;

        set_shop_joker_avail(joker_id, false);

        JokerObject* joker_object = joker_object_new(joker_new(joker_id));

        joker_object->sprite_object->x = int2fx(120 + i * CARD_SPRITE_SIZE);
        joker_object->sprite_object->y = int2fx(160);
        joker_object->sprite_object->tx = joker_object->sprite_object->x;
        joker_object->sprite_object->ty = int2fx(ITEM_SHOP_Y);

        print_price_under_sprite_object(joker_object->sprite_object, joker_object->joker->value);

        sprite_position(
            joker_object_get_sprite(joker_object),
            fx2int(joker_object->sprite_object->x),
            fx2int(joker_object->sprite_object->y)
        );

        list_push_back(&_shop_jokers_list, joker_object);
    }
}

// Intro sequence (menu and shop icon coming into frame)
void game_shop_intro()
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (timer == TM_CREATE_SHOP_ITEMS_WAIT)
    {
        game_shop_create_items();
    }

    if (timer >= TM_SHIFT_SHOP_ICON_WAIT) // Shift the shop icon
    {
        int timer_offset = timer - 6;

        // TODO: Extract to generic function?
        for (int y = 0; y < timer_offset; y++)
        {
            int y_from = 26 + y - timer_offset;
            int y_to = 0 + y;

            Rect from = {0, y_from, 8, y_from};
            BG_POINT to = {0, y_to};

            main_bg_se_copy_rect(from, to);
        }
    }

    if (timer == TM_END_GAME_SHOP_INTRO)
    {
        state_info[game_state].substate = GAME_SHOP_ACTIVE;
        timer = TM_ZERO; // Reset the timer
    }
}

int jokers_sel_row_get_size(void)
{
    return list_get_len(&_owned_jokers_list);
}

bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    // swap Jokers if the A button is held down and all Jokers are on the same row
    bool swapping =
        key_is_down(SELECT_CARD) && new_selection->y == row_idx && prev_selection->y == row_idx;

    if (prev_selection->y == row_idx)
    {
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(&_owned_jokers_list, prev_selection->x);
        // Don't change focus from current Joker if swapping
        if (joker_object != NULL && !swapping)
        {
            erase_price_under_sprite_object(joker_object->sprite_object);
            sprite_object_set_focus(joker_object->sprite_object, false);
        }
    }

    if (new_selection->y == row_idx)
    {
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(&_owned_jokers_list, new_selection->x);
        if (joker_object != NULL)
        {
            if (!swapping)
            {
                sprite_object_set_focus(joker_object->sprite_object, true);
            }
            // If we land on this row while the A button is being held, we are in swapping mode
            // This means that we need to hide the price, whether we were already
            // on this row or if we come from another
            if (!key_is_down(SELECT_CARD))
            {
                print_price_under_sprite_object(
                    joker_object->sprite_object,
                    joker_get_sell_value(joker_object->joker)
                );
            }
        }
    }

    if (swapping)
    {
        list_swap(
            &_owned_jokers_list,
            (unsigned int)prev_selection->x,
            (unsigned int)new_selection->x
        );
    }

    return true;
}

static inline void joker_start_discard_animation(JokerObject* joker_object)
{
    joker_object->sprite_object->tx = int2fx(JOKER_DISCARD_TARGET.x);
    joker_object->sprite_object->ty = int2fx(JOKER_DISCARD_TARGET.y);
    list_push_back(&_discarded_jokers_list, joker_object);
}

static inline void game_sell_joker(int joker_idx)
{
    if (joker_idx < 0 || joker_idx >= list_get_len(&_owned_jokers_list))
        return;

    JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_owned_jokers_list, joker_idx);
    money += joker_get_sell_value(joker_object->joker);
    display_money();
    erase_price_under_sprite_object(joker_object->sprite_object);

    remove_owned_joker(joker_idx);

    joker_start_discard_animation(joker_object);
}

void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_owned_jokers_list, selection->x);
    if (joker_object != NULL)
    {
        if (key_hit(SELECT_CARD))
        {
            erase_price_under_sprite_object(joker_object->sprite_object);
        }
        else if (key_released(SELECT_CARD))
        {
            print_price_under_sprite_object(
                joker_object->sprite_object,
                joker_get_sell_value(joker_object->joker)
            );
        }
    }

    if (key_hit(SELL_KEY))
    {
        int sold_joker_idx = selection->x;

        // Move the selection away from the jokers so it doesn't point to an invalid place
        // Do this before selling the joker so valid row sizes are used
        selection_grid_move_selection_vert(selection_grid, SCREEN_DOWN);

        game_sell_joker(sold_joker_idx);
    }
}

// Shop input
static int shop_top_row_get_size(void)
{
    // + 1 to account for next round button
    return list_get_len(&_shop_jokers_list) + 1;
}

static inline void add_to_held_jokers(JokerObject* joker_object)
{
    joker_object->sprite_object->ty = int2fx(HELD_JOKERS_POS.y);
    add_joker(joker_object);
}

static inline void game_shop_buy_joker(int shop_joker_idx)
{
    JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_shop_jokers_list, shop_joker_idx);

    money -= joker_object->joker->value; // Deduct the money spent on the joker
    display_money();                     // Update the money display
    erase_price_under_sprite_object(joker_object->sprite_object);
    sprite_object_set_focus(joker_object->sprite_object, false);
    add_to_held_jokers(joker_object);
    list_remove_at_idx(&_shop_jokers_list, shop_joker_idx); // Remove the joker from the shop
}

static void shop_top_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (!key_hit(SELECT_CARD))
        return;

    if (selection->x == NEXT_ROUND_BTN_SEL_X)
    {
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);

        // Go to next blind selection game state
        state_info[game_state].substate = GAME_SHOP_EXIT; // Go to the outro sequence state
        timer = TM_ZERO;                                  // Reset the timer
        reroll_cost = REROLL_BASE_COST;

        memcpy16(
            &pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[SHOP_PANEL_SHADOW_PID],
            1
        );

        // memcpy16(&pal_bg_mem[16], &pal_bg_mem[6], 1);
        // This changes the color of the button to a dark red.
        // However, it shares a palette with the shop icon, so it will change the color of the shop
        // icon as well. And I don't care enough to fix it right now.
    }
    else
    {
        int shop_joker_idx = selection->x - 1; // - 1 to account for next round button
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(&_shop_jokers_list, shop_joker_idx);
        if (joker_object == NULL || list_get_len(&_owned_jokers_list) >= MAX_JOKERS_HELD_SIZE ||
            money < joker_object->joker->value)
        {
            return;
        }

        game_shop_buy_joker(shop_joker_idx);

        // In Balatro the selection actually stays on the purchased joker it's easier to just move
        // it left
        selection_grid_move_selection_horz(selection_grid, -1);
    }
}

static bool shop_top_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    // The selection grid system only guarantees that the new selection is within bounds
    // but not the previous one...
    // This allows using INIT_SEL = {-1, 1} and move to set the initial selection in a hacky way...
    if (prev_selection->y == row_idx && prev_selection->x >= 0 &&
        prev_selection->x < shop_top_row_get_size())
    {
        if (prev_selection->x == NEXT_ROUND_BTN_SEL_X)
        {
            // Remove next round button highlight
            memcpy16(
                &pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID],
                &pal_bg_mem[NEXT_ROUND_BTN_PID],
                1
            );
        }
        else
        {
            int idx = prev_selection->x - 1; // -1 to account for next round button
            JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_shop_jokers_list, idx);
            sprite_object_set_focus(joker_object->sprite_object, false);
        }
    }

    if (new_selection->y == row_idx)
    {
        if (new_selection->x == NEXT_ROUND_BTN_SEL_X)
        {
            // Highlight next round button
            memset16(&pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID], BTN_HIGHLIGHT_COLOR, 1);
        }
        else
        {
            int idx = new_selection->x - 1; // -1 to account for next round button
            JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_shop_jokers_list, idx);
            sprite_object_set_focus(joker_object->sprite_object, true);
        }
    }

    return true;
}

static int shop_reroll_row_get_size()
{
    return 1; // Only the reroll button
}

static bool shop_reroll_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    if (row_idx == prev_selection->y)
    {
        // Remove highlight
        memcpy16(&pal_bg_mem[REROLL_BTN_SELECTED_BORDER_PID], &pal_bg_mem[REROLL_BTN_PID], 1);
    }
    else if (row_idx == new_selection->y)
    {
        memset16(&pal_bg_mem[REROLL_BTN_SELECTED_BORDER_PID], BTN_HIGHLIGHT_COLOR, 1);
    }

    return true;
}

static inline void game_shop_reroll(int* reroll_cost)
{
    money -= *reroll_cost;
    display_money(); // Update the money display

    ListItr itr = list_itr_create(&_shop_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL)
        {
            set_shop_joker_avail(joker_object->joker->id, true);
            joker_object_destroy(&joker_object); // Destroy the joker object if it exists
        }
    }

    list_clear(&_shop_jokers_list);
    _shop_jokers_list = list_create();

    game_shop_create_items();

    itr = list_itr_create(&_shop_jokers_list);

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL)
        {
            // Set the y position to the target position
            joker_object->sprite_object->y = joker_object->sprite_object->ty;

            // Give the joker a little wiggle animation
            joker_object_shake(joker_object, UNDEFINED);
        }
    }

    (*reroll_cost)++;
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}$%d",
        SHOP_REROLL_RECT.left,
        SHOP_REROLL_RECT.top,
        TTE_WHITE_PB,
        *reroll_cost
    );
}

static void shop_reroll_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (!key_hit(SELECT_CARD))
    {
        return;
    }

    if (money >= reroll_cost)
    {
        // TODO: Add money sound effect
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
        game_shop_reroll(&reroll_cost);
    }
}

// Shop menu input and selection
static void game_shop_process_user_input()
{
    if (timer == TM_SHOP_PRC_INPUT_START)
    {
        // TODO: Move to on_init?
        // The selection grid is initialized outside of bounds and moved
        // to trigger the selection change so the initial selection is visible
        shop_selection_grid.selection = SHOP_INIT_SEL;
        selection_grid_move_selection_horz(&shop_selection_grid, 1);
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            SHOP_REROLL_RECT.left,
            SHOP_REROLL_RECT.top,
            TTE_WHITE_PB,
            reroll_cost
        );
    }

    // Shop input logic
    selection_grid_process_input(&shop_selection_grid);
}

// Outro sequence (menu and shop icon going out of frame)
static void game_shop_outro()
{
    // Shift the shop panel
    main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_DOWN);

    main_bg_se_copy_rect_1_tile_vert(TOP_LEFT_PANEL_ANIM_RECT, SCREEN_UP);

    // TODO: make heads or tails of what's going on here and replace
    // magic numbers.
    if (timer == 1)
    {
        tte_erase_rect_wrapper(SHOP_PRICES_TEXT_RECT); // Erase the shop prices text

        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* joker_object;
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != NULL)
            {
                joker_object->sprite_object->ty = int2fx(160);
            }
        }

        reset_top_left_panel_bottom_row();
    }
    else if (timer == 2)
    {
        int y = 5;
        memset16(&se_mat[MAIN_BG_SBB][y - 1][0], 0x0001, 1);
        memset16(&se_mat[MAIN_BG_SBB][y - 1][1], 0x0002, 7);
        memset16(&se_mat[MAIN_BG_SBB][y - 1][8], SE_HFLIP | 0x0001, 1);
    }

    if (timer >= MENU_POP_OUT_ANIM_FRAMES)
    {
        state_info[game_state].substate = GAME_SHOP_MAX; // Go to the next state
        timer = TM_ZERO;                                 // Reset the timer
    }
}

static inline void game_shop_lights_anim_frame(void)
{
    // Shift palette around the border of the shop icon
    COLOR shifted_palette[4];
    memcpy16(&shifted_palette[0], &pal_bg_mem[SHOP_LIGHTS_2_PID], 1);
    memcpy16(&shifted_palette[1], &pal_bg_mem[SHOP_LIGHTS_3_PID], 1);
    memcpy16(&shifted_palette[2], &pal_bg_mem[SHOP_LIGHTS_4_PID], 1);
    memcpy16(&shifted_palette[3], &pal_bg_mem[SHOP_LIGHTS_1_PID], 1);

    // Circularly shift the palette
    int last = shifted_palette[3];

    for (int i = 3; i > 0; --i)
    {
        shifted_palette[i] = shifted_palette[i - 1];
    }

    shifted_palette[0] = last;

    // Copy the shifted palette to the next 4 slots
    memcpy16(&pal_bg_mem[SHOP_LIGHTS_2_PID], &shifted_palette[0], 1);
    memcpy16(&pal_bg_mem[SHOP_LIGHTS_3_PID], &shifted_palette[1], 1);
    memcpy16(&pal_bg_mem[SHOP_LIGHTS_4_PID], &shifted_palette[2], 1);
    memcpy16(&pal_bg_mem[SHOP_LIGHTS_1_PID], &shifted_palette[3], 1);
}

void game_shop_on_update()
{
    change_background(BG_SHOP);

    if (!list_is_empty(&_shop_jokers_list))
    {
        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* joker_object;
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != NULL)
            {
                joker_object_update(joker_object);
            }
        }
    }

    if (timer % 20 == 0)
    {
        game_shop_lights_anim_frame();
    }

    if (state_info[game_state].substate == GAME_SHOP_MAX)
    {
        game_change_state(GAME_STATE_BLIND_SELECT);
        return;
    }

    int substate = state_info[game_state].substate;

    shop_state_actions[substate]();
}

void game_shop_on_exit()
{
    ListItr itr = list_itr_create(&_shop_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL)
        {
            // Make the joker available back to shop
            set_shop_joker_avail(joker_object->joker->id, true);
        }
        joker_object_destroy(&joker_object); // Destroy the joker objects
    }

    list_clear(&_shop_jokers_list);

    increment_blind(BLIND_STATE_DEFEATED); // TODO: Move to game_round_end()?
}

void game_shop_change_background()
{
    toggle_windows(false, true);

    GRIT_CPY(pal_bg_mem, background_shop_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_shop_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_shop_gfxMap);

    // Set the outline colors for the shop background. This is used for the alternate shop
    // palettes when opening packs
    memset16(&pal_bg_mem[SHOP_BOTTOM_PANEL_BORDER_PID], 0x213D, 1);
    memset16(&pal_bg_mem[SHOP_PANEL_SHADOW_PID], 0x10B4, 1);

    // Reset the shop lights to correct colors
    memset16(&pal_bg_mem[SHOP_LIGHTS_2_PID], SHOP_LIGHTS_2_CLR, 1);
    memset16(&pal_bg_mem[SHOP_LIGHTS_3_PID], SHOP_LIGHTS_3_CLR, 1);
    memset16(&pal_bg_mem[SHOP_LIGHTS_4_PID], SHOP_LIGHTS_4_CLR, 1);
    memset16(&pal_bg_mem[SHOP_LIGHTS_1_PID], SHOP_LIGHTS_1_CLR, 1);

    // Disable the button highlight colors
    memcpy16(&pal_bg_mem[REROLL_BTN_SELECTED_BORDER_PID], &pal_bg_mem[REROLL_BTN_PID], 1);
    memcpy16(&pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID], &pal_bg_mem[NEXT_ROUND_BTN_PID], 1);
}