#include "game/common_ui.h"

#include "game.h"
#include "game/blind_select.h"
#include "game/main_menu.h"
#include "game/rect.h"
#include "game/round.h"
#include "game/round_end.h"
#include "game/shop.h"
#include "game/win_lose.h"

#include <tonc.h>

enum BackgroundId background = BG_NONE;

void reset_background()
{
    background = UNDEFINED;
}
void change_background(enum BackgroundId id)
{
    if (background == id)
    {
        return;
    }
    else if (id == BG_CARD_SELECTING)
    {
        game_selecting_change_background(background);
    }
    else if (id == BG_CARD_PLAYING)
    {
        game_playing_change_background(background);
    }
    else if (id == BG_ROUND_END)
    {
        game_round_end_change_background(background);
    }
    else if (id == BG_SHOP)
    {
        game_shop_change_background();
    }
    else if (id == BG_BLIND_SELECT)
    {
        game_blind_select_change_background();
    }
    else if (id == BG_MAIN_MENU)
    {
        game_main_menu_change_background();
    }
    else
    {
        return; // Invalid background ID
    }

    background = id;
}

void display_round(int value)
{
    int game_round = get_round();
    // tte_erase_rect_wrapper(ROUND_TEXT_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d",
        ROUND_TEXT_RECT.left,
        ROUND_TEXT_RECT.top,
        TTE_YELLOW_PB,
        game_round
    );
}

// Resets bottom row bg tiles of the top left panel (shop/blind) after
// it is dismissed to match the rest of the game panel background.
void reset_top_left_panel_bottom_row()
{
    BG_POINT top_left_panel_bottom_row_pos = TOP_LEFT_PANEL_POINT;
    // Use the source rect height to offset to the bottom row point
    top_left_panel_bottom_row_pos.y += rect_height(&TOP_LEFT_ITEM_SRC_RECT) - 1;
    main_bg_se_copy_rect(TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT, top_left_panel_bottom_row_pos);
}