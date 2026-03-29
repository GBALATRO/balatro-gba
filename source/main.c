#include "affine_background.h"
#include "blind.h"
#include "card.h"
#include "font.h"
#include "game.h"
#include "gbalatro_sys8.h"
#include "graphic_utils.h"
#include "joker.h"
#include "sprite.h"

#include <maxmod.h>
#include <string.h>
#include <tonc.h>

// Graphics
#include "affine_background_gfx.h"
#include "background_gfx.h"

// Audio
#include "soundbank.h"
#include "soundbank_bin.h"

void init()
{
    irq_init(NULL);
    irq_add(II_VBLANK, mmVBlank);
    irq_add(II_HBLANK, affine_background_hblank);

    // Initialize text engine
    tte_init_se(
        0,
        BG_CBB(TTE_CBB) | BG_SBB(TTE_SBB),
        0,
        CLR_WHITE,
        TTE_BIT_UNPACK_OFFSET,
        &gbalatro_sys8Font,

        // Explicitly use 8x8 tile text drawing function to improve performance
        // See https://gbadev.net/tonc/tte.html#ssec-map-reg
        se_drawg_w8h8
    );
    tte_erase_screen();
    tte_init_con();

    // TTE palette setup
    pal_bg_bank[TTE_YELLOW_PB][TTE_BIT_ON_CLR_IDX] = TEXT_CLR_YELLOW;
    pal_bg_bank[TTE_BLUE_PB][TTE_BIT_ON_CLR_IDX] = TEXT_CLR_BLUE;
    pal_bg_bank[TTE_RED_PB][TTE_BIT_ON_CLR_IDX] = TEXT_CLR_RED;
    pal_bg_bank[TTE_WHITE_PB][TTE_BIT_ON_CLR_IDX] = TEXT_CLR_WHITE;

    // Set up the video mode
    // BG0 is the TTE text layer
    REG_BG0CNT = BG_PRIO(0) | BG_CBB(TTE_CBB) | BG_SBB(TTE_SBB) | BG_4BPP;
    // BG1 is the main background layer
    REG_BG1CNT = BG_PRIO(1) | BG_CBB(MAIN_BG_CBB) | BG_SBB(MAIN_BG_SBB) | BG_8BPP;
    // BG2 is the affine background layer
    REG_BG2CNT = BG_PRIO(2) | BG_CBB(AFFINE_BG_CBB) | BG_SBB(AFFINE_BG_SBB) | BG_8BPP | BG_WRAP;

    int win1_left = 72;
    int win1_top = 44;
    int win1_right = 200;
    int win1_bottom = 128;

    int win2_left = 72;
    int win2_top = 0;
    int win2_right = 232;
    int win2_bottom = 44;

    REG_WIN0H = win1_left << 8 | win1_right;
    REG_WIN0V = win1_top << 8 | win1_bottom;
    REG_WIN0CNT = WIN_ALL | WIN_BLD;
    REG_WINOUTCNT = WIN_ALL;

    REG_WIN1H = win2_left << 8 | win2_right;
    REG_WIN1V = win2_top << 8 | win2_bottom;
    REG_WIN1CNT = WIN_ALL | WIN_BLD;

    REG_BLDCNT = BLD_BUILD(BLD_BG1, BLD_BG2, 1);

    REG_BLDALPHA = BLDA_BUILD(0, 13);

    REG_DISPCNT = DCNT_MODE1 | DCNT_OBJ_1D | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_WIN0 |
                  DCNT_WIN1;

    // Initialize subsystems
    mmInitDefault((mm_addr)soundbank_bin, 12);
    affine_background_init();
    sprite_init();
    card_init();
    blind_init();
    joker_init();
    game_init();
    game_change_state(GAME_STATE_SPLASH_SCREEN);
}

void update()
{
    affine_background_update();
    game_update();
}

void draw()
{
    sprite_draw();
}

int main()
{
    init();

    while (true)
    {
        VBlankIntrWait();
        mmFrame();
        key_poll();
        update();
        draw();
    }

    return 0;
}
