#include "pause_menu.h"

#include "background_pause_menu_gfx.h"
#include "button.h"
#include "game.h"
#include "selection_grid.h"
#include "state_machine.h"

// TODO: Palette indices

#define TAB_MAIN_COLOR_PAL_IDX          1
#define HANDS_TAB_OUTLINE_COLOR_PAL_IDX 2
#define DECK_TAB_OUTLINE_COLOR_PAL_IDX  3

// TODO: Rects and Points

enum PauseMenuState
{
    PAUSE_MENU_STATE_HAND_LEVELS,
    PAUSE_MENU_STATE_PEEK_DECK,
    PAUSE_MENU_STATE_MAX
};

enum HandLevelsPage
{
    HAND_LEVELS_PAGE_1,
    HAND_LEVELS_PAGE_2,
    HAND_LEVELS_PAGE_MAX
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

static void s_pause_menu_handle_tab_change(enum PauseMenuState new_state);

static enum HandLevelsPage s_hand_levels_current_page = HAND_LEVELS_PAGE_1;
static bool first_show = true;

static void hand_level_state_init(void)
{
    s_pause_menu_handle_tab_change(PAUSE_MENU_STATE_HAND_LEVELS);

    // No need to manipulate anything in the background tiles when they're first copied
    // as they already represent the first page of the Hand Levels menu
    if (first_show)
    {
        first_show = false;
        return;
    }

    switch (s_hand_levels_current_page)
    {
        case HAND_LEVELS_PAGE_1:
        {
        }
        break;

        case HAND_LEVELS_PAGE_2:
        {
        }
        break;

        default:
            break;
    }
}

static void peek_deck_state_init(void)
{

}

static void s_pause_menu_handle_tab_change(enum PauseMenuState new_state)
{
    if (new_state != PAUSE_MENU_STATE_MAX)
    {
        for (enum PauseMenuState i = 0; i < PAUSE_MENU_STATE_MAX; i++)
        {
            button_set_highlight(&tab_buttons[i], i == new_state);
        }
        state_machine_change_state(&pause_menu_sm, new_state);
    }
}

static void s_hand_level_handle_page_change(void)
{
    enum HandLevelsPage new_page = HAND_LEVELS_PAGE_MAX;

    if (key_hit(KEY_TAB_LEFT))
        new_page = (s_hand_levels_current_page - 1) % HAND_LEVELS_PAGE_MAX;
    else if (key_hit(KEY_TAB_RIGHT))
        new_page = (s_hand_levels_current_page + 1) % HAND_LEVELS_PAGE_MAX;

    if (new_page != HAND_LEVELS_PAGE_MAX)
    {
        s_hand_levels_current_page = new_page;
        hand_level_state_init();
    }
}

static void pause_menu_update(void)
{
    enum PauseMenuState new_state = PAUSE_MENU_STATE_MAX;
    if (key_hit(KEY_TAB_LEFT))
        new_state = (pause_menu_sm.state - 1) % PAUSE_MENU_STATE_MAX;
    else if (key_hit(KEY_TAB_RIGHT))
        new_state = (pause_menu_sm.state + 1) % PAUSE_MENU_STATE_MAX;
    s_pause_menu_handle_tab_change(new_state);

    if (pause_menu_sm.state == PAUSE_MENU_STATE_HAND_LEVELS)
        s_hand_level_handle_page_change();
}

void pause_menu_show(void)
{
    tte_erase_screen();
    GRIT_CPY(pal_bg_mem, background_pause_menu_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_pause_menu_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_pause_menu_gfxMap);

    first_show = true;
    s_hand_levels_current_page = HAND_LEVELS_PAGE_1;
    state_machine_change_state(&pause_menu_sm, PAUSE_MENU_STATE_HAND_LEVELS);
}
