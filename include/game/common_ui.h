#ifndef COMMON_UI_H
#define COMMON_UI_H

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

// Background functions
void reset_background();
void change_background(enum BackgroundId id);

void display_round(int value);
void reset_top_left_panel_bottom_row();

#endif // COMMON_UI_H