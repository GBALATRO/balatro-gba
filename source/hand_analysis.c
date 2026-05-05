#include "hand_analysis.h"

#include "card.h"
#include "game.h"

void get_hand_distribution(u8 ranks_out[NUM_RANKS], u8 suits_out[NUM_SUITS])
{
    for (int i = 0; i < NUM_RANKS; i++)
        ranks_out[i] = 0;
    for (int i = 0; i < NUM_SUITS; i++)
        suits_out[i] = 0;

    CardObject** cards = get_hand_array();
    int top = get_hand_top();
    for (int i = 0; i <= top; i++)
    {
        if (cards[i] && card_object_is_selected(cards[i]))
        {
            ranks_out[cards[i]->card->rank]++;
            suits_out[cards[i]->card->suit]++;
        }
    }
}

void get_played_distribution(u8 ranks_out[NUM_RANKS], u8 suits_out[NUM_SUITS])
{
    for (int i = 0; i < NUM_RANKS; i++)
        ranks_out[i] = 0;
    for (int i = 0; i < NUM_SUITS; i++)
        suits_out[i] = 0;

    CardObject** played = get_played_array();
    int top = get_played_top();
    for (int i = 0; i <= top; i++)
    {
        /* The difference from get_hand_distribution() (not checking if card is selected)
         * is in line Balatro behavior,
         * see https://github.com/GBALATRO/balatro-gba/issues/341#issuecomment-3691363488
         */
        if (!played[i])
            continue;
        ranks_out[played[i]->card->rank]++;
        suits_out[played[i]->card->suit]++;
    }
}

// Returns the highest N of a kind. So a full-house would return 3.
u8 hand_contains_n_of_a_kind(u8* ranks)
{
    u8 highest_n = 0;
    for (int i = 0; i < NUM_RANKS; i++)
    {
        if (ranks[i] > highest_n)
            highest_n = ranks[i];
    }
    return highest_n;
}

bool hand_contains_two_pair(u8* ranks)
{
    bool contains_other_pair = false;
    for (int i = 0; i < NUM_RANKS; i++)
    {
        if (ranks[i] >= 2)
        {
            if (contains_other_pair)
                return true;
            contains_other_pair = true;
        }
    }
    return false;
}

bool hand_contains_full_house(u8* ranks)
{
    int count_three = 0;
    int count_pair = 0;
    for (int i = 0; i < NUM_RANKS; i++)
    {
        if (ranks[i] >= 3)
        {
            count_three++;
        }
        else if (ranks[i] >= 2)
        {
            count_pair++;
        }
    }
    // Full house if there is:
    // - at least one three-of-a-kind and at least one other pair,
    // - OR at least two three-of-a-kinds (second "three" acts as pair).
    // This accounts for hands with 6 or more cards even though
    // they are currently not possible and probably never will be.
    return (count_three >= 2 || (count_three && count_pair));
}

