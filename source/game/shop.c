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
#include "pack.h"
#include "planet.h"
#include "layout.h"
#include "list.h"
#include "mgba_logger.h"
#include "random.h"
#include "save.h"
#include "soundbank.h"
#include "state_machine.h"
#include "timer.h"
#include "util.h"

#include <string.h>

// Timer defs
#define TM_END_GAME_SHOP_INTRO    12
#define TM_CREATE_SHOP_ITEMS_WAIT 1
#define TM_SHIFT_SHOP_ICON_WAIT   7
#define TM_SHOW_CARD_DESC_WAIT    12
#define TM_HIDE_DECK_WAIT         5
#define TM_PACK_FIRST_SHAKE       24
#define TM_PACK_SHAKE_INTERVAL    10
#define TM_PACK_NUM_SHAKES        3
#define TM_PACK_POP               56

// Pixel sized
#define ITEM_SHOP_Y               71
#define OWNED_CARDS_HIDE_Y_OFFSET 50

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
#define SHOP_DESC_RARITY_MAIN_COLOR_PAL_IDX    27
#define SHOP_DESC_RARITY_SHADOW_COLOR_PAL_IDX  28

#define SHOP_LIGHTS_1_CLR 0xFFFF
#define SHOP_LIGHTS_2_CLR 0x32BE
#define SHOP_LIGHTS_3_CLR 0x4B5F
#define SHOP_LIGHTS_4_CLR 0x5F9F

// clang-format off
// Positions in tiles
static const Rect     SHOP_ICON_FROM_RECT           = {  0, 26,  8, 26};
static const BG_POINT SHOP_ICON_TO_POS              = {  0,  0};
static const BG_POINT OWNED_CARDS_PANEL_3X3_SRC_POS = { 29, 21};
static const Rect     OWNED_JOKERS_PANEL_RECT       = {  9,  1, 21,  5};
static const Rect     OWNED_CONSUMABLES_PANEL_RECT  = { 23,  1, 28,  5};
static const Rect     OWNED_CARDS_PANEL_RECT        = {  9,  1, 28,  5};
static const Rect     OWNED_CARDS_PANEL_ANIM_CLEAR  = {  9,  0, 28,  1};
static const Rect     CARD_DESC_9_PTCH_TO_RECT      = {  9,  6, 28, 18};
static const NinePatchRect CARD_DESC_9_PTCH_SRC = {
                                        .patch_rect = { 27, 25, 31, 31},
                                        .margins    = {  2,  3,  2,  3}
};
static const int      CARD_DESC_MAX_TEXT_HEIGHT     = CARD_DESC_9_PTCH_TO_RECT.bottom -
                                                      CARD_DESC_9_PTCH_TO_RECT.top + 1 -
                                                      CARD_DESC_9_PTCH_SRC.margins.top -
                                                      CARD_DESC_9_PTCH_SRC.margins.bottom;
static const Rect     CARD_DESC_TEXT_RECT           = { 11,  9, 26, 18};
static const Rect     CARD_NAME_TEXT_RECT           = { 10,  7, 27,  7};

// Positions in pixels
static const BG_POINT SHOP_JOKER_SPRITES_INIT_POS = {120, 160};
static const BG_POINT CARD_DESCRIPTION_SPRITE_POS = {135,   9};
static const Rect     SHOP_PRICES_TEXT_RECT       = { 72,  56, 192, 160 };
static const Rect     SHOP_REROLL_RECT            = { 88,  96, UNDEFINED, UNDEFINED };
// clang-format on

// Bottom row slots: Planet in the left panel, Joker Pack in the right one
#define BOTTOM_ROW_SEL_Y        3
#define BOTTOM_ITEM_Y           113
#define BOTTOM_PLANET_SPRITE_X  83
#define BOTTOM_PACK_SPRITE_X    137
#define BOTTOM_PRICE_TEXT_Y     128
#define BOTTOM_PLANET_PRICE_X   112
#define BOTTOM_PACK_PRICE_X     168
#define BOTTOM_PRICE_TEXT_WIDTH 16
#define PACK_HINT_TEXT_X        136
#define PACK_HINT_TEXT_Y        120
// Where a pack moves to while it is being opened, between the two top row slots
#define PACK_OPEN_X             (SHOP_JOKER_SPRITES_INIT_POS.x + CARD_SPRITE_SIZE / 2)
#define PACK_SFX_VOLUME         255

// Colors of the shop panel border, changed while a pack is open
#define SHOP_PANEL_BORDER_CLR             0x213D
#define SHOP_PANEL_BORDER_JOKER_PACK_CLR  0x029F
#define SHOP_PANEL_BORDER_PLANET_PACK_CLR 0x7E88

static List s_shop_items_list = LIST_DEFAULT;
static List s_shop_bottom_items_list = LIST_DEFAULT;
// Regular top row items put aside while a Joker Pack is open
static List s_stashed_items_list = LIST_DEFAULT;
static bool s_pack_open = false;
// The pack currently playing its opening animation, NULL otherwise
static Item* s_opening_pack = NULL;
static int s_opening_pack_price = 0;

enum GameShopStates
{
    GAME_SHOP_INTRO,
    GAME_SHOP_ACTIVE,
    GAME_SHOP_SHOW_CARD_DESC,
    GAME_SHOP_HIDE_CARD_DESC,
    GAME_SHOP_EXIT,
    GAME_SHOP_OPEN_PACK,
    GAME_SHOP_MAX
};

