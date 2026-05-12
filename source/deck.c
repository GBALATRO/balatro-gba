#include "deck.h"

#include "game.h"

#include <tonc.h>

#define TTE_COLOR_TEXT_FORMAT "#{cx:0x%X000}%s"

static const char deck_names[DECK_TYPE_MAX][DECK_NAME_LENGTH] = {
    "  Red Deck   ",
    "  Blue Deck  ",
    " Yellow Deck ",
    " Green Deck  ",
    " Black Deck  ",
    "Painted Deck "
};

static void print_desc_red_deck(BG_POINT pos);
static void print_desc_blue_deck(BG_POINT pos);
static void print_desc_yellow_deck(BG_POINT pos);
static void print_desc_green_deck(BG_POINT pos);
static void print_desc_black_deck(BG_POINT pos);
static void print_desc_painted_deck(BG_POINT pos);

typedef void (*PrintDescCallback)(BG_POINT);
static const PrintDescCallback deck_description_functions[DECK_TYPE_MAX] = {
    print_desc_red_deck,
    print_desc_blue_deck,
    print_desc_yellow_deck,
    print_desc_green_deck,
    print_desc_black_deck,
    print_desc_painted_deck
};

const void print_deck_name(enum DeckType deck, BG_POINT pos)
{
    tte_printf("#{P:%d,%d; cx:0x%X000}%s", pos.x, pos.y, TTE_WHITE_PB, deck_names[deck]);
}

const void print_deck_description(enum DeckType deck, BG_POINT pos)
{
    if (deck_description_functions[deck] == NULL)
    {
        return;
    }

    deck_description_functions[deck](pos);
}

static inline void print_empty_desc_line(int pos_x, int pos_y)
{
    tte_printf("#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT, pos_x, pos_y, TTE_BLACK_PB, "             ");
}

// "             "
// " +1 discard  "
// " every Round "
// "             "
// "             "
static void print_desc_red_deck(BG_POINT pos)
{
    print_empty_desc_line(pos.x, pos.y);
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + TILE_SIZE,
        TTE_RED_PB,
        " +1 ",
        TTE_BLACK_PB,
        "discard  "
    );
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + 2 * TILE_SIZE,
        TTE_BLACK_PB,
        " every Round "
    );
    print_empty_desc_line(pos.x, pos.y + 3 * TILE_SIZE);
    print_empty_desc_line(pos.x, pos.y + 4 * TILE_SIZE);
}

// "             "
// "   +1 hand   "
// " every Round "
// "             "
// "             "
static void print_desc_blue_deck(BG_POINT pos)
{
    print_empty_desc_line(pos.x, pos.y);
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + TILE_SIZE,
        TTE_BLUE_PB,
        "   +1 ",
        TTE_BLACK_PB,
        "hand   "
    );
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + 2 * TILE_SIZE,
        TTE_BLACK_PB,
        " every Round "
    );
    print_empty_desc_line(pos.x, pos.y + 3 * TILE_SIZE);
    print_empty_desc_line(pos.x, pos.y + 4 * TILE_SIZE);
}

// "             "
// " Start with  "
// "an extra $10 "
// "             "
// "             "
static void print_desc_yellow_deck(BG_POINT pos)
{
    print_empty_desc_line(pos.x, pos.y);
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + TILE_SIZE,
        TTE_BLACK_PB,
        " Start with  "
    );
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + 2 * TILE_SIZE,
        TTE_BLACK_PB,
        "an extra",
        TTE_YELLOW_PB,
        " $10 "
    );
    print_empty_desc_line(pos.x, pos.y + 3 * TILE_SIZE);
    print_empty_desc_line(pos.x, pos.y + 4 * TILE_SIZE);
}

// "End of Round:"
// "             "
// "-$2 per Hand "
// "-$2 per Disc."
// "-No Interest "
static void print_desc_green_deck(BG_POINT pos)
{
    tte_printf("#{P:%d,%d; cx:0x%X000}%s", pos.x, pos.y, TTE_BLACK_PB, "End of Round:");
    print_empty_desc_line(pos.x, pos.y + TILE_SIZE);
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT
            TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + 2 * TILE_SIZE,
        TTE_BLACK_PB,
        "-",
        TTE_YELLOW_PB,
        "$2 ",
        TTE_BLACK_PB,
        "per ",
        TTE_RED_PB,
        "Hand "
    );
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT
            TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + 3 * TILE_SIZE,
        TTE_BLACK_PB,
        "-",
        TTE_YELLOW_PB,
        "$1 ",
        TTE_BLACK_PB,
        "per ",
        TTE_BLUE_PB,
        "Disc."
    );
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + 4 * TILE_SIZE,
        TTE_BLACK_PB,
        "-No ",
        TTE_YELLOW_PB,
        "Interest "
    );
}

// "+1 Joker slot"
// "             "
// "   -1 hand   "
// " every Round "
// "             "
static void print_desc_black_deck(BG_POINT pos)
{
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y,
        TTE_YELLOW_PB,
        "+1 ",
        TTE_BLACK_PB,
        "Joker slot"
    );
    print_empty_desc_line(pos.x, pos.y + TILE_SIZE);
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + 2 * TILE_SIZE,
        TTE_BLUE_PB,
        "   -1 ",
        TTE_BLACK_PB,
        "hand   "
    );
    tte_printf(
        "#{P:%d,%d}" TTE_COLOR_TEXT_FORMAT,
        pos.x,
        pos.y + 3 * TILE_SIZE,
        TTE_BLACK_PB,
        " every Round "
    );
    print_empty_desc_line(pos.x, pos.y + 4 * TILE_SIZE);
}

// "             "
// " Start with  "
// "an extra $10 "
// "             "
// "             "
static void print_desc_painted_deck(BG_POINT pos)
{
}
