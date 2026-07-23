#include "game/common_ui.h"

#include "blind_select.h"
#include "game.h"
#include "game/main_menu.h"
#include "game/options_menu.h"
#include "game/round.h"
#include "game/round_end.h"
#include "game/run_setup.h"
#include "game/shop.h"
#include "layout.h"

// Backgrounds

typedef void (*BackgroundRenderCallback)(void);

static enum BackgroundId s_background = BG_NONE;

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
    return s_background;
}

void change_background(enum BackgroundId id, bool force_redraw)
{
    if (force_redraw)
    {
        s_background = BG_NONE;
    }
    if (id != s_background && bgCallbacks[id] != NULL)
    {
        bgCallbacks[id]();
    }
    s_background = id;
}

void reset_top_left_panel_bottom_row(void)
{
    BG_POINT top_left_panel_bottom_row_pos = TOP_LEFT_PANEL_POINT;
    // Use the source rect height to offset to the bottom row point
    top_left_panel_bottom_row_pos.y += rect_height(&TOP_LEFT_ITEM_SRC_RECT) - 1;
    main_bg_se_copy_rect(TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT, top_left_panel_bottom_row_pos);
}
