#include "pause_menu.h"

#include "background_pause_menu_gfx.h"
#include "button.h"
#include "game.h"
#include "selection_grid.h"
#include "state_machine.h"

// Palette indices
#define TAB_MAIN_COLOR_PAL_IDX          1
#define HANDS_TAB_OUTLINE_COLOR_PAL_IDX 2
#define DECK_TAB_OUTLINE_COLOR_PAL_IDX  3

#define HAND_LEVELS_NB_HANDS_SHOWN 7

// TODO: Rects and Points
// clang-format off
static const BG_POINT PAUSE_MENU_CLEAR_SRC_POS            = { 1,  1};
static const Rect     PAUSE_MENU_CLEAR_DEST_RECT          = { 1,  3, 28, 17};

static const Rect     HAND_LEVELS_LVL_TEXT_SRC_RECT       = { 6, 24,  9, 27};
static const BG_POINT HAND_LEVELS_LVL_TEXT_DEST_POS       = { 1,  3};
static const Rect     HAND_LEVELS_LVL_BOX_SRC_RECT        = { 9, 24, 11, 27};
static const BG_POINT HAND_LEVELS_LVL_BOX_DEST_POS        = { 5,  3};

static const Rect     HAND_LEVELS_NAME_BOX_SRC_COL        = {12, 24, 12, 27};
static const BG_POINT HAND_LEVELS_NAME_BOX_DEST_POS       = { 8,  3};
static const u16      HAND_LEVELS_NAME_BOX_DEST_LENGTH    =   7;

static const BG_POINT HAND_LEVELS_CHIPS_BOX_SRC_3W_ROW    = {13, 24};
static const Rect     HAND_LEVELS_CHIPS_BOX_DEST_RECT     = {15,  3, 19,  3};
static const int      HAND_LEVELS_CHIPS_BOX_DEST_HEIGHT   =   4;

static const Rect     HAND_LEVELS_MULT_BOX_SRC_COL        = {16, 24, 16, 27};
static const BG_POINT HAND_LEVELS_MULT_BOX_DEST_POS       = {20,  3};
static const u16      HAND_LEVELS_MULT_BOX_DEST_LENGTH    =   3;

static const Rect     HAND_LEVELS_NB_USES_SRC_RECT        = {17, 24, 22, 27};
static const BG_POINT HAND_LEVELS_NB_USES_DEST_POS        = {23,  3};

static const Rect     HAND_LEVELS_BOTTOM_LINE_SRC_RECT    = { 1,  6, 28,  6};
static const BG_POINT HAND_LEVELS_BOTTOM_LINE_DEST_POS    = { 1, 17};

static const Rect     HAND_LEVELS_MIDDLE_LINE_SRC_RECT    = { 1,  4, 28,  5};
static const BG_POINT HAND_LEVELS_MIDDLE_LINE_BASE_POS    = { 1,  6};
static const int      HAND_LEVELS_MIDDLE_LINE_NB_COPY     = HAND_LEVELS_NB_HANDS_SHOWN - 1;

static const Rect     HAND_LEVELS_ARROW_UP_ON_SRC_RECR    = {23, 24, 24, 25};
static const Rect     HAND_LEVELS_ARROW_UP_OFF_SRC_RECR   = {25, 24, 26, 25};
static const BG_POINT HAND_LEVELS_ARROW_UP_DEST_POS       = {14,  2};

static const Rect     HAND_LEVELS_ARROW_DOWN_ON_SRC_RECR  = {23, 26, 24, 27};
static const Rect     HAND_LEVELS_ARROW_DOWN_OFF_SRC_RECR = {25, 26, 26, 27};
static const BG_POINT HAND_LEVELS_ARROW_DOWN_DEST_POS     = {14, 17};

static const BG_POINT HAND_LEVELS_TEXT_POS                = {32, 32};
// clang-format on

enum PauseMenuState
{
    PAUSE_MENU_STATE_HAND_LEVELS,
    PAUSE_MENU_STATE_PEEK_DECK,
    PAUSE_MENU_STATE_MAX
};

