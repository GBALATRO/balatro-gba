/**
 * @file run_history.h
 *
 * @brief Ranking of the best finished runs, kept across play sessions.
 *
 * Only data and ranking logic live here so they can be unit tested on the host:
 * saving is done by save.c and display by game/best_runs.c
 */
#ifndef RUN_HISTORY_H
#define RUN_HISTORY_H

#include <stdbool.h>
#include <stdint.h>

#define RUN_HISTORY_MAX_RECORDS 15

/**
 * @brief Returned by run_history_add() when the run did not make it into the ranking.
 */
#define RUN_HISTORY_NOT_RANKED (-1)

/**
 * @brief What is remembered of a finished run.
 */
typedef struct RunRecord
{
    uint32_t best_hand_score;
    uint32_t seed;

    /**
     * @brief Position of the run among all finished runs, starting at 1.
     * WHY: the GBA has no clock, so this stands in for the date of the run.
     * Set by run_history_add()
     */
    uint16_t run_number;

    uint8_t ante;
    uint8_t deck;
    bool won;
    uint8_t padding[3];
} RunRecord;

/**
 * @brief The best runs, sorted from the highest best hand score to the lowest.
 */
typedef struct RunHistory
{
    uint32_t magic;
    uint8_t version;
    uint8_t nb_records;
    uint16_t nb_runs_played;
    RunRecord records[RUN_HISTORY_MAX_RECORDS];
} RunHistory;

/**
 * @brief Make @p history a valid empty ranking.
 *
 * @param history the ranking to initialize
 */
void run_history_init(RunHistory* history);

/**
 * @brief Tell whether @p history holds a ranking this version of the game can read.
 *
 * @param history the ranking to check, typically just read from the save
 * @return true if it can be used as is, false if it must be initialized first
 */
bool run_history_is_valid(const RunHistory* history);

/**
 * @brief Count a finished run and add it to the ranking if its score is high enough.
 *
 * A run with a score of 0 is counted but never ranked. A run that ties with ranked ones is
 * placed after them.
 *
 * @param history the ranking to update
 * @param record the finished run, its `run_number` is ignored and set by this function
 * @return the rank of the run starting at 0, or RUN_HISTORY_NOT_RANKED
 */
int run_history_add(RunHistory* history, RunRecord record);

#endif // RUN_HISTORY_H
