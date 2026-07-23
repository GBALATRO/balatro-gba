/**
 * @file run_setup.c
 *
 * @brief Setup Run menu state functions implementation.
 */

#include "game/run_setup.h"

#include "background_run_setup_gfx.h"
#include "button.h"
#include "card.h"
#include "game.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "save.h"
#include "selection_grid.h"
#include "sprite.h"
#include "state_machine.h"
#include "util.h"

#include <string.h>
#include <tonc.h>

// Palette Indices

#define CHANGE_DECK_BTN_MAIN_COLOR_PAL_IDX   1
#define RED_BTN_MAIN_COLOR_PAL_IDX           10
#define RED_DISABLED_BTN_MAIN_COLOR_PAL_IDX  11
#define BLUE_BTN_MAIN_COLOR_PAL_IDX          12
#define BLUE_DISABLED_BTN_MAIN_COLOR_PAL_IDX 13
#define BACK_BTN_MAIN_COLOR_PAL_IDX          14

#define NEW_RUN_BTN_OUTLINE_COLOR_PAL_IDX     30
#define RESUME_BTN_OUTLINE_COLOR_PAL_IDX      31
#define CHANGE_DECK_BTN_OUTLINE_COLOR_PAL_IDX 32
#define SEED_CHECK_BTN_OUTLINE_COLOR_PAL_IDX  33
#define SEED_DECK_BTN_OUTLINE_COLOR_PAL_IDX   34
#define PLAY_BTN_OUTLINE_COLOR_PAL_IDX        35
#define BACK_BTN_OUTLINE_COLOR_PAL_IDX        36

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
static void choose_deck_substate_update(void);

static void seed_keyboard_substate_init(void);
static void seed_keyboard_substate_update(void);

static void resume_substate_init(void);

static StateInfo state_info[] = {
    STATE_INFO_INIT_UPDATE_FN(choose_deck_substate_init, choose_deck_substate_update),
    STATE_INFO_INIT_UPDATE_FN(seed_keyboard_substate_init, seed_keyboard_substate_update),
    STATE_INFO_INIT_UPDATE_FN(resume_substate_init, noop)
};

static StateMachine run_setup_sm = {
    .state_infos = &state_info[0],
    .num_infos = RUN_SETUP_SUBSTATE_MAX,
};

#pragma region LAYOUT
/*******************************************************************************
 * TILES LAYOUT
 ******************************************************************************/

// clang-format off
// Uncomment when tab row is re-added
//static const Rect     RUN_SETUP_RESUME_TAB_DISABLED_SRC                   = {19, 22, 23, 23};
//static const BG_POINT RUN_SETUP_RESUME_TAB_DISABLED_DEST_POS              = {15, 1};

static const BG_POINT RUN_SETUP_FRAME_BG_SE_FILL_SRC_POS                  = {9 , 31};

static const Rect          RUN_SETUP_CHOOSE_DECK_CHOICE_BG_9_PTCH_DEST    = {6 , 4 , 23, 13};
static const NinePatchRect RUN_SETUP_CHOOSE_DECK_CHOICE_BG_9_PTCH_SRC     = {
    { 0 , 21, 7 , 28},
    { 5 , 3 , 2 , 4 }
};

static const Rect     RUN_SETUP_CHOOSE_DECK_CLEAN_LEFT_DEST               = {4 , 6 , 5 , 13};
static const Rect     RUN_SETUP_CHOOSE_DECK_CLEAN_RIGHT_DEST              = {24, 6 ,25 , 13};

static const Rect     RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_OFF_SRC          = {30, 8 , 31, 10};
static const Rect     RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_ON_SRC           = {30, 11, 31, 13};
static const BG_POINT RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_DEST_POS         = {17, 14};

static const BG_POINT RUN_SETUP_CHOOSE_DECK_SEED_BTN_3X3_SRC_POS          = {0 , 29};
static const BG_POINT RUN_SETUP_CHOOSE_DECK_SEED_BTN_DISABLED_3X3_SRC_POS = {3 , 29};
static const Rect     RUN_SETUP_CHOOSE_DECK_SEED_BTN_DEST                 = {19, 14, 24, 16};

static const Rect     RUN_SETUP_CHOOSE_SEED_FRAME_CLEAN_DEST              = {4 , 3 , 25, 5 };
static const Rect     RUN_SETUP_CHOOSE_SEED_KEYBOARD_SRC                  = {10, 24, 31, 31};
static const Rect     RUN_SETUP_CHOOSE_SEED_FIELD_SRC                     = {24, 22, 31, 23};
static const BG_POINT RUN_SETUP_CHOOSE_SEED_KEYBOARD_DEST_POS             = {4 , 6};
static const BG_POINT RUN_SETUP_CHOOSE_SEED_FIELD_DEST_POS                = {11, 4};

