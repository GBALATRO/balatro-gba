#include "game/common_ui.h"

#include "game.h"
#include "game/blind_select.h"
#include "game/game_over.h"
#include "game/main_menu.h"
#include "game/rect.h"
#include "game/round.h"
#include "game/round_end.h"
#include "game/shop.h"

#include <tonc.h>

enum BackgroundId background = BG_NONE;

void set_background(enum BackgroundId id)
{
    background = id;
}

void reset_background()
{
    set_background(UNDEFINED);
}

enum BackgroundId get_background(void)
{
    return background;
}

void change_background(enum BackgroundId id)
{
    if (background == id)
    {
        return;
    }
    else if (id == BG_CARD_SELECTING)
    {
        game_selecting_change_background(background);
    }
    else if (id == BG_CARD_PLAYING)
    {
        game_playing_change_background(background);
    }
    else if (id == BG_ROUND_END)
    {
        game_round_end_change_background(background);
    }
    else if (id == BG_SHOP)
    {
        game_shop_change_background();
    }
    else if (id == BG_BLIND_SELECT)
    {
        game_blind_select_change_background();
    }
    else if (id == BG_MAIN_MENU)
    {
        game_main_menu_change_background();
    }
    else
    {
        return; // Invalid background ID
    }

    background = id;
}

// Show/Hide flaming score effect if we will score
// more than the required amount or not

bool score_flames_active = false;

static void check_flaming_score(void)
{
    u32 curr_score = u32_protected_mult(get_chips(), get_mult());
    u32 required_score = blind_get_requirement(get_current_blind(), get_ante());
    if (curr_score >= required_score && !score_flames_active)
    {
        // start flaming score
        score_flames_active = true;
        return;
    }
    if (curr_score < required_score && score_flames_active)
    {
        // stop flaming score and clear rect
        score_flames_active = false;

        Rect reset_rect = SCORE_FLAME_RESET;
        main_bg_se_copy_rect(reset_rect, SCORE_FLAME_CHIPS_POS);
        reset_rect.left += SCORE_FLAME_FRAME_WIDTH;
        reset_rect.right += SCORE_FLAME_FRAME_WIDTH;
        main_bg_se_copy_rect(reset_rect, SCORE_FLAME_MULT_POS);
    }
}

bool are_score_flames_active(void)
{
    return score_flames_active;
}

void display_round(int value)
{
    // tte_erase_rect_wrapper(ROUND_TEXT_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d",
        ROUND_TEXT_RECT.left,
        ROUND_TEXT_RECT.top,
        TTE_YELLOW_PB,
        value
    );
}

void display_money()
{
    Rect money_text_rect = MONEY_TEXT_RECT;
    tte_erase_rect_wrapper(MONEY_TEXT_RECT);

    char money_str_buff[INT_MAX_DIGITS + 2]; // + 2 for null terminator and "$" sign
    snprintf(money_str_buff, sizeof(money_str_buff), "$%d", get_money());

    // Bias left so the number is centered and the "$" sign is on the left
    update_text_rect_to_center_str(&money_text_rect, money_str_buff, SCREEN_LEFT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        money_text_rect.left,
        money_text_rect.top,
        TTE_YELLOW_PB,
        money_str_buff
    );
}

void display_chips(void)
{
    Rect chips_text_rect = CHIPS_TEXT_RECT;

    // In case of overflow, the rect overflow left by 1 char
    Rect chips_text_overflow_rect = chips_text_rect;
    chips_text_overflow_rect.left -= TTE_CHAR_SIZE;
    tte_erase_rect_wrapper(chips_text_overflow_rect);

    char chips_str_buff[UINT_MAX_DIGITS + 1];
    truncate_uint_to_suffixed_str(
        get_chips(),
        rect_width(&chips_text_rect) / TTE_CHAR_SIZE,
        chips_str_buff
    );

    update_text_rect_to_right_align_str(&chips_text_rect, chips_str_buff, OVERFLOW_LEFT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000;}%s",
        chips_text_rect.left,
        chips_text_rect.top,
        TTE_WHITE_PB,
        chips_str_buff
    );
    check_flaming_score();
}

