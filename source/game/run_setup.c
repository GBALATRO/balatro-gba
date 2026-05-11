#include "game/run_setup.h"

#include "background_run_setup_gfx.h"
#include "button.h"
#include "game.h"
#include "graphic_utils.h"
#include "selection_grid.h"

#include <tonc.h>

// Palette Indicess

#define RED_BTNS_MAIN_COLOR_PAL_IDX           10
#define SEED_DECK_PLAY_BTN_MAIN_COLOR_PAL_IDX 11
#define BACK_BTN_MAIN_COLOR_PAL_IDX           12

#define NEW_RUN_BTN_OUTLINE_COLOR_PAL_IDX    30
#define RESUME_BTN_OUTLINE_COLOR_PAL_IDX     31
#define SEED_CHECK_BTN_OUTLINE_COLOR_PAL_IDX 32
#define SEED_DECK_BTN_OUTLINE_COLOR_PAL_IDX  33
#define PLAY_BTN_OUTLINE_COLOR_PAL_IDX       34
#define BACK_BTN_OUTLINE_COLOR_PAL_IDX       35

#define KEYPAD_1_BTN_OUTLINE_COLOR_PAL_IDX 40
#define KEYPAD_2_BTN_OUTLINE_COLOR_PAL_IDX 41
#define KEYPAD_3_BTN_OUTLINE_COLOR_PAL_IDX 42
#define KEYPAD_4_BTN_OUTLINE_COLOR_PAL_IDX 43
#define KEYPAD_5_BTN_OUTLINE_COLOR_PAL_IDX 44
#define KEYPAD_6_BTN_OUTLINE_COLOR_PAL_IDX 45
#define KEYPAD_7_BTN_OUTLINE_COLOR_PAL_IDX 46
#define KEYPAD_8_BTN_OUTLINE_COLOR_PAL_IDX 47
#define KEYPAD_9_BTN_OUTLINE_COLOR_PAL_IDX 48
#define KEYPAD_0_BTN_OUTLINE_COLOR_PAL_IDX 49

#define KEYPAD_A_BTN_OUTLINE_COLOR_PAL_IDX 50
#define KEYPAD_B_BTN_OUTLINE_COLOR_PAL_IDX 51
#define KEYPAD_C_BTN_OUTLINE_COLOR_PAL_IDX 52
#define KEYPAD_D_BTN_OUTLINE_COLOR_PAL_IDX 53
#define KEYPAD_E_BTN_OUTLINE_COLOR_PAL_IDX 54
#define KEYPAD_F_BTN_OUTLINE_COLOR_PAL_IDX 55
#define KEYPAD_G_BTN_OUTLINE_COLOR_PAL_IDX 56
#define KEYPAD_H_BTN_OUTLINE_COLOR_PAL_IDX 57
#define KEYPAD_I_BTN_OUTLINE_COLOR_PAL_IDX 58
#define KEYPAD_J_BTN_OUTLINE_COLOR_PAL_IDX 59

#define KEYPAD_K_BTN_OUTLINE_COLOR_PAL_IDX 60
#define KEYPAD_L_BTN_OUTLINE_COLOR_PAL_IDX 61
#define KEYPAD_M_BTN_OUTLINE_COLOR_PAL_IDX 62
#define KEYPAD_N_BTN_OUTLINE_COLOR_PAL_IDX 63
#define KEYPAD_O_BTN_OUTLINE_COLOR_PAL_IDX 64
#define KEYPAD_P_BTN_OUTLINE_COLOR_PAL_IDX 65
#define KEYPAD_Q_BTN_OUTLINE_COLOR_PAL_IDX 66
#define KEYPAD_R_BTN_OUTLINE_COLOR_PAL_IDX 67
#define KEYPAD_S_BTN_OUTLINE_COLOR_PAL_IDX 68
#define KEYPAD_T_BTN_OUTLINE_COLOR_PAL_IDX 69

#define KEYPAD_RAND_BTN_OUTLINE_COLOR_PAL_IDX 70
#define KEYPAD_U_BTN_OUTLINE_COLOR_PAL_IDX    71
#define KEYPAD_V_BTN_OUTLINE_COLOR_PAL_IDX    72
#define KEYPAD_W_BTN_OUTLINE_COLOR_PAL_IDX    73
#define KEYPAD_X_BTN_OUTLINE_COLOR_PAL_IDX    74
#define KEYPAD_Y_BTN_OUTLINE_COLOR_PAL_IDX    75
#define KEYPAD_Z_BTN_OUTLINE_COLOR_PAL_IDX    76
#define KEYPAD_DEL_BTN_OUTLINE_COLOR_PAL_IDX  77

