/**
 * @file joker_desc.c
 *
 * @brief Shared Joker description overlay implementation.
 */

#include "game/joker_desc.h"

#include "background_gfx.h"
#include "background_shop_gfx.h"
#include "util.h"

#include <string.h>

// Shop 9-patch lives in the offscreen bank of background_shop_gfx
#define SHOP_DESC_9_PTCH_X 27
#define SHOP_DESC_9_PTCH_Y 25
#define DESC_9_PTCH_W      5
#define DESC_9_PTCH_H      7

// Play background uses tiles 0-204; free charblock tail starts at 205
#define ROUND_DESC_TILE_BASE 205

// Stage the remapped 5x7 source in the play atlas offscreen bank (same idea as shop)
#define ROUND_DESC_SRC_SE_X 0
#define ROUND_DESC_SRC_SE_Y 25

#define TILE8_BYTES           64
#define PANEL_UNDERLAY_MAX_SE (20 * 32)
#define STAGE_SE_COUNT        (DESC_9_PTCH_W * DESC_9_PTCH_H)

static JokerObject* s_active_joker = NULL;

static bool s_underlay_saved = false;
static EWRAM_BSS u16 s_panel_underlay_se[PANEL_UNDERLAY_MAX_SE];
static Rect s_panel_underlay_rect;

static EWRAM_BSS u16 s_stage_se_backup[STAGE_SE_COUNT];
static bool s_stage_se_saved = false;

static const NinePatchRect s_round_desc_9_ptch_src = {
    .patch_rect =
        {ROUND_DESC_SRC_SE_X,
         ROUND_DESC_SRC_SE_Y,
         ROUND_DESC_SRC_SE_X + DESC_9_PTCH_W - 1,
         ROUND_DESC_SRC_SE_Y + DESC_9_PTCH_H - 1},
    .margins = {2, 3, 2, 3},
};

JokerObject* joker_desc_get_active(void)
{
    return s_active_joker;
}

void joker_desc_set_active(JokerObject* joker_object)
{
    s_active_joker = joker_object;
}

static u8 shop_pal_index_to_play(u8 shop_idx)
{
    switch (shop_idx)
    {
        // Keep 0 transparent: remapping it to opaque buries the outer white rim
        // inside dark chrome and reads as a clipped vertical white strip.
        case 13:
            return 15; // white
        case 19:
            return 10; // dark gray fill
        case 25:
            return 20; // light gray
        // Shop uses 27/28 as rarity-stripe placeholders; play 27/28 are flame colors.
        case 27:
            return JOKER_DESC_RARITY_MAIN_COLOR_PAL_IDX;
        case 28:
            return JOKER_DESC_RARITY_SHADOW_COLOR_PAL_IDX;
        default:
            return shop_idx;
    }
}

static inline u16 patch_tid(int dx, int dy)
{
    return (u16)(ROUND_DESC_TILE_BASE + dy * DESC_9_PTCH_W + dx);
}

/** Shop atlas reuses left-edge tiles on the right with SE_HFLIP — preserve those flags. */
static inline u16 patch_se(int dx, int dy)
{
    int shop_map_idx = (SHOP_DESC_9_PTCH_Y + dy) * SE_ROW_LEN + (SHOP_DESC_9_PTCH_X + dx);
    u16 shop_se = background_shop_gfxMap[shop_map_idx];
    return (u16)(patch_tid(dx, dy) | (shop_se & (SE_HFLIP | SE_VFLIP)));
}

void joker_desc_install_shop_9patch_tiles(void)
{
    for (int dy = 0; dy < DESC_9_PTCH_H; dy++)
    {
        for (int dx = 0; dx < DESC_9_PTCH_W; dx++)
        {
            int shop_map_idx =
                (SHOP_DESC_9_PTCH_Y + dy) * SE_ROW_LEN + (SHOP_DESC_9_PTCH_X + dx);
            int shop_tid = background_shop_gfxMap[shop_map_idx] & 0x03FF;
            int dest_tid = ROUND_DESC_TILE_BASE + dy * DESC_9_PTCH_W + dx;
            const u8* src = (const u8*)background_shop_gfxTiles + shop_tid * TILE8_BYTES;

            u16 remapped[TILE8_BYTES / 2];
            for (int i = 0; i < TILE8_BYTES; i += 2)
            {
                u8 lo = shop_pal_index_to_play(src[i]);
                u8 hi = shop_pal_index_to_play(src[i + 1]);
                remapped[i / 2] = (u16)(lo | (hi << 8));
            }
            memcpy16(&tile8_mem[MAIN_BG_CBB][dest_tid], remapped, TILE8_BYTES / 2);
        }
    }
}

