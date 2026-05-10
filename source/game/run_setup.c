#include "game/run_setup.h"

#include "background_run_setup_gfx.h"
#include "button.h"
#include "game.h"
#include "graphic_utils.h"
#include "selection_grid.h"

#include <tonc.h>

// Palette Indicess

#define RED_BTNS_MAIN_COLOR_PAL_IDX               16
#define SEED_DECK_BTN_MAIN_COLOR_PAL_IDX          17
#define SEED_DECK_DISABLED_BTN_MAIN_COLOR_PAL_IDX 18
#define PLAY_BTN_MAIN_COLOR_PAL_IDX               19
#define BACK_BTN_MAIN_COLOR_PAL_IDX               20

#define NEW_RUN_BTN_OUTLINE_COLOR_PAL_IDX   32
#define RESUME_BTN_OUTLINE_COLOR_PAL_IDX    33
#define SEED_DECK_BTN_OUTLINE_COLOR_PAL_IDX 34
#define PLAY_BTN_OUTLINE_COLOR_PAL_IDX      35
#define BACK_BTN_OUTLINE_COLOR_PAL_IDX      36

#define KEYPAD_1_BTN_OUTLINE_COLOR_PAL_IDX 48
#define KEYPAD_2_BTN_OUTLINE_COLOR_PAL_IDX 49
#define KEYPAD_3_BTN_OUTLINE_COLOR_PAL_IDX 50
#define KEYPAD_F_BTN_OUTLINE_COLOR_PAL_IDX 51
#define KEYPAD_4_BTN_OUTLINE_COLOR_PAL_IDX 52
#define KEYPAD_5_BTN_OUTLINE_COLOR_PAL_IDX 53
#define KEYPAD_6_BTN_OUTLINE_COLOR_PAL_IDX 54
#define KEYPAD_E_BTN_OUTLINE_COLOR_PAL_IDX 55
#define KEYPAD_7_BTN_OUTLINE_COLOR_PAL_IDX 56
#define KEYPAD_8_BTN_OUTLINE_COLOR_PAL_IDX 57
#define KEYPAD_9_BTN_OUTLINE_COLOR_PAL_IDX 58
#define KEYPAD_D_BTN_OUTLINE_COLOR_PAL_IDX 59
#define KEYPAD_0_BTN_OUTLINE_COLOR_PAL_IDX 60
#define KEYPAD_A_BTN_OUTLINE_COLOR_PAL_IDX 61
#define KEYPAD_B_BTN_OUTLINE_COLOR_PAL_IDX 62
#define KEYPAD_C_BTN_OUTLINE_COLOR_PAL_IDX 63

enum RunSetupSubstates
{
    RUN_SETUP_SUBSTATE_CHOOSE_DECK,
    RUN_SETUP_SUBSTATE_CHOOSE_SEED,
    RUN_SETUP_SUBSTATE_RESUME
};

#pragma region TAB BAR
//------------------------------------------------------------------------------
// TAB BAR
//------------------------------------------------------------------------------

enum RunSetupTabs
{
    RUN_SETUP_TAB_NEW_RUN,
    RUN_SETUP_TAB_RESUME,
    RUN_SETUP_TAB_MAX
};
static enum RunSetupTabs current_tab = RUN_SETUP_TAB_NEW_RUN;

static void run_setup_tabs_update(void);
static void new_run_on_pressed(void);
static void resume_on_pressed(void);

// clang-format off
static Button tabs_buttons[RUN_SETUP_TAB_MAX] = {
    {NEW_RUN_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTNS_MAIN_COLOR_PAL_IDX, new_run_on_pressed, NULL},
    {RESUME_BTN_OUTLINE_COLOR_PAL_IDX,  RED_BTNS_MAIN_COLOR_PAL_IDX, resume_on_pressed,  NULL},
};
// clang-format on

#pragma endregion

#pragma region CHOOSE DECK
//------------------------------------------------------------------------------
// CHOOSE DECK SUBSTATE
//------------------------------------------------------------------------------

static void choose_deck_substate_update(void);

#pragma endregion

#pragma region SEED KEYPAD
//------------------------------------------------------------------------------
// SEED KEYPAD SUBSTATE
//------------------------------------------------------------------------------

#define KEYPAD_WIDTH  4
#define KEYPAD_HEIGHT 4

enum RunSetupKeypadButtons
{
    // Row 0
    RUN_SETUP_KEYPAD_1,
    RUN_SETUP_KEYPAD_2,
    RUN_SETUP_KEYPAD_3,
    RUN_SETUP_KEYPAD_F,
    // Row 1
    RUN_SETUP_KEYPAD_4,
    RUN_SETUP_KEYPAD_5,
    RUN_SETUP_KEYPAD_6,
    RUN_SETUP_KEYPAD_E,
    // Row 2
    RUN_SETUP_KEYPAD_7,
    RUN_SETUP_KEYPAD_8,
    RUN_SETUP_KEYPAD_9,
    RUN_SETUP_KEYPAD_D,
    // Row 3
    RUN_SETUP_KEYPAD_0,
    RUN_SETUP_KEYPAD_A,
    RUN_SETUP_KEYPAD_B,
    RUN_SETUP_KEYPAD_C
};

static const char keypad_buttons_to_char[KEYPAD_HEIGHT][KEYPAD_WIDTH] = {
    {'1', '2', '3', 'F'},
    {'4', '5', '6', 'E'},
    {'7', '8', '9', 'D'},
    {'0', 'A', 'B', 'C'}
};

static void seed_keypad_substate_update(void);

#pragma endregion

#pragma region RESUME
//------------------------------------------------------------------------------
// RESUME SUBSTATE
//------------------------------------------------------------------------------

static void resume_substate_update(void);

#pragma endregion

#pragma region BOTTOM BUTTONS
//------------------------------------------------------------------------------
// BOTTOM BUTTONS SELECTION GRID
//------------------------------------------------------------------------------

static Button botom_buttons_first_row[] = {
    {},
    {}
};
static Button back_button = {
    
};

#pragma endregion

//------------------------------------------------------------------------------
// FUNCTIONS IMPLEMENTATION
//------------------------------------------------------------------------------

void game_run_setup_change_background(void)
{
    tte_erase_screen();

    GRIT_CPY(pal_bg_mem, background_run_setup_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_run_setup_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_run_setup_gfxMap);
}

void game_run_setup_on_init(void)
{
    game_run_setup_change_background();
}

void game_run_setup_on_update(void){

}

void game_run_setup_on_exit(void)
{

}

/**
 * @brief Gives the width of the Tabs rows in selection grid.
 *
 * @returns 2 when a run can be resumed, else only 1.
 */
static int options_menu_return_row_size(void)
{
    // Need to expose a `is_game_save_valid` func when save update is merged
    // return is_game_save_valid() ? 2 : 1;
    return 1;
}
