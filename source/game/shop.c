/**
 * @file shop.c
 *
 * @brief Shop state functions implementation.
 */

#include "game/shop.h"

#include "audio_utils.h"
#include "background_shop_gfx.h"
#include "bitset.h"
#include "button.h"
#include "game.h"
#include "game/blind_select.h"
#include "game/joker_row.h"
#include "game_variables.h"
#include "joker.h"
#include "layout.h"
#include "list.h"
#include "save.h"
#include "soundbank.h"
#include "timer.h"
#include "util.h"

// Timer defs
#define TM_END_GAME_SHOP_INTRO    12
#define TM_CREATE_SHOP_ITEMS_WAIT 1
#define TM_SHIFT_SHOP_ICON_WAIT   7

// Pixel sized
#define ITEM_SHOP_Y 71

// Shop
#define REROLL_BASE_COST     5 // Base cost for rerolling the shop items
#define NEXT_ROUND_BTN_SEL_X 0

// Palette IDs
#define REROLL_BTN_PAL_IDX                     3
#define NEXT_ROUND_BTN_SELECTED_BORDER_PAL_IDX 5
#define SHOP_PANEL_SHADOW_PAL_IDX              6
#define REROLL_BTN_SELECTED_BORDER_PAL_IDX     7
#define SHOP_LIGHTS_1_PAL_IDX                  8
#define SHOP_LIGHTS_2_PAL_IDX                  14
#define NEXT_ROUND_BTN_PAL_IDX                 16
#define SHOP_LIGHTS_3_PAL_IDX                  17
#define SHOP_LIGHTS_4_PAL_IDX                  22
#define SHOP_BOTTOM_PANEL_BORDER_PAL_IDX       26

#define SHOP_LIGHTS_1_CLR 0xFFFF
#define SHOP_LIGHTS_2_CLR 0x32BE
#define SHOP_LIGHTS_3_CLR 0x4B5F
#define SHOP_LIGHTS_4_CLR 0x5F9F

// clang-format off
static const Rect     SHOP_ICON_FROM_RECT   = {  0,  26,   8,  26 };
static const BG_POINT SHOP_ICON_TO_POS      = {  0,   0 };
static const Rect     SHOP_PRICES_TEXT_RECT = { 72,  56, 192, 160 };
static const Rect     SHOP_REROLL_RECT      = { 88,  96, UNDEFINED, UNDEFINED };

// Positions in pixels
static const BG_POINT SHOP_JOKER_SPRITES_INIT_POS = {120, 160};
// clang-format on

// Shop Substates

enum GameShopStates
{
    GAME_SHOP_INTRO,
    GAME_SHOP_ACTIVE,
    GAME_SHOP_EXIT,
    GAME_SHOP_MAX
};

static void game_shop_intro();
static void game_shop_process_user_input();
static void game_shop_outro();

static const SubStateActionFn shop_state_actions[] = {
    game_shop_intro,
    game_shop_process_user_input,
    game_shop_outro
};

// Shop SelectionGrid

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

static SelectionGridRow shop_selection_rows[] = {
    {0, jokers_sel_row_get_size,  jokers_sel_row_on_selection_changed,  jokers_sel_row_on_key_transit,  {.wrap = false, .has_h_exit_idx = false, .h_exit_idx = 0}},
    {1, shop_top_row_get_size,    shop_top_row_on_selection_changed,    shop_top_row_on_key_transit,    {.wrap = false, .has_h_exit_idx = false, .h_exit_idx = 0}},
    {2, shop_reroll_row_get_size, shop_reroll_row_on_selection_changed, shop_reroll_row_on_key_transit, {.wrap = false, .has_h_exit_idx = true, .h_exit_idx = 1} },
};

static const Selection SHOP_INIT_SEL = {-1, 1};

static SelectionGrid shop_selection_grid = {
    shop_selection_rows,
    NUM_ELEM_IN_ARR(shop_selection_rows),
    SHOP_INIT_SEL
};

// Shop Buttons

static void next_round_on_pressed(void);
static void reroll_on_pressed(void);
static bool reroll_can_be_pressed(void);
static Button next_round_button =
    {NEXT_ROUND_BTN_SELECTED_BORDER_PAL_IDX, NEXT_ROUND_BTN_PAL_IDX, next_round_on_pressed, NULL};
static Button reroll_button = {
    REROLL_BTN_SELECTED_BORDER_PAL_IDX,
    REROLL_BTN_PAL_IDX,
    reroll_on_pressed,
    reroll_can_be_pressed
};

// Shop internal variables

static int timer;
static enum GameShopStates substate;

static int reroll_cost = REROLL_BASE_COST;

