/**
 * @file joker_row.h
 *
 * @brief All the functions used specifically to interact with Jokers.
 *         This includes browsing, swapping, selling, etc.
 */
#ifndef GAME_JOKER_ROW_H
#define GAME_JOKER_ROW_H

#include "selection_grid.h"

/**
 * @brief Returns the size of the Jokers' row at the top of the game screen.
 *         Is equal to the number of Jokers owned.
 *
 * @return int
 */
int jokers_sel_row_get_size(void);

/**
 * @brief Handle directional inputs on the Jokers' row at the top of the game screen.
 *
 * @param selection_grid pointer to the SelectionGrid the row is a part of.
 * @param row_idx index of the row this function is called on.
 * @param prev_selection pointer to the coordinates of the previously selected button
 *                        in `selection_grid`
 * @param new_selection pointer to the coordinates of the newly selected button in
 *                       `selection_grid`
 * @return bool
 */
bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

/**
 * @brief Handle button inputs on the Jokers' row at the top of the game screen.
 *
 * @param selection_grid pointer to the SelectionGrid the row is a part of.
 * @param selection pointer to the coordinates of the currently selected button in
 *                   `selection_grid`
 */
void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);

#endif // GAME_JOKER_ROW_H