#include "game/run_setup.h"

#include "background_run_setup_gfx.h"
#include "button.h"
#include "game.h"
#include "graphic_utils.h"
#include "selection_grid.h"
#include "save.h"

#include <tonc.h>

// Palette Indicess

#define CHANGE_DECK_BTN_MAIN_COLOR_PAL_IDX   1
#define RED_BTN_MAIN_COLOR_PAL_IDX           10
#define RED_DISABLED_BTN_MAIN_COLOR_PAL_IDX  11
#define BLUE_BTN_MAIN_COLOR_PAL_IDX          12
#define BLUE_DISABLED_BTN_MAIN_COLOR_PAL_IDX 13
#define BACK_BTN_MAIN_COLOR_PAL_IDX          14

#define NEW_RUN_BTN_OUTLINE_COLOR_PAL_IDX    30
#define RESUME_BTN_OUTLINE_COLOR_PAL_IDX     31
#define CHANGE_DECK_BTN_OUTINE_COLOR_PAL_IDX 32
#define SEED_CHECK_BTN_OUTLINE_COLOR_PAL_IDX 33
#define SEED_DECK_BTN_OUTLINE_COLOR_PAL_IDX  34
#define PLAY_BTN_OUTLINE_COLOR_PAL_IDX       35
#define BACK_BTN_OUTLINE_COLOR_PAL_IDX       36

#define KEYBOARD_1_BTN_OUTLINE_COLOR_PAL_IDX 40
#define KEYBOARD_2_BTN_OUTLINE_COLOR_PAL_IDX 41
#define KEYBOARD_3_BTN_OUTLINE_COLOR_PAL_IDX 42
#define KEYBOARD_4_BTN_OUTLINE_COLOR_PAL_IDX 43
#define KEYBOARD_5_BTN_OUTLINE_COLOR_PAL_IDX 44
#define KEYBOARD_6_BTN_OUTLINE_COLOR_PAL_IDX 45
#define KEYBOARD_7_BTN_OUTLINE_COLOR_PAL_IDX 46
#define KEYBOARD_8_BTN_OUTLINE_COLOR_PAL_IDX 47
#define KEYBOARD_9_BTN_OUTLINE_COLOR_PAL_IDX 48
#define KEYBOARD_0_BTN_OUTLINE_COLOR_PAL_IDX 49

#define KEYBOARD_A_BTN_OUTLINE_COLOR_PAL_IDX 50
#define KEYBOARD_B_BTN_OUTLINE_COLOR_PAL_IDX 51
#define KEYBOARD_C_BTN_OUTLINE_COLOR_PAL_IDX 52
#define KEYBOARD_D_BTN_OUTLINE_COLOR_PAL_IDX 53
#define KEYBOARD_E_BTN_OUTLINE_COLOR_PAL_IDX 54
#define KEYBOARD_F_BTN_OUTLINE_COLOR_PAL_IDX 55
#define KEYBOARD_G_BTN_OUTLINE_COLOR_PAL_IDX 56
#define KEYBOARD_H_BTN_OUTLINE_COLOR_PAL_IDX 57
#define KEYBOARD_I_BTN_OUTLINE_COLOR_PAL_IDX 58
#define KEYBOARD_J_BTN_OUTLINE_COLOR_PAL_IDX 59

#define KEYBOARD_K_BTN_OUTLINE_COLOR_PAL_IDX 60
#define KEYBOARD_L_BTN_OUTLINE_COLOR_PAL_IDX 61
#define KEYBOARD_M_BTN_OUTLINE_COLOR_PAL_IDX 62
#define KEYBOARD_N_BTN_OUTLINE_COLOR_PAL_IDX 63
#define KEYBOARD_O_BTN_OUTLINE_COLOR_PAL_IDX 64
#define KEYBOARD_P_BTN_OUTLINE_COLOR_PAL_IDX 65
#define KEYBOARD_Q_BTN_OUTLINE_COLOR_PAL_IDX 66
#define KEYBOARD_R_BTN_OUTLINE_COLOR_PAL_IDX 67
#define KEYBOARD_S_BTN_OUTLINE_COLOR_PAL_IDX 68
#define KEYBOARD_T_BTN_OUTLINE_COLOR_PAL_IDX 69

