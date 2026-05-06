/**
 * @file layout.h
 *
 * @brief Header of shared rects and points needed for ui
 */
#ifndef LAYOUT_H
#define LAYOUT_H

#include "graphic_utils.h"

// clang-format off
// Points                                      x        y
static const BG_POINT CUR_BLIND_TOKEN_POS   = {8,       18};
static const BG_POINT TOP_LEFT_PANEL_POINT  = {0,       0, };
// Rects                                       left     top     right   bottom
static const Rect TOP_LEFT_PANEL_ANIM_RECT  = {0,       0,      8,      4};
static const Rect POP_MENU_ANIM_RECT        = {9,       7,      24,     31};
static const Rect TOP_LEFT_ITEM_SRC_RECT    = {0,       20,     8,      25};
// clang-format on

#endif // LAYOUT_H