static void hand_level_state_init(void);
static void peek_deck_state_init(void);
static void pause_menu_update(void);

// clang-format off
static StateInfo state_info[] =
{
    [PAUSE_MENU_STATE_HAND_LEVELS] = STATE_INFO_INIT_UPDATE_FN(
        hand_level_state_init, 
        pause_menu_update
    ),
    [PAUSE_MENU_STATE_PEEK_DECK] = STATE_INFO_INIT_UPDATE_FN(
        peek_deck_state_init,
        pause_menu_update
    )
};
// clang-format on

static StateMachine pause_menu_sm = {
    .state_infos = &state_info[0],
    .num_infos = PAUSE_MENU_STATE_MAX,
};

// clang-format off
static Button tab_buttons[] = {
    [PAUSE_MENU_STATE_HAND_LEVELS] = {
        HANDS_TAB_OUTLINE_COLOR_PAL_IDX,
        TAB_MAIN_COLOR_PAL_IDX,
        NULL, NULL
    },
    [PAUSE_MENU_STATE_PEEK_DECK] = {
        DECK_TAB_OUTLINE_COLOR_PAL_IDX,
        TAB_MAIN_COLOR_PAL_IDX,
        NULL, NULL
    }
};
// clang-format on

static void pause_menu_compute_data_on_display(void);
static void pause_menu_handle_tab_change(enum PauseMenuState new_state);
static void hand_levels_print_info(void);
static void hand_levels_handle_vert_scroll(enum ScreenVertDir page_dir);
static void hand_levels_update_arrows(void);

/**
 * @brief Lists the Hand Types to display, in the right order and without any gaps.
 */
static enum HandType s_hands_on_display[HAND_TYPE_MAX + 1] = {0};

/**
 * @brief How many valid Hand Types are listed in @p s_hands_on_display
 *
 * By default, all Hands up to the STRAIGHT_FLUSH are shown, the ones above are secret
 *
 * @sa s_hands_on_display
 */
static u8 s_nb_hand_types = HAND_TYPE_NORMAL_MAX;

/**
 * @brief Scrolling offset in the Hands Levels tab, expressed as an offset from the highest Hand
 * shown
 */
static u8 s_hand_levels_offset = 0;

/**
 * @brief If true, do not update background Tiles, as they are already arranged as needed when
 * loaded from the ROM via GRIT_CPY
 */
static bool first_show = true;

void pause_menu_show(void)
{
    GRIT_CPY(pal_bg_mem, background_pause_menu_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_pause_menu_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_pause_menu_gfxMap);

    pause_menu_compute_data_on_display();

    first_show = true;
    state_machine_register(&pause_menu_sm);
    pause_menu_handle_tab_change(PAUSE_MENU_STATE_HAND_LEVELS);
}

void pause_menu_hide(void)
{
    tte_erase_screen();
    state_machine_remove(&pause_menu_sm);
}

/**
 * @brief Print menu text and, if needed, setup background tiles for the Hand Levels menu
 */