#define KEYBOARD_RAND_BTN_OUTLINE_COLOR_PAL_IDX 70
#define KEYBOARD_U_BTN_OUTLINE_COLOR_PAL_IDX    71
#define KEYBOARD_V_BTN_OUTLINE_COLOR_PAL_IDX    72
#define KEYBOARD_W_BTN_OUTLINE_COLOR_PAL_IDX    73
#define KEYBOARD_X_BTN_OUTLINE_COLOR_PAL_IDX    74
#define KEYBOARD_Y_BTN_OUTLINE_COLOR_PAL_IDX    75
#define KEYBOARD_Z_BTN_OUTLINE_COLOR_PAL_IDX    76
#define KEYBOARD_DEL_BTN_OUTLINE_COLOR_PAL_IDX  77

enum RunSetupSubstate
{
    RUN_SETUP_SUBSTATE_CHOOSE_DECK,
    RUN_SETUP_SUBSTATE_CHOOSE_SEED,
    RUN_SETUP_SUBSTATE_RESUME,
    RUN_SETUP_SUBSTATE_MAX
};

static void choose_deck_substate_init(void);
static void seed_keyboard_substate_init(void);
static void resume_substate_init(void);

static void choose_deck_substate_update(void);
static void seed_keyboard_substate_update(void);
static void resume_substate_update(void);

static const SubStateActionFn run_setup_actions[RUN_SETUP_SUBSTATE_MAX] = {
    choose_deck_substate_update,
    seed_keyboard_substate_update,
    resume_substate_update
};

static enum RunSetupSubstate substate = RUN_SETUP_SUBSTATE_CHOOSE_DECK;

#pragma region LAYOUT
/*******************************************************************************
 * TILES LAYOUT
 ******************************************************************************/

// clang-format off
static const Rect     RUN_SETUP_RESUME_TAB_DISABLED_SRC                   = {15, 22, 19, 23};
static const BG_POINT RUN_SETUP_RESUME_TAB_DISABLED_DEST_POS              = {15, 1};

static const BG_POINT RUN_SETUP_FRAME_BG_3X3_SRC_POS                      = {7 , 29};

static const Rect     RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_OFF_SRC          = {4 , 23, 5 , 25};
static const Rect     RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_ON_SRC           = {6 , 23, 7 , 25};
// static const Rect     RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_DISABLED_SRC     = {8 , 23, 9 , 25};
static const BG_POINT RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_DEST_POS         = {5 , 14};

static const BG_POINT RUN_SETUP_CHOOSE_DECK_SEED_BTN_3X3_SRC_POS          = {4 , 26};
static const BG_POINT RUN_SETUP_CHOOSE_DECK_SEED_BTN_DISABLED_3X3_SRC_POS = {7 , 26};
static const Rect     RUN_SETUP_CHOOSE_DECK_SEED_BTN_DEST                 = {7 , 14, 12, 16};

static const Rect     RUN_SETUP_CHOOSE_SEED_FRAME_CLEAN_DEST              = {4 , 3 , 25, 5 };
static const Rect     RUN_SETUP_CHOOSE_SEED_KEYBOARD_SRC                  = {10, 24, 31, 31};
static const Rect     RUN_SETUP_CHOOSE_SEED_FIELD_SRC                     = {24, 22, 31, 23};
static const BG_POINT RUN_SETUP_CHOOSE_SEED_KEYBOARD_DEST_POS             = {4 , 6};
static const BG_POINT RUN_SETUP_CHOOSE_SEED_FIELD_DEST_POS                = {11, 4};

static const BG_POINT RUN_SETUP_CHOOSE_SEED_DECK_BTN_3X3_SRC_POS = RUN_SETUP_CHOOSE_DECK_SEED_BTN_3X3_SRC_POS;
static const Rect     RUN_SETUP_CHOOSE_SEED_DECK_BTN_DEST                 = {5 , 14, 12, 16};