static const BG_POINT RUN_SETUP_CHOOSE_SEED_DECK_BTN_3X3_SRC_POS = RUN_SETUP_CHOOSE_DECK_SEED_BTN_3X3_SRC_POS;
static const Rect     RUN_SETUP_CHOOSE_SEED_DECK_BTN_DEST                 = {17, 14, 24, 16};

// Pixel sizes
#define RUN_SETUP_DECK_SPRITE_T_X 48
#define RUN_SETUP_DECK_SPRITE_T_Y 54
static const BG_POINT RUN_SETUP_DECK_NAME_TEXT_POS  = {80 , 40 };
static const BG_POINT RUN_SETUP_DECK_DESC_TEXT_POS  = {80 , 56 };
static const Rect     RUN_SETUP_DECK_NAME_DESC_RECT = {80 , 40 ,176, 96 };
static const BG_POINT RUN_SETUP_SEED_FIELD_TEXT_POS = {96 , 40 };
static const BG_POINT RUN_SETUP_SEED_DECK_TEXT_POS  = {152, 120};
static const BG_POINT RUN_SETUP_PLAY_TEXT_POS       = {76 , 120};
static const BG_POINT RUN_SETUP_BACK_TEXT_POS       = {104, 136};
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
 * to state so these Buttons can't be defined here. functions related to their
 * behaviour can, though.
 *
 * The Play and Back buttons will always be the same, but Play needs to be
 * duplicated anyway to be integrated to each SubState's SelectionGrid, since
 * the row it is on doesn't always have the same number of buttons.
 ******************************************************************************/

static int back_row_get_size(void);
static void back_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);

static inline void toggle_seed_enabled(bool enable);
static void use_seed_on_pressed(void);
static void seed_on_pressed(void);
static void deck_on_pressed(void);
static void play_on_pressed(void);
static void back_on_pressed(void);

// clang-format off
static Button back_button = {
    BACK_BTN_OUTLINE_COLOR_PAL_IDX, BACK_BTN_MAIN_COLOR_PAL_IDX,
    back_on_pressed, NULL
};
// clang-format on

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
    RUN_SETUP_DECK_BB_PLAY,
    RUN_SETUP_DECK_BB_USE_SEED,
    RUN_SETUP_DECK_BB_SEED,
    RUN_SETUP_DECK_BB_MAX
};

// CHOOSE DECK SELECTION GRID

static int change_deck_get_row_size(void);
static bool choose_deck_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static int seed_play_get_row_size(void);
static void seed_play_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);

// clang-format off
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
// clang-format on

static const Selection RUN_SETUP_CHOOSE_DECK_INIT_SEL = {0, 0};
static const Selection RUN_SETUP_CHOOSE_DECK_SEL_FROM_SEED = {2, 1};

static SelectionGrid choose_deck_selection_grid = {
    choose_deck_rows,
    RUN_SETUP_DECK_ROW_MAX,
    RUN_SETUP_CHOOSE_DECK_INIT_SEL
};

// CHOOSE DECK BUTTONS

static void change_deck_on_pressed(void);

static Button change_deck_button = {
    CHANGE_DECK_BTN_OUTLINE_COLOR_PAL_IDX,
    CHANGE_DECK_BTN_MAIN_COLOR_PAL_IDX,
    change_deck_on_pressed,
    NULL
};

// clang-format off
// Seed button is disabled by default but can be enabled by clicking the toggle just next to it
static Button choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_MAX] = {
    {
        PLAY_BTN_OUTLINE_COLOR_PAL_IDX, BLUE_BTN_MAIN_COLOR_PAL_IDX,
        play_on_pressed, NULL
    }, {
        SEED_CHECK_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX,
        use_seed_on_pressed, NULL
    }, {
        SEED_DECK_BTN_OUTLINE_COLOR_PAL_IDX, BLUE_DISABLED_BTN_MAIN_COLOR_PAL_IDX,
        seed_on_pressed, NULL
    }
};
// clang-format on

static bool use_seed = false;
static CardObject* run_setup_deck = NULL;

#pragma endregion

#pragma region SEED KEYBOARD
/*******************************************************************************
 * SEED KEYBOARD SUBSTATE
 ******************************************************************************/

