/**
 * @file overlay.c
 *
 * @brief Dimmed full-screen text layer implementation.
 */
#include "game/overlay.h"

#include "graphic_utils.h"
#include "sprite.h"

#include <tonc.h>

// Brightness removed from the game layers while the overlay is open, out of 16
#define OVERLAY_FADE_LEVEL 12

// WHY: sprites are drawn over the text layer (priority 0) by default and would hide the menu.
// Priority 1 puts them under the text while keeping them over the main background.
#define OVERLAY_SPRITES_PRIORITY 1
#define DEFAULT_SPRITES_PRIORITY 0

// The text layer uses a full screenblock
#define TEXT_SE_COUNT (SE_ROW_LEN * SE_ROW_LEN)

static bool s_is_open = false;

// Display state replaced while the overlay is open
static u16 s_saved_dispcnt = 0;
static u16 s_saved_bldcnt = 0;
static EWRAM_BSS u16 s_saved_text_se[TEXT_SE_COUNT];

void overlay_open(void)
{
    if (s_is_open)
        return;

    s_is_open = true;

    // Save display state
    s_saved_dispcnt = REG_DISPCNT;
    s_saved_bldcnt = REG_BLDCNT;
    memcpy16(s_saved_text_se, se_mem[TTE_SBB], TEXT_SE_COUNT);

    // Dim game layers
    // WHY: windows limit blending to their own area, and the fade must cover the whole screen
    REG_DISPCNT &= ~(DCNT_WIN0 | DCNT_WIN1);
    REG_BLDCNT = BLD_BG1 | BLD_BG2 | BLD_OBJ | BLD_BACKDROP | BLD_BLACK;
    REG_BLDY = BLDY_BUILD(OVERLAY_FADE_LEVEL);

    sprite_set_all_priority(OVERLAY_SPRITES_PRIORITY);
    tte_erase_screen();
}

void overlay_close(void)
{
    if (!s_is_open)
        return;

    s_is_open = false;

    // Restore display state
    memcpy16(se_mem[TTE_SBB], s_saved_text_se, TEXT_SE_COUNT);
    sprite_set_all_priority(DEFAULT_SPRITES_PRIORITY);
    REG_BLDCNT = s_saved_bldcnt;
    REG_DISPCNT = s_saved_dispcnt;
}

bool overlay_are_keys_released(void)
{
    return key_curr_state() == 0;
}
