#include "state_machine.h"

#include "game.h"
#include "game_variables.h"
#include "list.h"
#include "timer.h"
#include "util.h"

#include <stdlib.h>

static List update_cbs;

void noop(void) {};

void state_machine_init(StateMachine* state_machine)
{
    if (list_is_empty(&update_cbs))
        update_cbs = list_create();

    state_machine->active_update = noop;
    state_machine->state = UNDEFINED;

    list_push_back(&update_cbs, &state_machine->active_update);
}

void state_machine_deinit(StateMachine* state_machine)
{
    list_remove_at(&update_cbs, &state_machine->active_update);
}

void state_machine_update(void)
{
    ListItr itr = list_itr_create(&update_cbs);
    StateCallback* cb;
    while ((cb = list_itr_next(&itr)))
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

    // This has an inadvertant "feature". If you are in the calling function of a state, calling
    // this function on it's own state machine. Aka, you are changing states mid update. Then
    // Because it is only updating the active pointer of code to-be ran, the update function
    // finishes before continuing. This is incredibly helpful as you can initiate a state change in
    // the update, but just don't think the code is breaking on that change state call.
    state_machine->active_update = state_machine->state_infos[new_game_state].on_update;

    state_machine->state = new_game_state;
}

void state_machine_change_state(StateMachine* state_machine, int new_state)
{
    game_change_state_new(state_machine, new_state);
}
