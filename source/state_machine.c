#include "state_machine.h"

#include "game.h"
#include "list.h"
#include "timer.h"
#include "game_variables.h"

#include <stdlib.h>

static List update_cbs;

static void noop()
{
    tte_printf("#{P:10,10; cx:0x%X000} AHHHHHHH.", TTE_WHITE_PB);
}

void state_machine_init(StateMachine* state_machine)
{
    if(list_is_empty(&update_cbs)) 
        update_cbs = list_create();


    list_push_back(&update_cbs, &state_machine->active_update);
    state_machine->active_update = noop;
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
    state_machine->active_update = state_machine->state_infos[new_game_state].on_update;
    //g_game_vars.timer = TM_ZERO; // Reset the timer

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
