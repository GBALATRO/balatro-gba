#ifndef CARD_H
#define CARD_H

#include "sprite.h"

#include <maxmod.h>
#include <tonc.h>

#define MAX_CARDS           (NUM_SUITS * NUM_RANKS)
#define MAX_CARDS_ON_SCREEN 16

#define CARD_TID            0
#define CARD_SPRITE_OFFSET  16
#define CARD_PB             0
#define CARD_STARTING_LAYER 0

// Card suits
#define DIAMONDS  0
#define CLUBS     1
#define HEARTS    2
#define SPADES    3
#define NUM_SUITS 4

// Card ranks
#define TWO         0
#define THREE       1
#define FOUR        2
#define FIVE        3
#define SIX         4
#define SEVEN       5
#define EIGHT       6
#define NINE        7
#define TEN         8
#define JACK        9
#define QUEEN       10
#define KING        11
#define ACE         12
#define NUM_RANKS   13
#define RANK_OFFSET 2 // Because the first rank is 2 and ranks start at 0

#define IMPOSSIBLY_HIGH_CARD_VALUE 100

// Card types
typedef struct Card
{
    u8 suit;
    u8 rank;
} Card;

typedef struct CardObject
{
    Card* card;
    SpriteObject* sprite_object;
    bool selected;
} CardObject;

// Card functions
void card_init();

// Card methods
Card* card_new(u8 suit, u8 rank);
void card_destroy(Card** card);
u8 card_get_value(Card* card);

// CardObject methods
CardObject* card_object_new(Card* card);
void card_object_destroy(CardObject** card_object);
void card_object_update(CardObject* card_object); // Update the card object position and scale
void card_object_set_sprite(CardObject* card_object, int layer);
void card_object_shake(CardObject* card_object, mm_word sound_id);

void card_object_set_selected(CardObject* card_object, bool selected);
bool card_object_is_selected(CardObject* card_object);
Sprite* card_object_get_sprite(CardObject* card_object);

#endif // CARD_H