static void game_shop_intro(void);
static void game_shop_process_user_input(void);
static void game_shop_show_card_desc(void);
static void game_shop_hide_card_desc(void);
static void game_shop_outro(void);
static void game_shop_open_pack_anim(void);

static StateInfo shop_state_actions[GAME_SHOP_MAX] = {
    STATE_INFO_UPDATE_FN_ONLY(game_shop_intro),
    STATE_INFO_UPDATE_FN_ONLY(game_shop_process_user_input),
    STATE_INFO_UPDATE_FN_ONLY(game_shop_show_card_desc),
    STATE_INFO_UPDATE_FN_ONLY(game_shop_hide_card_desc),
    STATE_INFO_UPDATE_FN_ONLY(game_shop_outro),
    STATE_INFO_UPDATE_FN_ONLY(game_shop_open_pack_anim),
};

static StateMachine shop_sm = STATE_MACHINE_DEFINE(shop_state_actions, GAME_SHOP_MAX);

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
static int shop_bottom_row_get_size(void);
static bool shop_bottom_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void shop_bottom_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);

static SelectionGridRow shop_selection_rows[] = {
    {0, jokers_sel_row_get_size,  jokers_sel_row_on_selection_changed,  jokers_sel_row_on_key_transit,  {.wrap = false, .has_h_exit_idx = false, .h_exit_idx = 0}},
    {1, shop_top_row_get_size,    shop_top_row_on_selection_changed,    shop_top_row_on_key_transit,    {.wrap = false, .has_h_exit_idx = false, .h_exit_idx = 0}},
    {2, shop_reroll_row_get_size, shop_reroll_row_on_selection_changed, shop_reroll_row_on_key_transit, {.wrap = false, .has_h_exit_idx = true, .h_exit_idx = 1} },
    {BOTTOM_ROW_SEL_Y, shop_bottom_row_get_size, shop_bottom_row_on_selection_changed, shop_bottom_row_on_key_transit, {.wrap = false, .has_h_exit_idx = false, .h_exit_idx = 0}},
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

static int s_timer;

static int s_reroll_cost = REROLL_BASE_COST;

// Variables relative to the Card we are showing the description of

// TODO: Change this to item once it has description printing API.
static JokerObject* s_description_card = NULL;
static FIXED s_description_card_original_x_pos = UNDEFINED;
static FIXED s_description_card_original_y_pos = UNDEFINED;
static List* s_description_card_original_list = NULL;
static int s_show_description_anim_progress = 0;

JokerObject* game_shop_get_description_card(void)
{
    return s_description_card;
}

void game_shop_reset(void)
{
    list_clear(&s_shop_items_list);
    s_shop_items_list = list_init();
    list_clear(&s_shop_bottom_items_list);
    s_shop_bottom_items_list = list_init();
    list_clear(&s_stashed_items_list);
    s_stashed_items_list = list_init();
    s_pack_open = false;
    s_opening_pack = NULL;
    joker_reset_rollable_jokers();
}

void game_shop_change_background(void)
{
    toggle_windows(false, true);

    GRIT_CPY(pal_bg_mem, background_shop_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_shop_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_shop_gfxMap);

    // Set the outline colors for the shop background. This is used for the alternate shop
    // palettes when opening packs
    pal_bg_mem[SHOP_BOTTOM_PANEL_BORDER_PAL_IDX] = SHOP_PANEL_BORDER_CLR;
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

    s_timer = TM_ZERO;

    state_machine_register(&shop_sm);
    state_machine_change_state(&shop_sm, GAME_SHOP_INTRO);

    // The selection grid is initialized outside of bounds and moved
    // to trigger the selection change so the initial selection is visible
    shop_selection_grid.selection = SHOP_INIT_SEL;
    selection_grid_move_selection_horz(&shop_selection_grid, 1);
}

/**
 * @brief Create a shop top row item - jokers, consumables, and possibly playing cards
 * Currently only jokers are implemented.
 */
static Item* game_shop_create_top_row_item(void)
{
    return item_roll_new(ITEM_TYPE_JOKER, RNG_SEQ_SHOP_ITEMS);
}

/**
 * @brief Print the price under a top row item, Jokers offered by an open pack are free.
 */
static void shop_print_top_item_price(Item* item)
{
    if (s_pack_open)
    {
        sprite_object_print_price_under((SpriteObject*)item, 0);
    }
    else
    {
        item_print_buy_price_under(item);
    }
}

static inline int shop_bottom_item_price_x(const Item* item)
{
    return (item->type == ITEM_TYPE_PACK) ? BOTTOM_PACK_PRICE_X : BOTTOM_PLANET_PRICE_X;
}

static void shop_erase_bottom_item_price(const Item* item)
{
    int price_x = shop_bottom_item_price_x(item);
    Rect price_rect = {
        price_x,
        BOTTOM_PRICE_TEXT_Y,
        price_x + BOTTOM_PRICE_TEXT_WIDTH,
        BOTTOM_PRICE_TEXT_Y + TILE_SIZE
    };
    tte_erase_rect_wrapper(price_rect);
}

static void shop_erase_pack_hint(void)
{
    Rect hint_rect = {PACK_HINT_TEXT_X, PACK_HINT_TEXT_Y, 192, PACK_HINT_TEXT_Y + 2 * TILE_SIZE};
    tte_erase_rect_wrapper(hint_rect);
}

/**
 * @brief Print the prices next to the bottom row items, and the pack hint if a pack is open.
 */
static void shop_print_bottom_row_text(void)
{
    ListItr itr = list_itr_create(&s_shop_bottom_items_list);
    Item* item;
    while (!s_pack_open && (item = list_itr_next(&itr)))
    {
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            shop_bottom_item_price_x(item),
            BOTTOM_PRICE_TEXT_Y,
            TTE_WHITE_PB,
            item_get_buy_price(item)
        );
    }

    if (s_pack_open)
    {
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}Pick 1#{P:%d,%d}or Next",
            PACK_HINT_TEXT_X,
            PACK_HINT_TEXT_Y,
            TTE_WHITE_PB,
            PACK_HINT_TEXT_X,
            PACK_HINT_TEXT_Y + TILE_SIZE
        );
    }
}