void game_shop_change_background(void)
{
    toggle_windows(false, true);

    GRIT_CPY(pal_bg_mem, background_shop_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_shop_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_shop_gfxMap);

    // Set the outline colors for the shop background. This is used for the alternate shop
    // palettes when opening packs
    pal_bg_mem[SHOP_BOTTOM_PANEL_BORDER_PAL_IDX] = 0x213D;
    pal_bg_mem[SHOP_PANEL_SHADOW_PAL_IDX] = 0x10B4;

    // Reset the shop lights to correct colors
    pal_bg_mem[SHOP_LIGHTS_2_PAL_IDX] = SHOP_LIGHTS_2_CLR;
    pal_bg_mem[SHOP_LIGHTS_3_PAL_IDX] = SHOP_LIGHTS_3_CLR;
    pal_bg_mem[SHOP_LIGHTS_4_PAL_IDX] = SHOP_LIGHTS_4_CLR;
    pal_bg_mem[SHOP_LIGHTS_1_PAL_IDX] = SHOP_LIGHTS_1_CLR;

    // Disable the button highlight colors
    pal_bg_mem[REROLL_BTN_SELECTED_BORDER_PAL_IDX] = pal_bg_mem[REROLL_BTN_PAL_IDX];
    pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PAL_IDX] = pal_bg_mem[NEXT_ROUND_BTN_PAL_IDX];
}

void game_shop_on_init(void)
{
    game_shop_change_background();

    timer = TM_ZERO;
    substate = GAME_SHOP_INTRO;

    // The selection grid is initialized outside of bounds and moved
    // to trigger the selection change so the initial selection is visible
    shop_selection_grid.selection = SHOP_INIT_SEL;
    selection_grid_move_selection_horz(&shop_selection_grid, 1);
}

/**
 * @brief Computes the number of Jokers we can currently roll in the Shop.
 *         The Jokers we own is taken into account and can't be rolled again.
 */
static inline int get_num_shop_jokers_avail(void)
{
    return bitset_num_set_bits(get_avail_jokers_bitset());
}

/**
 * @brief Rolls a random Joker among the available ones
 */
static inline int game_shop_get_rand_available_joker_id(void)
{
    // Roll for what rarity the joker will be
    int joker_rarity = joker_get_random_rarity();

    // Now determine how many jokers are available based on the rarity
    int jokers_avail_size = get_num_shop_jokers_avail();

    if (jokers_avail_size == 0)
        return UNDEFINED;

    int matching_joker_ids[jokers_avail_size];
    int fallback_random_idx = get_rand() % jokers_avail_size;
    int fallback_random_joker_id = UNDEFINED;
    int match_count = 0;

    BitsetItr itr = bitset_itr_create(get_avail_jokers_bitset());

    int i = 0;
    int joker_id = UNDEFINED;
    while ((joker_id = bitset_itr_next(&itr)) != UNDEFINED)
    {
        if (i++ == fallback_random_idx)
            fallback_random_joker_id = joker_id;
        const JokerInfo* info = get_joker_registry_entry(joker_id);
        if (info->rarity == joker_rarity)
        {
            matching_joker_ids[match_count++] = joker_id;
        }
    }

    int selected_joker_id =
        (match_count > 0) ? matching_joker_ids[get_rand() % match_count] : fallback_random_joker_id;

    return selected_joker_id;
}

/**
 * @brief Returns true if we can't roll any Joker
 */
static inline bool no_avail_jokers(void)
{
    return bitset_is_empty(get_avail_jokers_bitset());
}

/**
 * @brief Setup for the lists of items we can purchase in the Shop.
 *         Only Jokers are available for now, but this is where consumables and
 *         booster packs will be rolled when they are implemented.
 */
static void game_shop_create_items(void)
{
    tte_erase_rect_wrapper(SHOP_PRICES_TEXT_RECT);

    if (no_avail_jokers())
        return;

    List* shop_jokers_list = get_shop_jokers_list();

    list_clear(shop_jokers_list);
    *shop_jokers_list = list_create();

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

        joker_object->sprite_object->x =
            int2fx(SHOP_JOKER_SPRITES_INIT_POS.x + i * CARD_SPRITE_SIZE);
        joker_object->sprite_object->y = int2fx(SHOP_JOKER_SPRITES_INIT_POS.y);
        joker_object->sprite_object->tx = joker_object->sprite_object->x;
        joker_object->sprite_object->ty = int2fx(ITEM_SHOP_Y);

        sprite_object_print_price_under(joker_object->sprite_object, joker_object->joker->value);

        sprite_position(
            joker_object_get_sprite(joker_object),
            fx2int(joker_object->sprite_object->x),
            fx2int(joker_object->sprite_object->y)
        );

        list_push_back(shop_jokers_list, joker_object);
    }
}

