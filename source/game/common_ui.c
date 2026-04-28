#include "game/common_ui.h"

#include "game.h"
#include "game/main_menu.h"

typedef void (*BackgroundRenderCallback)(void);

static enum BackgroundId background = BG_NONE;

// Map to fill in for refactor
static const BackgroundRenderCallback bgCallbacks[] = {
    [BG_NONE] = NULL,
    [BG_CARD_SELECTING] = NULL,
    [BG_CARD_PLAYING] = NULL,
    [BG_ROUND_END] = NULL,
    [BG_SHOP] = NULL,
    [BG_BLIND_SELECT] = NULL,
    [BG_MAIN_MENU] = game_main_menu_change_background,
};

void change_background(enum BackgroundId id)
{
    if (id == BG_MAIN_MENU)
    {
        if (id != background)
        {
            bgCallbacks[id]();
        }
    }
    change_background_legacy(id);
    background = id;
}