/**
 * @brief Create the bottom row items: one Planet Pack and one Joker Pack per shop visit.
 *        They are not affected by rerolls.
 */
static void game_shop_create_bottom_row_items(void)
{
    List* bottom_list = &s_shop_bottom_items_list;
    list_clear(bottom_list);
    *bottom_list = list_init();

    Item* planet = item_roll_new(ITEM_TYPE_PLANET_PACK, RNG_SEQ_SHOP_ITEMS);
    if (planet != NULL)
    {
        planet->x = int2fx(BOTTOM_PLANET_SPRITE_X);
        planet->y = int2fx(SHOP_JOKER_SPRITES_INIT_POS.y);
        planet->tx = planet->x;
        planet->ty = int2fx(BOTTOM_ITEM_Y);
        list_push_back(bottom_list, planet);
    }

    Item* pack = item_roll_new(ITEM_TYPE_PACK, RNG_SEQ_SHOP_ITEMS);
    if (pack != NULL)
    {
        pack->x = int2fx(BOTTOM_PACK_SPRITE_X);
        pack->y = int2fx(SHOP_JOKER_SPRITES_INIT_POS.y);
        pack->tx = pack->x;
        pack->ty = int2fx(BOTTOM_ITEM_Y);
        list_push_back(bottom_list, pack);
    }

    shop_print_bottom_row_text();
}

/**
 * @brief Start opening a bought pack: put the regular top row items aside, move the other
 *        bottom row item away and send the pack to the middle of the shop for its animation.
 */
static void game_shop_begin_open_pack(Item* pack, int price)
{
    List* shop_items_list = &s_shop_items_list;
    ListItr itr = list_itr_create(shop_items_list);
    Item* item;

    while ((item = list_itr_next(&itr)))
    {
        sprite_object_erase_text_under((SpriteObject*)item);
        sprite_object_set_focus((SpriteObject*)item, false);
        item->ty = int2fx(SHOP_JOKER_SPRITES_INIT_POS.y + TILE_SIZE);
        list_push_back(&s_stashed_items_list, item);
    }

    list_clear(shop_items_list);
    *shop_items_list = list_init();

    // The remaining bottom row items step aside while the pack is open
    itr = list_itr_create(&s_shop_bottom_items_list);
    while ((item = list_itr_next(&itr)))
    {
        shop_erase_bottom_item_price(item);
        item->ty = int2fx(SHOP_JOKER_SPRITES_INIT_POS.y + TILE_SIZE);
    }

    s_pack_open = true;
    s_opening_pack = pack;
    s_opening_pack_price = price;

    pack->tx = int2fx(PACK_OPEN_X);
    pack->ty = int2fx(ITEM_SHOP_Y);

    pal_bg_mem[SHOP_BOTTOM_PANEL_BORDER_PAL_IDX] = (pack->type == ITEM_TYPE_PLANET_PACK)
                                                     ? SHOP_PANEL_BORDER_PLANET_PACK_CLR
                                                     : SHOP_PANEL_BORDER_JOKER_PACK_CLR;

    state_machine_change_state(&shop_sm, GAME_SHOP_OPEN_PACK);
    s_timer = TM_ZERO;
}

/**
 * @brief Offer the content of the pack that just opened, the items come out of the pack's
 *        position and move to the top row slots.
 * @param content_type the type of items offered, Jokers or Planets
 * @return false if nothing could be offered
 */
static bool game_shop_spawn_pack_content(enum ItemType content_type)
{
    List* shop_items_list = &s_shop_items_list;

    for (int i = 0; i < PACK_NUM_CHOICES; i++)
    {
        Item* joker = item_roll_new(content_type, RNG_SEQ_SHOP_ITEMS);

        // Don't offer the same Planet twice in one pack
        if (content_type == ITEM_TYPE_PLANET && i > 0)
        {
            PlanetObject* first = (PlanetObject*)list_get_at_idx(shop_items_list, 0);
            for (int tries = 0; tries < 8 && joker != NULL && first != NULL &&
                                ((PlanetObject*)joker)->hand_type == first->hand_type;
                 tries++)
            {
                item_dispose(&joker);
                joker = item_roll_new(content_type, RNG_SEQ_SHOP_ITEMS);
            }
        }

        if (joker == NULL)
            break;

        joker->x = int2fx(PACK_OPEN_X);
        joker->y = int2fx(ITEM_SHOP_Y);
        joker->tx = int2fx(SHOP_JOKER_SPRITES_INIT_POS.x + i * CARD_SPRITE_SIZE);
        joker->ty = int2fx(ITEM_SHOP_Y);
        shop_print_top_item_price(joker);
        list_push_back(shop_items_list, joker);
    }

    shop_print_bottom_row_text();

    return !list_is_empty(shop_items_list);
}

