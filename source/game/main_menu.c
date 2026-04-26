#include "game/main_menu.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "background_main_menu_gfx.h"
#include "button.h"
#include "card.h"
#include "game.h"
#include "game/common_ui.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "soundbank.h"
#include "sprite.h"

#include <stdint.h>
#include <tonc.h>
#include <tonc_math.h>
#include <tonc_memdef.h>

static const u32 PLAY_BUTTON_OUTLINE_PID = 2;
static const u32 PLAY_BUTTON_MAIN_COLOR_PID = 5;

static const u32 IMPLEMENTED_BUTTONS = 1; // Remove this once all buttons are implemented
static const u32 PLAY_BTN_IDX = 0;

// Pixel sizes
#define MAIN_MENU_ACE_T_X 88
#define MAIN_MENU_ACE_T_Y 26

// Main menu sprite - the ace of spades
static CardObject* main_menu_ace = NULL;

// Current selected button index
static int selection_x = 0;

void game_main_menu_change_background(void)
{
    toggle_windows(false, false);

    tte_erase_screen();
    GRIT_CPY(pal_bg_mem, background_main_menu_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_main_menu_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_main_menu_gfxMap);

    // Disable the button highlight colors
    memcpy16(&pal_bg_mem[PLAY_BUTTON_OUTLINE_PID], &pal_bg_mem[PLAY_BUTTON_MAIN_COLOR_PID], 1);
}

void game_main_menu_on_init()
{
    affine_background_change_background(AFFINE_BG_MAIN_MENU);
    change_background(BG_MAIN_MENU);
    main_menu_ace = card_object_new(card_new(SPADES, ACE));
    card_object_set_sprite(main_menu_ace, 0);
    main_menu_ace->sprite_object->sprite->obj->attr0 |= ATTR0_AFF_DBL;
    main_menu_ace->sprite_object->tx = int2fx(MAIN_MENU_ACE_T_X);
    main_menu_ace->sprite_object->x = main_menu_ace->sprite_object->tx;
    main_menu_ace->sprite_object->ty = int2fx(MAIN_MENU_ACE_T_Y);
    main_menu_ace->sprite_object->y = main_menu_ace->sprite_object->ty;
    main_menu_ace->sprite_object->tscale = float2fx(0.8f);
    selection_x = 0;
}

void game_main_menu_on_update()
{
    card_object_update(main_menu_ace);
    main_menu_ace->sprite_object->trotation = lu_sin((g_game_vars.timer << 8) / 2) / 3;
    main_menu_ace->sprite_object->rotation = main_menu_ace->sprite_object->trotation;

    // Seed randomization
    g_game_vars.rng_seed++;
    // If the keys have changed, make it more pseudo-random
    if (key_curr_state() != key_prev_state())
    {
        g_game_vars.rng_seed *= 2;
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
        if (selection_x < IMPLEMENTED_BUTTONS - 1)
        {
            selection_x++;
        }
    }

    if (selection_x == PLAY_BTN_IDX)
    {
        memset16(&pal_bg_mem[PLAY_BUTTON_OUTLINE_PID], BTN_HIGHLIGHT_COLOR, 1);

        if (key_hit(SELECT_CARD))
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            game_change_state(GAME_STATE_GAME_START);
        }
    }
    else
    {
        memcpy16(&pal_bg_mem[PLAY_BUTTON_OUTLINE_PID], &pal_bg_mem[PLAY_BUTTON_MAIN_COLOR_PID], 1);
    }
}

void game_main_menu_on_exit()
{
    // Normally I would just cache these and hide/unhide but I didn't feel like dealing with
    // defining a layer for it
    card_destroy(&main_menu_ace->card);
    card_object_destroy(&main_menu_ace);
}