// Text positions in pixels
static const BG_POINT RUN_SETUP_SEED_DECK_TEXT_POS                        = {56 , 120};
static const BG_POINT RUN_SETUP_PLAY_TEXT_POS                             = {136, 120};
static const BG_POINT RUN_SETUP_BACK_TEXT_POS                             = {104, 136};
// clang-format on

#pragma endregion

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

static void tab_set_highlight(enum RunSetupTab tab_sel);
static void run_setup_tabs_update(void);

// clang-format off
static Button tabs_buttons[RUN_SETUP_TAB_MAX] = {
    {NEW_RUN_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX, NULL, NULL},
    {RESUME_BTN_OUTLINE_COLOR_PAL_IDX,  RED_BTN_MAIN_COLOR_PAL_IDX, NULL, NULL},
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

static inline void toggle_seed_enabled(bool enable);
static void use_seed_on_pressed(void);
static void seed_on_pressed(void);
// static void deck_on_pressed(void);
static void play_on_pressed(void);
static void back_on_pressed(void);

static Button back_button = {
    BACK_BTN_OUTLINE_COLOR_PAL_IDX, BACK_BTN_MAIN_COLOR_PAL_IDX,
    back_on_pressed, NULL
};

#pragma endregion

#pragma region CHOOSE DECK
/*******************************************************************************
 * CHOOSE DECK SUBSTATE
 ******************************************************************************/

enum RunSetupDeckRows
{
    RUN_SETUP_DECK_ROW_CHANGE_DECK,
    RUN_SETUP_DECK_ROW_SEED_PLAY,
    RUN_SETUP_DECK_ROW_BACK,
    RUN_SETUP_DECK_ROW_MAX
};

enum RunSetupDeckBottomButtons
{
    RUN_SETUP_DECK_BB_USE_SEED,
    RUN_SETUP_DECK_BB_SEED,
    RUN_SETUP_DECK_BB_PLAY,
    RUN_SETUP_DECK_BB_MAX
};

static void change_deck_on_pressed(void);
static int change_deck_get_row_size(void);
static bool choose_deck_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static int seed_play_get_row_size(void);
static void seed_play_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);

static SelectionGridRow choose_deck_rows[RUN_SETUP_DECK_ROW_MAX] = {
    {
        RUN_SETUP_DECK_ROW_CHANGE_DECK,
        change_deck_get_row_size,
        choose_deck_row_on_selection_changed,
        NULL,
        {.wrap = true}
    }, {
        RUN_SETUP_DECK_ROW_SEED_PLAY,
        seed_play_get_row_size,
        choose_deck_row_on_selection_changed,
        seed_play_row_on_key_transit,
        {.wrap = false}
    }, {
        RUN_SETUP_DECK_ROW_BACK,
        back_row_get_size,
        choose_deck_row_on_selection_changed,
        back_row_on_key_transit,
        {.wrap = false}
    }
};

static const Selection RUN_SETUP_CHOOSE_DECH_INIT_SEL = {0, 0};

static SelectionGrid choose_deck_selection_grid = {
    choose_deck_rows,
    RUN_SETUP_DECK_ROW_MAX,
    RUN_SETUP_CHOOSE_DECH_INIT_SEL
};

static Button change_deck_button = {
    CHANGE_DECK_BTN_OUTINE_COLOR_PAL_IDX, CHANGE_DECK_BTN_MAIN_COLOR_PAL_IDX,
    change_deck_on_pressed, NULL
};

// Seed button is disabled by default but can be enabled by clicking the toggle just next to it
static Button choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_MAX] = {
    {
        SEED_CHECK_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        use_seed_on_pressed, NULL
    }, {
        SEED_DECK_BTN_OUTLINE_COLOR_PAL_IDX, BLUE_DISABLED_BTN_MAIN_COLOR_PAL_IDX,
        seed_on_pressed, NULL
    }, {
        PLAY_BTN_OUTLINE_COLOR_PAL_IDX, BLUE_BTN_MAIN_COLOR_PAL_IDX,
        play_on_pressed, NULL
    }
};

