/**
 * @file affine_background.h
 *
 * @brief Utilities for affine background on GBA
 */
#ifndef AFFINE_BACKGROUND_H
#define AFFINE_BACKGROUND_H

#include "graphic_utils.h"

#include <tonc.h>

/**
 * @def AFFINE_BG_IDX
 * @brief The index of the affine background BGCNT register etc.
 */
#define AFFINE_BG_IDX 2

/**
 * @def AFFINE_BG_PAL_LEN
 * @brief Number of u16 colors available in the palette.
 */
#define AFFINE_BG_PAL_LEN 16

/**
 * @def AFFINE_BG_PB
 * @brief The starting index of the background palette.
 */
#define AFFINE_BG_PB (PAL_ROW_LEN * 10)

/**
 * @brief An ID to specify background rendering types.
 */
enum AffineBackgroundID
{
    /**
     * @brief Display background for main menu.
     *
     * Signifies which background to use and provides a higher quality affine
     * mode to display when there is no game logic.
     */
    AFFINE_BG_MAIN_MENU,
    /**
     * @brief Display background for game play.
     *
     * Signifies which background to use and provides a lower quality affine
     * mode to keep resources down during play.
     */
    AFFINE_BG_GAME,
};

/**
 * @brief Initialize resources for affine background rendering
 */
void affine_background_init();

/**
 * @brief Interrupt routine to update display on HBLANK
 */
IWRAM_CODE void affine_background_hblank();

/**
 * @brief Per-frame update of the affine background
 */
IWRAM_CODE void affine_background_update();

/**
 * @brief Update the affine background color
 *
 * @param color @ref COLOR to set
 */
void affine_background_set_color(COLOR color);

/**
 * @brief Set the background palette
 *
 * Must be called with an array of size at least @ref AFFINE_BG_PAL_LEN
 *
 * @param src pointer to palette to set
 */
void affine_background_load_palette(const u16* src);

/**
 * @brief Update the background id
 *
 * Update the background id by configuring display registers and loading
 * the image and palette used for the specified id.
 *
 * @param new_bg @ref AffineBackgrounID to set
 */
void affine_background_change_background(enum AffineBackgroundID new_bg);

#endif // AFFINE_BACKGROUND_H
