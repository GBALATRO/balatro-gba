/**
 * @file run_setup.h
 *
 * @brief Setup Run menu state functions.
 */
#ifndef GAME_RUN_SETUP_H
#define GAME_RUN_SETUP_H

/**
 * @brief Change to the Run Setup menu background
 */
void game_run_setup_change_background(void);

/**
 * @brief Run Setup menu state initialization
 */
void game_run_setup_on_init(void);

/**
 * @brief Run Setup menu state update
 */
void game_run_setup_on_update(void);

/**
 * @brief Run Setup menu cleanup
 */
void game_run_setup_on_exit(void);

#endif // GAME_START_RUN_H