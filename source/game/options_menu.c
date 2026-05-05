#include "game/options_menu.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "background_options_menu_gfx.h"
#include "button.h"
#include "game.h"
#include "game/common_ui.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "save.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "util.h"

#include <stdint.h>
#include <string.h>
#include <tonc.h>
#include <tonc_math.h>
#include <tonc_memdef.h>

// Button indices
enum OptionButtons
{
    GAME_SPEED_BTN_IDX,
    HIGH_CONTRAST_BTN_IDX,
    MUSIC_VOLUME_BTN_IDX,
    SOUND_VOLUME_BTN_IDX,
    BACK_BTN_IDX,
    NB_OPTIONS_BUTTONS
};
enum OptionSpeedButtons
{
    GAME_SPEED_DOWN_BTN_IDX,
    GAME_SPEED_UP_BTN_IDX,
    NB_GAME_SPEED_BUTTONS
};

// Color palette indices
#define MENU_BUTTON_MAIN_COLOR_PAL_IDX          1
#define BACK_BUTTON_MAIN_COLOR_PAL_IDX          3
#define SPEED_DOWN_BUTTON_OUTLINE_COLOR_PAL_IDX 4
#define SPEED_BUTTON_OUTLINE_COLOR_PAL_IDX      5
#define SPEED_UP_BUTTON_OUTLINE_COLOR_PAL_IDX   6
#define CONTRAST_BUTTON_OUTLINE_COLOR_PAL_IDX   7
#define MUSIC_BUTTON_OUTLINE_COLOR_PAL_IDX      8
#define SOUND_BUTTON_OUTLINE_COLOR_PAL_IDX      9
#define BACK_BUTTON_OUTLINE_COLOR_PAL_IDX       10

// Define selection grid for the menu buttons

static void game_speed_down_on_pressed(void);
static void game_speed_up_on_pressed(void);
static void high_contrast_on_pressed(void);
static void back_on_pressed(void);
static int options_menu_return_row_size(void);
static void options_menu_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool game_speed_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static bool music_volume_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static bool sound_volume_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static bool regular_button_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

// clang-format off
static SelectionGridRow options_menu_selection_rows[] = {
    {
        GAME_SPEED_BTN_IDX,
        options_menu_return_row_size,
        game_speed_row_on_selection_changed,
        NULL,
        {.wrap = true} // The wrapping is used to allow button activation with Left/Right arrows by
                       // putting logic into the `game_speed_row_on_selection_changed` function
    },
    {
        HIGH_CONTRAST_BTN_IDX,
        options_menu_return_row_size,
        regular_button_row_on_selection_changed,
        options_menu_row_on_key_transit,
        {.wrap = false}
    },
    {
        MUSIC_VOLUME_BTN_IDX,
        options_menu_return_row_size,
        music_volume_row_on_selection_changed,
        NULL,
        {.wrap = true} // Same trick as Game Speed button
    },
    {
        SOUND_VOLUME_BTN_IDX,
        options_menu_return_row_size,
        sound_volume_row_on_selection_changed,
        NULL,
        {.wrap = true} // Same trick as Game Speed button
    },
    {
        BACK_BTN_IDX,
        options_menu_return_row_size,
        regular_button_row_on_selection_changed,
        options_menu_row_on_key_transit,
        {.wrap = false}
    }
};

static Button options_menu_buttons[] = {
    {SPEED_BUTTON_OUTLINE_COLOR_PAL_IDX,    MENU_BUTTON_MAIN_COLOR_PAL_IDX, NULL,                     NULL},
    {CONTRAST_BUTTON_OUTLINE_COLOR_PAL_IDX, MENU_BUTTON_MAIN_COLOR_PAL_IDX, high_contrast_on_pressed, NULL},
    {MUSIC_BUTTON_OUTLINE_COLOR_PAL_IDX,    MENU_BUTTON_MAIN_COLOR_PAL_IDX, NULL,                     NULL},
    {SOUND_BUTTON_OUTLINE_COLOR_PAL_IDX,    MENU_BUTTON_MAIN_COLOR_PAL_IDX, NULL,                     NULL},
    {BACK_BUTTON_OUTLINE_COLOR_PAL_IDX,     BACK_BUTTON_MAIN_COLOR_PAL_IDX, back_on_pressed,          NULL}
};

static Button game_speed_buttons[] = {
    {SPEED_DOWN_BUTTON_OUTLINE_COLOR_PAL_IDX, MENU_BUTTON_MAIN_COLOR_PAL_IDX, game_speed_down_on_pressed, NULL},
    {SPEED_UP_BUTTON_OUTLINE_COLOR_PAL_IDX,   MENU_BUTTON_MAIN_COLOR_PAL_IDX, game_speed_up_on_pressed,   NULL},
};
// clang-format on

