#include "splash_screen.h"

#include "font.h"
#include "game.h"
#include "graphic_utils.h"
#include "maxmod.h"
#include "soundbank.h"

#include <tonc.h>

static const Rect COUNTDOWN_TIMER_RECT = {208, 144, 240, 152};
static uint timer = 0;

void splash_screen_on_init()
{
    timer = 0;

    tte_printf("#{P:72,8; cx:0xF000}DISCLAIMER");
    tte_printf(
        "#{P:8,24; cx:0xF000}This project is NOT endorsed \n by or affiliated with \n Playstack or "
        "LocalThunk.\n\n If you have paid for this, \n you have been scammed \n and should request "
        "a refund \n IMMEDIATELY. \n\n The only official place \n to obtain this is from: \n\n "
        "'github.com/\n  GBALATRO/balatro-gba'"
    );
    tte_printf("#{P:8,144; cx:0xF000}(Press any key to skip)");
}

void splash_screen_on_update()
{
    timer++;

    if (timer < SPLASH_DURATION_FRAMES)
    {
        tte_erase_rect_wrapper(COUNTDOWN_TIMER_RECT);
        tte_printf(
            "#{P:%d,%d; cx:0xF000}%d",
            COUNTDOWN_TIMER_RECT.left,
            COUNTDOWN_TIMER_RECT.top,
            1 + (SPLASH_DURATION_FRAMES - timer) / SPLASH_FPS
        );

        if (!key_hit(KEY_ANY))
        {
            return;
        }
    }

    game_change_state(GAME_STATE_MAIN_MENU);
    tte_erase_screen();
}

void splash_screen_on_exit()
{
    mmStart(MOD_MAIN_THEME, MM_PLAY_LOOP);
}