// This is mostly from Google Gemini
bool hand_contains_straight(u8* ranks)
{
    if (!is_shortcut_joker_active())
    {
        int straight_size = get_straight_and_flush_size();
        // This is the regular case of detecting straights
        int run = 0;
        for (int i = 0; i < NUM_RANKS; ++i)
        {
            if (ranks[i])
            {
                if (++run >= straight_size)
                    return true;
            }
            else
            {
                run = 0;
            }
        }

        // Check for ace low straight
        if (straight_size >= 2 && ranks[ACE])
        {
            // With A as low, the highest rank you can use is FIVE.
            // -1 for inclusive integer distance and another -1 for the Ace e.g. need=5 -> need 2..5
            int last_needed = TWO + (straight_size - 2);
            if (last_needed <= FIVE)
            {
                bool ok = true;
                for (int r = TWO; r <= last_needed; ++r)
                {
                    if (!ranks[r])
                    {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                    return true;
            }
        }

        return false;
    }
    else
    {
        // Shortcut Joker is active, we have to detect straights where any card may "skip" 1 rank
        // We do this with a dynamic programming algorithm that calculates
        // the longest possible straight that can end on each rank
        // and stopping when we find one that is {straight-size} cards long
        u8 longest_short_cut_at[NUM_RANKS] = {0};

        // A low ace can start a sequence. 'ace_low_len' is 1 if an ace is present,
        // acting as a potential predecessor for TWO and THREE.
        int ace_low_len = ranks[ACE] ? 1 : 0;

        // Iterate through all ranks from TWO up to ACE.
        for (int i = 0; i < NUM_RANKS; i++)
        {
            // No cards in this rank, no straight can end here, continue
            if (ranks[i] == 0)
            {
                longest_short_cut_at[i] = 0;
                continue;
            }

            int prev_len1 = 0;
            int prev_len2 = 0;

            // This logic handles the special connections for ace-low straights.
            if (i == TWO)
            {
                // A TWO can be preceded by a low ACE (no skip).
                prev_len1 = ace_low_len;
            }
            else if (i == THREE)
            {
                // A THREE can be preceded by a TWO (no skip) or a low ACE (skip).
                prev_len1 = longest_short_cut_at[TWO];
                prev_len2 = ace_low_len;
            }
            else if (i == ACE)
            {
                // An ACE (as the highest card) can be preceded by a KING or a QUEEN.
                prev_len1 = longest_short_cut_at[KING];
                prev_len2 = longest_short_cut_at[QUEEN];
            }
            else // For all other cards (FOUR through KING).
            {
                // A card can be preceded by the rank directly below or two ranks below.
                prev_len1 = longest_short_cut_at[i - 1];
                prev_len2 = longest_short_cut_at[i - 2];
            }

            // The length of the straight ending at rank 'i' is 1 (for the card itself)
            // plus the length of the longest valid preceding straight.
            longest_short_cut_at[i] = 1 + max(prev_len1, prev_len2);

            // If we've formed a sequence of {straight-size} or more cards, we have a straight.
            if (longest_short_cut_at[i] >= get_straight_and_flush_size())
            {
                return true;
            }
        }
    }

    return false;
}

bool hand_contains_flush(u8* suits)
{
    for (int i = 0; i < NUM_SUITS; i++)
    {
        if (suits[i] >= get_straight_and_flush_size())
        {
            return true;
        }
    }
    return false;
}

// Returns the number of cards in the best flush found
// or 0 if no flush of min_len is found, and marks them in out_selection.
/**
 * Finds the largest flush (set of cards with the same suit) in the given array of played cards.
 * Marks the cards belonging to the best flush in the out_selection array.
 *
 * @param played        Array of pointers to CardObject representing played cards.
 * @param top           Index of the top of the played stack.
 * @param min_len       Minimum number of cards required for a flush.
 * @param out_selection Output array of bools; set to true for cards in the best flush, false
 * otherwise.
 * @return              The number of cards in the best flush found, or 0 if no flush meets min_len.
 */
int find_flush_in_played_cards(CardObject** played, int top, int min_len, bool* out_selection)
{
    if (top < 0)
        return 0;
    for (int i = 0; i <= top; i++)
        out_selection[i] = false;

    int suit_counts[NUM_SUITS] = {0};
    for (int i = 0; i <= top; i++)
    {
        if (played[i] && played[i]->card)
        {
            suit_counts[played[i]->card->suit]++;
        }
    }

    int best_suit = -1;
    int best_count = 0;
    for (int i = 0; i < NUM_SUITS; i++)
    {
        if (suit_counts[i] > best_count)
        {
            best_count = suit_counts[i];
            best_suit = i;
        }
    }

    if (best_count >= min_len)
    {
        for (int i = 0; i <= top; i++)
        {
            if (played[i] && played[i]->card && played[i]->card->suit == best_suit)
            {
                out_selection[i] = true;
            }
        }
        return best_count;
    }
    return 0;
}

// Returns the number of cards in the best straight or 0 if no straight of min_len is found, marks
// as true them in out_selection[]. This is mostly from Google Gemini
int find_straight_in_played_cards(
    CardObject** played,
    int top,
    bool shortcut_active,
    int min_len,
    bool* out_selection
)
{
    if (top < 0)
        return 0;
    for (int i = 0; i <= top; i++)
        out_selection[i] = false;

    // --- Setup for Backtracking DP ---
    u8 longest_straight_at[NUM_RANKS] = {0};
    int parent[NUM_RANKS];
    for (int i = 0; i < NUM_RANKS; i++)
        parent[i] = -1;

    u8 ranks[NUM_RANKS] = {0};
    for (int i = 0; i <= top; i++)
    {
        if (played[i] && played[i]->card)
        {
            ranks[played[i]->card->rank]++;
        }
    }

    // --- Run DP to find longest straight ---
    // This is nearly identical to hand_contains_straight() logic
    // TODO: Consolidate functions to avoid code duplication?
    // Might cost performance because this does a little more
    int ace_low_len = ranks[ACE] ? 1 : 0;
    for (int i = 0; i < NUM_RANKS; i++)
    {
        if (ranks[i] > 0)
        {
            int prev1 = 0, prev2 = 0;
            int parent1 = -1, parent2 = -1;

            if (shortcut_active)
            {
                if (i == TWO)
                {
                    prev1 = ace_low_len;
                    parent1 = ACE;
                }
                else if (i == THREE)
                {
                    prev1 = longest_straight_at[TWO];
                    parent1 = TWO;
                    prev2 = ace_low_len;
                    parent2 = ACE;
                }
                else if (i == ACE)
                {
                    prev1 = longest_straight_at[KING];
                    parent1 = KING;
                    prev2 = longest_straight_at[QUEEN];
                    parent2 = QUEEN;
                }
                else
                {
                    prev1 = longest_straight_at[i - 1];
                    parent1 = i - 1;
                    if (i > 1)
                    {
                        prev2 = longest_straight_at[i - 2];
                        parent2 = i - 2;
                    }
                }
            }
            else
            {
                if (i == TWO)
                {
                    prev1 = ace_low_len;
                    parent1 = ACE;
                }
                else if (i == ACE)
                {
                    prev1 = longest_straight_at[KING];
                    parent1 = KING;
                }
                else
                {
                    prev1 = longest_straight_at[i - 1];
                    parent1 = i - 1;
                }
            }

            // Parallels longest_short_cut_at[i] = 1 + max(prev_len1, prev_len2);
            // in hand_contains_straight()
            if (prev1 >= prev2)
            {
                longest_straight_at[i] = 1 + prev1;
                parent[i] = parent1;
            }
            else
            {
                longest_straight_at[i] = 1 + prev2;
                parent[i] = parent2;
            }
        }
    }

    // --- Find best straight and backtrack ---
    int best_len = 0;
    int end_rank = -1;
    for (int i = 0; i < NUM_RANKS; i++)
    {
        if (longest_straight_at[i] >= best_len)
        {
            best_len = longest_straight_at[i];
            end_rank = i;
        }
    }

    if (best_len >= min_len)
    {
        u8 needed_ranks[NUM_RANKS] = {0};
        int current_rank = end_rank;
        while (current_rank != -1 && best_len > 0)
        {
            needed_ranks[current_rank]++;
            current_rank = parent[current_rank];
            best_len--;
        }

        for (int i = 0; i <= top; i++)
        {
            if (played[i] && played[i]->card && needed_ranks[played[i]->card->rank] > 0)
            {
                out_selection[i] = true;
                needed_ranks[played[i]->card->rank]--;
            }
        }

        int final_card_count = 0;
        for (int i = 0; i <= top; i++)
        {
            if (out_selection[i])
                final_card_count++;
        }
        return final_card_count;
    }
    return 0;
}

// This is used for the special case in "Four Fingers" where you can add a pair into a straight
// (e.g. AA234 should score all 5 cards)
void select_paired_cards_in_hand(CardObject** played, int played_top, bool* selection)
{
    // Build a set of ranks that are already selected
    bool rank_selected[NUM_RANKS] = {0};
    bool any_selected_rank = false;

    for (int i = 0; i <= played_top; i++)
    {
        if (selection[i] && played[i] && played[i]->card)
        {
            rank_selected[played[i]->card->rank] = true;
            any_selected_rank = true;
        }
    }

    // If no ranks were selected initially, nothing to do
    if (!any_selected_rank)
        return;

    // Add any unselected card to the selection if if shares a rank with the selected ranks
    for (int i = 0; i <= played_top; i++)
    {
        if (played[i] && played[i]->card && !selection[i])
        {
            if (rank_selected[played[i]->card->rank])
            {
                selection[i] = true;
            }
        }
    }
}