/**
 * @brief Close the open Joker Pack: discard what was not picked and bring the regular
 *        top row items back.
 */
static void game_shop_close_pack(void)
{
    List* shop_items_list = &s_shop_items_list;
    ListItr itr = list_itr_create(shop_items_list);
    Item* item;

    while ((item = list_itr_next(&itr)))
    {
        sprite_object_erase_text_under((SpriteObject*)item);
        item_dispose(&item);
    }

    list_clear(shop_items_list);
    *shop_items_list = list_init();

    s_pack_open = false;
    shop_erase_pack_hint();
    pal_bg_mem[SHOP_BOTTOM_PANEL_BORDER_PAL_IDX] = SHOP_PANEL_BORDER_CLR;

    if (s_opening_pack != NULL)
    {
        item_dispose(&s_opening_pack);
    }

    itr = list_itr_create(&s_stashed_items_list);
    while ((item = list_itr_next(&itr)))
    {
        item->ty = int2fx(ITEM_SHOP_Y);
        list_push_back(shop_items_list, item);
        shop_print_top_item_price(item);
    }

    list_clear(&s_stashed_items_list);
    s_stashed_items_list = list_init();

    itr = list_itr_create(&s_shop_bottom_items_list);
    while ((item = list_itr_next(&itr)))
    {
        item->ty = int2fx(BOTTOM_ITEM_Y);
    }
    shop_print_bottom_row_text();

    // Put the cursor back on the "Next Round" button
    shop_selection_grid.selection = (Selection){NEXT_ROUND_BTN_SEL_X, 1};
    button_set_highlight(&next_round_button, true);
}

/**
 * @brief Setup for the lists of items we can purchase in the top row of the Shop.
 *        i.e. Jokers and consumables and possibly playing cards.
 */
static void game_shop_create_top_row_items(void)
{
    tte_erase_rect_wrapper(SHOP_PRICES_TEXT_RECT);

    List* shop_items_list = &s_shop_items_list;

    list_clear(shop_items_list);
    *shop_items_list = list_init();

    for (int i = 0; i < MAX_SHOP_ITEMS; i++)
    {
        Item* item = game_shop_create_top_row_item();

        if (item == NULL)
        {
            MGBA_FUNC_WARN("Could not create shop item");

            // TODO: Decide how to handle this case gracefully
            // If the issue for example is that we can't generate any more jokers,
            // because for example the user owns all of them,
            // maybe we need to generate consumables instead.
            return;
        }

        item->x = int2fx(SHOP_JOKER_SPRITES_INIT_POS.x + i * CARD_SPRITE_SIZE);
        item->y = int2fx(SHOP_JOKER_SPRITES_INIT_POS.y);
        item->tx = item->x;
        item->ty = int2fx(ITEM_SHOP_Y);

        shop_print_top_item_price(item);

        list_push_back(shop_items_list, item);
    }
}

/**
 * @brief Intro sequence (menu and shop icon coming into frame)
 */
