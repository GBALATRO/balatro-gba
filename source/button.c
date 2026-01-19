#include "button.h"

#include "audio_utils.h"
#include "soundbank.h"

#include <tonc.h>

void button_set_highlight(Button* button, bool highlight)
{
    if (button == NULL)
        return;

    u16 set_color = highlight ? BTN_HIGHLIGHT_COLOR : pal_bg_mem[button->button_pal_idx];

    memset16(&pal_bg_mem[button->border_pal_idx], set_color, 1);
}

void button_press(Button* button, void* ctx)
{
    if (button == NULL || button->on_pressed == NULL ||
        (button->can_be_pressed != NULL && !button->can_be_pressed(ctx)))
    {
        return;
    }

    play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);

    button->on_pressed(ctx);
}