#define KEYBOARD_WIDTH  10
#define KEYBOARD_HEIGHT 4

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

enum RunSetupKeyboardButtons
{
    RUN_SETUP_KEYBOARD_BEGIN,
    // Row 0
    RUN_SETUP_KEYBOARD_1 = RUN_SETUP_KEYBOARD_BEGIN,
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
    RUN_SETUP_KEYBOARD_DEL,
    RUN_SETUP_KEYBOARD_MAX
};

enum RunSetupSeedBottomButtons
{
    RUN_SETUP_SEED_BB_PLAY,
    RUN_SETUP_SEED_BB_DECK,
    RUN_SETUP_SEED_BB_MAX
};

// CHOOSE SEED SELECTION GRID

static int choose_seed_get_keyboard_row_size(void);
static int choose_seed_get_keyboard_short_row_size(void);
static int choose_seed_get_bottom_row_size(void);
static void choose_seed_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool choose_seed_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

// clang-format off
static SelectionGridRow choose_seed_selection_rows[] = {
    // First 4 rows are the seed keyboard itself.
    {
        RUN_SETUP_SEED_ROW_KEY0,
        choose_seed_get_keyboard_row_size,
        choose_seed_row_on_selection_changed,
        choose_seed_row_on_key_transit,
        {.wrap = true}
    }, {
        RUN_SETUP_SEED_ROW_KEY1,
        choose_seed_get_keyboard_row_size,
        choose_seed_row_on_selection_changed,
        choose_seed_row_on_key_transit,
        {.wrap = true}
    }, {
        RUN_SETUP_SEED_ROW_KEY2,
        choose_seed_get_keyboard_row_size,
        choose_seed_row_on_selection_changed,
        choose_seed_row_on_key_transit,
        {.wrap = true}
    }, {
        RUN_SETUP_SEED_ROW_KEY3,
        choose_seed_get_keyboard_short_row_size,
        choose_seed_row_on_selection_changed,
        choose_seed_row_on_key_transit,
        {.wrap = true}
    },
    // Then we have the Deck/PLAY and Back rows
    {
        RUN_SETUP_SEED_ROW_DECK_PLAY,
        choose_seed_get_bottom_row_size,
        choose_seed_row_on_selection_changed,
        choose_seed_row_on_key_transit,
        {.wrap = false}
    }, {
        RUN_SETUP_SEED_ROW_BACK,
        back_row_get_size,
        choose_seed_row_on_selection_changed,
        back_row_on_key_transit,
        {.wrap = false}
    }
};
// clang-format on

static const Selection RUN_SETUP_CHOOSE_SEED_INIT_SEL = {1, 4};

static SelectionGrid choose_seed_selection_grid = {
    choose_seed_selection_rows,
    RUN_SETUP_SEED_ROW_MAX,
    RUN_SETUP_CHOOSE_SEED_INIT_SEL
};

// CHOOSE SEED BUTTONS

static void keyboard_button_on_pressed(void);

// clang-format off

#define DEF_SEED_KEYBOARD_BUTTON_OBJECT(key)                                    \
    {                                                                           \
        KEYBOARD_##key##_BTN_OUTLINE_COLOR_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX, \
        keyboard_button_on_pressed, NULL                                        \
    }                                                                           \

// Button at [3][0] is there for consistency but is not used since it corresponds to the "Random" key
static Button keyboard_buttons[KEYBOARD_HEIGHT * KEYBOARD_WIDTH] = {
    // Row 0
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(1),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(2),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(3),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(4),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(5),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(6),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(7),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(8),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(9),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(0),
    // Row 1
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(A),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(B),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(C),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(D),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(E),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(F),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(G),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(H),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(I),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(J),
    // Row 2
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(K),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(L),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(M),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(N),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(O),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(P),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(Q),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(R),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(S),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(T),
    // Row 3
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(RAND),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(U),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(V),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(W),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(X),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(Y),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(Z),
    DEF_SEED_KEYBOARD_BUTTON_OBJECT(DEL)
};

static Button choose_seed_bottom_buttons[2] = {
    {
        PLAY_BTN_OUTLINE_COLOR_PAL_IDX, BLUE_BTN_MAIN_COLOR_PAL_IDX,
        play_on_pressed, NULL
    }, {
        SEED_DECK_BTN_OUTLINE_COLOR_PAL_IDX, BLUE_BTN_MAIN_COLOR_PAL_IDX,
        deck_on_pressed, NULL
    }
};

