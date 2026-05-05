#include "state_machine.h"

#include "game.h"
#include "list.h"
#include "timer.h"
#include "game_variables.h"
#include "util.h"

#include <stdlib.h>

static List update_cbs;

static void noop() {}

void state_machine_init(StateMachine* state_machine)
{
    if(list_is_empty(&update_cbs)) 
        update_cbs = list_create();


    state_machine->active_update = noop;
    state_machine->state = UNDEFINED;

    list_push_back(&update_cbs, &state_machine->active_update);
}

void state_machine_update(void)
{
    ListItr itr = list_itr_create(&update_cbs);
    StateCallback* cb;
    while((cb = list_itr_next(&itr)))
    {
        (*cb)();
    }
}

void game_change_state_new(StateMachine* state_machine, int new_game_state)
{
    if (state_machine->state >= 0 && state_machine->state < state_machine->num_infos)
    {
        state_machine->state_infos[state_machine->state].substate = 0;
        state_machine->state_infos[state_machine->state].on_exit();
    }

    if (new_game_state >= 0 && new_game_state < state_machine->num_infos)
    {
        state_machine->state_infos[new_game_state].on_init();
    }

    state_machine->active_update = state_machine->state_infos[new_game_state].on_update;

    state_machine->state = new_game_state;
}
