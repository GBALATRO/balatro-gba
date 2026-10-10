/**
 * @file common_ui.h
 *
 * @brief Common functions to render UI elements.
 */
#ifndef COMMON_UI_H
#define COMMON_UI_H

#include <stdbool.h>
#include <tonc.h>

/**
 * @brief Indices of the palettes for the different type of sprites
 */
enum SpritePaletteBanks
{
    CARD_PB,
    DECK_PB,
    SKIP_TAGS_PB,
    NORMAL_BLIND_PB,
    BOSS_BLIND_PB,
    JOKER_BASE_PB,
    JOKER_LAST_PB = 15,
    NUM_PALETTES
};

// This won't be more than the number of jokers in your current deck
// plus the amount that can fit in the shop, 8 should be fine. For now...
#define MAX_ACTIVE_JOKERS  8
#define MAX_HAND_SIZE      16
#define MAX_SELECTION_SIZE 5
#define MAX_BLIND_TOKEN    5
#define MAX_SKIP_TAGS      16

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
    BG_RUN_SETUP,
    BG_OPTIONS_MENU,
    BG_MAIN_MENU
};

enum BackgroundId get_current_background(void);

/**
 * Change the background
 *
 * @param id @ref BackgroundId to render to screen
 */
void change_background(enum BackgroundId id, bool force_redraw);

/**
 * @brief Restores the bottom row of the top-left panel from the background map.
 */
void reset_top_left_panel_bottom_row(void);

#endif // COMMON_UI_H
