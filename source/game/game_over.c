#include "game/game_over.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "background_game_over_gfx.h"
#include "button.h"
#include "card.h"
#include "game.h"
#include "graphic_utils.h"
#include "hand.h"
#include "joker.h"
#include "layout.h"
#include "list.h"
#include "random.h"
#include "selection_grid.h"
#include "skip_tag.h"
#include "soundbank.h"
#include "timer.h"
#include "util.h"

#include <tonc.h>

#define RED_BTN_MAIN_COLOR_PAL_IDX     6
#define BLUE_BTN_MAIN_COLOR_PAL_IDX    7
#define REUSE_SEED_BTN_OUTLINE_PAL_IDX 8
#define NEW_RUN_BTN_OUTLINE_PAL_IDX    9
#define MAIN_MENU_BTN_OUTLINE_PAL_IDX  10
#define ENDLESS_BTN_OUTLINE_PAL_IDX    11

enum EndCondition
{
    END_CONDITION_NONE,
    END_CONDITION_WIN,
    END_CONDITION_LOSS
};

// clang-format off
static const Rect     GAME_OVER_ANIM_RECT             = {  3,   5,  26,  31};

static const Rect     GAME_OVER_TEXT_TILES_SRC_RECT_1 = { 27,  24,  31,  25};
static const Rect     GAME_OVER_TEXT_TILES_SRC_RECT_2 = { 27,  26,  31,  27};
static const BG_POINT GAME_OVER_TEXT_TILES_DEST_POS_1 = { 10,  21};
static const BG_POINT GAME_OVER_TEXT_TILES_DEST_POS_2 = { 15,  21};

static const Rect     DEFEATED_BY_TEXT_SRC_RECT       = { 28,  30,  31,  31};
static const BG_POINT DEFEATED_BY_TEXT_DEST_POS       = { 22,  24};
static const BG_POINT DEFEATED_BY_TOKEN_INIT_POS      = {180, 207};

static const Rect     REUSE_SEED_BTN_OFF_SRC_RECT     = { 28,  28,  29,  29};
static const Rect     REUSE_SEED_BTN_ON_SRC_RECT      = { 30,  28,  31,  29};
static const BG_POINT REUSE_SEED_BTN_DEST_POS         = {  4,  11};

static const Rect     BEST_HAND_SCORE_RECT            = {126,  64, 166,  72};
static const Rect     MOST_PLAYED_HAND_RECT           = {112,  80, 168,  88};
static const BG_POINT SEED_VALUE_POS                  = {112,  96};

static const BG_POINT NEW_RUN_BTN_TEXT_POS            = { 48, 112};
static const BG_POINT MAIN_MENU_BTN_TEXT_POS          = {120, 112};
// clang-format on

static const u32 GAME_OVER_ANIM_FRAMES = 17;
static const mm_byte GAME_OVER_SFX_VOL = 178;     // 70% of MM_SFX_FULL_VOLUME
static const mm_word GAME_OVER_WHOOSH_RATE = 922; // 90% of MM_BASE_PITCH_RATE

// Selection Grid

enum GameOverRows
{
    GAME_OVER_SEED_ROW,
    GAME_OVER_RUN_MENU_ROW,
    GAME_OVER_ROW_MAX
};

#define GAME_OVER_SEED_BTN_COL      0
#define GAME_OVER_NEW_RUN_BTN_COL   0
#define GAME_OVER_MAIN_MENU_BTN_COL 1

static int game_over_get_row_size1(void)
{
    return 1;
}
static int game_over_get_row_size2(void)
{
    return 2;
}

static void game_over_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool game_over_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

static const SelectionGridRow game_over_selection_rows[] = {
    {GAME_OVER_SEED_ROW,     game_over_get_row_size1, game_over_on_selection_changed, game_over_on_key_transit, {.wrap = false}},
    {GAME_OVER_RUN_MENU_ROW, game_over_get_row_size2, game_over_on_selection_changed, game_over_on_key_transit, {.wrap = false}}
};

static const Selection GAME_OVER_INIT_SEL = {0, GAME_OVER_RUN_MENU_ROW};
static SelectionGrid game_over_selection_grid = {
    game_over_selection_rows,
    GAME_OVER_ROW_MAX,
    GAME_OVER_INIT_SEL
};

// Buttons

