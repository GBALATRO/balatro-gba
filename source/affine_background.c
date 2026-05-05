#include "affine_background.h"

#include "affine_background_gfx.h"
#include "affine_main_menu_background_gfx.h"
#include "graphic_utils.h"

#define ANIMATION_SPEED_DIVISOR 16

// Prepare screen during VBLANK
// Pre-computes the affine matrices values for each scanline and stores in bgaff_arr. This is to be
// done in VBLANK so the HBLANK code can just fetch the values quickly.
static IWRAM_CODE void s_affine_background_prep_bgaff_arr();

static BG_AFFINE _bgaff_arr[SCREEN_HEIGHT + 1];
static AFF_SRC_EX _asx = {0};
static enum AffineBackgroundID _background = AFFINE_BG_NONE;
static uint _timer = 0;

void affine_background_init()
{
    affine_background_update();

    REG_BG_AFFINE[AFFINE_BG_IDX] = bg_aff_default;
}

IWRAM_CODE void affine_background_hblank()
{
    vu16 vcount = REG_VCOUNT;

    if ((vcount >= SCREEN_HEIGHT)) // Exit the function if the scanline is outside the screen
    {
        return;
    }

    // See comment in affine_background_prep_bgaff_arr()
    REG_BG_AFFINE[AFFINE_BG_IDX] = _bgaff_arr[vcount + 1];
}

void affine_background_update()
{
    if (REG_IE & IRQ_HBLANK) // High quality mode with HBLANK interrupt
    {
        s_affine_background_prep_bgaff_arr();
    }
    else // Low quality mode without HBLANK interrupt
    {
        _asx.scr_x = 0;
        _asx.scr_y = 0;
        _asx.tex_x += 5;
        _asx.tex_y += 12;
        // Scale the sine value to fit in a s16
        _asx.sx = ((lu_sin(_timer * 100)) >> 8) + 256;
        // Scale the sine value to fit in a s16
        _asx.sy = ((lu_sin(_timer * 100 + 0x4000)) >> 8) + 256;
        _asx.alpha = 0;

        bg_rotscale_ex(&_bgaff_arr[0], &_asx);
        REG_BG_AFFINE[AFFINE_BG_IDX] = _bgaff_arr[0];
    }

    _timer++;
}

void affine_background_set_color(COLOR color)
{
    // Reload the palette to reset any previous color scaling
    affine_background_change_background(_background);
    for (int i = 0; i < AFFINE_BG_PAL_LEN; i++)
    {
        clr_rgbscale(&pal_bg_mem[AFFINE_BG_PB] + i, &pal_bg_mem[AFFINE_BG_PB] + i, 1, color);
    }
}

void affine_background_load_palette(const u16* src)
{
    memcpy16(&pal_bg_mem[AFFINE_BG_PB], src, AFFINE_BG_PAL_LEN);
}

void affine_background_change_background(enum AffineBackgroundID new_bg)
{
    if (_background == new_bg)
    {
        return;
    }

    _background = new_bg;

    switch (_background)
    {
        case AFFINE_BG_MAIN_MENU:
            REG_BG2CNT &= ~BG_AFF_32x32;
            REG_BG2CNT |= BG_AFF_16x16;
            REG_IE |= IRQ_HBLANK; // Enable HBLANK

            memcpy32_tile8_with_palette_offset(
                (u32*)&tile8_mem[AFFINE_BG_CBB],
                (const u32*)affine_main_menu_background_gfxTiles,
                affine_main_menu_background_gfxTilesLen / 4,
                AFFINE_BG_PB
            );
            GRIT_CPY(&se_mem[AFFINE_BG_SBB], affine_main_menu_background_gfxMap);
            affine_background_load_palette(affine_main_menu_background_gfxPal);
            break;
        case AFFINE_BG_GAME:
            REG_BG2CNT &= ~BG_AFF_16x16;
            REG_BG2CNT |= BG_AFF_32x32;
            REG_IE &= ~IRQ_HBLANK; // Disable HBLANK

            memcpy32_tile8_with_palette_offset(
                (u32*)&tile8_mem[AFFINE_BG_CBB],
                (const u32*)affine_background_gfxTiles,
                affine_background_gfxTilesLen / 4,
                AFFINE_BG_PB
            );
            GRIT_CPY(&se_mem[AFFINE_BG_SBB], affine_background_gfxMap);
            affine_background_load_palette(affine_background_gfxPal);
            break;
        default:
            break;
    }
}

static IWRAM_CODE void s_affine_background_prep_bgaff_arr()
{
    for (u16 vcount = 0; vcount < SCREEN_HEIGHT; vcount++)
    {
        const s32 timer_s32 = _timer << 8;
        const s32 vcount_s32 = vcount << 8;
        const s16 vcount_s16 = vcount;
        const s32 vcount_sine = lu_sin(vcount_s32 + timer_s32 / ANIMATION_SPEED_DIVISOR);

        _asx.scr_x = (SCREEN_WIDTH / 2);
        // scr_y must equal vcount otherwise the background will have no vertical difference
        _asx.scr_y = vcount_s16 - (SCREEN_HEIGHT / 2);
        _asx.tex_x = (1000 * 1000) + (vcount_sine);
        _asx.tex_y = (1000 * 1000);
        _asx.sx = 128;
        _asx.sy = 128;
        _asx.alpha = vcount_sine + (timer_s32 / ANIMATION_SPEED_DIVISOR);

        bg_rotscale_ex(&_bgaff_arr[vcount], &_asx);
    }

    /* HBLANK occurs after the scanline so REG_VCOUNT represents the
     * the scanline that just passed, so when it's SCREEN_HEIGHT we will
     * actually be updating the first line
     */
    _bgaff_arr[SCREEN_HEIGHT] = _bgaff_arr[0];
}