/**
 * @brief Intro sequence (menu and shop icon coming into frame)
 */
static void game_shop_intro()
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
            Rect from = SHOP_ICON_FROM_RECT;
            from.top += y - timer_offset;
            from.bottom += y - timer_offset;

            BG_POINT to = SHOP_ICON_TO_POS;
            to.y = y;

            main_bg_se_copy_rect(from, to);
        }
    }

    if (timer == TM_END_GAME_SHOP_INTRO)
    {
        substate = GAME_SHOP_ACTIVE;
        timer = TM_ZERO; // Reset the timer

        // print initial reroll cost only when the panel is in place
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            SHOP_REROLL_RECT.left,
            SHOP_REROLL_RECT.top,
            TTE_WHITE_PB,
            reroll_cost
        );
    }
}

/**
 * @brief Computes the number of Jokers up for sale, aka the number of
 *         "buttons" there are on the top row of the Shop.
 */
static int shop_top_row_get_size(void)
{
    // + 1 to account for next round button
    return list_get_len(get_shop_jokers_list()) + 1;
}

/**
 * @brief Add a newly purchased Joker to the list of owned Jokers.
 */
static inline void add_to_held_jokers(JokerObject* joker_object)
{
    joker_object->sprite_object->ty = int2fx(HELD_JOKERS_POS.y);
    add_joker(joker_object);
}

/**
 * @brief Called when pressing A on a Shop Joker to buy it.
 */
static inline void game_shop_buy_joker(int shop_joker_idx)
{
    List* shop_jokers_list = get_shop_jokers_list();
    JokerObject* joker_object = (JokerObject*)list_get_at_idx(shop_jokers_list, shop_joker_idx);

    g_game_vars.money -= joker_object->joker->value;
    display_money();
    sprite_object_erase_text_under(joker_object->sprite_object);
    sprite_object_set_focus(joker_object->sprite_object, false);
    add_to_held_jokers(joker_object);
    list_remove_at_idx(shop_jokers_list, shop_joker_idx); // Remove the joker from the shop
}

/**
 * @brief Handle button inputs for the "Next Round" button and shop Jokers.
 */
static void shop_top_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (!key_hit(SELECT_CARD))
        return;

    if (selection->x == NEXT_ROUND_BTN_SEL_X)
    {
        button_press(&next_round_button);
    }
    else
    {
        int shop_joker_idx = selection->x - 1; // - 1 to account for next round button
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(get_shop_jokers_list(), shop_joker_idx);
        if (joker_object == NULL || list_get_len(get_jokers_list()) >= MAX_JOKERS_HELD_SIZE ||
            g_game_vars.money < joker_object->joker->value)
        {
            return;
        }

        game_shop_buy_joker(shop_joker_idx);
        selection_grid_move_selection_horz(selection_grid, -1);
    }
}

/**
 * @brief Handle d-pad inputs for the "Next Round" button and shop Jokers' row.
 */
static bool shop_top_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    List* shop_jokers_list = get_shop_jokers_list();
    // Guard if we move down while on jokers
    if (new_selection->y > row_idx && prev_selection->x > 0)
        return false;

    // The selection grid system only guarantees that the new selection is within bounds
    // but not the previous one...
    // This allows using INIT_SEL = {-1, 1} and move to set the initial selection in a hacky way...
    if (prev_selection->y == row_idx && prev_selection->x >= 0 &&
        prev_selection->x < shop_top_row_get_size())
    {
        if (prev_selection->x == NEXT_ROUND_BTN_SEL_X)
        {
            button_set_highlight(&next_round_button, false);
        }
        else
        {
            int idx = prev_selection->x - 1; // -1 to account for next round button
            JokerObject* joker_object = (JokerObject*)list_get_at_idx(shop_jokers_list, idx);
            sprite_object_set_focus(joker_object->sprite_object, false);
        }
    }

    if (new_selection->y == row_idx)
    {
        if (new_selection->x == NEXT_ROUND_BTN_SEL_X)
        {
            button_set_highlight(&next_round_button, true);
        }
        else
        {
            int idx = new_selection->x - 1; // -1 to account for next round button
            JokerObject* joker_object = (JokerObject*)list_get_at_idx(shop_jokers_list, idx);
            sprite_object_set_focus(joker_object->sprite_object, true);
        }
    }

    return true;
}

/**
 * @brief Get size of the row with the "Reroll" button
 *
 * @returns Always 1, because there is only the "Reroll" button on that row.
 */
static int shop_reroll_row_get_size()
{
    return 1;
}

/**
 * @brief Handle d-pad inputs for the "Reroll" button's row.
 */
