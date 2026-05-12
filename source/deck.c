#include "deck.h"

#include "game.h"

#include <tonc.h>

// clang-format off
static const char deck_names[DECK_MAX][DECK_NAME_LENGTH] = {
    "  Red Deck   ",
    "  Blue Deck  ",
    " Yellow Deck ",
    " Green Deck  ",
    " Black Deck  ",
    "Painted Deck " 
};

static void print_desc_red_deck(BG_POINT pos);

typedef void (*PrintDescCallback)(BG_POINT);
static const PrintDescCallback deck_description_functions[DECK_MAX] = {
    print_desc_red_deck, NULL, NULL, NULL, NULL, NULL
};
//    {
//        "             ",
//        " +1 Discard  ",
//        " every Round ",
//        "             ",
//        "             "
//    },{
//        "             ",
//        " +1 Discard  ",
//        " every Round ",
//        "             ",
//        "             "
//    },{
//        "             ",
//        " +1 Discard  ",
//        " every Round ",
//        "             ",
//        "             "
//    },{
//        "             ",
//        " +1 Discard  ",
//        " every Round ",
//        "             ",
//        "             "
//    },{
//        "             ",
//        " +1 Discard  ",
//        " every Round ",
//        "             ",
//        "             "
//    },{
//        "             ",
//        " +1 Discard  ",
//        " every Round ",
//        "             ",
//        "             "
//    }
//};
// clang-format off

const void print_deck_name(enum DeckType deck, BG_POINT pos)
{
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        pos.x,
        pos.y,
        TTE_WHITE_PB,
        deck_names[deck]
    );
}

const void print_deck_description(enum DeckType deck, BG_POINT pos)
{
    if (deck_description_functions[deck] == NULL)
    {
        return;
    }

    deck_description_functions[deck](pos);
}

// "             "
// " +1 Discard  "
// " every Round "
// "             "
// "             "
static void print_desc_red_deck(BG_POINT pos)
{
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        pos.x,
        pos.y,
        TTE_BLACK_PB,
        "             "
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s#{cx:0x%X000}%s",
        pos.x,
        pos.y + TILE_SIZE,
        TTE_RED_PB,
        " +1 ",
        TTE_BLACK_PB,
        "Discard  "
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        pos.x,
        pos.y + 2 * TILE_SIZE,
        TTE_BLACK_PB,
        " every Round "
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        pos.x,
        pos.y + 3 * TILE_SIZE,
        TTE_BLACK_PB,
        "             "
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        pos.x,
        pos.y + 4 * TILE_SIZE,
        TTE_BLACK_PB,
        "             "
    );
}
