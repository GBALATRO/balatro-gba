/**
 * @file layout.h
 *
 * @brief Header of shared rects and points needed for ui
 */
#ifndef LAYOUT_H
#define LAYOUT_H

#include "graphic_utils.h"

// clang-format off
// Points                                                 x        y
static const BG_POINT CUR_BLIND_TOKEN_POS              = {8,       18};
static const BG_POINT TOP_LEFT_PANEL_POINT             = {0,       0};
static const BG_POINT ROUND_END_REWARDS_ELLIPSIS_POS   = {10,      13};
static const BG_POINT JOKER_DISCARD_TARGET             = {240,     30};
static const BG_POINT HELD_JOKERS_POS                  = {108,     10};
// Rects                                                  left     top     right   bottom
static const Rect TOP_LEFT_PANEL_ANIM_RECT             = {0,       0,      8,      4};
static const Rect POP_MENU_ANIM_RECT                   = {9,       7,      24,     31};
static const Rect DECK_ANIM_RECT                       = {25,      14,     28,     23}; // Can be used for the Shop and Blind Select screen
static const Rect TOP_LEFT_ITEM_SRC_RECT               = {0,       20,     8,      25};
static const Rect TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT = {0,       28,     8,      28};
static const Rect BLIND_REWARD_RECT                    = {40,      32,     64,     40};
static const Rect BLIND_REQ_TEXT_RECT                  = {32,      24,     64,     32};
static const Rect PLAYING_SCREEN_RECT                  = {72,      0,      240,    160};
static const Rect HAND_SIZE_RECT                       = {128,     128,    160,    160}; // Seems to include both SELECT and PLAYING
// clang-format on

#endif // LAYOUT_H