static const char KEYBOARD_BUTTONS_TO_CHAR[KEYBOARD_HEIGHT * KEYBOARD_WIDTH] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    ' ', 'U', 'V', 'W', 'X', 'Y', 'Z'
};
// clang-format on

// Size BASE36_MAX_DIGITS + 1 to always have '\0' at the end
static char s_seed_str[BASE36_MAX_DIGITS + 1] = {'\0'};
static u8 s_seed_cursor_pos = 0;

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

// RESUME SELECTION GRID

// RESUME BUTTONS

#pragma endregion

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
    state_machine_register(&run_setup_sm);
    game_run_setup_change_background();

    // Apply the current use_seed value if seed is UNDEFINED
    if (g_game_vars.rng_info.seed == UNDEFINED)
    {
        // Make the string empty instead of showing all zeroes
        memset(s_seed_str, '\0', BASE36_MAX_DIGITS + 1);
        s_seed_cursor_pos = 0;
    }
    // Or use previous Run's seed if it hasn't been reset
    else
    {
        u32_to_base36(g_game_vars.rng_info.seed, s_seed_str);
        s_seed_cursor_pos = BASE36_MAX_DIGITS;
        use_seed = true;
    }
    toggle_seed_enabled(use_seed);

    // Rank doesn't matter, won't see it
    run_setup_deck = card_object_new(card_new(SPADES, ACE));

    // We put it at the same layer as the main menu Ace card, but it's okay because
    // both cards do not exist at the same time, one is destroyed before the other is created.
    card_object_set_sprite_face_down(run_setup_deck, g_game_vars.deck, DECK_SPRITE, 0);

    sprite_object_position(
        (SpriteObject*)run_setup_deck,
        RUN_SETUP_DECK_SPRITE_T_X,
        RUN_SETUP_DECK_SPRITE_T_Y
    );

    /* Uncomment these lines when we figure out how to properly restore a game save
    is_saved_game_valid = is_game_data_valid();
    if (is_saved_game_valid)
    {
        state_machine_change_state(&run_setup_sm, RUN_SETUP_SUBSTATE_RESUME);
    }
    */

    // Land on the deck swapping button when landing on this state from the Main Menu
    choose_deck_selection_grid.selection = RUN_SETUP_CHOOSE_DECK_INIT_SEL;
    state_machine_change_state(&run_setup_sm, RUN_SETUP_SUBSTATE_CHOOSE_DECK);
}

void game_run_setup_on_update(void)
{
    run_setup_tabs_update();
}

void game_run_setup_on_exit(void)
{
    state_machine_remove(&run_setup_sm);

    card_destroy(&run_setup_deck->card);
    card_object_destroy(&run_setup_deck);

    tte_erase_screen();
}

#pragma endregion

#pragma region IMPLEMENTATION
/*******************************************************************************
 * STATIC FUNCTIONS IMPLEMENTATION
 ******************************************************************************/

// CHOOSE DECK

/**
 * @brief Initializes the Deck changing substate.
 */
static void choose_deck_substate_init(void)
{
    // Show Deck sprite, name and TODO: description
    {
        Sprite* deck_sprite = sprite_object_get_sprite((SpriteObject*)run_setup_deck);
        if (deck_sprite != NULL)
        {
            obj_unhide(deck_sprite->obj, ATTR0_AFF);
        }
    }
    print_deck_name(g_game_vars.deck, RUN_SETUP_DECK_NAME_TEXT_POS);
    print_deck_description(g_game_vars.deck, RUN_SETUP_DECK_DESC_TEXT_POS);

    // Clean frame and expand 9-patch for the Deck choice background
    main_bg_se_copy_expand_tile(
        RUN_SETUP_CHOOSE_DECK_CLEAN_LEFT_DEST,
        RUN_SETUP_FRAME_BG_SE_FILL_SRC_POS
    );
    main_bg_se_copy_expand_tile(
        RUN_SETUP_CHOOSE_DECK_CLEAN_RIGHT_DEST,
        RUN_SETUP_FRAME_BG_SE_FILL_SRC_POS
    );
    main_bg_se_copy_expand_9_patch(
        RUN_SETUP_CHOOSE_DECK_CHOICE_BG_9_PTCH_DEST,
        &RUN_SETUP_CHOOSE_DECK_CHOICE_BG_9_PTCH_SRC
    );

    // TODO: add left/right navigation arrows once more decks have been implemented

    // Set Tab to "New Run"
    // Uncomment when tab row is re-added (clang-format made it ugly, sorry)
    // main_bg_se_copy_rect(RUN_SETUP_RESUME_TAB_DISABLED_SRC,
    // RUN_SETUP_RESUME_TAB_DISABLED_DEST_POS); tab_set_highlight(RUN_SETUP_TAB_NEW_RUN);

    // Set button highlights
    button_set_highlight(&change_deck_button, true);
    button_set_highlight(&choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_USE_SEED], false);
    button_set_highlight(&choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_PLAY], false);
    button_set_highlight(&back_button, false);
    toggle_seed_enabled(use_seed);

    // Print button text
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

