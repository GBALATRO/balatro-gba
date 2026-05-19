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
    StateCallback on_init;
    StateCallback on_update;
    StateCallback on_exit;
} StateInfo;

typedef struct
{
    StateCallback active_update;
    StateInfo* state_infos;
    unsigned int num_infos;
    int state;
} StateMachine;

void state_machine_init(StateMachine* state_machine);
void state_machine_deinit(StateMachine* state_machine);
void state_machine_update(void);

void game_change_state_new(StateMachine* state_machine, int new_game_state);
void state_machine_change_state(StateMachine* state_machine, int new_state);

// Used as a No Operation for game states that have no init and/or exit function.
// ricfehr3 did the work of determining whether a noop or a NULL check was more
// efficient. Well, this is the answer.
// Thanks!
// https://github.com/cellos51/balatro-gba/issues/137#issuecomment-3322485129
void noop(void);

// clang-format off
#define STATE_INFO_UPDATE_FN_ONLY(fn) {.on_init = noop, .on_update = fn, .on_exit = noop}
// clang-format on

#endif // STATE_MACHINE_H