static void game_shop_intro()
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (s_timer == TM_CREATE_SHOP_ITEMS_WAIT)
    {
        game_shop_create_top_row_items();
        game_shop_create_bottom_row_items();
    }

    if (s_timer >= TM_SHIFT_SHOP_ICON_WAIT) // Shift the shop icon
    {
        int timer_offset = s_timer - 6;

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

    if (s_timer == TM_END_GAME_SHOP_INTRO)
    {
        state_machine_change_state(&shop_sm, GAME_SHOP_ACTIVE);
        s_timer = TM_ZERO; // Reset the timer

        // print initial reroll cost only when the panel is in place
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            SHOP_REROLL_RECT.left,
            SHOP_REROLL_RECT.top,
            TTE_WHITE_PB,
            s_reroll_cost
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
    return list_get_len(&s_shop_items_list) + 1;
}

/**
 * @brief Called when pressing A on a Shop Joker to buy it.
 */
static inline void game_shop_buy_item(int shop_item_idx)
{
    List* shop_items_list = &s_shop_items_list;
    Item* item = (Item*)list_get_at_idx(shop_items_list, shop_item_idx);

    g_game_vars.money -= item_get_buy_price(item);
    display_money();
    sprite_object_erase_text_under((SpriteObject*)item);
    sprite_object_set_focus((SpriteObject*)item, false);
    item_acquire(item);
    list_remove_at_idx(shop_items_list, shop_item_idx); // Remove the joker from the shop
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
        if (s_pack_open)
        {
            // "Next Round" skips the pack instead of leaving the shop
            game_shop_close_pack();
            return;
        }

        button_press(&next_round_button);
    }
    else
    {
        int shop_item_idx = selection->x - 1; // - 1 to account for next round button
        Item* item = (Item*)list_get_at_idx(&s_shop_items_list, shop_item_idx);

        if (s_pack_open)
        {
            // Jokers from a pack are free, picking one closes the pack
            if (item == NULL || !item_can_acquire(item))
            {
                return;
            }

            sprite_object_erase_text_under((SpriteObject*)item);
            sprite_object_set_focus((SpriteObject*)item, false);
            item_acquire(item);
            list_remove_at_idx(&s_shop_items_list, shop_item_idx);
            game_shop_close_pack();
            return;
        }

        if (!item_can_acquire(item) || g_game_vars.money < item_get_buy_price(item))
        {
            return;
        }

        game_shop_buy_item(shop_item_idx);
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
    List* shop_items_list = &s_shop_items_list;
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
            SpriteObject* sprite_object = (SpriteObject*)list_get_at_idx(shop_items_list, idx);
            sprite_object_set_focus(sprite_object, false);
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
            SpriteObject* sprite_object = (SpriteObject*)list_get_at_idx(shop_items_list, idx);
            sprite_object_set_focus(sprite_object, true);
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

        if (new_selection->y == 1 && new_selection->x != NEXT_ROUND_BTN_SEL_X)
        {
            int idx = new_selection->x - 1;
            SpriteObject* sprite_object = (SpriteObject*)list_get_at_idx(&s_shop_items_list, idx);
            sprite_object_set_focus(sprite_object, true);
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
static inline void game_shop_reroll(void)
{
    g_game_vars.money -= s_reroll_cost;
    display_money(); // Update the money display

    List* shop_items_list = &s_shop_items_list;
    ListItr itr = list_itr_create(shop_items_list);
    Item* item;

    while ((item = list_itr_next(&itr)))
    {
        if (item != NULL)
        {
            item_dispose(&item);
        }
    }

    list_clear(shop_items_list);
    *shop_items_list = list_init();

    game_shop_create_top_row_items();
    shop_print_bottom_row_text();

    itr = list_itr_create(shop_items_list);

    SpriteObject* item_sprite_object;
    while ((item_sprite_object = list_itr_next(&itr)))
    {
        if (item_sprite_object != NULL)
        {
            item_sprite_object->y = item_sprite_object->ty;

            sprite_object_shake(item_sprite_object, UNDEFINED);
        }
    }

    s_reroll_cost++;
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}$%d",
        SHOP_REROLL_RECT.left,
        SHOP_REROLL_RECT.top,
        TTE_WHITE_PB,
        s_reroll_cost
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

static int shop_bottom_row_get_size(void)
{
    // The bottom row can't be reached while a pack is open
    return s_pack_open ? 0 : list_get_len(&s_shop_bottom_items_list);
}

/**
 * @brief Handle d-pad inputs for the bottom row (Planet and Joker Pack).
 */
static bool shop_bottom_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    List* bottom_list = &s_shop_bottom_items_list;
    int row_size = list_get_len(bottom_list);

    if (prev_selection->y == row_idx && prev_selection->x >= 0 && prev_selection->x < row_size)
    {
        sprite_object_set_focus(
            (SpriteObject*)list_get_at_idx(bottom_list, prev_selection->x),
            false
        );
    }

    if (new_selection->y == row_idx && new_selection->x >= 0 && new_selection->x < row_size)
    {
        sprite_object_set_focus((SpriteObject*)list_get_at_idx(bottom_list, new_selection->x), true);
    }

    return true;
}

/**
 * @brief Handle button inputs for the bottom row: buy the selected Planet or Joker Pack.
 */
static void shop_bottom_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (!key_hit(SELECT_CARD) || s_pack_open)
        return;

    List* bottom_list = &s_shop_bottom_items_list;
    Item* item = (Item*)list_get_at_idx(bottom_list, selection->x);
    if (item == NULL)
        return;

    int price = item_get_buy_price(item);
    if (!item_can_acquire(item) || g_game_vars.money < price)
        return;

    g_game_vars.money -= price;
    display_money();
    shop_erase_bottom_item_price(item);
    sprite_object_set_focus((SpriteObject*)item, false);
    list_remove_at_idx(bottom_list, selection->x);

    if (item_is_pack(item))
    {
        // The pack is consumed at the end of its opening animation
        game_shop_begin_open_pack(item, price);
        return;
    }

    item_acquire(item); // Planets are consumed on purchase

    if (!list_is_empty(bottom_list))
    {
        selection_grid->selection = (Selection){0, BOTTOM_ROW_SEL_Y};
        sprite_object_set_focus((SpriteObject*)list_get_at_idx(bottom_list, 0), true);
    }
    else
    {
        selection_grid->selection = (Selection){0, 2};
        button_set_highlight(&reroll_button, true);
    }
}

/**
 * @brief Pack opening substate: the pack shakes in the middle of the shop, then pops and
 *        its content comes out.
 */
static void game_shop_open_pack_anim(void)
{
    if (s_opening_pack == NULL)
    {
        state_machine_change_state(&shop_sm, GAME_SHOP_ACTIVE);
        s_timer = TM_ZERO;
        return;
    }

    int shake_timer = s_timer - TM_PACK_FIRST_SHAKE;
    if (shake_timer >= 0 && shake_timer % TM_PACK_SHAKE_INTERVAL == 0 &&
        shake_timer / TM_PACK_SHAKE_INTERVAL < TM_PACK_NUM_SHAKES)
    {
        sprite_object_shake((SpriteObject*)s_opening_pack, SFX_CARD_FOCUS);
    }

    if (s_timer < TM_PACK_POP)
        return;

    enum ItemType content_type = pack_get_content_type(s_opening_pack->type);
    item_acquire(s_opening_pack); // Consumes the pack
    s_opening_pack = NULL;
    play_sfx(SFX_POP, MM_BASE_PITCH_RATE, PACK_SFX_VOLUME);

    if (game_shop_spawn_pack_content(content_type))
    {
        // Move the cursor on the first item offered by the pack
        shop_selection_grid.selection = (Selection){1, 1};
        sprite_object_set_focus((SpriteObject*)list_get_at_idx(&s_shop_items_list, 0), true);
    }
    else
    {
        // Nothing to offer, refund and restore the shop
        g_game_vars.money += s_opening_pack_price;
        display_money();
        game_shop_close_pack();
    }

    state_machine_change_state(&shop_sm, GAME_SHOP_ACTIVE);
    s_timer = TM_ZERO;
}

static void next_round_on_pressed(void)
{
    // Go to next blind selection game state
    state_machine_change_state(&shop_sm, GAME_SHOP_EXIT);
    s_timer = TM_ZERO;
    s_reroll_cost = REROLL_BASE_COST;

    pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PAL_IDX] = pal_bg_mem[SHOP_PANEL_SHADOW_PAL_IDX];
}

