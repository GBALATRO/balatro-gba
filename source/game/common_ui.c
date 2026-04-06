#include "game/common_ui.h"

#include "game/main_menu.h"
#include "game/blind_select.h"

#include "game.h"

typedef void (*BackgroundRenderCallback)(void);

static enum BackgroundId background = BG_NONE;

// Map to fill in for refactor
BackgroundRenderCallback bgCallbacks[] =
{
    [BG_NONE] = NULL,
    [BG_CARD_SELECTING] = NULL,
    [BG_CARD_PLAYING] = NULL,
    [BG_ROUND_END] = NULL,
    [BG_SHOP] = NULL,
    [BG_BLIND_SELECT] = game_blind_select_change_background,
    [BG_MAIN_MENU] = game_main_menu_change_background,
};

void change_background(enum BackgroundId id)
{
    if(id == BG_MAIN_MENU || id == BG_BLIND_SELECT)
    {
        if(id != background)
        {
            bgCallbacks[id]();
        }
    }
    change_background_legacy(id);
    id = background;
}
