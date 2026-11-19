/**
 * @file deck_pile.c
 *
 * @brief Deck pile card back implementation.
 */
#include "game/deck_pile.h"

#include "card.h"
#include "game/common_ui.h"
#include "sprite.h"

// WHY: layer 0 belongs to the Deck shown on the Run Setup screen
#define DECK_PILE_SPRITE_LAYER 1

// Top-left corner of the top card of the Deck pile in the backgrounds, in pixels
static const BG_POINT DECK_PILE_POS = {204, 115};

static Sprite* s_deck_pile_sprite = NULL;
static int s_offset_y = 0;

static void deck_pile_update_position(void)
{
    if (s_deck_pile_sprite == NULL)
        return;

    sprite_position(s_deck_pile_sprite, DECK_PILE_POS.x, DECK_PILE_POS.y + s_offset_y);
}

void deck_pile_show(enum DeckType deck)
{
    deck_pile_hide();

    card_load_face_down_tiles(deck, DECK_PILE_SPRITE_LAYER);
    s_deck_pile_sprite = sprite_new(
        ATTR0_SQUARE | ATTR0_4BPP,
        ATTR1_SIZE_32,
        sprite_get_tid(DECK_SPRITE, DECK_PILE_SPRITE_LAYER),
        DECK_PB,
        sprite_get_starting_layer(DECK_SPRITE) + DECK_PILE_SPRITE_LAYER
    );

    deck_pile_reset_position();
}

void deck_pile_hide(void)
{
    sprite_destroy(&s_deck_pile_sprite);
}

void deck_pile_move_vert(enum ScreenVertDir direction)
{
    s_offset_y += direction * TILE_SIZE;

    deck_pile_update_position();
}

void deck_pile_reset_position(void)
{
    s_offset_y = 0;

    deck_pile_update_position();
}