static void reuse_seed_on_pressed(void);
static void new_run_on_pressed(void);
static void main_menu_on_pressed(void);
static Button* game_over_get_button_from_sel(const Selection* selection);

// clang-format off
static Button game_over_buttons[GAME_OVER_ROW_MAX][2] = {
    // GAME_OVER_SEED_ROW
    {
        { REUSE_SEED_BTN_OUTLINE_PAL_IDX, RED_BTN_MAIN_COLOR_PAL_IDX, reuse_seed_on_pressed, NULL }
    },
    // GAME_OVER_RUN_MENU_ROW
    {
        { NEW_RUN_BTN_OUTLINE_PAL_IDX,    RED_BTN_MAIN_COLOR_PAL_IDX, new_run_on_pressed,    NULL },
        { MAIN_MENU_BTN_OUTLINE_PAL_IDX,  RED_BTN_MAIN_COLOR_PAL_IDX, main_menu_on_pressed,  NULL }
    }
};
// clang-format on

// Internal Variables

static enum EndCondition s_end_condition = END_CONDITION_NONE;
static u32 s_timer = TM_ZERO;
static bool s_reuse_seed = false;
static bool s_continue_run = false;

static void game_over_common_init(enum EndCondition init_condition)
{
    s_end_condition = init_condition;
    s_timer = TM_ZERO;
    s_reuse_seed = false;
    s_continue_run = false;

    // Hide all Joker sprites
    JokerObject* joker_object = NULL;
    ListItr itr = list_itr_create(get_jokers_list());
    while ((joker_object = list_itr_next(&itr)))
    {
        obj_hide(joker_object->sprite->obj);
    }

    // Destroy any preexisting cards in the deck, which will be present if we restart a run
    Card* card = NULL;
    while (get_deck_top() >= 0)
    {
        card = deck_pop();
        card_destroy(&card);
    }

    // Destro all owned Skip Tags
    while (list_get_len(get_owned_skip_tags()))
    {
        remove_skip_tag(0);
    }

    // Clears the round end menu
    toggle_windows(false, false);
    GRIT_CPY(pal_bg_mem, background_game_over_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_game_over_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_game_over_gfxMap);

    // Move blind token offscreen at initial position before moving it up
    sprite_position(
        g_game_vars.playing_blind_token,
        DEFEATED_BY_TOKEN_INIT_POS.x,
        DEFEATED_BY_TOKEN_INIT_POS.y
    );

    // Highlight New Run button
    game_over_selection_grid.selection = GAME_OVER_INIT_SEL;
    button_set_highlight(&game_over_buttons[GAME_OVER_SEED_ROW][GAME_OVER_SEED_BTN_COL], false);
    button_set_highlight(
        &game_over_buttons[GAME_OVER_RUN_MENU_ROW][GAME_OVER_NEW_RUN_BTN_COL],
        true
    );
    button_set_highlight(
        &game_over_buttons[GAME_OVER_RUN_MENU_ROW][GAME_OVER_MAIN_MENU_BTN_COL],
        false
    );
}

void game_win_on_init(void)
{
    play_sfx(SFX_GAME_WIN, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);

    game_over_common_init(END_CONDITION_WIN);
    affine_background_set_color(TEXT_CLR_BLUE);
}

void game_lose_on_init(void)
{
    play_sfx(SFX_NEGATIVE_LOSE, MM_BASE_PITCH_RATE / 2, GAME_OVER_SFX_VOL);
    play_sfx(SFX_WHOOSH2_LOSE, GAME_OVER_WHOOSH_RATE, GAME_OVER_SFX_VOL);

    play_lose_music();

    game_over_common_init(END_CONDITION_LOSS);
    affine_background_set_color(TEXT_CLR_RED);

    // Change top text to "GAME OVER"
    main_bg_se_copy_rect(GAME_OVER_TEXT_TILES_SRC_RECT_1, GAME_OVER_TEXT_TILES_DEST_POS_1);
    main_bg_se_copy_rect(GAME_OVER_TEXT_TILES_SRC_RECT_2, GAME_OVER_TEXT_TILES_DEST_POS_2);

    // Change blind frame text
    main_bg_se_copy_rect(DEFEATED_BY_TEXT_SRC_RECT, DEFEATED_BY_TEXT_DEST_POS);
}

