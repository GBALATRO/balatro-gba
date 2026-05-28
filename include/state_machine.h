/**
 * @file state_machine.h
 *
 * @brief State Machine
 *
 * This file is the interface into a generic state machine system.
 *
 * State machines are defined as an array of function callbacks where each state
 * is an index in the array with three functions per state: `on_init()`,
 * `on_update()`, and `on_exit()`.
 *
 * **`on_init()`** Ran once when transitioning into the new state
 * **`on_update()`** Ran once every frame
 * **`on_exit()`** Ran once when exiting a state, for cleanup.
 *
 * The state machine `on_update()` function is "registered" to a linked-list of
 * other state machine update functions. This list will call the state machines
 * active `on_update()` function. This allows much of the complexity of state
 * transitions to remain central to `state_machine.c`. Also, multiple state
 * machines can be registered to this list. This allows using substates within
 * states, or have one off state machines like animation controllers.
 *
 * When a state machine is finished, it can "remove" itself from the main update
 * callback list. This can be done within the update method of its own state
 * machine. This can be used to start a self destructing state machine.
 */
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

/**
 * @brief State machine callback function pointer type
 */
typedef void (*StateCallback)(void);

/**
 * @brief State machine callbacks
 */
typedef struct
{
    StateCallback on_init;
    StateCallback on_update;
    StateCallback on_exit;
} StateInfo;

/**
 * @brief State machine instance
 */
typedef struct
{
    /**
     * @brief Pointer to the active update function in `state_infos`
     */
    StateCallback active_update;

    /**
     * @brief Array of @ref StateCallbacks , one entry per state
     */
    StateInfo* state_infos;

    /**
     * @brief Number of elements in the `state_infos` array
     */
    unsigned int num_infos;

    /**
     * @brief The current state of the state machine, the offset into state_infos
     */
    int state;

    /**
     * @brief Flag to determine if statemachine is already running it's active_update function
     */
    bool registered;
} StateMachine;

/**
 * @brief Register a statemachine to run it's update function once per frame
 *
 * @param state_machine pointer to @ref StateMachine to register, cannot be NULL
 */
void state_machine_register(StateMachine* state_machine);

/**
 * @brief Remove a statemachine's update function
 *
 * @param state_machine pointer to @ref StateMachine to remove, cannot be NULL
 */
void state_machine_remove(StateMachine* state_machine);

/**
 * @brief Update registered state machines' update functions
 */
void state_machine_update(void);

/**
 * @brief Calls the current state's on_exit, the new state's on_init, and sets the active update fn
 *
 * @param state_machine pointer to @ref StateMachine, cannot be NULL
 * @param new_state offset into `state_infos` array to transition to
 */
void state_machine_change_state(StateMachine* state_machine, int new_state);

/**
 * @brief no operation
 */
void noop(void);

// clang-format off
#define STATE_INFO_UPDATE_FN_ONLY(fn)                 {.on_init = noop,    .on_update = fn,        .on_exit = noop}
#define STATE_INFO_INIT_UPDATE_FN(init_fn, update_fn) {.on_init = init_fn, .on_update = update_fn, .on_exit = noop}
#define STATE_MACHINE_DEFINE(infos, num) \
{                                        \
    .state_infos = &infos[0],            \
    .num_infos = num,                    \
    .registered = false,                 \
};
// clang-format on

#endif // STATE_MACHINE_H
