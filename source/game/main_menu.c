#include "game/main_menu.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "background_main_menu_gfx.h"
#include "card.h"
#include "game.h"
#include "game/palette.h"
#include "graphic_utils.h"
#include "soundbank.h"
#include "sprite.h"
#include "util.h"

#include <stdint.h>
#include <tonc.h>
#include <tonc_math.h>
#include <tonc_memdef.h>

#define MAIN_MENU_BUTTONS             2
#define MAIN_MENU_IMPLEMENTED_BUTTONS 1 // Remove this once all buttons are implemented
#define MAIN_MENU_PLAY_BTN_IDX        0

#define HIGHLIGHT_COLOR 0xFFFF

#define BUTTON_SFX_VOLUME 154 // 60% of MM_FULL_VOLUME

#define MENU_POP_OUT_ANIM_FRAMES 20

// Pixel sizes
#define MAIN_MENU_ACE_T_X 88
#define MAIN_MENU_ACE_T_Y 26

// Forward declarations from game.c
extern unsigned int timer;
extern int game_speed;
extern enum BackgroundId background;
extern unsigned int rng_seed;
extern int selection_x;

// External functions from game.c
extern void change_background(enum BackgroundId id);
extern void game_start(void);

// Main menu sprite - the ace of spades
static CardObject* main_menu_ace = NULL;

void game_main_menu_on_init(void)
{
    affine_background_change_background(AFFINE_BG_MAIN_MENU);
    change_background(BG_MAIN_MENU);
    main_menu_ace = card_object_new(card_new(SPADES, ACE));
    card_object_set_sprite(main_menu_ace, 0); // Set the sprite for the ace of spades
    main_menu_ace->sprite_object->sprite->obj->attr0 |=
        ATTR0_AFF_DBL; // Make the sprite double sized
    main_menu_ace->sprite_object->tx = int2fx(MAIN_MENU_ACE_T_X);
    main_menu_ace->sprite_object->x = main_menu_ace->sprite_object->tx;
    main_menu_ace->sprite_object->ty = int2fx(MAIN_MENU_ACE_T_Y);
    main_menu_ace->sprite_object->y = main_menu_ace->sprite_object->ty;
    main_menu_ace->sprite_object->tscale = float2fx(0.8f);
}

void game_main_menu_change_background(void)
{
    toggle_windows(false, false);

    tte_erase_screen();
    GRIT_CPY(pal_bg_mem, background_main_menu_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_main_menu_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_main_menu_gfxMap);

    // Disable the button highlight colors
    memcpy16(
        &pal_bg_mem[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID],
        &pal_bg_mem[MAIN_MENU_PLAY_BUTTON_MAIN_COLOR_PID],
        1
    );
}

void game_main_menu_on_update(void)
{
    change_background(BG_MAIN_MENU);

    card_object_update(main_menu_ace);
    main_menu_ace->sprite_object->trotation = lu_sin((timer << 8) / 2) / 3;
    main_menu_ace->sprite_object->rotation = main_menu_ace->sprite_object->trotation;

    // Seed randomization
    rng_seed++;
    // If the keys have changed, make it more pseudo-random
    if (key_curr_state() != key_prev_state())
    {
        rng_seed *= 2;
    }

    if (key_hit(KEY_LEFT))
    {
        if (selection_x > 0)
        {
            selection_x--;
        }
    }
    else if (key_hit(KEY_RIGHT))
    {
        if (selection_x < MAIN_MENU_IMPLEMENTED_BUTTONS - 1)
        {
            selection_x++;
        }
    }

    if (selection_x == MAIN_MENU_PLAY_BTN_IDX)
    {
        memset16(&pal_bg_mem[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID], BTN_HIGHLIGHT_COLOR, 1);

        if (key_hit(SELECT_CARD))
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            game_start();
        }
    }
    else
    {
        memcpy16(
            &pal_bg_mem[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID],
            &pal_bg_mem[MAIN_MENU_PLAY_BUTTON_MAIN_COLOR_PID],
            1
        );
    }
}

void game_main_menu_cleanup(void)
{
    // Normally I would just cache these and hide/unhide but I didn't feel like dealing with
    // defining a layer for it
    card_destroy(&main_menu_ace->card);
    card_object_destroy(&main_menu_ace);
}