static void stage_round_9patch_se(void)
{
    int i = 0;
    for (int dy = 0; dy < DESC_9_PTCH_H; dy++)
    {
        for (int dx = 0; dx < DESC_9_PTCH_W; dx++)
        {
            s_stage_se_backup[i++] =
                se_mat[MAIN_BG_SBB][ROUND_DESC_SRC_SE_Y + dy][ROUND_DESC_SRC_SE_X + dx];
            se_mat[MAIN_BG_SBB][ROUND_DESC_SRC_SE_Y + dy][ROUND_DESC_SRC_SE_X + dx] =
                patch_se(dx, dy);
        }
    }
    s_stage_se_saved = true;
}

static void restore_round_9patch_se(void)
{
    if (!s_stage_se_saved)
        return;

    int i = 0;
    for (int dy = 0; dy < DESC_9_PTCH_H; dy++)
    {
        for (int dx = 0; dx < DESC_9_PTCH_W; dx++)
            se_mat[MAIN_BG_SBB][ROUND_DESC_SRC_SE_Y + dy][ROUND_DESC_SRC_SE_X + dx] =
                s_stage_se_backup[i++];
    }
    s_stage_se_saved = false;
}

const NinePatchRect* joker_desc_install_shop_9patch(void)
{
    joker_desc_install_shop_9patch_tiles();
    stage_round_9patch_se();
    return &s_round_desc_9_ptch_src;
}

void joker_desc_release_shop_9patch_se(void)
{
    restore_round_9patch_se();
}

void joker_desc_save_underlay(Rect se_rect)
{
    s_panel_underlay_rect = se_rect;
    int i = 0;

    for (int y = s_panel_underlay_rect.top; y <= s_panel_underlay_rect.bottom; y++)
    {
        for (int x = s_panel_underlay_rect.left; x <= s_panel_underlay_rect.right; x++)
        {
            if (i < PANEL_UNDERLAY_MAX_SE)
                s_panel_underlay_se[i++] = se_mat[MAIN_BG_SBB][y][x];
        }
    }

    s_underlay_saved = true;
}

void joker_desc_restore_underlay(void)
{
    if (!s_underlay_saved)
        return;

    int i = 0;
    for (int y = s_panel_underlay_rect.top; y <= s_panel_underlay_rect.bottom; y++)
    {
        for (int x = s_panel_underlay_rect.left; x <= s_panel_underlay_rect.right; x++)
        {
            if (i < PANEL_UNDERLAY_MAX_SE)
                se_mat[MAIN_BG_SBB][y][x] = s_panel_underlay_se[i++];
        }
    }

    s_underlay_saved = false;
}

static inline bool rect_contains_xy(Rect r, int x, int y)
{
    return x >= r.left && x <= r.right && y >= r.top && y <= r.bottom;
}

void joker_desc_restore_underlay_except(Rect exclude)
{
    if (!s_underlay_saved)
        return;

    int i = 0;
    for (int y = s_panel_underlay_rect.top; y <= s_panel_underlay_rect.bottom; y++)
    {
        for (int x = s_panel_underlay_rect.left; x <= s_panel_underlay_rect.right; x++)
        {
            if (i >= PANEL_UNDERLAY_MAX_SE)
                return;
            u16 se = s_panel_underlay_se[i++];
            if (!rect_contains_xy(exclude, x, y))
                se_mat[MAIN_BG_SBB][y][x] = se;
        }
    }
}

void joker_desc_restore_underlay_rect(Rect only)
{
    if (!s_underlay_saved)
        return;

    int i = 0;
    for (int y = s_panel_underlay_rect.top; y <= s_panel_underlay_rect.bottom; y++)
    {
        for (int x = s_panel_underlay_rect.left; x <= s_panel_underlay_rect.right; x++)
        {
            if (i >= PANEL_UNDERLAY_MAX_SE)
                return;
            u16 se = s_panel_underlay_se[i++];
            if (rect_contains_xy(only, x, y))
                se_mat[MAIN_BG_SBB][y][x] = se;
        }
    }
}