static bool use_seed = false;

#pragma endregion

#pragma region SEED KEYBOARD
/*******************************************************************************
 * SEED KEYBOARD SUBSTATE
 ******************************************************************************/

#define SEED_MAX_CHAR_WIDTH 6
#define KEYBOARD_WIDTH      10
#define KEYBOARD_HEIGHT     4

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
    RUN_SETUP_KEYBOARD_1,
    RUN_SETUP_KEYBOARD_2,
    RUN_SETUP_KEYBOARD_3,
    RUN_SETUP_KEYBOARD_4,
    RUN_SETUP_KEYBOARD_5,
    RUN_SETUP_KEYBOARD_6,
    RUN_SETUP_KEYBOARD_7,
    RUN_SETUP_KEYBOARD_8,
    RUN_SETUP_KEYBOARD_9,
    RUN_SETUP_KEYBOARD_0,
    // Row 1
    RUN_SETUP_KEYBOARD_A,
    RUN_SETUP_KEYBOARD_B,
    RUN_SETUP_KEYBOARD_C,
    RUN_SETUP_KEYBOARD_D,
    RUN_SETUP_KEYBOARD_E,
    RUN_SETUP_KEYBOARD_F,
    RUN_SETUP_KEYBOARD_G,
    RUN_SETUP_KEYBOARD_H,
    RUN_SETUP_KEYBOARD_I,
    RUN_SETUP_KEYBOARD_J,
    // Row 2
    RUN_SETUP_KEYBOARD_K,
    RUN_SETUP_KEYBOARD_L,
    RUN_SETUP_KEYBOARD_M,
    RUN_SETUP_KEYBOARD_N,
    RUN_SETUP_KEYBOARD_O,
    RUN_SETUP_KEYBOARD_P,
    RUN_SETUP_KEYBOARD_Q,
    RUN_SETUP_KEYBOARD_R,
    RUN_SETUP_KEYBOARD_S,
    RUN_SETUP_KEYBOARD_T,
    // Row 3
    RUN_SETUP_KEYBOARD_RAND,
    RUN_SETUP_KEYBOARD_U,
    RUN_SETUP_KEYBOARD_V,
    RUN_SETUP_KEYBOARD_W,
    RUN_SETUP_KEYBOARD_X,
    RUN_SETUP_KEYBOARD_Y,
    RUN_SETUP_KEYBOARD_Z,
    RUN_SETUP_KEYBOARD_DEL
    
};

