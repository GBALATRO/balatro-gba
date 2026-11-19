/**
 * @file best_runs.c
 *
 * @brief Best runs ranking recording and screen implementation.
 */
#include "game/best_runs.h"

#include "background_options_menu_gfx.h"
#include "deck_types.h"
#include "game.h"
#include "game/common_ui.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "run_history.h"
#include "save.h"
#include "util.h"

#include <tonc.h>

#define SCORE_MAX_CHARS 6
// WHY: the run number column is 3 characters wide
#define RUN_NUMBER_MAX_DISPLAYED 999

// clang-format off
// Positions in tiles: the panel of the Options Menu background and where its tiles are reused
static const Rect     OPTIONS_PANEL_TOP_LEFT_SRC     = { 3,  0,  3,  0};
static const Rect     OPTIONS_PANEL_TOP_RIGHT_SRC    = {26,  0, 26,  0};
static const Rect     OPTIONS_PANEL_BOTTOM_LEFT_SRC  = { 3, 19,  3, 19};
static const Rect     OPTIONS_PANEL_BOTTOM_RIGHT_SRC = {26, 19, 26, 19};
static const BG_POINT OPTIONS_PANEL_LEFT_SRC_POS     = { 3,  1};
static const BG_POINT OPTIONS_PANEL_RIGHT_SRC_POS    = {26,  1};
static const BG_POINT OPTIONS_PANEL_TOP_SRC_POS      = { 4,  0};
static const BG_POINT OPTIONS_PANEL_BOTTOM_SRC_POS   = { 4, 19};
static const BG_POINT OPTIONS_PANEL_FILL_SRC_POS     = { 4,  1};

static const BG_POINT PANEL_TOP_LEFT_POS     = { 0,  0};
static const BG_POINT PANEL_TOP_RIGHT_POS    = {29,  0};
static const BG_POINT PANEL_BOTTOM_LEFT_POS  = { 0, 19};
static const BG_POINT PANEL_BOTTOM_RIGHT_POS = {29, 19};
static const Rect     PANEL_LEFT_RECT        = { 0,  1,  0, 18};
static const Rect     PANEL_RIGHT_RECT       = {29,  1, 29, 18};
static const Rect     PANEL_TOP_RECT         = { 1,  0, 28,  0};
static const Rect     PANEL_BOTTOM_RECT      = { 1, 19, 28, 19};
static const Rect     PANEL_FILL_RECT        = { 1,  1, 28, 18};

// Positions in pixels
static const BG_POINT TITLE_POS     = { 80,   8};
static const BG_POINT BACK_HINT_POS = {168,   8};
static const BG_POINT HEADER_POS    = { 16,  16};
static const BG_POINT FIRST_ROW_POS = { 16,  24};
static const BG_POINT EMPTY_POS     = { 44,  72};

// A row is 26 characters wide, the header must stay aligned with the row format
static const char HEADER[]     = " #  SCORE SEED   DCK A RUN";
static const char ROW_FORMAT[] = "#{P:%d,%d; cx:0x%X000}%2d %6s %-6s #{cx:0x%X000}%-3s"
                                 "#{cx:0x%X000} %c %3d";

/**
 * @brief How a Deck is named on a row, and the text color that tells it apart at a glance.
 */
typedef struct DeckLabel
{
    const char* name;
    u8 text_color;
} DeckLabel;

static const DeckLabel DECK_LABELS[DECK_TYPE_MAX] = {
    [DECK_TYPE_RED]     = {"Red", TTE_RED_PB},
    [DECK_TYPE_BLUE]    = {"Blu", TTE_BLUE_PB},
    [DECK_TYPE_YELLOW]  = {"Yel", TTE_YELLOW_PB},
    [DECK_TYPE_GREEN]   = {"Grn", TTE_GREEN_PB},
    [DECK_TYPE_BLACK]   = {"Blk", TTE_WHITE_PB},
    [DECK_TYPE_PAINTED] = {"Pnt", TTE_PURPLE_PB}
};
static const DeckLabel UNKNOWN_DECK_LABEL = {"???", TTE_WHITE_PB};
// clang-format on

static EWRAM_BSS RunHistory s_history;

