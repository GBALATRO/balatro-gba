#include "state_machine.h"

#include "game.h"
#include "list.h"
#include "util.h"

static List update_cbs = LIST_DEFAULT;

// Used as a No Operation for game states that have no init and/or exit function.
// ricfehr3 did the work of determining whether a noop or a NULL check was more
// efficient. Well, this is the answer.
// Thanks!
// https://github.com/cellos51/balatro-gba/issues/137#issuecomment-3322485129
void noop(void) {};

void state_machine_register(StateMachine* state_machine)
{
    // Always try to remove the state machine first. Only one can exist at a time
    // So ensure that calling this function doesn't add two update functions
    state_machine_remove(state_machine);

    state_machine->active_update = noop;
    state_machine->state = UNDEFINED;

    list_push_back(&update_cbs, &state_machine->active_update);
}

void state_machine_remove(StateMachine* state_machine)
{
    list_remove_data(&update_cbs, &state_machine->active_update);
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

void state_machine_change_state(StateMachine* state_machine, int new_state)
{
    if (state_machine->state >= 0 && state_machine->state < state_machine->num_infos)
    {
        state_machine->state_infos[state_machine->state].on_exit();
    }

    if (new_state >= 0 && new_state < state_machine->num_infos)
    {
        state_machine->state_infos[new_state].on_init();

        state_machine->active_update = state_machine->state_infos[new_state].on_update;

        state_machine->state = new_state;
    }
    else
    {
        state_machine->active_update = noop;
        state_machine->state = UNDEFINED;
    }
}