static void keyboard_button_on_pressed(void);
static int keyboard_get_row_size(void);
static void keyboard_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool keyboard_row_on_selection_changed(
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

// Button at [3][0] is there for consistency but is not used since it corresponds to the "Random" key
static Button keyboard_buttons[KEYBOARD_HEIGHT * KEYBOARD_WIDTH] = {
    // Row 0
    {
        KEYBOARD_1_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_2_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_3_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_4_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_5_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_6_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_7_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_8_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_9_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_0_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    },
    // Row 1
    {
        KEYBOARD_A_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_B_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_C_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_D_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_E_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_F_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_G_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_H_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_I_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_J_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    },
    // Row 2
    {
        KEYBOARD_K_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_L_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_M_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_N_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_O_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_P_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_Q_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_R_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_S_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_T_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    },
    // Row 3
    {
        KEYBOARD_RAND_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_U_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_V_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_W_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_X_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_Y_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_Z_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }, {
        KEYBOARD_DEL_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        keyboard_button_on_pressed, NULL
    }
};
static const char keyboard_buttons_to_char[KEYBOARD_HEIGHT * KEYBOARD_WIDTH] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    ' ', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

static char seed_str[SEED_MAX_CHAR_WIDTH] = {'\0', '\0', '\0', '\0', '\0', '\0'};
static u8 seed_cursor_pos = 0;

// clang-format off
static SelectionGridRow keyboard_selection_rows[] = {
    // First 4 rows are the seed keyboard itself.
    {
        RUN_SETUP_SEED_ROW_KEY0,
        keyboard_get_row_size,
        keyboard_row_on_selection_changed,
        keyboard_row_on_key_transit,
        {.wrap = false}
    }, {
        RUN_SETUP_SEED_ROW_KEY1,
        keyboard_get_row_size,
        keyboard_row_on_selection_changed,
        keyboard_row_on_key_transit,
        {.wrap = false}
    }, {
        RUN_SETUP_SEED_ROW_KEY2,
        keyboard_get_row_size,
        keyboard_row_on_selection_changed,
        keyboard_row_on_key_transit,
        {.wrap = false}
    }, {
        RUN_SETUP_SEED_ROW_KEY3,
        keyboard_get_row_size,
        keyboard_row_on_selection_changed,
        keyboard_row_on_key_transit,
        {.wrap = false}
    },
    // Then we have the Deck/PLAY and Back rows
    {
        RUN_SETUP_SEED_ROW_DECK_PLAY,
        deck_play_get_row_size,
        deck_play_row_on_selection_changed,
        deck_play_row_on_key_transit,
        {.wrap = false}
    }, {
        RUN_SETUP_SEED_ROW_BACK,
        back_row_get_size,
        back_row_on_selection_changed,
        back_row_on_key_transit,
        {.wrap = false}
    }
};

static const Selection KEYBOARD_INIT_SEL = {1, 4};

static SelectionGrid keyboard_selection_grid = {
    keyboard_selection_rows,
    RUN_SETUP_SEED_ROW_MAX,
    KEYBOARD_INIT_SEL
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

#pragma region STATE FUNCTIONS
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
    is_saved_game_valid = is_game_data_valid();
    if (is_saved_game_valid)
    {
        resume_substate_init();
    }
    else
    {
        choose_deck_substate_init();
    }
}

void game_run_setup_on_update(void){

    run_setup_tabs_update();
    run_setup_actions[substate]();
}

void game_run_setup_on_exit(void)
{
    tte_erase_screen();
}

#pragma endregion


#pragma region IMPLEMENTATION
/*******************************************************************************
 * STATIC FUNCTIONS IMPLEMENTATION
 ******************************************************************************/

// CHOOSE DECK

static void choose_deck_substate_init(void)
{
    substate = RUN_SETUP_SUBSTATE_CHOOSE_DECK;
    game_run_setup_change_background();

    // Set Tab to "New Run"
    main_bg_se_copy_rect(RUN_SETUP_RESUME_TAB_DISABLED_SRC, RUN_SETUP_RESUME_TAB_DISABLED_DEST_POS);
    tab_set_highlight(RUN_SETUP_TAB_NEW_RUN);

    // Land on the deck swapping button when changing for this state
    choose_deck_selection_grid.selection = RUN_SETUP_CHOOSE_DECH_INIT_SEL;

    // Set button highlights
    button_set_highlight(&change_deck_button, true);
    button_set_highlight(&choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_USE_SEED], false);
    toggle_seed_enabled(use_seed); // This just re-applies the current value
    button_set_highlight(&choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_PLAY], false);
    button_set_highlight(&back_button, false);

    // Print buttonn text
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        RUN_SETUP_PLAY_TEXT_POS.x,
        RUN_SETUP_PLAY_TEXT_POS.y,
        TTE_WHITE_PB,
        "PLAY"
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        RUN_SETUP_BACK_TEXT_POS.x,
        RUN_SETUP_BACK_TEXT_POS.y,
        TTE_WHITE_PB,
        "Back"
    );
}

static void choose_deck_substate_update(void)
{
    selection_grid_process_input(&choose_deck_selection_grid);
}

static int change_deck_get_row_size(void)
{
    return 1;
}

