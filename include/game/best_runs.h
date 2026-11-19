/**
 * @file best_runs.h
 *
 * @brief Records finished runs and shows the ranking of the best ones on its own screen.
 */
#ifndef GAME_BEST_RUNS_H
#define GAME_BEST_RUNS_H

#include <stdbool.h>

/**
 * @brief Add the run that just ended to the saved ranking if its score is high enough.
 *
 * @param won true if the run was won, false if it was lost
 */
void best_runs_record_finished_run(bool won);

/**
 * @brief Draw the Best Runs screen: the Options Menu panel stretched to the whole screen.
 */
void best_runs_change_background(void);

/**
 * @brief Best Runs state initialization
 */
void best_runs_on_init(void);

/**
 * @brief Best Runs state update
 */
void best_runs_on_update(void);

/**
 * @brief Best Runs state exit
 */
void best_runs_on_exit(void);

#endif // GAME_BEST_RUNS_H
