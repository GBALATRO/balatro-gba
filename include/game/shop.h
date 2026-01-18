#ifndef GAME_SHOP_H
#define GAME_SHOP_H

#include "selection_grid.h"

void game_shop_on_update(void* ctx);
void game_shop_on_exit(void* ctx);

void reset_shop_jokers(void);
void set_shop_joker_avail(int joker_id, bool avail);

int jokers_sel_row_get_size(void);
bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);

void game_shop_change_background();

#endif // GAME_SHOP_H
