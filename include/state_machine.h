/**
 * @file state_machine.h
 *
 * @brief State Machine
 *
 */
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

typedef void (*StateCallback)(void);

typedef struct
{
    int substate;
    StateCallback on_init;
    StateCallback on_update;
    StateCallback on_exit;
    int state;
} StateInfo;

typedef struct
{
    StateCallback active_update;
    StateInfo* state_infos;
} StateMachine;

void state_machine_init(StateMachine* state_machine);
void state_machine_update(void);

void game_change_state_new(StateMachine* state_machine, int new_game_state);

#endif // STATE_MACHINE_H