enum RunSetupSubstate
{
    RUN_SETUP_SUBSTATE_CHOOSE_DECK,
    RUN_SETUP_SUBSTATE_CHOOSE_SEED,
    RUN_SETUP_SUBSTATE_RESUME,
    RUN_SETUP_SUBSTATE_MAX
};
static void choose_deck_substate_update(void);
static void seed_keypad_substate_update(void);
static void resume_substate_update(void);

static const SubStateActionFn run_setup_actions[RUN_SETUP_SUBSTATE_MAX] = {
    choose_deck_substate_update,
    seed_keypad_substate_update,
    resume_substate_update
};

static enum RunSetupSubstate substate = RUN_SETUP_SUBSTATE_CHOOSE_DECK;

#pragma region TAB BAR
/*******************************************************************************
 * TAB BAR BUTTONS
 *
 * Part of all substate but not state-dependent, they are always updated the
 * same way, thus `run_setup_tabs_update()` will always be called.
 ******************************************************************************/

enum RunSetupTab
{
    RUN_SETUP_TAB_NEW_RUN,
    RUN_SETUP_TAB_RESUME,
    RUN_SETUP_TAB_MAX
};
static bool is_saved_game_valid = false;
static enum RunSetupTab current_tab = RUN_SETUP_TAB_RESUME;

static void run_setup_tabs_update(void);

// clang-format off
static Button tabs_buttons[RUN_SETUP_TAB_MAX] = {
    {NEW_RUN_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTNS_MAIN_COLOR_PAL_IDX, NULL, NULL},
    {RESUME_BTN_OUTLINE_COLOR_PAL_IDX,  RED_BTNS_MAIN_COLOR_PAL_IDX, NULL, NULL},
};
// clang-format on

#pragma endregion

#pragma region BOTTOM BUTTONS
/*******************************************************************************
 * BOTTOM BUTTONS
 *
 * Declared before any SubState because they are common to all of them.
 * 
 * The behaviour and appearance of the Seed/Deck button will change from state
 * to state.
 *
 * The Play and Back buttons will always be the same, but need to be duplicated
 * anyway to be integrated to each SubState's SelectionGrid.
 ******************************************************************************/

static int back_row_get_size();
static void back_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool back_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

/*
static void use_seed_on_pressed(void);
static void seed_on_pressed(void);
static void deck_on_pressed(void);
static void play_on_pressed(void);
static void back_on_pressed(void);

static Button seed_button = {
    
};
static Button disabled_seed_button = {
    
};
static Button deck_button = {
    
};
static Button play_button = {
    
};
*/
static Button back_button = {
    
};

#pragma endregion

#pragma region CHOOSE DECK
/*******************************************************************************
 * CHOOSE DECK SUBSTATE
 ******************************************************************************/

enum RunSetupDeckRows
{
    RUN_SETUP_DECK_ROW_CHOOSE_DECK,
    RUN_SETUP_DECK_ROW_SEED_PLAY,
    RUN_SETUP_DECK_ROW_BACK,
    RUN_SETUP_DECK_ROW_MAX
};

/*
static int seed_play_get_row_size(void);
static void seed_play_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool seed_play_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

#pragma endregion
*/

#pragma region SEED KEYPAD
/*******************************************************************************
 * SEED KEYPAD SUBSTATE
 ******************************************************************************/

#define SEED_MAX_CHAR_WIDTH 6
#define KEYPAD_WIDTH        10
#define KEYPAD_HEIGHT       4

enum RunSetupSeedRows
{
    RUN_SETUP_SEED_ROW_KEY0,
    RUN_SETUP_SEED_ROW_KEY1,
    RUN_SETUP_SEED_ROW_KEY2,
    RUN_SETUP_SEED_ROW_KEY3,
    RUN_SETUP_SEED_ROW_DECK_PLAY,
    RUN_SETUP_SEED_ROW_BACK,
    RUN_SETUP_SEED_ROW_MAX
};

