#include "game/common_ui.h"

#include "blind_select.h"
#include "game.h"
#include "game/main_menu.h"
#include "game/options_menu.h"
#include "game/round.h"
#include "game/round_end.h"
#include "game/run_setup.h"
#include "game/shop.h"

typedef void (*BackgroundRenderCallback)(void);

static enum BackgroundId background = BG_NONE;

// Map to fill in for refactor
static const BackgroundRenderCallback bgCallbacks[] = {
    [BG_NONE] = NULL,
    [BG_CARD_SELECTING] = game_round_change_background_selecting,
    [BG_CARD_PLAYING] = game_round_change_background_playing,
    [BG_ROUND_END] = game_round_end_change_background,
    [BG_SHOP] = game_shop_change_background,
    [BG_BLIND_SELECT] = game_blind_select_change_background,
    [BG_RUN_SETUP] = game_run_setup_change_background,
    [BG_OPTIONS_MENU] = game_options_menu_change_background,
    [BG_MAIN_MENU] = game_main_menu_change_background,
};

enum BackgroundId get_current_background(void)
{
    return background;
}

void change_background(enum BackgroundId id, bool force_redraw)
{
    if (force_redraw)
    {
        background = BG_NONE;
    }
    if (id != background && bgCallbacks[id] != NULL)
    {
        bgCallbacks[id]();
    }
    background = id;
}
