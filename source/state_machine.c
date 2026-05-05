#include "state_machine.h"

#include "game.h"
#include "list.h"
#include "timer.h"
#include "game_variables.h"

static List update_cbs;

static void noop(){}

void state_machine_init(StateMachine* state_machine)
{
    if(list_is_empty(&update_cbs)) 
        update_cbs = list_create();

    *state_machine->active_update = noop;

    list_push_back(&update_cbs, state_machine->active_update);
}

void state_machine_update(void)
{
    return;
    ListItr itr = list_itr_create(&update_cbs);
    StateCallback* cb;
    while((cb = list_itr_next(&itr)))
    {
        (*cb)();
    }
}

void game_change_state_new(StateMachine* state_machine, int new_game_state)
{
    g_game_vars.timer = TM_ZERO; // Reset the timer

    /*
    if (state >= 0 && state < GAME_STATE_MAX)
    {
        state_info[state].substate = 0;
        state_info[_state].on_exit();
    }

    if (new_game_state >= 0 && new_game_state < GAME_STATE_MAX)
    {
        state_info[new_game_state].on_init();

        game_state = new_game_state;
    }
    */
}