/**
 * @brief Update the deck changing substate by handling user input though its
 *         corresponding SelectionGrid.
 */
static void choose_deck_substate_update(void)
{
    selection_grid_process_input(&choose_deck_selection_grid);
}

/**
 * @brief Returns the width of the Change Deck Button's row
 *
 * @return 1
 */
static int change_deck_get_row_size(void)
{
    return 1;
}

/**
 * @brief Gets the Button corresponding to the current Selection in `choose_deck_selection_grid`
 *
 * @param sel Selection within `choose_deck_selection_grid`
 * @return a pointer to the currently hovered Button
 */
static inline Button* change_deck_get_button_from_sel(const Selection* sel)
{
    switch (sel->y)
    {
        case RUN_SETUP_DECK_ROW_CHANGE_DECK:
            return &change_deck_button;

        case RUN_SETUP_DECK_ROW_SEED_PLAY:
            if (sel->x >= RUN_SETUP_DECK_BB_MAX)
                return NULL;
            return &choose_deck_bottom_buttons[sel->x];

        case RUN_SETUP_DECK_ROW_BACK:
            return &back_button;
    }
    return NULL;
}

/**
 * @brief Handle directional input in `choose_deck_selection_grid`
 *
 * @param selection_grid pointer to `choose_deck_selection_grid`
 * @param row_idx row index in `choose_deck_selection_grid` this function is being called for
 * @param prev_selection previous selection in selection_grid
 * @param new_selection new selection in selection_grid
 * @return bool
 */
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

/**
 * @brief Returns the width of the Seed checkbox/Seed/Play buttons' row
 *
 * @return 3
 */
static int seed_play_get_row_size(void)
{
    return 3;
}

/**
 * @brief Handle button inputs for `choose_deck_selection_grid`
 *
 * @param selection_grid pointer to `choose_deck_selection_grid`
 * @param selection current Selection in selection_grid
 */
static void seed_play_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
        button_press(&choose_deck_bottom_buttons[selection->x]);
}

// CHOOSE SEED

/**
 * @brief Prints the string representing the chosen seed in base-36
 */
static inline void update_seed_text(void)
{
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%-*s",
        RUN_SETUP_SEED_FIELD_TEXT_POS.x,
        RUN_SETUP_SEED_FIELD_TEXT_POS.y,
        TTE_BLACK_PB,
        BASE36_MAX_DIGITS + 1,
        s_seed_str
    );
}

/**
 * @brief Initialize the Choose Seed substate
 */
static void seed_keyboard_substate_init(void)
{
    choose_seed_selection_grid.selection = RUN_SETUP_CHOOSE_SEED_INIT_SEL;
    tte_erase_rect_wrapper(RUN_SETUP_DECK_NAME_DESC_RECT);

    // Hide Deck card sprite
    {
        Sprite* deck_sprite = sprite_object_get_sprite((SpriteObject*)run_setup_deck);
        if (deck_sprite != NULL)
        {
            obj_hide(deck_sprite->obj);
        }
    }

    // Clean deck swap screen with frame BG color
    main_bg_se_copy_expand_tile(
        RUN_SETUP_CHOOSE_SEED_FRAME_CLEAN_DEST,
        RUN_SETUP_FRAME_BG_SE_FILL_SRC_POS
    );

    // Copy keyboard tiles
    main_bg_se_copy_rect(
        RUN_SETUP_CHOOSE_SEED_KEYBOARD_SRC,
        RUN_SETUP_CHOOSE_SEED_KEYBOARD_DEST_POS
    );
    main_bg_se_copy_rect(RUN_SETUP_CHOOSE_SEED_FIELD_SRC, RUN_SETUP_CHOOSE_SEED_FIELD_DEST_POS);

    // Replace seed toggle and button by "Deck" button that allows going back to deck substate
    main_bg_se_copy_expand_3x3_rect(
        RUN_SETUP_CHOOSE_SEED_DECK_BTN_DEST,
        RUN_SETUP_CHOOSE_SEED_DECK_BTN_3X3_SRC_POS
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        RUN_SETUP_SEED_DECK_TEXT_POS.x,
        RUN_SETUP_SEED_DECK_TEXT_POS.y,
        TTE_WHITE_PB,
        "Deck " // extra space after to clean potential "Seed" text
    );

    // Set buttons highlight
    for (enum RunSetupKeyboardButtons key = RUN_SETUP_KEYBOARD_BEGIN; key < RUN_SETUP_KEYBOARD_MAX;
         key++)
    {
        button_set_highlight(&keyboard_buttons[key], false);
    }
    button_set_highlight(&choose_seed_bottom_buttons[RUN_SETUP_SEED_BB_PLAY], false);
    button_set_highlight(&choose_seed_bottom_buttons[RUN_SETUP_SEED_BB_DECK], true);
    button_set_highlight(&back_button, false);

    // Print seed text
    update_seed_text();
}