void game_over_on_update(void)
{
    s_timer++;

    // Need to clear text here or the number of cards remaining in deck stays for some reason
    if (s_timer == 1)
        tte_erase_screen();

    // Move whole panel and Blind Token sprite up by a tile each frame
    if (s_timer < GAME_OVER_ANIM_FRAMES)
    {
        main_bg_se_move_rect_1_tile_vert(GAME_OVER_ANIM_RECT, SCREEN_UP);

        sprite_position(
            g_game_vars.playing_blind_token,
            g_game_vars.playing_blind_token->pos.x,
            g_game_vars.playing_blind_token->pos.y - TILE_SIZE
        );
    }

    // Print values and buttons text
    else if (s_timer == GAME_OVER_ANIM_FRAMES)
    {
        char best_hand_str[UINT_MAX_DIGITS + 1];
        truncate_uint_to_suffixed_str(
            g_game_vars.best_hand_score,
            rect_width(&BEST_HAND_SCORE_RECT) / TTE_CHAR_SIZE,
            best_hand_str
        );
        Rect best_hand_rect = BEST_HAND_SCORE_RECT;
        update_text_rect_to_center_str(&best_hand_rect, best_hand_str, SCREEN_LEFT);
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%s",
            best_hand_rect.left,
            best_hand_rect.top,
            TTE_RED_PB,
            best_hand_str
        );

        // Most played hand
        enum HandType most_played_hand = HIGH_CARD;
        for (enum HandType hand_type = PAIR; hand_type <= HAND_TYPE_MAX; hand_type++)
        {
            if (g_game_vars.nb_played_hands[hand_type - 1] >
                g_game_vars.nb_played_hands[most_played_hand - 1])
                most_played_hand = hand_type;
        }

        const char* hand_name_str = get_hand_type_name(most_played_hand);
        Rect hand_type_rect = MOST_PLAYED_HAND_RECT;
        update_text_rect_to_center_str(&hand_type_rect, hand_name_str, SCREEN_LEFT);
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%s",
            hand_type_rect.left,
            hand_type_rect.top,
            TTE_WHITE_PB,
            hand_name_str
        );

        // Run's seed
        char seed_str[BASE36_MAX_DIGITS + 1] = {'\0'};
        u32_to_base36(g_game_vars.rng_info.seed, seed_str);
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%s",
            SEED_VALUE_POS.x,
            SEED_VALUE_POS.y,
            TTE_WHITE_PB,
            seed_str
        );

        // Buttons' text
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}New Run",
            NEW_RUN_BTN_TEXT_POS.x,
            NEW_RUN_BTN_TEXT_POS.y,
            TTE_WHITE_PB
        );
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}Main Menu",
            MAIN_MENU_BTN_TEXT_POS.x,
            MAIN_MENU_BTN_TEXT_POS.y,
            TTE_WHITE_PB
        );
    }

    if (s_timer >= GAME_OVER_ANIM_FRAMES)
        selection_grid_process_input(&game_over_selection_grid);
}

void game_over_on_exit(void)
{
    toggle_windows(false, false);

    play_regular_music();
    s_end_condition = END_CONDITION_NONE;

    u32 previous_seed = g_game_vars.rng_info.seed;

    if (!s_continue_run)
        game_reset();

    if (s_reuse_seed)
        rng_set_seed(previous_seed);
    else
        g_game_vars.rng_info.seed = UNDEFINED;
}

// SelectionGrid Implementation

static Button* game_over_get_button_from_sel(const Selection* selection)
{
    return &game_over_buttons[selection->y][selection->x];
}

static void game_over_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
        button_press(&game_over_buttons[selection->y][selection->x]);
}

static bool game_over_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    if (row_idx == prev_selection->y)
        button_set_highlight(game_over_get_button_from_sel(prev_selection), false);

    if (row_idx == new_selection->y)
        button_set_highlight(game_over_get_button_from_sel(new_selection), true);

    return true;
}

// Buttons

static void reuse_seed_on_pressed(void)
{
    s_reuse_seed = !s_reuse_seed;
    main_bg_se_copy_rect(
        s_reuse_seed ? REUSE_SEED_BTN_ON_SRC_RECT : REUSE_SEED_BTN_OFF_SRC_RECT,
        REUSE_SEED_BTN_DEST_POS
    );
}

static void new_run_on_pressed(void)
{
    game_change_state(GAME_STATE_RUN_SETUP);
}

static void main_menu_on_pressed(void)
{
    game_change_state(GAME_STATE_MAIN_MENU);
}
