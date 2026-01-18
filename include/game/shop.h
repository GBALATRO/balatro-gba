#ifndef GAME_SHOP_H
#define GAME_SHOP_H

#include "blind.h"
#include "list.h"
#include "selection_grid.h"

#include <tonc.h>

typedef struct
{
    uint timer;
    int substate;
    int money;
    enum BlindState blinds_states[BLIND_TYPE_MAX];
    int current_blind;
    List* owned_jokers_list;
    List* discarded_jokers_list;
} ShopProps;

typedef struct
{
    List* owned_jokers_list;
    int* money;
    int* substate;
    uint* timer;
} JokerSellProps;

void game_shop_on_update(void* ctx);
void game_shop_on_exit(void* ctx);

void reset_shop_jokers(void);
void set_shop_joker_avail(int joker_id, bool avail);

int jokers_sel_row_get_size(void* ctx);
bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection,
    void* ctx
);
void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection, void* ctx);

void game_shop_change_background();

#endif // GAME_SHOP_H