/**
 * @brief Update the Choose Seed substate by handling user inputs though the
 *         corresponding `choose_seed_selection_grid` SelectionGrid
 */
static void seed_keyboard_substate_update(void)
{
    selection_grid_process_input(&choose_seed_selection_grid);
}

/**
 * @brief Returns the width of the 3 top keyboard rows
 *
 * @return `KEYBOARD_WIDTH = 10`
 */
static int choose_seed_get_keyboard_row_size(void)
{
    return KEYBOARD_WIDTH;
}

/**
 * @brief Returns the width of the bottom keyboard row, the shortest one
 *
 * @return `KEYBOARD_WIDTH - 2 = 8`
 */
static int choose_seed_get_keyboard_short_row_size(void)
{
    return KEYBOARD_WIDTH - 2;
}

/**
 * @brief Returns the width of the Deck/Play buttons' row
 *
 * @return 2
 */
static int choose_seed_get_bottom_row_size(void)
{
    return 2;
}

/**
 * @brief Deletes the last char in the seed string base-36 representation
 */
static inline void reroll_seed_str(void)
{
    // Need to roll a new seed and get a "real" random number. Otherwise the seed is still stuck at
    // 0 and we'll have the same sequence of generated seeds each time.
    // Also, don't use the shuffled seed as is, or we'll just end up with sequential seeds when
    // rolling multiple times.
    rng_shuffle_seed();
    u32 new_seed = rng_get_u32();
    rng_set_seed(new_seed);
    u32_to_base36(new_seed, s_seed_str);
    update_seed_text();
    s_seed_cursor_pos = BASE36_MAX_DIGITS;
}

/**
 * @brief Deletes the last char in the seed string base-36 representation
 */
static inline void delete_seed_char(void)
{
    if (s_seed_cursor_pos == 0)
        return;

    s_seed_str[--s_seed_cursor_pos] = '\0';
    update_seed_text();
}

/**
 * @brief Add a new char at the end of the seed string
 *
 * @param key index of the keyboard key corresponding to the char we want to add
 */
static inline void type_seed_char(enum RunSetupKeyboardButtons key)
{
    if (s_seed_cursor_pos >= BASE36_MAX_DIGITS)
        return;

    s_seed_str[s_seed_cursor_pos++] = KEYBOARD_BUTTONS_TO_CHAR[key];
    update_seed_text();
}

/**
 * @brief Get the keyboard key index from a SelectionGrid's Selection. The returned
 *         value is valid only if the Selection is within the keyboard Rows.
 *
 * @param sel the Selection within the `choose_seed_selection_grid`
 * @return the key index
 */
static inline enum RunSetupKeyboardButtons get_keyboard_index_from_sel(const Selection* sel)
{
    return sel->x + KEYBOARD_WIDTH * sel->y;
}

/**
 * @brief Keyboard key Button callback
 */
static void keyboard_button_on_pressed(void)
{
    // Type something only if the button pressed in on the keyboard
    if (choose_seed_selection_grid.selection.y > RUN_SETUP_SEED_ROW_KEY3)
        return;

    // Type only if the seed string is not full.
    // The cursor position is unsigned so always positive, but we still need to
    // ensure it doersn't go out of bounds by more than 1 so that we can always
    // substract 1 from it when erasing a character from the seed string.
    if (s_seed_cursor_pos > BASE36_MAX_DIGITS)
        s_seed_cursor_pos = BASE36_MAX_DIGITS;

    if (key_hit(DESELECT_CARDS))
        delete_seed_char();

    else if (key_hit(SELECT_CARD))
    {
        // Get keyboard button index from selection
        enum RunSetupKeyboardButtons key =
            get_keyboard_index_from_sel(&choose_seed_selection_grid.selection);

        switch (key)
        {
            case RUN_SETUP_KEYBOARD_RAND:
                reroll_seed_str();
                break;

            // Erase last character
            case RUN_SETUP_KEYBOARD_DEL:
                delete_seed_char();
                break;

            default:
                type_seed_char(key);
                break;
        }
    }
}