const Selection OPTIONS_MENU_INIT_SEL = {0, 0};

static SelectionGrid options_menu_selection_grid = {
    options_menu_selection_rows,
    NB_OPTIONS_BUTTONS,
    OPTIONS_MENU_INIT_SEL
};

// Positions/Rects used to construct and update the menu
// clang-format off

// Values in tiles
static const Rect     OPTIONS_SPEED_DOWN_ACTIVE_BTN_SRC_RECT   = { 3, 20,  3, 21};
static const Rect     OPTIONS_SPEED_DOWN_DISABLED_BTN_SRC_RECT = { 9, 22,  9, 23};
static const BG_POINT OPTIONS_SPEED_DOWN_BTN_DEST_POS          = {11,  3};
static const Rect     OPTIONS_SPEED_UP_ACTIVE_BTN_SRC_RECT     = {10, 20, 10, 21};
static const Rect     OPTIONS_SPEED_UP_DISABLED_BTN_SRC_RECT   = {10, 22, 10, 23};
static const BG_POINT OPTIONS_SPEED_UP_BTN_DEST_POS            = {18,  3};
static const Rect     OPTIONS_SPEED_VALUES[GAME_SPEED_MAX]   = { { 6, 20,  7, 21},
                                                                 { 3, 22,  4, 23},
                                                                 { 5, 22,  6, 23},
                                                                 { 7, 22,  8, 23} };
static const BG_POINT OPTIONS_SPEED_VALUE_DEST_POS             = {14,  3};

static const Rect     OPTIONS_CONTRAST_VALUE_YES_SRC_RECT      = { 4, 24,  7, 25};
static const Rect     OPTIONS_CONTRAST_VALUE_NO_SRC_RECT       = { 9, 24, 12, 25};
static const BG_POINT OPTIONS_CONTRAST_VALUE_DEST_POS          = {13,  7};

static const Rect     OPTIONS_MUSIC_SLIDER_FULL_SRC            = {12, 20, 12, 20};
static const Rect     OPTIONS_MUSIC_SLIDER_MID_SRC             = {13, 20, 13, 20};
static const Rect     OPTIONS_MUSIC_SLIDER_EMPTY_SRC           = {14, 20, 14, 20};
static const Rect     OPTIONS_SOUND_SLIDER_FULL_SRC            = {12, 22, 12, 22};
static const Rect     OPTIONS_SOUND_SLIDER_MID_SRC             = {13, 22, 13, 22};
static const Rect     OPTIONS_SOUND_SLIDER_EMPTY_SRC           = {14, 22, 14, 22};
static const BG_POINT OPTIONS_MUSIC_SLIDER_START_POS           = { 5, 11};
static const BG_POINT OPTIONS_SOUND_SLIDER_START_POS           = { 5, 14};
static const BG_POINT OPTIONS_MUSIC_SLIDER_END_POS             = {24, 11};
static const BG_POINT OPTIONS_SOUND_SLIDER_END_POS             = {24, 14};
static const u8       OPTIONS_MUSIC_SLIDER_SEGMENT_LENGTH      = (OPTIONS_MUSIC_SLIDER_END_POS.x - OPTIONS_MUSIC_SLIDER_START_POS.x + 1)
                                                                    / VOLUME_OPTION_MAX;
static const u8       OPTIONS_SOUND_SLIDER_SEGMENT_LENGTH      = (OPTIONS_SOUND_SLIDER_END_POS.x - OPTIONS_SOUND_SLIDER_START_POS.x + 1)
                                                                    / VOLUME_OPTION_MAX;

// Values in pixels
static const BG_POINT OPTIONS_GAME_SPEED_TEXT_POS    = { 82,  16};
static const BG_POINT OPTIONS_HIGH_CONTRAST_TEXT_POS = { 40,  48};
static const BG_POINT OPTIONS_MUSIC_VOLUME_TEXT_POS  = { 56,  80};
static const BG_POINT OPTIONS_MUSIC_VALUE_TEXT_POS   = {160,  80};
static const BG_POINT OPTIONS_SOUND_VOLUME_TEXT_POS  = { 56, 104};
static const BG_POINT OPTIONS_SOUND_VALUE_TEXT_POS   = {160, 104};
static const BG_POINT OPTIONS_BACK_SAVE_TEXT_POS     = { 72, 136};
// clang-format on

