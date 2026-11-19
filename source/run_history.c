/**
 * @file run_history.c
 *
 * @brief Implementation of the ranking of the best finished runs.
 */
#include "run_history.h"

#define RUN_HISTORY_MAGIC   0x53524247 // Spells GBRS, tells a saved ranking from junk
#define RUN_HISTORY_VERSION 1

void run_history_init(RunHistory* history)
{
    *history = (RunHistory){.magic = RUN_HISTORY_MAGIC, .version = RUN_HISTORY_VERSION};
}

bool run_history_is_valid(const RunHistory* history)
{
    return history->magic == RUN_HISTORY_MAGIC && history->version == RUN_HISTORY_VERSION &&
           history->nb_records <= RUN_HISTORY_MAX_RECORDS;
}

int run_history_add(RunHistory* history, RunRecord record)
{
    // Number run
    if (history->nb_runs_played < UINT16_MAX)
        history->nb_runs_played++;
    record.run_number = history->nb_runs_played;

    if (record.best_hand_score == 0)
        return RUN_HISTORY_NOT_RANKED;

    // Find rank
    int rank = 0;
    while (rank < history->nb_records &&
           history->records[rank].best_hand_score >= record.best_hand_score)
    {
        rank++;
    }

    if (rank >= RUN_HISTORY_MAX_RECORDS)
        return RUN_HISTORY_NOT_RANKED;

    // Insert record
    // WHY: when the ranking is full its last record is overwritten, which drops the lowest score
    if (history->nb_records < RUN_HISTORY_MAX_RECORDS)
        history->nb_records++;

    for (int i = history->nb_records - 1; i > rank; i--)
        history->records[i] = history->records[i - 1];

    history->records[rank] = record;

    return rank;
}