static bool shop_reroll_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    if (row_idx == prev_selection->y)
    {
        button_set_highlight(&reroll_button, false);

        if (new_selection->x != NEXT_ROUND_BTN_SEL_X)
        {
            int idx = new_selection->x - 1;
            JokerObject* joker_object = (JokerObject*)list_get_at_idx(get_shop_jokers_list(), idx);
            sprite_object_set_focus(joker_object->sprite_object, true);
        }
    }
    else if (row_idx == new_selection->y)
    {
        button_set_highlight(&reroll_button, true);
    }

    return true;
}

/**
 * @brief Reroll items up for sale in the Shop.
 *         Reroll cost will go up by a rate that increases by 1 each reroll.
 */
static inline void game_shop_reroll(int* reroll_cost)
{
    g_game_vars.money -= *reroll_cost;
    display_money(); // Update the money display

    List* shop_jokers_list = get_shop_jokers_list();
    ListItr itr = list_itr_create(shop_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL)
        {
            set_shop_joker_avail(joker_object->joker->id, true);
            joker_object_destroy(&joker_object); // Destroy the joker object if it exists
        }
    }

    list_clear(shop_jokers_list);
    *shop_jokers_list = list_create();

    game_shop_create_items();

    itr = list_itr_create(shop_jokers_list);

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

/**
 * @brief Handle button inputs for the "Reroll" button's row.
 */
static void shop_reroll_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (!key_hit(SELECT_CARD))
    {
        return;
    }

    button_press(&reroll_button);
}

static void next_round_on_pressed(void)
{
    // Go to next blind selection game state
    substate = GAME_SHOP_EXIT; // Go to the outro sequence state
    timer = TM_ZERO;
    reroll_cost = REROLL_BASE_COST;

    pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PAL_IDX] = pal_bg_mem[SHOP_PANEL_SHADOW_PAL_IDX];
}

static void reroll_on_pressed(void)
{
    // TODO: Add money sound effect
    game_shop_reroll(&reroll_cost);
}

static bool reroll_can_be_pressed(void)
{
    return g_game_vars.money >= reroll_cost;
}

/**
 * @brief Handle user inputs logic in the Shop though a SelectionGrid.
 */
static void game_shop_process_user_input()
{
    selection_grid_process_input(&shop_selection_grid);
}

/**
 * @brief Outro sequence substate update.
 *         This makes the menu and shop icon go out of frame.
 */
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

        ListItr itr = list_itr_create(get_shop_jokers_list());
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
        substate = GAME_SHOP_MAX; // Go to the next state
        timer = TM_ZERO;          // Reset the timer
    }
}

/**
 * @brief Cycling shop lights animation substate update.
 */
static inline void game_shop_lights_anim_frame(void)
{
    // Shift palette around the border of the shop icon
    COLOR shifted_palette[4];
    shifted_palette[0] = pal_bg_mem[SHOP_LIGHTS_2_PAL_IDX];
    shifted_palette[1] = pal_bg_mem[SHOP_LIGHTS_3_PAL_IDX];
    shifted_palette[2] = pal_bg_mem[SHOP_LIGHTS_4_PAL_IDX];
    shifted_palette[3] = pal_bg_mem[SHOP_LIGHTS_1_PAL_IDX];

    // Circularly shift the palette
    int last = shifted_palette[3];

    for (int i = 3; i > 0; --i)
    {
        shifted_palette[i] = shifted_palette[i - 1];
    }

    shifted_palette[0] = last;

    // Copy the shifted palette to the next 4 slots
    pal_bg_mem[SHOP_LIGHTS_2_PAL_IDX] = shifted_palette[0];
    pal_bg_mem[SHOP_LIGHTS_3_PAL_IDX] = shifted_palette[1];
    pal_bg_mem[SHOP_LIGHTS_4_PAL_IDX] = shifted_palette[2];
    pal_bg_mem[SHOP_LIGHTS_1_PAL_IDX] = shifted_palette[3];
}

void game_shop_on_update(void)
{
    timer++;

    List* shop_jokers_list = get_shop_jokers_list();

    if (!list_is_empty(shop_jokers_list))
    {
        ListItr itr = list_itr_create(shop_jokers_list);
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

    if (substate == GAME_SHOP_MAX)
    {
        game_change_state(GAME_STATE_BLIND_SELECT);
        return;
    }

    shop_state_actions[substate]();
}

void game_shop_on_exit(void)
{
    List* shop_jokers_list = get_shop_jokers_list();
    ListItr itr = list_itr_create(shop_jokers_list);
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

    list_clear(shop_jokers_list);

    increment_blind(BLIND_STATE_DEFEATED); // TODO: Move to game_round_end()?

    save_game();
}