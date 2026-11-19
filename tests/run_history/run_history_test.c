#include <assert.h>
#include <run_history.h>
#include <stdio.h>
#include <string.h>

static RunRecord record_with_score(uint32_t score)
{
    RunRecord record = {.best_hand_score = score, .seed = score * 7, .ante = 1, .deck = 0};
    return record;
}

static void fill_ranking(RunHistory* history)
{
    run_history_init(history);

    // Scores 1500, 1400, ... 100
    for (int i = RUN_HISTORY_MAX_RECORDS; i > 0; i--)
        run_history_add(history, record_with_score(i * 100));
}

void test_init_and_validity(void)
{
    RunHistory history;

    // SRAM is filled with 1s before anything is saved
    memset(&history, 0xFF, sizeof(history));
    assert(!run_history_is_valid(&history));

    run_history_init(&history);
    assert(run_history_is_valid(&history));
    assert(history.nb_records == 0);
    assert(history.nb_runs_played == 0);

    history.nb_records = RUN_HISTORY_MAX_RECORDS + 1;
    assert(!run_history_is_valid(&history));
}

void test_add_keeps_highest_scores_first(void)
{
    RunHistory history;
    run_history_init(&history);

    assert(run_history_add(&history, record_with_score(100)) == 0);
    assert(run_history_add(&history, record_with_score(300)) == 0);
    assert(run_history_add(&history, record_with_score(200)) == 1);

    assert(history.nb_records == 3);
    assert(history.records[0].best_hand_score == 300);
    assert(history.records[1].best_hand_score == 200);
    assert(history.records[2].best_hand_score == 100);

    // The other fields move with their score
    assert(history.records[0].seed == 300 * 7);
}

void test_add_numbers_runs(void)
{
    RunHistory history;
    run_history_init(&history);

    run_history_add(&history, record_with_score(100));
    run_history_add(&history, record_with_score(300));

    assert(history.nb_runs_played == 2);
    assert(history.records[0].run_number == 2);
    assert(history.records[1].run_number == 1);
}

void test_add_places_ties_after_ranked_runs(void)
{
    RunHistory history;
    run_history_init(&history);

    run_history_add(&history, record_with_score(200));
    assert(run_history_add(&history, record_with_score(200)) == 1);

    assert(history.records[0].run_number == 1);
    assert(history.records[1].run_number == 2);
}

void test_add_counts_but_does_not_rank_zero_scores(void)
{
    RunHistory history;
    run_history_init(&history);

    assert(run_history_add(&history, record_with_score(0)) == RUN_HISTORY_NOT_RANKED);

    assert(history.nb_records == 0);
    assert(history.nb_runs_played == 1);
}

void test_add_to_full_ranking(void)
{
    RunHistory history;
    fill_ranking(&history);
    assert(history.nb_records == RUN_HISTORY_MAX_RECORDS);

    // Too low: the ranking is unchanged
    assert(run_history_add(&history, record_with_score(50)) == RUN_HISTORY_NOT_RANKED);
    assert(history.nb_records == RUN_HISTORY_MAX_RECORDS);
    assert(history.records[RUN_HISTORY_MAX_RECORDS - 1].best_hand_score == 100);

    // High enough: the lowest score drops out
    assert(run_history_add(&history, record_with_score(1450)) == 1);
    assert(history.nb_records == RUN_HISTORY_MAX_RECORDS);
    assert(history.records[0].best_hand_score == 1500);
    assert(history.records[1].best_hand_score == 1450);
    assert(history.records[2].best_hand_score == 1400);
    assert(history.records[RUN_HISTORY_MAX_RECORDS - 1].best_hand_score == 200);

    // New lowest rank
    assert(run_history_add(&history, record_with_score(250)) == RUN_HISTORY_MAX_RECORDS - 1);
    assert(history.records[RUN_HISTORY_MAX_RECORDS - 1].best_hand_score == 250);
}

int main()
{
    printf("Testing Run History init and validity.\n");
    test_init_and_validity();

    printf("Testing Run History ranking.\n");
    test_add_keeps_highest_scores_first();
    test_add_numbers_runs();
    test_add_places_ties_after_ranked_runs();
    test_add_counts_but_does_not_rank_zero_scores();
    test_add_to_full_ranking();

    printf("-------------------------------------------------------------------------------\n");
    printf("Run History Tests Passed :)\n");
    printf("-------------------------------------------------------------------------------\n");

    return 0;
}