void joker_desc_discard_underlay(void)
{
    s_underlay_saved = false;
}

void joker_desc_clear_se_rect(Rect se_rect)
{
    for (int y = se_rect.top; y <= se_rect.bottom; y++)
    {
        if (y < 0 || y >= 32)
            continue;
        int left = se_rect.left < 0 ? 0 : se_rect.left;
        int right = se_rect.right > 31 ? 31 : se_rect.right;
        if (left > right)
            continue;
        memset16(&se_mat[MAIN_BG_SBB][y][left], 0, right - left + 1);
    }
}

void joker_desc_draw_panel(
    Joker* joker,
    const NinePatchRect* src_9_ptch,
    u8 rarity_main_pal_idx,
    u8 rarity_shadow_pal_idx
)
{
    GBAL_RETURN_IF_NULL_VOID(joker);
    GBAL_RETURN_IF_NULL_VOID(src_9_ptch);

    const JokerInfo* info = get_joker_registry_entry(joker->id);
    GBAL_RETURN_IF_NULL_VOID(info);
    GBAL_RETURN_IF_NULL_VOID(info->joker_print_desc);

    const char* rarity_str = joker_get_rarity_string(info->rarity);
    if (rarity_str == NULL)
        rarity_str = "";

    int max_text_height = rect_height(&JOKER_DESC_9_PTCH_TO_RECT) - src_9_ptch->margins.top -
                          src_9_ptch->margins.bottom;
    int lines_used = info->joker_print_desc(joker, JOKER_DESC_TEXT_RECT);
    int desc_bottom_offset = max_text_height - lines_used;
    if (desc_bottom_offset < 0)
        desc_bottom_offset = 0;

    tte_printf(
        TTE_WHITE_TAG "#{P:%d,%d}%*s%s",
        JOKER_DESC_TEXT_RECT.left * TILE_SIZE,
        (JOKER_DESC_TEXT_RECT.bottom - desc_bottom_offset - 1) * TILE_SIZE,
        (rect_width(&JOKER_DESC_TEXT_RECT) - (int)strlen(rarity_str)) / 2,
        "",
        rarity_str
    );
    pal_bg_mem[rarity_main_pal_idx] = joker_get_rarity_color(info->rarity, true);
    pal_bg_mem[rarity_shadow_pal_idx] = joker_get_rarity_color(info->rarity, false);

    Rect actual_dest_rect = JOKER_DESC_9_PTCH_TO_RECT;
    actual_dest_rect.bottom -= desc_bottom_offset;
    main_bg_se_copy_expand_9_patch(actual_dest_rect, src_9_ptch);

    tte_printf(
        TTE_WHITE_TAG "#{P:%d,%d}%*s%s",
        JOKER_DESC_NAME_TEXT_RECT.left * TILE_SIZE,
        JOKER_DESC_NAME_TEXT_RECT.top * TILE_SIZE,
        (rect_width(&JOKER_DESC_NAME_TEXT_RECT) - (int)strlen(info->name)) / 2,
        "",
        info->name
    );
}

void joker_desc_draw_round_panel(Joker* joker, u8 rarity_main_pal_idx, u8 rarity_shadow_pal_idx)
{
    // Same dest/text rects and 9-patch expand as the shop (via staged remapped tiles).
    const NinePatchRect* src = joker_desc_install_shop_9patch();
    joker_desc_draw_panel(joker, src, rarity_main_pal_idx, rarity_shadow_pal_idx);
    joker_desc_release_shop_9patch_se();
}

/** Heal score-flame colors if an older desc path overwrote pal slots 27/28. */
void joker_desc_restore_flame_palette(void)
{
    // Flame anim uses play-pal indices 24-31; 27/28 were previously reused for rarity.
    for (int i = 24; i <= 31; i++)
        pal_bg_mem[i] = background_gfxPal[i];
}

void joker_desc_clear_panel(void)
{
    joker_desc_clear_se_rect(JOKER_DESC_9_PTCH_TO_RECT);
}