void display_mult(void)
{
    Rect mult_text_overflow_rect = MULT_TEXT_RECT;
    // In case of overflow the rect will overflow right by 1 char
    mult_text_overflow_rect.right += TTE_CHAR_SIZE;
    tte_erase_rect_wrapper(mult_text_overflow_rect);

    char mult_str_buff[UINT_MAX_DIGITS + 1];
    truncate_uint_to_suffixed_str(
        get_mult(),
        rect_width(&MULT_TEXT_RECT) / TTE_CHAR_SIZE,
        mult_str_buff
    );

    tte_printf(
        "#{P:%d,%d; cx:0x%X000;}%s",
        MULT_TEXT_RECT.left,
        MULT_TEXT_RECT.top,
        TTE_WHITE_PB,
        mult_str_buff
    );

    check_flaming_score();
}

void display_ante(int value)
{
    tte_printf(
        "#{P:%d,%d; cx:0xC000}%d#{cx:0xF000}/%d",
        ANTE_TEXT_RECT.left,
        ANTE_TEXT_RECT.top,
        value,
        MAX_ANTE
    );
}

void display_temp_score(u32 value)
{
    char temp_score_str_buff[UINT_MAX_DIGITS + 1];
    Rect temp_score_rect = TEMP_SCORE_RECT;
    truncate_uint_to_suffixed_str(
        value,
        rect_width(&temp_score_rect) / TTE_CHAR_SIZE,
        temp_score_str_buff
    );
    update_text_rect_to_center_str(&temp_score_rect, temp_score_str_buff, SCREEN_RIGHT);

    tte_erase_rect_wrapper(TEMP_SCORE_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        temp_score_rect.left,
        temp_score_rect.top,
        TTE_WHITE_PB,
        temp_score_str_buff
    );
}

void display_score(u32 value)
{
    Rect score_rect = SCORE_RECT;
    // Clear the existing text before redrawing
    tte_erase_rect_wrapper(SCORE_RECT);

    char score_str_buff[UINT_MAX_DIGITS + 1];

    truncate_uint_to_suffixed_str(value, rect_width(&score_rect) / TTE_CHAR_SIZE, score_str_buff);
    update_text_rect_to_center_str(&score_rect, score_str_buff, SCREEN_RIGHT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        score_rect.left,
        score_rect.top,
        TTE_WHITE_PB,
        score_str_buff
    );
}

void display_hands()
{
    // tte_erase_rect_wrapper(HANDS_TEXT_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0xD000}%d",
        HANDS_TEXT_RECT.left,
        HANDS_TEXT_RECT.top,
        get_num_hands_remaining()
    ); // Hand
}

void display_discards()
{
    // tte_erase_rect_wrapper(DISCARDS_TEXT_RECT);
    // Discard
    tte_printf(
        "#{P:%d,%d; cx:0xE000}%d",
        DISCARDS_TEXT_RECT.left,
        DISCARDS_TEXT_RECT.top,
        get_discards()
    );
}

Rect get_text_rect_under_sprite_object(SpriteObject* sprite_object)
{
    int height = 0;
    int width = 0;

    if (sprite_object_get_dimensions(sprite_object, &width, &height) == false)
    {
        // fallback
        height = CARD_SPRITE_SIZE;
        width = CARD_SPRITE_SIZE;
    }

    Rect ret_rect = {0};

    ret_rect.left = fx2int(sprite_object->tx);
    ret_rect.top = fx2int(sprite_object->ty) + height + TILE_SIZE;
    ret_rect.right = ret_rect.left + width;
    ret_rect.bottom = ret_rect.top + TTE_CHAR_SIZE;

    return ret_rect;
}

// Resets bottom row bg tiles of the top left panel (shop/blind) after
// it is dismissed to match the rest of the game panel background.
void reset_top_left_panel_bottom_row()
{
    BG_POINT top_left_panel_bottom_row_pos = TOP_LEFT_PANEL_POINT;
    // Use the source rect height to offset to the bottom row point
    top_left_panel_bottom_row_pos.y += rect_height(&TOP_LEFT_ITEM_SRC_RECT) - 1;
    main_bg_se_copy_rect(TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT, top_left_panel_bottom_row_pos);
}