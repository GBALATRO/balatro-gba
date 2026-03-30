#ifndef COMMON_UI_H_
#define COMMON_UI_H_

#include <tonc.h>

enum BackgroundId
{
    BG_NONE,
    BG_CARD_SELECTING,
    BG_CARD_PLAYING,
    BG_ROUND_END,
    BG_SHOP,
    BG_BLIND_SELECT,
    BG_MAIN_MENU
};

void change_background(enum BackgroundId id);

#endif // COMMON_UI_H_