static void hand_level_state_init(void)
{
    tte_erase_screen();

    hand_levels_print_info();

    // No need to manipulate anything in the background tiles when they're first copied
    // as they already represent the first page of the Hand Levels menu
    if (first_show)
    {
        first_show = false;
        return;
    }

    main_bg_se_copy_rect(HAND_LEVELS_LVL_TEXT_SRC_RECT, HAND_LEVELS_LVL_TEXT_DEST_POS);
    main_bg_se_copy_rect(HAND_LEVELS_LVL_BOX_SRC_RECT, HAND_LEVELS_LVL_BOX_DEST_POS);

    main_bg_se_copy_expand_column_hor(
        HAND_LEVELS_NAME_BOX_DEST_POS,
        HAND_LEVELS_NAME_BOX_DEST_LENGTH,
        HAND_LEVELS_NAME_BOX_SRC_COL
    );

    Rect chips_dest_rect = HAND_LEVELS_CHIPS_BOX_DEST_RECT;
    BG_POINT chips_src_pos = HAND_LEVELS_CHIPS_BOX_SRC_3W_ROW;
    for (int i = 0; i < HAND_LEVELS_CHIPS_BOX_DEST_HEIGHT; i++)
    {
        main_bg_se_copy_expand_3w_row(chips_dest_rect, chips_src_pos);
        chips_dest_rect.top++;
        chips_dest_rect.bottom++;
        chips_src_pos.y++;
    }

    main_bg_se_copy_expand_column_hor(
        HAND_LEVELS_MULT_BOX_DEST_POS,
        HAND_LEVELS_MULT_BOX_DEST_LENGTH,
        HAND_LEVELS_MULT_BOX_SRC_COL
    );

    main_bg_se_copy_rect(HAND_LEVELS_NB_USES_SRC_RECT, HAND_LEVELS_NB_USES_DEST_POS);

    // Copy the last line's bottom before it gets erased
    main_bg_se_copy_rect(HAND_LEVELS_BOTTOM_LINE_SRC_RECT, HAND_LEVELS_BOTTOM_LINE_DEST_POS);

    // Copy the full line we just completed 5 times over
    Rect middle_line_src = HAND_LEVELS_MIDDLE_LINE_SRC_RECT;
    BG_POINT middle_line_dest = HAND_LEVELS_MIDDLE_LINE_BASE_POS;
    int middle_line_height = rect_height(&HAND_LEVELS_MIDDLE_LINE_SRC_RECT);
    for (int i = 0; i < HAND_LEVELS_MIDDLE_LINE_NB_COPY; i++)
    {
        main_bg_se_copy_rect(middle_line_src, middle_line_dest);
        middle_line_dest.y += middle_line_height;
        // Don't copy the joint between two lines on the last one, as there is no line underneath
        if (i == HAND_LEVELS_MIDDLE_LINE_NB_COPY - 2)
            middle_line_src.bottom -= 1;
    }

    hand_levels_update_arrows();
}

/**
 * @brief Print menu text and setup background tiles for the Hand Levels menu
 */
static void peek_deck_state_init(void)
{
    tte_erase_screen();
    main_bg_se_copy_rect(HAND_LEVELS_ARROW_UP_OFF_SRC_RECR, HAND_LEVELS_ARROW_UP_DEST_POS);
    main_bg_se_copy_rect(HAND_LEVELS_ARROW_DOWN_OFF_SRC_RECR, HAND_LEVELS_ARROW_DOWN_DEST_POS);
    main_bg_se_copy_expand_tile(PAUSE_MENU_CLEAR_DEST_RECT, PAUSE_MENU_CLEAR_SRC_POS);
}

/**
 * @brief Handle user input and change the Hands shown or the menu Tab accordingly
 */
static void pause_menu_update(void)
{
    enum PauseMenuState new_state = PAUSE_MENU_STATE_MAX;

    if (key_hit(TAB_LEFT))
        new_state = (pause_menu_sm.state - 1 + PAUSE_MENU_STATE_MAX) % PAUSE_MENU_STATE_MAX;
    else if (key_hit(TAB_RIGHT))
        new_state = (pause_menu_sm.state + 1) % PAUSE_MENU_STATE_MAX;

    if (new_state != PAUSE_MENU_STATE_MAX)
    {
        pause_menu_handle_tab_change(new_state);
        return;
    }

    enum ScreenVertDir page_dir = 0;

    if (key_hit(KEY_UP))
        page_dir = SCREEN_UP;
    else if (key_hit(KEY_DOWN))
        page_dir = SCREEN_DOWN;

    if (page_dir != 0)
        hand_levels_handle_vert_scroll(page_dir);
}

/**
 * @brief Given a scrolling direction, update the Hand Levels tab
 *
 * @param scroll_dir which direction we scrolled by pressing UP or DOWN
 */
static void hand_levels_handle_vert_scroll(enum ScreenVertDir scroll_dir)
{
    s_hand_levels_offset =
        max(0,
            min(s_nb_hand_types - HAND_LEVELS_NB_HANDS_SHOWN, s_hand_levels_offset + scroll_dir));

    hand_levels_print_info();
    hand_levels_update_arrows();
}

