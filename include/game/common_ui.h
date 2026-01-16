#ifndef COMMON_UI_H
#define COMMON_UI_H

#include "graphic_utils.h"
#include "sprite.h"

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

// Background functions
void reset_background();
void change_background(enum BackgroundId id);

// Flaming score animation frames
#define SCORE_FLAMES_ANIM_FREQ  5 // animation will run at 12FPS
#define NUM_SCORE_FLAMES_FRAMES 8 // Chips and Mult flame frames are next to one another
#define SCORE_FLAME_FRAME_WIDTH 3 // so we only need to offset to get the next ones

// Display functions
void display_round(int value);
void display_money();
void display_chips(void);
void display_mult(void);
void display_ante(int value);
void display_temp_score(u32 value);
void display_score(u32 value);
void display_hands();
void display_discards();

Rect get_text_rect_under_sprite_object(SpriteObject* sprite_object);
void reset_top_left_panel_bottom_row();

#endif // COMMON_UI_H