#define GAME_SPEED_ARROW_HIGHLIGHT_DURATION 10
enum OptionSpeedButtons game_speed_arrow_highlight_button = GAME_SPEED_UP_BTN_IDX;
static s32 game_speed_arrow_highlight_start = UNDEFINED;

static bool any_value_changed = false;
static bool back_btn_is_save_state = false;

static void disable_all_game_speed_outlines_except_self(enum OptionSpeedButtons highlighted_btn)
{
    for (int i = 0; i < NB_GAME_SPEED_BUTTONS; i++)
    {
        button_set_highlight(&game_speed_buttons[i], i == highlighted_btn);
    }
}

static void disable_all_outlines_except_self(enum OptionButtons highlighted_btn)
{
    // These two get disabled no matter what here
    disable_all_game_speed_outlines_except_self(NB_GAME_SPEED_BUTTONS);

    for (int i = 0; i < NB_OPTIONS_BUTTONS; i++)
    {
        button_set_highlight(&options_menu_buttons[i], i == highlighted_btn);
    }
}

static void update_game_speed_button_graphics()
{
    // check if need to disable game speed arrows
    if (g_game_vars.game_speed == GAME_SPEED_MIN)
    {
        main_bg_se_copy_rect(
            OPTIONS_SPEED_DOWN_DISABLED_BTN_SRC_RECT,
            OPTIONS_SPEED_DOWN_BTN_DEST_POS
        );
    }
    else
    {
        main_bg_se_copy_rect(
            OPTIONS_SPEED_DOWN_ACTIVE_BTN_SRC_RECT,
            OPTIONS_SPEED_DOWN_BTN_DEST_POS
        );
    }

    if (g_game_vars.game_speed == GAME_SPEED_MAX)
    {
        main_bg_se_copy_rect(OPTIONS_SPEED_UP_DISABLED_BTN_SRC_RECT, OPTIONS_SPEED_UP_BTN_DEST_POS);
    }
    else
    {
        main_bg_se_copy_rect(OPTIONS_SPEED_UP_ACTIVE_BTN_SRC_RECT, OPTIONS_SPEED_UP_BTN_DEST_POS);
    }

    main_bg_se_copy_rect(
        OPTIONS_SPEED_VALUES[g_game_vars.game_speed - 1],
        OPTIONS_SPEED_VALUE_DEST_POS
    );

    any_value_changed = true;
}

static void update_high_contrast_button_graphics()
{
    if (g_game_vars.high_contrast)
    {
        main_bg_se_copy_rect(OPTIONS_CONTRAST_VALUE_YES_SRC_RECT, OPTIONS_CONTRAST_VALUE_DEST_POS);
    }
    else
    {
        main_bg_se_copy_rect(OPTIONS_CONTRAST_VALUE_NO_SRC_RECT, OPTIONS_CONTRAST_VALUE_DEST_POS);
    }

    any_value_changed = true;
}

static void update_volume_slider_graphics(enum OptionButtons slider_idx)
{
    u8 slider_value;
    BG_POINT slider_segment_dest;
    int slider_segment_length;
    Rect slider_segment_full;
    Rect slider_segment_mid;
    Rect slider_segment_empty;
    BG_POINT slider_text_pos;

    switch (slider_idx)
    {
        case MUSIC_VOLUME_BTN_IDX:
            slider_value = g_game_vars.music_volume;
            slider_segment_dest = OPTIONS_MUSIC_SLIDER_START_POS;
            slider_segment_length = OPTIONS_MUSIC_SLIDER_SEGMENT_LENGTH;
            slider_segment_full = OPTIONS_MUSIC_SLIDER_FULL_SRC;
            slider_segment_mid = OPTIONS_MUSIC_SLIDER_MID_SRC;
            slider_segment_empty = OPTIONS_MUSIC_SLIDER_EMPTY_SRC;
            slider_text_pos = OPTIONS_MUSIC_VALUE_TEXT_POS;
            break;
        case SOUND_VOLUME_BTN_IDX:
            slider_value = g_game_vars.sound_volume;
            slider_segment_dest = OPTIONS_SOUND_SLIDER_START_POS;
            slider_segment_length = OPTIONS_SOUND_SLIDER_SEGMENT_LENGTH;
            slider_segment_full = OPTIONS_SOUND_SLIDER_FULL_SRC;
            slider_segment_mid = OPTIONS_SOUND_SLIDER_MID_SRC;
            slider_segment_empty = OPTIONS_SOUND_SLIDER_EMPTY_SRC;
            slider_text_pos = OPTIONS_SOUND_VALUE_TEXT_POS;
            break;
        // This function only takes in a volume slider
        default:
            return;
    }

    int i = 0;

    // full part of the bar
    for (; i < slider_value * slider_segment_length - 1; i++)
    {
        main_bg_se_copy_rect(slider_segment_full, slider_segment_dest);
        slider_segment_dest.x++;
    }

    // at exactly `slider_value` we either:
    //  - are in the middle of the bar, then we draw the frontier between full and empty tiles
    //  - are at the end, then draw a full segment
    if (slider_value == VOLUME_OPTION_MAX)
    {
        main_bg_se_copy_rect(slider_segment_full, slider_segment_dest);
    }
    else
    {
        if (slider_value != VOLUME_OPTION_MIN)
        {
            // draw middle point
            main_bg_se_copy_rect(slider_segment_mid, slider_segment_dest);
            i++;
            slider_segment_dest.x++;
        }

        // empty part of the bar
        for (; i < VOLUME_OPTION_MAX * slider_segment_length; i++)
        {
            main_bg_se_copy_rect(slider_segment_empty, slider_segment_dest);
            slider_segment_dest.x++;
        }
    }

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%3d",
        slider_text_pos.x,
        slider_text_pos.y,
        TTE_WHITE_PB,
        (slider_value * VOLUME_OPTION_INCREMENT)
    );

    any_value_changed = true;
}

