#include "game/main_menu.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "background_main_menu_gfx.h"
#include "button.h"
#include "card.h"
#include "game.h"
#include "game/common_ui.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "sprite.h"
#include "version.h"

#include <stdint.h>
#include <tonc.h>
#include <tonc_math.h>
#include <tonc_memdef.h>

#define PLAY_BUTTON_MAIN_COLOR_PID    1
#define PLAY_BUTTON_OUTLINE_PID       2
#define OPTIONS_BUTTON_MAIN_COLOR_PID 3
#define OPTIONS_BUTTON_OUTLINE_PID    4

enum MainButtons
{
    PLAY_BTN_IDX,
    OPTIONS_BTN_IDX,
    MAIN_MENU_NB_BTN
};

// Pixel sizes
#define MAIN_MENU_ACE_T_X 88
#define MAIN_MENU_ACE_T_Y 26

// clang-format off
static const BG_POINT GBALATRO_VERSION_TEXT_POS  = {0, 152};
static const Rect     GBALATRO_VERSION_TEXT_RECT = {0, 152, 240, 160};
// clang-format on

// Define SelectionGrid for the main menu buttons

static int main_menu_return_row_size(void);
static bool main_menu_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void main_menu_on_key_transit(SelectionGrid* selection_grid, Selection* selection);

static void play_on_pressed(void);
static void options_on_pressed(void);

// clang-format off
static SelectionGridRow main_menu_selection_rows[] = {
    {
        0,
        main_menu_return_row_size,
        main_menu_on_selection_changed,
        main_menu_on_key_transit,
        {.wrap = false}
    },
};

static Button main_menu_buttons[] = {
    {PLAY_BUTTON_OUTLINE_PID,    PLAY_BUTTON_MAIN_COLOR_PID,    play_on_pressed,    NULL},
    {OPTIONS_BUTTON_OUTLINE_PID, OPTIONS_BUTTON_MAIN_COLOR_PID, options_on_pressed, NULL},
};

static SelectionGrid main_menu_selection_grid = {
    main_menu_selection_rows,
    1,
    {0, 0}
};
// clang-format on

// Main menu sprite - the ace of spades
static CardObject* main_menu_ace = NULL;

// Keep track of last highlighted button
static enum MainButtons last_highlighted_button = PLAY_BTN_IDX;

void game_main_menu_change_background(void)
{
    toggle_windows(false, false);

    tte_erase_screen();
    GRIT_CPY(pal_bg_mem, background_main_menu_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_main_menu_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_main_menu_gfxMap);

    // Disable all button highlight colors
    for (int i = 0; i < MAIN_MENU_NB_BTN; i++)
    {
        button_set_highlight(&main_menu_buttons[i], false);
    }
}

void game_main_menu_on_init(void)
{
    affine_background_change_background(AFFINE_BG_MAIN_MENU);
    change_background(BG_MAIN_MENU);
    main_menu_ace = card_object_new(card_new(SPADES, ACE));
    card_object_set_sprite(main_menu_ace, 0);
    main_menu_ace->sprite_object->sprite->obj->attr0 |= ATTR0_AFF_DBL;
    main_menu_ace->sprite_object->tx = int2fx(MAIN_MENU_ACE_T_X);
    main_menu_ace->sprite_object->x = main_menu_ace->sprite_object->tx;
    main_menu_ace->sprite_object->ty = int2fx(MAIN_MENU_ACE_T_Y);
    main_menu_ace->sprite_object->y = main_menu_ace->sprite_object->ty;
    main_menu_ace->sprite_object->tscale = float2fx(0.8f);
    card_object_update(main_menu_ace);

    // Select last highlighted button, Play button by default.
    // e.g. if we return from the options menu, we want the Options button to be highlighted.
    Selection sel_init = {last_highlighted_button, 0};
    main_menu_selection_grid.selection = sel_init;

    // Highlight current button
    button_set_highlight(&main_menu_buttons[main_menu_selection_grid.selection.x], true);
}

void game_main_menu_on_update(void)
{
    card_object_update(main_menu_ace);
    main_menu_ace->sprite_object->trotation = lu_sin((g_game_vars.timer << 8) / 2) / 3;
    main_menu_ace->sprite_object->rotation = main_menu_ace->sprite_object->trotation;

    // Seed randomization
    g_game_vars.rng_seed++;
    // If the keys have changed, make it more pseudo-random
    if (key_curr_state() != key_prev_state())
    {
        g_game_vars.rng_seed *= 2;
    }

    selection_grid_process_input(&main_menu_selection_grid);
}

void game_main_menu_on_exit(void)
{
    // Save selected button
    last_highlighted_button = main_menu_selection_grid.selection.x;

    // Normally I would just cache these and hide/unhide but I didn't feel like dealing with
    // defining a layer for it
    card_destroy(&main_menu_ace->card);
    card_object_destroy(&main_menu_ace);
}

// Implement SelectionGrid handler functions

static int main_menu_return_row_size(void)
{
    return MAIN_MENU_NB_BTN;
}

static bool main_menu_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    if (prev_selection->x >= 0 && prev_selection->x < MAIN_MENU_NB_BTN)
    {
        button_set_highlight(&main_menu_buttons[prev_selection->x], false);
    }

    play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
    button_set_highlight(&main_menu_buttons[new_selection->x], true);

    return true;
}

static void main_menu_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    // DEBUG: toggle git hash display by pressing L+R+START+SELECT
    static bool combo_pressed = false;
    static bool show_version = false;
    if (key_is_down(KEY_START) && key_is_down(KEY_SELECT) && key_is_down(KEY_L) &&
        key_is_down(KEY_R))
    {
        if (!combo_pressed)
        {
            show_version = !show_version;
            if (show_version)
            {
                tte_printf(
                    "#{P:%d,%d; cx:0x%X000}%s",
                    GBALATRO_VERSION_TEXT_POS.x,
                    GBALATRO_VERSION_TEXT_POS.y,
                    TTE_WHITE_PB,
                    gbalatro_version
                );
            }
            else
            {
                tte_erase_rect_wrapper(GBALATRO_VERSION_TEXT_RECT);
            }
        }

        combo_pressed = true;
    }
    else
    {
        combo_pressed = false;
    }

    if (key_hit(SELECT_CARD))
    {
        button_press(&main_menu_buttons[selection->x]);
    }
}

static void play_on_pressed(void)
{
    game_change_state(GAME_STATE_GAME_START);
}

static void options_on_pressed(void)
{
    game_change_state(GAME_STATE_OPTIONS_MENU);
}
