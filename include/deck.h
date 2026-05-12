#ifndef DECK_H
#define DECK_H

#include "graphic_utils.h"

#define DECK_SPRITES_PB 1

#define DECK_NAME_LENGTH 14 // 13 chars + '\0'
#define DECK_DESC_WIDTH  DECK_NAME_LENGTH
#define DECK_DESC_HEIGHT 5

enum DeckType
{
    DECK_TYPE_RED,
    DECK_TYPE_BLUE,
    DECK_TYPE_YELLOW,
    DECK_TYPE_GREEN,
    DECK_TYPE_BLACK,
    DECK_TYPE_PAINTED,
    DECK_TYPE_MAX
};

const void print_deck_name(enum DeckType deck, BG_POINT pos);
const void print_deck_description(enum DeckType deck, BG_POINT pos);

#endif // DECK_H