/**
 * @brief Get the Button corresponding to a Selection within `choose_seed_selection_grid`
 *
 * @param sel Selection in `choose_seed_selection_grid`
 * @return pointer to the Button at coordinates `sel` in grid
 */
static inline Button* choose_seed_get_button_from_sel(const Selection* sel)
{
    switch (sel->y)
    {
        case RUN_SETUP_SEED_ROW_KEY0:
        case RUN_SETUP_SEED_ROW_KEY1:
        case RUN_SETUP_SEED_ROW_KEY2:
        case RUN_SETUP_SEED_ROW_KEY3:
            return &keyboard_buttons[get_keyboard_index_from_sel(sel)];

        case RUN_SETUP_SEED_ROW_DECK_PLAY:
            return &choose_seed_bottom_buttons[sel->x];

        case RUN_SETUP_SEED_ROW_BACK:
            return &back_button;
    }
    return NULL;
}

/**
 * @brief Handle button imputs in `choose_seed_selection_grid`
 *
 * Always register A press, but only B press when we are on a keyboard row
 *
 * @param selection_grid pointer to `choose_seed_selection_grid`
 * @param selection Selected button in `choose_seed_selection_grid`
 */
static void choose_seed_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD) ||
        (selection->y <= RUN_SETUP_SEED_ROW_KEY3 && key_hit(DESELECT_CARDS)))
    {
        button_press(choose_seed_get_button_from_sel(selection));
    }
}

/**
 * @brief Handle directional imputs in `choose_seed_selection_grid`
 *
 * @param selection_grid pointer to `choose_seed_selection_grid`
 * @param row_idx index of the row in selection_grid the function is being called for
 * @param prev_selection previous Selection in `choose_seed_selection_grid`
 * @param new_selection new selection in `choose_seed_selection_grid`
 * @return bool
 */
static bool choose_seed_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    // TODO: since some rows don't have the same number of buttons but their widths line up,
    // need to find a way to shift selection.x to left or right to keep navigation consistent
    bool proceed_selection = true;

    if (row_idx == prev_selection->y)
        button_set_highlight(choose_seed_get_button_from_sel(prev_selection), false);

    if (row_idx == new_selection->y)
    {
        Selection shifted_selection = *new_selection;

        // From row 3 to 4
        if (prev_selection->y == RUN_SETUP_SEED_ROW_KEY2 &&
            new_selection->y == RUN_SETUP_SEED_ROW_KEY3 && prev_selection->x >= 5 &&
            prev_selection->x <= 8)
        {
            shifted_selection.x++;
            selection_grid->selection = shifted_selection;
            proceed_selection = false;
        }

        // From row 4 to 3
        else if (prev_selection->y == RUN_SETUP_SEED_ROW_KEY3 &&
                 new_selection->y == RUN_SETUP_SEED_ROW_KEY2 && prev_selection->x >= 1 &&
                 prev_selection->x <= 3)
        {
            shifted_selection.x++;
            selection_grid->selection = shifted_selection;
            proceed_selection = false;
        }

        button_set_highlight(choose_seed_get_button_from_sel(&shifted_selection), true);
    }

    return proceed_selection;
}

/**
 * @brief Deck Button Callback
 */
static void deck_on_pressed(void)
{
    state_machine_change_state(&run_setup_sm, RUN_SETUP_SUBSTATE_CHOOSE_DECK);
    choose_deck_selection_grid.selection = RUN_SETUP_CHOOSE_DECK_SEL_FROM_SEED;
    button_set_highlight(&change_deck_button, false);
    button_set_highlight(&choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_SEED], true);
}

// RESUME GAME

/**
 * @brief Initialize the "Resume Previous Run" substate
 */
static void resume_substate_init(void)
{
    tab_set_highlight(RUN_SETUP_TAB_RESUME);

    // Show Deck card sprite
    {
        Sprite* deck_sprite = sprite_object_get_sprite((SpriteObject*)run_setup_deck);
        if (deck_sprite != NULL)
        {
            obj_unhide(deck_sprite->obj, ATTR0_AFF);
        }
    }
}