enum RunSetupKeypadButtons
{
    // Row 0
    RUN_SETUP_KEYPAD_1,
    RUN_SETUP_KEYPAD_2,
    RUN_SETUP_KEYPAD_3,
    RUN_SETUP_KEYPAD_4,
    RUN_SETUP_KEYPAD_5,
    RUN_SETUP_KEYPAD_6,
    RUN_SETUP_KEYPAD_7,
    RUN_SETUP_KEYPAD_8,
    RUN_SETUP_KEYPAD_9,
    RUN_SETUP_KEYPAD_0,
    // Row 1
    RUN_SETUP_KEYPAD_A,
    RUN_SETUP_KEYPAD_B,
    RUN_SETUP_KEYPAD_C,
    RUN_SETUP_KEYPAD_D,
    RUN_SETUP_KEYPAD_E,
    RUN_SETUP_KEYPAD_F,
    RUN_SETUP_KEYPAD_G,
    RUN_SETUP_KEYPAD_H,
    RUN_SETUP_KEYPAD_I,
    RUN_SETUP_KEYPAD_J,
    // Row 2
    RUN_SETUP_KEYPAD_K,
    RUN_SETUP_KEYPAD_L,
    RUN_SETUP_KEYPAD_M,
    RUN_SETUP_KEYPAD_N,
    RUN_SETUP_KEYPAD_O,
    RUN_SETUP_KEYPAD_P,
    RUN_SETUP_KEYPAD_Q,
    RUN_SETUP_KEYPAD_R,
    RUN_SETUP_KEYPAD_S,
    RUN_SETUP_KEYPAD_T,
    // Row 3
    RUN_SETUP_KEYPAD_RAND,
    RUN_SETUP_KEYPAD_U,
    RUN_SETUP_KEYPAD_V,
    RUN_SETUP_KEYPAD_W,
    RUN_SETUP_KEYPAD_X,
    RUN_SETUP_KEYPAD_Y,
    RUN_SETUP_KEYPAD_Z,
    RUN_SETUP_KEYPAD_DEL
    
};

// Char [3][0] is there for consistency but is not used since it corresponds to the "Random" key
static const char keypad_buttons_to_char[KEYPAD_HEIGHT * KEYPAD_WIDTH] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    ' ', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

static char seed_str[SEED_MAX_CHAR_WIDTH] = {'\0', '\0', '\0', '\0', '\0', '\0'};
static u8 seed_cursor_pos = 0;

static int keypad_get_row_size(void);
static void keypad_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool keypad_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static int deck_play_get_row_size(void);
static void deck_play_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool deck_play_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

// clang-format off
static SelectionGridRow keypad_selection_rows[] = {
    // First 4 rows are the seed keypad itself.
    {
        RUN_SETUP_SEED_ROW_KEY0,
        keypad_get_row_size,
        keypad_row_on_selection_changed,
        keypad_row_on_key_transit,
        {.wrap = false}
    },
    {
        RUN_SETUP_SEED_ROW_KEY1,
        keypad_get_row_size,
        keypad_row_on_selection_changed,
        keypad_row_on_key_transit,
        {.wrap = false}
    },
    {
        RUN_SETUP_SEED_ROW_KEY2,
        keypad_get_row_size,
        keypad_row_on_selection_changed,
        keypad_row_on_key_transit,
        {.wrap = false}
    },
    {
        RUN_SETUP_SEED_ROW_KEY3,
        keypad_get_row_size,
        keypad_row_on_selection_changed,
        keypad_row_on_key_transit,
        {.wrap = false}
    },
    // Then we have the Deck/PLAY and Back rows
    {
        RUN_SETUP_SEED_ROW_DECK_PLAY,
        deck_play_get_row_size,
        deck_play_row_on_selection_changed,
        deck_play_row_on_key_transit,
        {.wrap = false}
    },
    {
        RUN_SETUP_SEED_ROW_BACK,
        back_row_get_size,
        back_row_on_selection_changed,
        back_row_on_key_transit,
        {.wrap = false}
    }
};

static const Selection KEYPAD_INIT_SEL = {4, 0};

SelectionGrid keypad_selection_grid = {
    keypad_selection_rows,
    RUN_SETUP_SEED_ROW_MAX,
    KEYPAD_INIT_SEL
};
// clang-format on

#pragma endregion

#pragma region RESUME
/*******************************************************************************
 * RESUME SUBSTATE
 ******************************************************************************/

enum RunSetupResumeRows
{
    RUN_SETUP_RESUME_ROW_PLAY,
    RUN_SETUP_RESUME_ROW_BACK,
    RUN_SETUP_RESUME_ROW_MAX
};

