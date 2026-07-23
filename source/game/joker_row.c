#include "game/joker_row.h"

#include "game.h"
#include "game_variables.h"
#include "joker.h"
#include "layout.h"
#include "list.h"
#include "sprite.h"
#include "util.h"

int jokers_sel_row_get_size(void)
{
    return list_get_len(get_jokers_list());
}

bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    List* owned_jokers_list = get_jokers_list();

    // swap Jokers if the A button is held down and all Jokers are on the same row
    bool swapping =
        key_is_down(SELECT_CARD) && new_selection->y == row_idx && prev_selection->y == row_idx;

    if (prev_selection->y == row_idx)
    {
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(owned_jokers_list, prev_selection->x);
        // Don't change focus from current Joker if swapping
        if (joker_object != NULL && !swapping)
        {
            sprite_object_erase_text_under((SpriteObject*)joker_object);
            sprite_object_set_focus((SpriteObject*)joker_object, false);
        }
    }

    if (new_selection->y == row_idx)
    {
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(owned_jokers_list, new_selection->x);
        if (joker_object != NULL)
        {
            if (!swapping)
            {
                sprite_object_set_focus((SpriteObject*)joker_object, true);
            }
            // If we land on this row while the A button is being held, we are in swapping mode
            // This means that we need to hide the price, whether we were already
            // on this row or if we come from another
            if (!key_is_down(SELECT_CARD))
            {
                sprite_object_print_price_under(
                    (SpriteObject*)joker_object,
                    joker_get_sell_value(joker_object->joker)
                );
            }
        }
    }

    if (swapping)
    {
        list_swap_at_idx(
            owned_jokers_list,
            (unsigned int)prev_selection->x,
            (unsigned int)new_selection->x
        );
        sprite_object_swap_layers(
            list_get_at_idx(owned_jokers_list, prev_selection->x),
            list_get_at_idx(owned_jokers_list, new_selection->x)
        );
    }

    return true;
}

static inline void joker_start_discard_animation(JokerObject* joker_object)
{
    joker_object->tx = int2fx(JOKER_DISCARD_TARGET.x);
    joker_object->ty = int2fx(JOKER_DISCARD_TARGET.y);
    list_push_back(get_discarded_jokers_list(), joker_object);
}

static inline void game_sell_joker(int joker_idx)
{
    List* owned_jokers_list = get_jokers_list();

    if (joker_idx < 0 || joker_idx >= list_get_len(owned_jokers_list))
        return;

    JokerObject* joker_object = (JokerObject*)list_get_at_idx(owned_jokers_list, joker_idx);
    g_game_vars.money += joker_get_sell_value(joker_object->joker);
    display_money();
    sprite_object_erase_text_under((SpriteObject*)joker_object);

    sprite_object_sort_list((void*)owned_jokers_list, true);

    remove_owned_joker(joker_idx);

    joker_start_discard_animation(joker_object);
}

void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    JokerObject* joker_object = (JokerObject*)list_get_at_idx(get_jokers_list(), selection->x);
    if (joker_object != NULL)
    {
        if (key_hit(SELECT_CARD))
        {
            sprite_object_erase_text_under((SpriteObject*)joker_object);
        }
        else if (key_released(SELECT_CARD))
        {
            sprite_object_print_price_under(
                (SpriteObject*)joker_object,
                joker_get_sell_value(joker_object->joker)
            );
        }
    }

    if (key_hit(SELL_KEY))
    {
        int sold_joker_idx = selection->x;

        // Move the selection away from the jokers so it doesn't point to an invalid place
        // Do this before selling the joker so valid row sizes are used
        selection_grid_move_selection_vert(selection_grid, SCREEN_DOWN);

        game_sell_joker(sold_joker_idx);
    }
}