static void reroll_on_pressed(void)
{
    // TODO: Add money sound effect
    game_shop_reroll();
}

static bool reroll_can_be_pressed(void)
{
    return !s_pack_open && g_game_vars.money >= s_reroll_cost;
}

/**
 * @brief Handle user inputs logic in the Shop though a SelectionGrid.
 */
static void game_shop_process_user_input(void)
{
    selection_grid_process_input(&shop_selection_grid);

    static JokerObject* tmp_card = NULL;

    // Determine the Joker we would show the description of
    switch (shop_selection_grid.selection.y)
    {
        // Owned Joker
        case 0:
        {
            s_description_card_original_list = get_jokers_list();
            tmp_card = list_get_at_idx(get_jokers_list(), shop_selection_grid.selection.x);
            break;
        }

        // Jokers for sale
        case 1:
        {
            s_description_card_original_list = &s_shop_items_list;
            tmp_card = (shop_selection_grid.selection.x > 0)
                         ? list_get_at_idx(&s_shop_items_list, shop_selection_grid.selection.x - 1)
                         : NULL;
            break;
        }

        // Planet and Joker Pack
        case BOTTOM_ROW_SEL_Y:
        {
            s_description_card_original_list = &s_shop_bottom_items_list;
            tmp_card =
                list_get_at_idx(&s_shop_bottom_items_list, shop_selection_grid.selection.x);
            break;
        }

        default:
        {
            s_description_card_original_list = NULL;
            tmp_card = NULL;
            break;
        }
    }

    // Show description of selected card when pressing B.
    // Always wait for the card in question to be immobile to avoid accumulating
    // errors when pressing and releasing B in quick succession.
    if (tmp_card != NULL && tmp_card->vx == 0 && tmp_card->vy == 0 && key_held(DESELECT_CARDS))
    {
        s_description_card = tmp_card;
        s_description_card_original_x_pos = s_description_card->x;
        s_description_card_original_y_pos = s_description_card->y;

        s_timer = TM_ZERO;
        state_machine_change_state(&shop_sm, GAME_SHOP_SHOW_CARD_DESC);
    }
}

