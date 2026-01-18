#ifndef RECT_H
#define RECT_H

#include "graphic_utils.h"

// clang-format off

// Game rects - Screenblock rects (in tiles)
// Round end/shop menus
static const Rect ROUND_END_MENU_RECT       = {9,       7,      24,     20 };
static const Rect POP_MENU_ANIM_RECT        = {9,       7,      24,     31 };

// Blind select
static const Rect SINGLE_BLIND_SELECT_RECT  = {9,       7,      13,     31 };
static const Rect BLIND_SKIP_BTN_GRAY_RECT  = {0,       24,     4,      27 };
static const Rect BLIND_SKIP_BTN_PREANIM_DEST_RECT = {9,29,     19,     31 };
static const Rect SINGLE_BLIND_SEL_REQ_SCORE_RECT = {80, 120, 104, 128};

// Hand background
static const Rect HAND_BG_RECT_SELECTING    = {9,       11,     24,     17 };

// Top left panel
static const Rect TOP_LEFT_ITEM_SRC_RECT    = {0,       20,     8,      25 };
static const BG_POINT TOP_LEFT_PANEL_POINT  = {0,       0, };
static const Rect TOP_LEFT_PANEL_ANIM_RECT  = {0,       0,      8,      4  };
static const Rect TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT = {0, 28, 8,     28 };
static const BG_POINT TOP_LEFT_BLIND_TITLE_POINT = {0,  21, };
static const Rect BIG_BLIND_TITLE_SRC_RECT  = {0,       26,     8,      26 };
static const Rect BOSS_BLIND_TITLE_SRC_RECT = {0,       27,     8,      27 };

// Cashout and game over dialogs
static const Rect CASHOUT_DEST_RECT =         {10,      8,      23,     10 };
static const BG_POINT CASHOUT_SRC_3X3_RECT_POS =   {5,  29};
static const BG_POINT GAME_OVER_SRC_RECT_3X3_POS = {25, 29};
static const Rect GAME_OVER_DIALOG_DEST_RECT= {11,      21,      23,     28};
static const Rect GAME_OVER_ANIM_RECT       = {11,      8,       23,     28};
static const BG_POINT NEW_RUN_BTN_DEST_POS  = {15,      26};
static const Rect NEW_RUN_BTN_SRC_RECT      = {0,       30,      4,      31};
static const BG_POINT ROUND_END_REWARDS_ELLIPSIS_POS = {10, 13};

// Score flames animation
static const Rect SCORE_FLAME_RESET         = {26,      20,      28,     20};
static const Rect SCORE_FLAME_FRAMES_START  = {26,      21,      28,     21};
static const BG_POINT SCORE_FLAME_CHIPS_POS = {1,       9};
static const BG_POINT SCORE_FLAME_MULT_POS  = {5,       9};

// TTE rects (in pixels)
static const Rect HAND_SIZE_RECT            = {128,     128,    152,    160 };
static const Rect HAND_SIZE_RECT_SELECT     = {128,     128,    152,    136 };
static const Rect HAND_SIZE_RECT_PLAYING    = {128,     152,    152,    160 };
static const Rect HAND_TYPE_RECT            = {8,       64,     64,     72  };
static const Rect TEMP_SCORE_RECT           = {8,       64,     64,     72  };
static const Rect SCORE_RECT                = {24,      48,     64,     56  };

// clang-format on
#endif // RECT_H
