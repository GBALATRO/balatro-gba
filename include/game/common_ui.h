/**
 * @file common_ui.h
 *
 * @brief Common functions to render UI elements.
 */
#ifndef COMMON_UI_H
#define COMMON_UI_H

#include <stdbool.h>

/**
 * @brief Enum of possible backgrounds to render with @ref change_background
 */
enum BackgroundId
{
    BG_NONE,
    BG_CARD_SELECTING,
    BG_CARD_PLAYING,
    BG_ROUND_END,
    BG_SHOP,
    BG_BLIND_SELECT,
    BG_OPTIONS_MENU,
    BG_MAIN_MENU
};

/**
 * Change the background
 *
 * @param id @ref BackgroundId to render to screen
 */
void change_background(enum BackgroundId id, bool force_redraw);

#endif // COMMON_UI_H
