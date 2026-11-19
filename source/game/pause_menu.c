/**
 * @file pause_menu.c
 *
 * @brief Pause overlay implementation.
 */
#include "game/pause_menu.h"

#include "audio_utils.h"
#include "game.h"
#include "game/overlay.h"
#include "graphic_utils.h"
#include "soundbank.h"

#include <string.h>
#include <tonc.h>

#define PAUSE_OPTIONS_SPACING (2 * TILE_SIZE)

enum PauseOption
{
    PAUSE_OPTION_RESUME,
    PAUSE_OPTION_QUIT_RUN,
    PAUSE_OPTION_MAX
};

enum PauseStage
{
    PAUSE_STAGE_CLOSED,
    PAUSE_STAGE_OPEN,
    // An option was chosen, the menu closes once every key is released
    PAUSE_STAGE_CLOSING
};

// Positions in pixels
#define PAUSE_TITLE_Y   56
#define PAUSE_OPTIONS_Y 80

static const char* const PAUSE_OPTION_NAMES[PAUSE_OPTION_MAX] = {
    [PAUSE_OPTION_RESUME] = "Resume",
    [PAUSE_OPTION_QUIT_RUN] = "Quit run"
};

static enum PauseStage s_stage = PAUSE_STAGE_CLOSED;
static enum PauseOption s_selected_option = PAUSE_OPTION_RESUME;
static PauseMenuQuitFunc s_on_quit_run = NULL;

/**
 * @brief Print @p text horizontally centered on the screen.
 */
static void pause_menu_print_centered(const char* text, int y, u8 text_color)
{
    int x = (SCREEN_WIDTH - (int)strlen(text) * TTE_CHAR_SIZE) / 2;

    tte_printf("#{P:%d,%d; cx:0x%X000}%s", x, y, text_color, text);
}

static void pause_menu_print(void)
{
    pause_menu_print_centered("PAUSED", PAUSE_TITLE_Y, TTE_WHITE_PB);

    // WHY: the selected option is only told apart by its color, the font has no arrow glyph
    for (int option = 0; option < PAUSE_OPTION_MAX; option++)
    {
        bool is_selected = (option == (int)s_selected_option);

        pause_menu_print_centered(
            PAUSE_OPTION_NAMES[option],
            PAUSE_OPTIONS_Y + option * PAUSE_OPTIONS_SPACING,
            is_selected ? TTE_YELLOW_PB : TTE_WHITE_PB
        );
    }
}

static void pause_menu_finish_closing(void)
{
    if (!overlay_are_keys_released())
        return;

    overlay_close();
    s_stage = PAUSE_STAGE_CLOSED;

    if (s_selected_option == PAUSE_OPTION_QUIT_RUN && s_on_quit_run != NULL)
        s_on_quit_run();
}

static void pause_menu_choose(enum PauseOption option)
{
    s_selected_option = option;
    s_stage = PAUSE_STAGE_CLOSING;

    play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
}

static void pause_menu_move_selection(int direction)
{
    s_selected_option = (s_selected_option + PAUSE_OPTION_MAX + direction) % PAUSE_OPTION_MAX;

    pause_menu_print();
}

void pause_menu_open(PauseMenuQuitFunc on_quit_run)
{
    if (s_stage != PAUSE_STAGE_CLOSED)
        return;

    s_on_quit_run = on_quit_run;
    s_selected_option = PAUSE_OPTION_RESUME;
    s_stage = PAUSE_STAGE_OPEN;

    overlay_open();
    pause_menu_print();
}

bool pause_menu_is_open(void)
{
    return s_stage != PAUSE_STAGE_CLOSED;
}

void pause_menu_update(void)
{
    if (s_stage == PAUSE_STAGE_CLOSING)
    {
        pause_menu_finish_closing();
        return;
    }

    if (s_stage != PAUSE_STAGE_OPEN)
        return;

    if (key_hit(KEY_PAUSE_GAME | KEY_CANCEL))
        pause_menu_choose(PAUSE_OPTION_RESUME);
    else if (key_hit(KEY_CONFIRM))
        pause_menu_choose(s_selected_option);
    else if (key_hit(KEY_UP))
        pause_menu_move_selection(-1);
    else if (key_hit(KEY_DOWN))
        pause_menu_move_selection(1);
}