static void change_back_save_text(bool is_back)
{
    char* btn_text = is_back ? "    Back    " : "Save Changes";
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        OPTIONS_BACK_SAVE_TEXT_POS.x,
        OPTIONS_BACK_SAVE_TEXT_POS.y,
        TTE_WHITE_PB,
        btn_text
    );
}

void game_options_menu_change_background(void)
{
    tte_erase_screen();
    CBB_CLEAR(MAIN_BG_CBB);

    GRIT_CPY(pal_bg_mem, background_options_menu_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_options_menu_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_options_menu_gfxMap);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}Game Speed",
        OPTIONS_GAME_SPEED_TEXT_POS.x,
        OPTIONS_GAME_SPEED_TEXT_POS.y,
        TTE_WHITE_PB
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}High Contrast Cards",
        OPTIONS_HIGH_CONTRAST_TEXT_POS.x,
        OPTIONS_HIGH_CONTRAST_TEXT_POS.y,
        TTE_WHITE_PB
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}Music Volume",
        OPTIONS_MUSIC_VOLUME_TEXT_POS.x,
        OPTIONS_MUSIC_VOLUME_TEXT_POS.y,
        TTE_WHITE_PB
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}Sound Volume",
        OPTIONS_SOUND_VOLUME_TEXT_POS.x,
        OPTIONS_SOUND_VOLUME_TEXT_POS.y,
        TTE_WHITE_PB
    );
    change_back_save_text(true);
}

void game_options_menu_on_init(void)
{
    change_background(BG_OPTIONS_MENU);

    // Select game speed button by default
    options_menu_selection_grid.selection = OPTIONS_MENU_INIT_SEL;
    disable_all_outlines_except_self(GAME_SPEED_BTN_IDX);

    // Do an update on the first frame
    update_game_speed_button_graphics();
    update_high_contrast_button_graphics();
    update_volume_slider_graphics(MUSIC_VOLUME_BTN_IDX);
    update_volume_slider_graphics(SOUND_VOLUME_BTN_IDX);

    // Prevent "Back" button to become "Save Changes" when we first load the data,
    // the player hasn't technically changed anything so it would be confusing.
    any_value_changed = false;
}

void game_options_menu_on_update(void)
{
    selection_grid_process_input(&options_menu_selection_grid);

    // game speed arrows small animation: they stay highlighted for a few frames
    if (game_speed_arrow_highlight_start != UNDEFINED &&
        (g_game_vars.timer - game_speed_arrow_highlight_start) >
            GAME_SPEED_ARROW_HIGHLIGHT_DURATION)
    {
        game_speed_arrow_highlight_start = UNDEFINED;
        button_set_highlight(&game_speed_buttons[game_speed_arrow_highlight_button], false);
    }

    // Except on the first update where it is forced to display the loaded values
    // instead of the default ones, if any value was changed, edit the last button
    // text to read "Save Changes" instead of "Back"
    if (any_value_changed && !back_btn_is_save_state)
    {
        change_back_save_text(false);
        back_btn_is_save_state = true;
    }

    any_value_changed = false;
}

void game_options_menu_on_exit(void)
{
    tte_erase_screen();
}

static void game_speed_down_on_pressed(void)
{
    g_game_vars.game_speed--;
    game_speed_arrow_highlight_start = g_game_vars.timer;
    game_speed_arrow_highlight_button = GAME_SPEED_DOWN_BTN_IDX;
    disable_all_game_speed_outlines_except_self(GAME_SPEED_DOWN_BTN_IDX);
    update_game_speed_button_graphics();
}

