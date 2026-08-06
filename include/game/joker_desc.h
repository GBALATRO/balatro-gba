/**
 * @file joker_desc.h
 *
 * @brief Shared Joker description overlay (panel draw + active card tracking).
 */
#ifndef GAME_JOKER_DESC_H
#define GAME_JOKER_DESC_H

#include "graphic_utils.h"
#include "joker.h"

#include <tonc.h>

/** Frames to slide UI / joker into place when showing a description. */
#define TM_SHOW_CARD_DESC_WAIT 12

/** Frames near the end of the show anim used to tuck the deck away. */
#define TM_HIDE_DECK_WAIT 5

/** Pixel Y offset used to tuck owned jokers off the top while viewing a description. */
#define JOKER_DESC_OWNED_HIDE_Y_OFFSET 50

/** Palette indices for the description rarity stripe on the play BG.
 *  Must not reuse 24-31 — those are the score-flame colors (blue/red). */
#define JOKER_DESC_RARITY_MAIN_COLOR_PAL_IDX   32
#define JOKER_DESC_RARITY_SHADOW_COLOR_PAL_IDX 33

// clang-format off
/** Destination rect for the description 9-patch (tiles) — shop and round. */
static const Rect JOKER_DESC_9_PTCH_TO_RECT = { 9,  6, 28, 18};

/** Text area inside the description panel (tiles). */
static const Rect JOKER_DESC_TEXT_RECT = {11,  9, 26, 18};

/** Name line at the top of the description panel (tiles). */
static const Rect JOKER_DESC_NAME_TEXT_RECT = {10,  7, 27,  7};

/** Screen position (pixels) for the joker sprite while its description is shown. */
static const BG_POINT JOKER_DESC_SPRITE_POS = {135, 9};
// clang-format on

/**
 * @brief Get the joker currently shown in a description overlay, if any.
 *
 * Used so movement loops do not fight the description animation.
 */
JokerObject* joker_desc_get_active(void);

/**
 * @brief Set which joker is currently shown in a description overlay (or NULL).
 */
void joker_desc_set_active(JokerObject* joker_object);

/**
 * @brief Install the shop's description 9-patch into the currently loaded play background.
 *
 * Copies tiles into free VRAM slots, stages source SE offscreen, and returns the source rect.
 * Call @ref joker_desc_release_shop_9patch_se after expanding.
 */
const NinePatchRect* joker_desc_install_shop_9patch(void);

/**
 * @brief Copy shop 9-patch tiles into VRAM without leaving staged SE behind.
 */
void joker_desc_install_shop_9patch_tiles(void);

/**
 * @brief Restore atlas SE overwritten while staging the shop 9-patch source.
 */
void joker_desc_release_shop_9patch_se(void);

/**
 * @brief Save main-BG SE under @p se_rect before drawing the description overlay over it.
 *
 * @param se_rect Tile rect to preserve (must fit in the underlay buffer)
 */
void joker_desc_save_underlay(Rect se_rect);

/**
 * @brief Restore the SE saved by @ref joker_desc_save_underlay.
 */
void joker_desc_restore_underlay(void);

/**
 * @brief Discard any saved underlay without writing it back.
 */
void joker_desc_discard_underlay(void);

/**
 * @brief Clear an SE rect inclusively (unlike @c main_bg_se_clear_rect).
 */
void joker_desc_clear_se_rect(Rect se_rect);

/**
 * @brief Draw name, description text, rarity label, and the description panel (shop path).
 *
 * @param joker Joker whose registry entry supplies name/desc/rarity
 * @param src_9_ptch 9-patch source tiles in the currently loaded background
 * @param rarity_main_pal_idx Palette index overwritten with the rarity main color
 * @param rarity_shadow_pal_idx Palette index overwritten with the rarity shadow color
 */
void joker_desc_draw_panel(
    Joker* joker,
    const NinePatchRect* src_9_ptch,
    u8 rarity_main_pal_idx,
    u8 rarity_shadow_pal_idx
);

/**
 * @brief Draw the description panel during a round using installed shop 9-patch tiles.
 */
void joker_desc_draw_round_panel(Joker* joker, u8 rarity_main_pal_idx, u8 rarity_shadow_pal_idx);

/**
 * @brief Restore play-BG flame palette slots (24-31) from the stock play palette.
 *
 * Older rarity-stripe code overwrote 27/28 and left blue pixels in the red flame.
 */
void joker_desc_restore_flame_palette(void);

/**
 * @brief Clear the description panel tiles from the main background.
 */
void joker_desc_clear_panel(void);

#endif // GAME_JOKER_DESC_H
