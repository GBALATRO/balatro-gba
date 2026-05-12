#include "graphic_utils.h"

#define DECK_NAME_LENGTH 14 // 13 chars + '\0'
#define DECK_DESC_WIDTH  DECK_NAME_LENGTH
#define DECK_DESC_HEIGHT 5

enum Deck
{
    DECK_RED,
    DECK_BLUE,
    DECK_YELLOW,
    DECK_GREEN,
    DECK_BLACK,
    DECK_PAINTED,
    DECK_MAX
};

const void print_deck_name(enum Deck deck, BG_POINT pos);
const void print_deck_description(enum Deck deck, BG_POINT pos);