static void game_shop_show_card_desc(void)
{
    // Anim start
    if (s_timer == 1)
    {
        // This starts at 0, then gets incremented up to TM_SHOW_CARD_DESC_WAIT. Will be used to
        // revert the animation if the B button is released midway through it
        s_show_description_anim_progress = 0;

        // Erase shop text and disable transparency window

        tte_erase_rect_wrapper(PLAYING_SCREEN_RECT);
        toggle_windows(false, true);

        // Move all other Jokers offscreen

        JokerObject* joker_object = NULL;

        // Owned Jokers
        ListItr itr = list_itr_create(get_jokers_list());
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != s_description_card)
                joker_object->ty -= int2fx(OWNED_CARDS_HIDE_Y_OFFSET);
        }

        // Shop Jokers
        itr = list_itr_create(&s_shop_items_list);
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != s_description_card)
                joker_object->ty = int2fx(SHOP_JOKER_SPRITES_INIT_POS.y + TILE_SIZE);
        }

        // Bottom row items
        itr = list_itr_create(&s_shop_bottom_items_list);
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != s_description_card)
                joker_object->ty = int2fx(SHOP_JOKER_SPRITES_INIT_POS.y + TILE_SIZE);
        }

        // Set description_card new target position

        s_description_card->tx = int2fx(CARD_DESCRIPTION_SPRITE_POS.x);
        s_description_card->ty = int2fx(CARD_DESCRIPTION_SPRITE_POS.y);
    }

    if (s_timer <= TM_SHOW_CARD_DESC_WAIT)
    {
        s_show_description_anim_progress++;

        // Hide Deck (last frames only)
        if (TM_SHOW_CARD_DESC_WAIT - s_timer < TM_HIDE_DECK_WAIT)
            main_bg_se_move_rect_1_tile_vert(DECK_ANIM_RECT, SCREEN_DOWN);
        // Hide shop panel
        main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_DOWN);
        // Hide Owned Cards panels
        main_bg_se_move_rect_1_tile_vert(OWNED_CARDS_PANEL_RECT, SCREEN_UP);
    }

    // Anim end
    else if (s_timer == TM_SHOW_CARD_DESC_WAIT + 1)
    {
        // Compute needed space for the description
        const char* desc_name = NULL;
        const char* rarity_str = NULL;
        u8 desc_rarity = COMMON_JOKER;
        int desc_num_lines = 0;

        if (((Item*)s_description_card)->type == ITEM_TYPE_PLANET)
        {
            PlanetObject* planet = (PlanetObject*)s_description_card;
            desc_name = planet_get_name(planet->hand_type);
            rarity_str = "Planet";
            desc_num_lines = planet_object_print_desc(planet, CARD_DESC_TEXT_RECT);
        }
        else if (item_is_pack((Item*)s_description_card))
        {
            enum ItemType pack_type = ((Item*)s_description_card)->type;
            desc_name = pack_get_name(pack_type);
            rarity_str = "Pack";
            desc_num_lines = pack_object_print_desc(pack_type, CARD_DESC_TEXT_RECT);
        }
        else
        {
            const JokerInfo* info = get_joker_registry_entry(s_description_card->joker->id);
            desc_name = info->name;
            desc_rarity = info->rarity;
            rarity_str = joker_get_rarity_string(info->rarity);
            desc_num_lines =
                info->joker_print_desc(s_description_card->joker, CARD_DESC_TEXT_RECT);
        }

        int desc_bottom_offset = CARD_DESC_MAX_TEXT_HEIGHT - desc_num_lines;

        // Print Rarity and change color or the panel
        // Do it before drawing the panel so the color is already set
        tte_printf(
            TTE_WHITE_TAG "#{P:%d,%d}%*s%s",
            CARD_DESC_TEXT_RECT.left * TILE_SIZE,
            (CARD_DESC_TEXT_RECT.bottom - desc_bottom_offset - 1) * TILE_SIZE,
            (rect_width(&CARD_DESC_TEXT_RECT) - strlen(rarity_str)) / 2,
            "",
            rarity_str
        );
        pal_bg_mem[SHOP_DESC_RARITY_MAIN_COLOR_PAL_IDX] =
            joker_get_rarity_color(desc_rarity, true);
        pal_bg_mem[SHOP_DESC_RARITY_SHADOW_COLOR_PAL_IDX] =
            joker_get_rarity_color(desc_rarity, false);

        // Draw description panel
        Rect actual_dest_rect = CARD_DESC_9_PTCH_TO_RECT;
        actual_dest_rect.bottom -= desc_bottom_offset;
        main_bg_se_copy_expand_9_patch(actual_dest_rect, &CARD_DESC_9_PTCH_SRC);

        // Print joker name
        tte_printf(
            TTE_WHITE_TAG "#{P:%d,%d}%*s%s",
            CARD_NAME_TEXT_RECT.left * TILE_SIZE,
            CARD_NAME_TEXT_RECT.top * TILE_SIZE,
            (rect_width(&CARD_NAME_TEXT_RECT) - strlen(desc_name)) / 2,
            "",
            desc_name
        );
    }

    // Actively wait for the B button to be released
    if (!key_held(DESELECT_CARDS))
    {
        s_timer = TM_ZERO;
        state_machine_change_state(&shop_sm, GAME_SHOP_HIDE_CARD_DESC);
    }
}