static inline Button* change_deck_get_button_from_sel(const Selection* sel)
{
    switch (sel->y)
    {
        case RUN_SETUP_DECK_ROW_CHANGE_DECK:
            return &change_deck_button;
        case RUN_SETUP_DECK_ROW_SEED_PLAY:
            if (sel->x > 3)
            {
                return NULL;
            }
            return &choose_deck_bottom_buttons[sel->x];
        case RUN_SETUP_DECK_ROW_BACK:
            return &back_button;
    }
    return NULL;
}

static bool choose_deck_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    button_set_highlight(change_deck_get_button_from_sel(prev_selection), false);
    button_set_highlight(change_deck_get_button_from_sel(new_selection), true);

    // TODO: detect left/right press on Change Deck row to allow swapping
    // Decks need to be implemented for this


    return true;
}

static int seed_play_get_row_size(void)
{
    return 3;
}

static void seed_play_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    button_press(&choose_deck_bottom_buttons[selection->x]);
}

// CHOOSE SEED

static void seed_keyboard_substate_init(void)
{
    substate = RUN_SETUP_SUBSTATE_CHOOSE_SEED;

    // Clean deck swap screen with frame BG color
    main_bg_se_copy_expand_3x3_rect(RUN_SETUP_CHOOSE_SEED_FRAME_CLEAN_DEST, RUN_SETUP_FRAME_BG_3X3_SRC_POS);
    
    // Copy keyboard tiles
    main_bg_se_copy_rect(RUN_SETUP_CHOOSE_SEED_KEYBOARD_SRC, RUN_SETUP_CHOOSE_SEED_KEYBOARD_DEST_POS);
    main_bg_se_copy_rect(RUN_SETUP_CHOOSE_SEED_FIELD_SRC, RUN_SETUP_CHOOSE_SEED_FIELD_DEST_POS);

    // Replace seed toggle and button by "Deck" button that allows going back to deck substate
    main_bg_se_copy_expand_3x3_rect(RUN_SETUP_CHOOSE_SEED_DECK_BTN_DEST, RUN_SETUP_CHOOSE_SEED_DECK_BTN_3X3_SRC_POS);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        RUN_SETUP_SEED_DECK_TEXT_POS.x,
        RUN_SETUP_SEED_DECK_TEXT_POS.y,
        TTE_WHITE_PB,
        "Deck " // extra space after to clean potential "Seed" text
    );
}

static void seed_keyboard_substate_update(void)
{

}

// Will not get called for last (Back button) row, `back_row_get_size()` will instead.
static int keyboard_get_row_size(void)
{
    switch (keyboard_selection_grid.selection.y)
    {
        case RUN_SETUP_SEED_ROW_DECK_PLAY:
            return 2;
        default:
            return KEYBOARD_WIDTH;
    }
}

static inline void delete_seed_char(void)
{
    if (seed_cursor_pos == 0)
    {
        return;
    }
    seed_str[seed_cursor_pos] = '\0';
    seed_cursor_pos--;
}

static void keyboard_button_on_pressed(void)
{
    // Type something only if the button pressed in on the keyboard
    if (keyboard_selection_grid.selection.y > RUN_SETUP_SEED_ROW_KEY3)
        return;

    // Type only if the seed string is not full.
    // The cursor position is unsigned so always positive, but we still need to
    // ensure it doersn't go out of bounds by more than 1 so that we can always
    // substract 1 from it when erasing a character from the seed string.
    if (seed_cursor_pos > SEED_MAX_CHAR_WIDTH)
    {
        seed_cursor_pos = SEED_MAX_CHAR_WIDTH;
    }

    // Get keyboard button index from selection
    enum RunSetupKeypadButtons key =
        keyboard_selection_grid.selection.x + KEYBOARD_WIDTH * keyboard_selection_grid.selection.y;

    switch (key)
    {
        // TODO: Generate a new 6-digit key at random
        // Implement it in rng.c/h
        case RUN_SETUP_KEYBOARD_RAND:
            break;
        // Erase last character
        case RUN_SETUP_KEYBOARD_DEL:
            delete_seed_char();
            break;
        default:
            if (seed_cursor_pos < SEED_MAX_CHAR_WIDTH)
            {
                seed_str[seed_cursor_pos] = keyboard_buttons_to_char[key];
                seed_cursor_pos++;
            }
            break;
    }
}