/*
// The two bottom rows are the same size here so we can group them
static int play_back_get_row_size(void);
static void play_back_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool play_back_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

#pragma endregion
*/

/*******************************************************************************
 * STATE FUNCTIONS
 ******************************************************************************/

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

    run_setup_tabs_update();
    run_setup_actions[substate]();
}

void game_run_setup_on_exit(void)
{

}

/*******************************************************************************
 * STATIC FUNCTIONS IMPLEMENTATION
 ******************************************************************************/

static void choose_deck_substate_update(void)
{

}

static void seed_keypad_substate_update(void)
{

}

static void resume_substate_update(void)
{

}

/*
static void use_seed_on_pressed(void)
{

}

static void seed_on_pressed(void)
{

}

static void deck_on_pressed(void)
{

}

static void play_on_pressed(void)
{

}

static void back_on_pressed(void)
{
    game_change_state(GAME_STATE_MAIN_MENU);
}
*/


static void inline tab_set_highlight(enum RunSetupTab tab_sel)
{
    for (enum RunSetupTab tab = 0; tab < RUN_SETUP_TAB_MAX; tab++)
    {
        button_set_highlight(&tabs_buttons[tab], tab == tab_sel);
    }
}

static void run_setup_tabs_update(void)
{
    // If there is no saved data, "Resume" tab is grayed out and only one tab is available
    if (!is_saved_game_valid)
    {
        return;
    }

    // Not all the way to the right and pressed R
    if (key_hit(TAB_RIGHT) && current_tab < RUN_SETUP_TAB_MAX - 1)
    {
        current_tab++;
    }
    // Not all the way to the left and pressed L
    else if (key_hit(TAB_LEFT) && current_tab > 0)
    {
        current_tab--;
    }
    // Either not pressed anything or no space to move, return early to not change button highlight
    else
    {
        return;
    }

    // We only reach here after successfuly changing Tabs
    tab_set_highlight(current_tab);
}

static int back_row_get_size()
{
    return 1;
}

static void back_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
    {
        game_change_state(GAME_STATE_MAIN_MENU);
    }
}

static bool back_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    return true;
}

// Will not get called for last (Back button) row, `back_row_get_size()` will instead.
static int keypad_get_row_size(void)
{
    switch (keypad_selection_grid.selection.y)
    {
        case RUN_SETUP_SEED_ROW_DECK_PLAY:
            return 2;
        default:
            return KEYPAD_WIDTH;
    }
}

static inline void keypad_type_char(enum RunSetupKeypadButtons key)
{
    // Exclude these that do not print characters on the screen
    if (key == RUN_SETUP_KEYPAD_RAND && key == RUN_SETUP_KEYPAD_DEL)
        return;

    // The cursor position is unsigned so always positive, but we still need to
    // ensure it doersn't go out of bounds by more than 1 so that we can always
    // substract 1 from it when erasing a character from the seed string.
    if (seed_cursor_pos >= SEED_MAX_CHAR_WIDTH)
    {
        seed_cursor_pos = SEED_MAX_CHAR_WIDTH;
        return;
    }

    seed_str[seed_cursor_pos] = keypad_buttons_to_char[key];
    seed_cursor_pos++;
}

static void keypad_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    // Handle keypad itself
    if (selection->y <= RUN_SETUP_SEED_ROW_KEY3)
    {
        // press A to type a character
        if (key_hit(SELECT_CARD))
        {

        }
        // press B to erase the last one written
        else if (key_hit(DESELECT_CARDS))
        {

        }
    }
}

static bool keypad_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    return true;
}

static int deck_play_get_row_size(void)
{
    return 2;
}

static void deck_play_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{

}

static bool deck_play_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    bool is_back_highlighted =
        (substate == RUN_SETUP_SUBSTATE_CHOOSE_DECK &&
         new_selection->y == RUN_SETUP_DECK_ROW_BACK) ||
        (substate == RUN_SETUP_SUBSTATE_CHOOSE_SEED &&
         new_selection->y == RUN_SETUP_SEED_ROW_BACK) ||
        (substate == RUN_SETUP_SUBSTATE_RESUME && new_selection->y == RUN_SETUP_RESUME_ROW_BACK);

    button_set_highlight(&back_button, is_back_highlighted);
    return true;
}

