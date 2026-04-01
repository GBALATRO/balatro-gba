#include "game/common_ui.h"

#include "game.h"

typedef void (*BackgroundRenderCallback)(void);

// Map to fill in for refactor
BackgroundRenderCallback bgCallbacks[] =
{
    [BG_NONE] = NULL,
    [BG_CARD_SELECTING] = NULL,
    [BG_CARD_PLAYING] = NULL,
    [BG_ROUND_END] = NULL,
    [BG_SHOP] = NULL,
    [BG_BLIND_SELECT] = NULL,
    [BG_MAIN_MENU] = NULL,
};

void change_background(enum BackgroundId id)
{
    change_background_legacy(id);
}