static void game_speed_up_on_pressed(void)
{
    g_game_vars.game_speed++;
    game_speed_arrow_highlight_start = g_game_vars.timer;
    game_speed_arrow_highlight_button = GAME_SPEED_UP_BTN_IDX;
    disable_all_game_speed_outlines_except_self(GAME_SPEED_UP_BTN_IDX);
    update_game_speed_button_graphics();
}

/**
 * @brief Handles input for the high contrast card toggle button and nothing more.
 */
static void high_contrast_on_pressed(void)
{
    g_game_vars.high_contrast = (g_game_vars.high_contrast == 1) ? false : true;
    update_high_contrast_button_graphics();
}

/**
 * @brief Handles input for the Back/Save button.
 */
static void back_on_pressed(void)
{
    if (back_btn_is_save_state)
    {
        save_game();
        change_back_save_text(true);
        back_btn_is_save_state = false;
    }
    else
    {
        game_change_state(GAME_STATE_MAIN_MENU);
    }
}

/**
 * @brief Gives the width of options menu rows in selection grid.
 *
 * @returns Always 1, all rows contain only 1 button. Could have been a define
 *          but I had to make this a function so it could be given to the
 *          SelectionGrid constructor.
 */
static int options_menu_return_row_size(void)
{
    return 1;
}

static void change_button_highlight(
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    if (prev_selection->y == row_idx && prev_selection->x >= 0 &&
        prev_selection->x < NB_OPTIONS_BUTTONS)
    {
        button_set_highlight(&options_menu_buttons[row_idx], false);
    }

    if (new_selection->y == row_idx)
    {
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
        button_set_highlight(&options_menu_buttons[row_idx], true);
    }
}

/**
 * @brief Handles left/Right inputs on the row of the Game Speed button.
 *        Is used to increased/decrease the speed value, instead of relying on a
 *        `on_key_transit` function because the latter does not take directional
 *        inputs.
 */
static bool game_speed_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    change_button_highlight(row_idx, prev_selection, new_selection);
    disable_all_game_speed_outlines_except_self(NB_GAME_SPEED_BUTTONS);

    // Can only change game speed by pressing left/right while staying on the button's row
    if (prev_selection->y != new_selection->y)
        return true;

    if (key_hit(KEY_LEFT) && g_game_vars.game_speed > GAME_SPEED_MIN)
    {
        button_press(&game_speed_buttons[GAME_SPEED_DOWN_BTN_IDX]);
    }
    else if (key_hit(KEY_RIGHT) && g_game_vars.game_speed < GAME_SPEED_MAX)
    {
        button_press(&game_speed_buttons[GAME_SPEED_UP_BTN_IDX]);
    }

    return true;
}

/**
 * @brief Handles input for all SelectionGridRows in options menu, only actually does
 *        something on rows containing "normal" buttons (those for which the on_pressed
 *        method in options_menu_buttons is not NULL).
 */
static void options_menu_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
    {
        button_press(&options_menu_buttons[selection->y]);
    }
}

static bool regular_button_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    change_button_highlight(row_idx, prev_selection, new_selection);
    return true;
}

static bool music_volume_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    change_button_highlight(row_idx, prev_selection, new_selection);

    if (prev_selection->y != new_selection->y)
        return true;

    if (key_hit(KEY_LEFT) && g_game_vars.music_volume > VOLUME_OPTION_MIN)
    {
        g_game_vars.music_volume--;
    }
    else if (key_hit(KEY_RIGHT) && g_game_vars.music_volume < VOLUME_OPTION_MAX)
    {
        g_game_vars.music_volume++;
    }

    update_volume_slider_graphics(MUSIC_VOLUME_BTN_IDX);
    mmSetModuleVolume(MM_MODULE_FULL_VOLUME * g_game_vars.music_volume / VOLUME_OPTION_MAX);

    return true;
}

static bool sound_volume_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    change_button_highlight(row_idx, prev_selection, new_selection);

    if (prev_selection->y != new_selection->y)
        return true;

    if (key_hit(KEY_LEFT) && g_game_vars.sound_volume > VOLUME_OPTION_MIN)
    {
        g_game_vars.sound_volume--;
    }
    else if (key_hit(KEY_RIGHT) && g_game_vars.sound_volume < VOLUME_OPTION_MAX)
    {
        g_game_vars.sound_volume++;
    }

    update_volume_slider_graphics(SOUND_VOLUME_BTN_IDX);

    return true;
}