// COMMON BUTTONS

/**
 * @brief Toggle whether or not we will use the seed chosen by the user.
 *         Changes the looks of the Seed button to signal if it is clickable or not,
 *         and that of the checkbox next to it.
 *
 * @param enable whether the seed is enabled or not
 */
static inline void toggle_seed_enabled(bool enable)
{
    use_seed = enable;

    // Apply right main color to the Button
    choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_SEED].button_pal_idx =
        use_seed ? BLUE_BTN_MAIN_COLOR_PAL_IDX : BLUE_DISABLED_BTN_MAIN_COLOR_PAL_IDX;
    button_set_highlight(&choose_deck_bottom_buttons[RUN_SETUP_DECK_BB_SEED], false);

    // Replace Seed button tiles with the disabled one's
    BG_POINT button_tiles = use_seed ? RUN_SETUP_CHOOSE_DECK_SEED_BTN_3X3_SRC_POS
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
    Rect toggle_tiles = use_seed ? RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_ON_SRC
                                 : RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_OFF_SRC;
    main_bg_se_copy_rect(toggle_tiles, RUN_SETUP_CHOOSE_DECK_USE_SEED_BTN_DEST_POS);
}

/**
 * @brief Seed checkbox Button Callback. Toggles custom seed usage.
 */
static void use_seed_on_pressed(void)
{
    toggle_seed_enabled(!use_seed);
}

/**
 * @brief Seed Button Callback. Opens the seed input keyboard.
 */
static void seed_on_pressed(void)
{
    if (use_seed)
        state_machine_change_state(&run_setup_sm, RUN_SETUP_SUBSTATE_CHOOSE_SEED);
}

/**
 * @brief Play Button Callback. Applies the chosen seed if enabled and starts a new run.
 */
static void play_on_pressed(void)
{
    // Apply provided Seed if enabled
    if (use_seed)
        rng_set_seed(base36_to_u32(s_seed_str));
    else
        rng_shuffle_seed();

    use_seed = false;

    game_change_state(GAME_STATE_GAME_START);
}

/**
 * @brief Back Button Callback. Goes back to the Main Menu.
 */
static void back_on_pressed(void)
{
    game_change_state(GAME_STATE_MAIN_MENU);
}

/**
 * @brief Button Callback used for cycling through the Decks.
 */
static void change_deck_on_pressed(void)
{
}

/**
 * @brief Highlights the `tab_sel` Tab at the top of the screen,
 *         and removes highlight from all the others.
 *
 * @param tab_sel index of the selected tab
 */
static void tab_set_highlight(enum RunSetupTab tab_sel)
{
    for (enum RunSetupTab tab = 0; tab < RUN_SETUP_TAB_MAX; tab++)
    {
        button_set_highlight(&tabs_buttons[tab], tab == tab_sel);
    }
}

/**
 * @brief Handle user input in regards to the Tabs. Tabs are cycled through using the L/R buttons.
 */
static void run_setup_tabs_update(void)
{
    static enum RunSetupTab current_tab = RUN_SETUP_TAB_RESUME;

    // Either not pressed anything or there is no saved data, "Resume" tab is left grayed out and
    // only one tab is available. Return early to not spend time on tab-changing logic
    if (!key_hit(KEY_ANY) || !is_saved_game_valid)
        return;

    // Not all the way to the right and pressed R
    if (key_hit(TAB_RIGHT) && current_tab < RUN_SETUP_TAB_MAX - 1)
    {
        button_set_highlight(&tabs_buttons[current_tab], false);
        current_tab++;
    }
    // Not all the way to the left and pressed L
    else if (key_hit(TAB_LEFT) && current_tab > 0)
    {
        button_set_highlight(&tabs_buttons[current_tab], false);
        current_tab--;
    }

    button_set_highlight(&tabs_buttons[current_tab], true);

    switch (current_tab)
    {
        case RUN_SETUP_TAB_NEW_RUN:
            state_machine_change_state(&run_setup_sm, RUN_SETUP_SUBSTATE_CHOOSE_DECK);
            break;

        case RUN_SETUP_TAB_RESUME:
            state_machine_change_state(&run_setup_sm, RUN_SETUP_SUBSTATE_RESUME);
            break;

        default:
            break;
    }
}

/**
 * @brief Retuns the width of the Back button's row.
 *
 * @return 1, as there is only the Back button on that row.
 */
static int back_row_get_size()
{
    return 1;
}

static void back_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
        button_press(&back_button);
}

#pragma endregion