static void best_runs_print_record(int rank, const RunRecord* record)
{
    // Format values
    char score_str[UINT_MAX_DIGITS + 1] = {'\0'};
    truncate_uint_to_suffixed_str(record->best_hand_score, SCORE_MAX_CHARS, score_str);

    char seed_str[BASE36_MAX_DIGITS + 1] = {'\0'};
    u32_to_base36(record->seed, seed_str);

    const DeckLabel* deck_label =
        (record->deck < DECK_TYPE_MAX) ? &DECK_LABELS[record->deck] : &UNKNOWN_DECK_LABEL;

    // WHY: a won run always went through the last Ante, so "W" says more than its number
    char ante_char = record->won ? 'W' : (char)('0' + record->ante % 10);

    // Print row
    tte_printf(
        ROW_FORMAT,
        FIRST_ROW_POS.x,
        FIRST_ROW_POS.y + rank * TILE_SIZE,
        TTE_WHITE_PB,
        rank + 1,
        score_str,
        seed_str,
        deck_label->text_color,
        deck_label->name,
        TTE_WHITE_PB,
        ante_char,
        min(record->run_number, RUN_NUMBER_MAX_DISPLAYED)
    );
}

static void best_runs_print(void)
{
    load_run_history(&s_history);

    // Print title
    tte_printf("#{P:%d,%d; cx:0x%X000}BEST RUNS", TITLE_POS.x, TITLE_POS.y, TTE_YELLOW_PB);
    tte_printf("#{P:%d,%d; cx:0x%X000}B: Back", BACK_HINT_POS.x, BACK_HINT_POS.y, TTE_WHITE_PB);

    if (s_history.nb_records == 0)
    {
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}No run recorded yet",
            EMPTY_POS.x,
            EMPTY_POS.y,
            TTE_WHITE_PB
        );
        return;
    }

    // Print ranking
    tte_printf("#{P:%d,%d; cx:0x%X000}%s", HEADER_POS.x, HEADER_POS.y, TTE_YELLOW_PB, HEADER);

    for (int rank = 0; rank < s_history.nb_records; rank++)
        best_runs_print_record(rank, &s_history.records[rank]);
}

void best_runs_record_finished_run(bool won)
{
    load_run_history(&s_history);

    RunRecord record = {
        .best_hand_score = g_game_vars.best_hand_score,
        .seed = g_game_vars.rng_info.seed,
        .ante = g_game_vars.ante,
        .deck = g_game_vars.deck,
        .won = won
    };
    run_history_add(&s_history, record);

    // WHY: saved even when the run is not ranked, to keep the count of runs played
    save_run_history(&s_history);
}

void best_runs_change_background(void)
{
    tte_erase_screen();

    GRIT_CPY(pal_bg_mem, background_options_menu_gfxPal);
    GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_options_menu_gfxTiles);
    GRIT_CPY(&se_mem[MAIN_BG_SBB], background_options_menu_gfxMap);

    // WHY: the Main Menu palette overwrote some of the text colors
    tte_colors_setup();

    // Copy corners
    // WHY: done first because the sides and the fill then cover the original panel
    main_bg_se_copy_rect(OPTIONS_PANEL_TOP_LEFT_SRC, PANEL_TOP_LEFT_POS);
    main_bg_se_copy_rect(OPTIONS_PANEL_TOP_RIGHT_SRC, PANEL_TOP_RIGHT_POS);
    main_bg_se_copy_rect(OPTIONS_PANEL_BOTTOM_LEFT_SRC, PANEL_BOTTOM_LEFT_POS);
    main_bg_se_copy_rect(OPTIONS_PANEL_BOTTOM_RIGHT_SRC, PANEL_BOTTOM_RIGHT_POS);

    // Stretch sides
    main_bg_se_copy_expand_tile(PANEL_LEFT_RECT, OPTIONS_PANEL_LEFT_SRC_POS);
    main_bg_se_copy_expand_tile(PANEL_RIGHT_RECT, OPTIONS_PANEL_RIGHT_SRC_POS);
    main_bg_se_copy_expand_tile(PANEL_TOP_RECT, OPTIONS_PANEL_TOP_SRC_POS);
    main_bg_se_copy_expand_tile(PANEL_BOTTOM_RECT, OPTIONS_PANEL_BOTTOM_SRC_POS);

    // Fill panel, which also removes the options and the original sides
    main_bg_se_copy_expand_tile(PANEL_FILL_RECT, OPTIONS_PANEL_FILL_SRC_POS);
}

void best_runs_on_init(void)
{
    change_background(BG_BEST_RUNS, false);

    best_runs_print();
}

void best_runs_on_update(void)
{
    if (key_hit(KEY_CANCEL | KEY_CONFIRM | KEY_PAUSE_GAME))
        game_change_state(GAME_STATE_MAIN_MENU);
}

void best_runs_on_exit(void)
{
    tte_erase_screen();
}