static void game_shop_hide_card_desc(void)
{
    // just so we don't print the price of an owned Joker too many times
    static bool owned_joker_price_printed = false;

    // Anim start
    if (s_timer == 1)
    {
        // Erase shop text and Joker Description frame if we had time to draw them
        if (s_show_description_anim_progress >= TM_SHOW_CARD_DESC_WAIT)
        {
            main_bg_se_clear_rect(CARD_DESC_9_PTCH_TO_RECT);
        }
        // Or clear the owned cards' panel that haven't finished moving up
        else
        {
            main_bg_se_clear_rect(OWNED_CARDS_PANEL_ANIM_CLEAR);
        }

        tte_erase_rect_wrapper(PLAYING_SCREEN_RECT);

        // Enable transparency window
        toggle_windows(false, true);

        // Redraw Jokers/Consumables frames
        main_bg_se_copy_expand_3x3_rect(OWNED_JOKERS_PANEL_RECT, OWNED_CARDS_PANEL_3X3_SRC_POS);
        main_bg_se_copy_expand_3x3_rect(
            OWNED_CONSUMABLES_PANEL_RECT,
            OWNED_CARDS_PANEL_3X3_SRC_POS
        );

        // Move Jokers back to their positions
        JokerObject* joker_object = NULL;

        // Owned Jokers
        ListItr itr = list_itr_create(get_jokers_list());
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != s_description_card)
                joker_object->ty = int2fx(HELD_JOKERS_POS.y);
        }

        // Shop Jokers
        itr = list_itr_create(&s_shop_items_list);
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != s_description_card)
                joker_object->ty = int2fx(ITEM_SHOP_Y);
        }

        // Bottom row items
        itr = list_itr_create(&s_shop_bottom_items_list);
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != s_description_card)
            {
                joker_object->ty = s_pack_open
                                     ? int2fx(SHOP_JOKER_SPRITES_INIT_POS.y + TILE_SIZE)
                                     : int2fx(BOTTOM_ITEM_Y);
            }
        }

        s_description_card->tx = s_description_card_original_x_pos;
        s_description_card->ty = s_description_card_original_y_pos;
    }

    if (s_timer <= s_show_description_anim_progress)
    {
        // Show Deck (last frames only)
        if (s_show_description_anim_progress > TM_HIDE_DECK_WAIT &&
            s_timer < (s_show_description_anim_progress - (TM_HIDE_DECK_WAIT + 1)))
        {
            main_bg_se_move_rect_1_tile_vert(DECK_ANIM_RECT, SCREEN_UP);
        }
        // Show shop panel
        main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);
    }

    // Last anim frame (no need to wait for the Joker to have stopped for this):
    else if (s_timer == s_show_description_anim_progress + 1)
    {
        // Need to account for the description_card being selected if it came from the shop.
        if (s_description_card_original_list == &s_shop_items_list)
            s_description_card->ty += int2fx(TILE_SIZE);

        // Print price under shop Jokers
        Item* item = NULL;
        ListItr itr = list_itr_create(&s_shop_items_list);
        while ((item = list_itr_next(&itr)))
        {
            shop_print_top_item_price(item);
        }
        shop_print_bottom_row_text();

        if (s_description_card_original_list == &s_shop_items_list)
            s_description_card->ty -= int2fx(TILE_SIZE);

        // Print Reroll prince
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            SHOP_REROLL_RECT.left,
            SHOP_REROLL_RECT.top,
            TTE_WHITE_PB,
            s_reroll_cost
        );

        // Print Deck size that was erased
        display_deck_size_max();
    }

    // Cleanup and change state
    else if (s_description_card->vx == 0 && s_description_card->vy == 0)
    {
        owned_joker_price_printed = false;
        s_description_card = NULL;
        s_timer = TM_ZERO;
        state_machine_change_state(&shop_sm, GAME_SHOP_ACTIVE);
    }

    // At any point after the other prices have been printed, and while the card is still moving,
    // if we are NOT pressing A, print the price under it.
    else if (!owned_joker_price_printed && !key_held(SELECT_CARD) &&
             s_description_card_original_list == get_jokers_list())
    {
        owned_joker_price_printed = true;
        sprite_object_print_price_under(
            (SpriteObject*)s_description_card,
            joker_get_sell_value(s_description_card->joker)
        );
    }
}

/**
 * @brief Outro sequence substate update.
 *         This makes the menu and shop icon go out of frame.
 */
static void game_shop_outro(void)
{
    // Shift the shop panel
    main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_DOWN);

    main_bg_se_copy_rect_1_tile_vert(TOP_LEFT_PANEL_ANIM_RECT, SCREEN_UP);

    if (s_timer == 1)
    {
        tte_erase_rect_wrapper(SHOP_PRICES_TEXT_RECT); // Erase the shop prices text

        ListItr itr = list_itr_create(&s_shop_items_list);
        SpriteObject* shop_item;
        while ((shop_item = list_itr_next(&itr)))
        {
            if (shop_item != NULL)
            {
                shop_item->ty = int2fx(160);
            }
        }

        itr = list_itr_create(&s_shop_bottom_items_list);
        while ((shop_item = list_itr_next(&itr)))
        {
            shop_item->ty = int2fx(160);
        }

        reset_top_left_panel_bottom_row();
    }
    else if (s_timer == 2)
    {
        // TODO: make heads or tails of what's going on here and replace
        // magic numbers.
        int y = 5;
        memset16(&se_mat[MAIN_BG_SBB][y - 1][0], 0x0001, 1);
        memset16(&se_mat[MAIN_BG_SBB][y - 1][1], 0x0002, 7);
        memset16(&se_mat[MAIN_BG_SBB][y - 1][8], SE_HFLIP | 0x0001, 1);
    }

    if (s_timer >= MENU_POP_OUT_ANIM_FRAMES)
    {
        game_change_state(GAME_STATE_BLIND_SELECT);
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
    s_timer++;

    if (s_timer % 20 == 0)
    {
        game_shop_lights_anim_frame();
    }
}

void game_shop_on_exit(void)
{
    List* shop_items_list = &s_shop_items_list;
    ListItr itr = list_itr_create(shop_items_list);
    Item* item;

    while ((item = list_itr_next(&itr)))
    {
        item_dispose(&item);
    }

    list_clear(shop_items_list);

    itr = list_itr_create(&s_shop_bottom_items_list);
    while ((item = list_itr_next(&itr)))
    {
        item_dispose(&item);
    }
    list_clear(&s_shop_bottom_items_list);

    itr = list_itr_create(&s_stashed_items_list);
    while ((item = list_itr_next(&itr)))
    {
        item_dispose(&item);
    }
    list_clear(&s_stashed_items_list);
    s_pack_open = false;
    if (s_opening_pack != NULL)
    {
        item_dispose(&s_opening_pack);
    }

    increment_blind(BLIND_STATE_DEFEATED); // TODO: Move to game_round_end()?

    state_machine_remove(&shop_sm);

    save_game();
}
