/**
 * @file common_ui.h
 *
 * @brief Common functions to render UI elements.
 */
#ifndef COMMON_UI_H
#define COMMON_UI_H

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
    BG_MAIN_MENU
};

/**
 * Change the background
 *
 * @param id @ref BackgroundId to render to screen
 */
void change_background(enum BackgroundId id);

#endif // COMMON_UI_H