static void keyboard_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    // Handle keyboard itself
    if (selection->y <= RUN_SETUP_SEED_ROW_KEY3)
    {
        // press A to type a character
        if (key_hit(SELECT_CARD))
        {
            keyboard_button_on_pressed();
        }
        // press B to erase the last one written
        else if (key_hit(DESELECT_CARDS))
        {
            delete_seed_char();
        }
    }
}

static bool keyboard_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    if (prev_selection->y == row_idx)
    {
        enum RunSetupKeypadButtons prev_key = prev_selection->x + KEYBOARD_WIDTH * prev_selection->y;
        button_set_highlight(&keyboard_buttons[prev_key], false);
    }
    if (new_selection->y == row_idx)
    {
        enum RunSetupKeypadButtons new_key = new_selection->x + KEYBOARD_WIDTH * new_selection->y;
        button_set_highlight(&keyboard_buttons[new_key], true);
    }

    return true;
}

// RESUME GAME

static void resume_substate_init(void)
{
    tab_set_highlight(RUN_SETUP_TAB_RESUME);
}

static void resume_substate_update(void)
{

}

// COMMON BUTTONS

static inline void toggle_seed_enabled(bool enable)
{
    use_seed = enable;

    // Apply right main color to the Button
    choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_SEED].button_pal_idx = use_seed
        ? BLUE_BTN_MAIN_COLOR_PAL_IDX
        : BLUE_DISABLED_BTN_MAIN_COLOR_PAL_IDX;
    button_set_highlight(&choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_SEED], false);

    // Replace Seed button tiles with the disabled one's
    BG_POINT button_tiles = use_seed
        ? RUN_SETUP_CHOOSE_DECK_SEED_BTN_3X3_SRC_POS
        : RUN_SETUP_CHOOSE_DECK_SEED_BTN_DISABLED_3X3_SRC_POS;
    main_bg_se_copy_expand_3x3_rect(RUN_SETUP_CHOOSE_DECK_SEED_BTN_DEST, button_tiles);

    // Print Seed button text with the right color
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        RUN_SETUP_SEED_DECK_TEXT_POS.x,
        RUN_SETUP_SEED_DECK_TEXT_POS.y,
        use_seed ? TTE_WHITE_PB : TTE_BLACK_PB,
        " Seed" // Extra space before to clean potential "Deck" text
    );

    // Replace toggle button tiles with either checkmark of empty circle
    Rect toggle_tiles = use_seed
        ? RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_ON_SRC
        : RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_OFF_SRC;
    main_bg_se_copy_rect(toggle_tiles, RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_DEST_POS);
}

static void use_seed_on_pressed(void)
{
    if (key_hit(SELECT_CARD))
    {
        toggle_seed_enabled(!use_seed);
    }
}

static void seed_on_pressed(void)
{
    if (key_hit(SELECT_CARD) && use_seed)
    {
        seed_keyboard_substate_init();
    }
}

//static void deck_on_pressed(void)
//{
//
//}

static void play_on_pressed(void)
{
    game_change_state(GAME_STATE_GAME_START);
}

static void back_on_pressed(void)
{
    game_change_state(GAME_STATE_MAIN_MENU);
}

static void change_deck_on_pressed(void)
{

}

static void tab_set_highlight(enum RunSetupTab tab_sel)
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
    bool is_back_highlighted =
        (substate == RUN_SETUP_SUBSTATE_CHOOSE_DECK &&
         new_selection->y == RUN_SETUP_DECK_ROW_BACK) ||
        (substate == RUN_SETUP_SUBSTATE_CHOOSE_SEED &&
         new_selection->y == RUN_SETUP_SEED_ROW_BACK) ||
        (substate == RUN_SETUP_SUBSTATE_RESUME && new_selection->y == RUN_SETUP_RESUME_ROW_BACK);

    button_set_highlight(&back_button, is_back_highlighted);
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
    return true;
}

#pragma endregion