/**
 * @brief Show the appropriate arrows, indicating whether we can still scroll up or down in the
 * Hands Levels tab
 */
static inline void hand_levels_update_arrows(void)
{
    Rect arrow_up_src_rect;
    Rect arrow_down_src_rect;

    if (s_hand_levels_offset == 0)
        arrow_up_src_rect = HAND_LEVELS_ARROW_UP_OFF_SRC_RECR;
    else
        arrow_up_src_rect = HAND_LEVELS_ARROW_UP_ON_SRC_RECR;

    if (s_hand_levels_offset == s_nb_hand_types - HAND_LEVELS_NB_HANDS_SHOWN)
        arrow_down_src_rect = HAND_LEVELS_ARROW_DOWN_OFF_SRC_RECR;
    else
        arrow_down_src_rect = HAND_LEVELS_ARROW_DOWN_ON_SRC_RECR;

    main_bg_se_copy_rect(arrow_up_src_rect, HAND_LEVELS_ARROW_UP_DEST_POS);
    main_bg_se_copy_rect(arrow_down_src_rect, HAND_LEVELS_ARROW_DOWN_DEST_POS);
}

/**
 * @brief Print all the relevant information about the Hand Types shown.
 *
 * This includes the Level, Name, Chips and Mult, and the number of times a particular Hand Type has
 * been played this run
 */
static inline void hand_levels_print_info(void)
{
    int hand_info_y = HAND_LEVELS_TEXT_POS.y;

    // Printed highest-hand first
    for (u8 i = 0; i < HAND_LEVELS_NB_HANDS_SHOWN; i++)
    {
        enum HandType hand_type = s_hands_on_display[s_hand_levels_offset + i];
        HandBonus hand_bonus = get_hand_total_bonus(hand_type);
        int nb_played_padding = (g_game_vars.nb_played_hands[hand_type - 1] < 10) ? 1 : 0;

        tte_printf(
            "#{P:%d,%d}%s%-3ld %s%-7s %3ld %-3ld  %s%*s%ld",
            HAND_LEVELS_TEXT_POS.x,
            hand_info_y,
            TTE_DARK_GREEN_TAG,
            g_game_vars.hand_levels[hand_type - 1] + 1,
            TTE_WHITE_TAG,
            get_hand_type_name(hand_type),
            hand_bonus.chips,
            hand_bonus.mult,
            TTE_YELLOW_TAG,
            nb_played_padding,
            "",
            g_game_vars.nb_played_hands[hand_type - 1]
        );

        hand_info_y += 2 * TILE_SIZE;
    };
}

/**
 * @brief This function changes the state of the menu to the new one, as determined during input
 * ptocessing, and highlights the corresponding Tab at the top of the screen
 *
 * @param new_state new menu StateMachine state to change to
 *
 * @sa pause_menu_update
 */
static inline void pause_menu_handle_tab_change(enum PauseMenuState new_state)
{
    for (enum PauseMenuState i = 0; i < PAUSE_MENU_STATE_MAX; i++)
    {
        button_set_highlight(&tab_buttons[i], i == new_state);
    }
    state_machine_change_state(&pause_menu_sm, new_state);
}

/**
 * @brief Compute all data that will need to be displayed if it's not already available though @p
 * g_game_vars
 */
static inline void pause_menu_compute_data_on_display(void)
{
    s_nb_hand_types = 0;
    s_hand_levels_offset = 0;

    // This will fill the whole array every time, padding it with value NONE
    enum HandType hand_type = HAND_TYPE_MAX;
    int idx = 0;
    while (idx <= HAND_TYPE_MAX)
    {
        if (!is_hand_type_secret(hand_type) || g_game_vars.nb_played_hands[hand_type - 1] > 0)
        {
            s_hands_on_display[idx] = hand_type;
            idx++;
            if (hand_type > NONE)
                s_nb_hand_types++;
        }

        if (hand_type > NONE)
            hand_type--;
    }
}
