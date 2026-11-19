/**
 * @file deck_pile.h
 *
 * @brief Card back of the run's Deck, shown on top of the Deck pile drawn in the backgrounds.
 *
 * The backgrounds only contain the Red Deck's pile, so a sprite covers its top card.
 */
#ifndef GAME_DECK_PILE_H
#define GAME_DECK_PILE_H

#include "deck_types.h"
#include "graphic_utils.h"

/**
 * @brief Show the card back of @p deck on top of the Deck pile, at its resting position.
 *
 * @param deck the Deck whose card back is shown
 */
void deck_pile_show(enum DeckType deck);

/**
 * @brief Remove the card back from the screen, for screens that have no Deck pile.
 */
void deck_pile_hide(void);

/**
 * @brief Move the card back by one tile, to follow the Deck pile when its background tiles move.
 *
 * @param direction the direction the Deck pile's tiles were moved in
 */
void deck_pile_move_vert(enum ScreenVertDir direction);

/**
 * @brief Put the card back at its resting position again.
 */
void deck_pile_reset_position(void);

#endif // GAME_DECK_PILE_H
