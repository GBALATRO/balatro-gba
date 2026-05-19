#include "hand.h"

#include "audio_utils.h"
#include "card.h"
#include "game.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "soundbank.h"
#include "util.h"

#include <tonc.h>

typedef struct
{
    u32 chips;
    u32 mult;
    char* display_name;
} HandValues;

static const HandValues hand_base_values[] = {
    {.chips = 0,   .mult = 0,  .display_name = NULL     }, // NONE
    {.chips = 5,   .mult = 1,  .display_name = "Hi-Card"}, // HIGH_CARD
    {.chips = 10,  .mult = 2,  .display_name = "Pair"   }, // PAIR
    {.chips = 20,  .mult = 2,  .display_name = "2 Pair" }, // TWO_PAIR
    {.chips = 30,  .mult = 3,  .display_name = "3 OAK"  }, // THREE_OF_A_KIND
    {.chips = 30,  .mult = 4,  .display_name = "Strt"   }, // STRAIGHT
    {.chips = 35,  .mult = 4,  .display_name = "Flush"  }, // FLUSH
    {.chips = 40,  .mult = 4,  .display_name = "Full H" }, // FULL_HOUSE
    {.chips = 60,  .mult = 7,  .display_name = "4 OAK"  }, // FOUR_OF_A_KIND
    {.chips = 100, .mult = 8,  .display_name = "Strt F" }, // STRAIGHT_FLUSH
    {.chips = 100, .mult = 8,  .display_name = "Royal F"}, // ROYAL_FLUSH
    {.chips = 120, .mult = 12, .display_name = "5 OAK"  }, // FIVE_OF_A_KIND
    {.chips = 140, .mult = 14, .display_name = "Flush H"}, // FLUSH_HOUSE
    {.chips = 160, .mult = 16, .display_name = "Flush 5"}  // FLUSH_FIVE
};

// clang-format off
// Rects for TTE (in pixels)        left   top    right  bottom
static const Rect HAND_TYPE_RECT = {8,     64,    64,    72};
// clang-format on

// Hand stack
static enum HandState hand_state = HAND_DRAW;

static CardObject* hand[MAX_HAND_SIZE] = {NULL};
static bool sort_by_suit = false;

// Hand Type
static enum HandType hand_type = NONE;
static ContainedHandTypes contained_hands = {0};

// Forward declarations
static ContainedHandTypes compute_contained_hand_types(void);
static enum HandType compute_hand_type(struct ContainedHandTypes contained_types);

// Hand Struct Manipulation

enum HandState get_hand_state(void)
{
    return hand_state;
}

void set_hand_state(enum HandState new_hand_state)
{
    hand_state = new_hand_state;
}

CardObject** get_hand_array(void)
{
    return hand;
}

int hand_get_size(void)
{
    return g_game_vars.hand_top + 1;
}

enum HandType get_hand_type(void)
{
    return hand_type;
}

ContainedHandTypes* get_contained_hands(void)
{
    return &contained_hands;
}

static void print_hand_type(const char* hand_type_str)
{
    if (hand_type_str == NULL)
        return; // NULL-checking paranoia

    Rect hand_type_rect = HAND_TYPE_RECT;
    update_text_rect_to_center_str(&hand_type_rect, hand_type_str, SCREEN_LEFT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        hand_type_rect.left,
        hand_type_rect.top,
        TTE_WHITE_PB,
        hand_type_str
    );
}

void compute_hand_value_info(void)
{
    tte_erase_rect_wrapper(HAND_TYPE_RECT);
    contained_hands = compute_contained_hand_types();
    hand_type = compute_hand_type(contained_hands);

    HandValues hand = hand_base_values[hand_type];

    set_chips(hand.chips);
    set_mult(hand.mult);

    print_hand_type(hand.display_name);
    display_chips();
    display_mult();
}

// idx_a and idx_b are assumed to be valid indexes within the hand array
// no checks will be performed here for performance's sake
void swap_cards_in_hand(int idx_a, int idx_b)
{
    CardObject* temp = hand[idx_a];
    hand[idx_a] = hand[idx_b];
    hand[idx_b] = temp;
}

static inline void sort_hand_by_suit(void)
{
    for (int idx_a = 0; idx_a < g_game_vars.hand_top; idx_a++)
    {
        for (int idx_b = idx_a + 1; idx_b <= g_game_vars.hand_top; idx_b++)
        {
            if (hand[idx_a] == NULL ||
                (hand[idx_b] != NULL && (hand[idx_a]->card->suit > hand[idx_b]->card->suit ||
                                         (hand[idx_a]->card->suit == hand[idx_b]->card->suit &&
                                          hand[idx_a]->card->rank > hand[idx_b]->card->rank))))
            {
                swap_cards_in_hand(idx_a, idx_b);
            }
        }
    }
}

static inline void sort_hand_by_rank(void)
{
    for (int idx_a = 0; idx_a < g_game_vars.hand_top; idx_a++)
    {
        for (int idx_b = idx_a + 1; idx_b <= g_game_vars.hand_top; idx_b++)
        {
            if (hand[idx_a] == NULL ||
                (hand[idx_b] != NULL && hand[idx_a]->card->rank > hand[idx_b]->card->rank))
            {
                swap_cards_in_hand(idx_a, idx_b);
            }
        }
    }
}

static inline bool shift_null_card_to_end(int null_card_idx)
{
    // Start by searching any non NULL cards after the NULL one
    // don't start at null_card_idx+1 to avoid potential illegal array access
    int non_null_card_idx = null_card_idx;
    for (; non_null_card_idx <= g_game_vars.hand_top; non_null_card_idx++)
    {
        if (hand[non_null_card_idx] != NULL)
        {
            break;
        }
    }

    // return false if there are no non-NULL cards left/there are no more sprites to destroy
    if (non_null_card_idx > g_game_vars.hand_top)
    {
        return false;
    }

    // If there is one, shift it and all the cards that follow forward
    // This way we close the gap and ensure the next card is not NULL

    // Iterating up to `hand_top - non_null_card_idx + 1` should end up out of bounds
    // but for some reason it doesn't pose any issue, and taking out the +1 breaks
    // the code, so I'll be elaving it here until someone figures it out ^^'
    for (int j = 0; j <= g_game_vars.hand_top - non_null_card_idx + 1; j++)
    {
        hand[null_card_idx + j] = hand[non_null_card_idx + j];
    }

    return true;
}

void reorder_card_sprites_layers(void)
{
    // Update the sprites in the hand by destroying them and creating new ones in the correct order
    // (This feels like a diabolical solution but like literally how else would you do this)
    for (int i = 0; i <= g_game_vars.hand_top; i++)
    {
        // a NULL card will only happen if we rearrange the sprites without having sorted them
        // before. Any NULL CardObject will be sent to the end by shifting all elements forward
        if (hand[i] == NULL)
        {
            if (!shift_null_card_to_end(i))
            {
                break;
            }
        }

        // card_object_get_sprite() will not work here since we need the address
        sprite_destroy(&(hand[i]->sprite_object->sprite));
    }

    // Recreate the sprites for the remaining non NULL cards, in order
    for (int i = 0; i <= g_game_vars.hand_top; i++)
    {
        if (hand[i] != NULL)
        {
            // Set the sprite for the card object
            card_object_set_sprite(hand[i], i);
            sprite_position(
                card_object_get_sprite(hand[i]),
                fx2int(hand[i]->sprite_object->x),
                fx2int(hand[i]->sprite_object->y)
            );
        }
    }
}

void sort_cards()
{
    if (sort_by_suit)
    {
        sort_hand_by_suit();
    }
    else
    {
        sort_hand_by_rank();
    }

    reorder_card_sprites_layers();
}

void hand_change_sort(bool to_sort_by_suit)
{
    if (to_sort_by_suit != sort_by_suit)
    {
        sort_by_suit = to_sort_by_suit;
        sort_cards();
    }
}

void hand_select_card(int index)
{
    if (index < 0 || index >= hand_get_size() || hand_state != HAND_SELECT || hand[index] == NULL)
        return;

    if (card_object_is_selected(hand[index]))
    {
        card_object_set_selected(hand[index], false);
        g_game_vars.hand_selections--;
        play_sfx(SFX_CARD_DESELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
    else if (g_game_vars.hand_selections < MAX_SELECTION_SIZE)
    {
        card_object_set_selected(hand[index], true);
        g_game_vars.hand_selections++;
        play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
    compute_hand_value_info();
}

void hand_deselect_all_cards(void)
{
    bool any_cards_deselected = false;
    for (int i = 0; i <= g_game_vars.hand_top; i++)
    {
        if (card_object_is_selected(hand[i]))
        {
            card_object_set_selected(hand[i], false);
            g_game_vars.hand_selections--;
            any_cards_deselected = true;
        }
    }

    if (any_cards_deselected)
    {
        play_sfx(SFX_CARD_DESELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
}

// Hand Analysis

/**
 * @brief Outputs the distribution of ranks and suits in the hand
 * @param ranks_out output - updated such as ranks_out[rank] is the number of cards of rank in the
 *                  hand. Must be of size NUM_RANKS.
 * @param suits_out output - updated such as suits_out[suit] is the number of cards if suit in the
 *                  hand Must be of size NUM_SUITS
 */
static void get_hand_distribution(u8 ranks_out[NUM_RANKS], u8 suits_out[NUM_SUITS])
{
    for (int i = 0; i < NUM_RANKS; i++)
        ranks_out[i] = 0;
    for (int i = 0; i < NUM_SUITS; i++)
        suits_out[i] = 0;

    int top = g_game_vars.hand_top;
    for (int i = 0; i <= top; i++)
    {
        if (hand[i] && card_object_is_selected(hand[i]))
        {
            ranks_out[hand[i]->card->rank]++;
            suits_out[hand[i]->card->suit]++;
        }
    }
}

/**
 * @brief Outputs the distribution of ranks and suits in the played stack
 * @param ranks_out output - updated such as ranks_out[rank] is the number of cards of rank in the
 *                  played stack. Must be of size NUM_RANKS.
 * @param suits_out output - updated such as suits_out[suit] is the number of cards if suit in the
 *                  played stack. Must be of size NUM_SUITS
 */
GBAL_UNUSED
static void get_played_distribution(u8 ranks_out[NUM_RANKS], u8 suits_out[NUM_SUITS])
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
static u8 hand_contains_n_of_a_kind(u8* ranks)
{
    u8 highest_n = 0;
    for (int i = 0; i < NUM_RANKS; i++)
    {
        if (ranks[i] > highest_n)
            highest_n = ranks[i];
    }
    return highest_n;
}

static bool hand_contains_two_pair(u8* ranks)
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

static bool hand_contains_full_house(u8* ranks)
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
static bool hand_contains_straight(u8* ranks)
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

static bool hand_contains_flush(u8* suits)
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

static ContainedHandTypes compute_contained_hand_types(void)
{
    ContainedHandTypes hand_types = {0};

    // Idk if this is how Balatro does it but this is how I'm doing it
    if (g_game_vars.hand_selections == 0 || hand_state == HAND_DISCARD)
    {
        return hand_types;
    }

    hand_types.HIGH_CARD = 1;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_hand_distribution(ranks, suits);

    // The following can be optimized better but not sure how much it matters
    u8 n_of_a_kind = hand_contains_n_of_a_kind(ranks);

    // Pair and 2 Pair
    if (n_of_a_kind >= 2)
    {
        hand_types.PAIR = 1;

        if (hand_contains_two_pair(ranks))
        {
            hand_types.TWO_PAIR = 1;
        }
    }

    // 3 OAK
    if (n_of_a_kind >= 3)
    {
        hand_types.THREE_OF_A_KIND = 1;
    }

    // Straight
    if (hand_contains_straight(ranks))
    {
        hand_types.STRAIGHT = 1;
    }

    // Flush
    if (hand_contains_flush(suits))
    {
        hand_types.FLUSH = 1;
    }

    // Full House
    if (n_of_a_kind >= 3 && hand_contains_full_house(ranks))
    {
        hand_types.FULL_HOUSE = 1;
    }

    // 4 OAK
    if (n_of_a_kind >= 4)
    {
        hand_types.FOUR_OF_A_KIND = 1;
    }

    // Straight Flush
    if (hand_types.STRAIGHT && hand_types.FLUSH)
    {
        hand_types.STRAIGHT_FLUSH = 1;
    }

    // Royal Flush
    if (hand_types.STRAIGHT_FLUSH)
    {
        if (ranks[TEN] && ranks[JACK] && ranks[QUEEN] && ranks[KING] && ranks[ACE])
        {
            hand_types.ROYAL_FLUSH = 1;
        }
    }

    // 5 OAK
    if (n_of_a_kind >= 5)
    {
        hand_types.FIVE_OF_A_KIND = 1;
    }

    // Flush House and Five
    if (hand_types.FLUSH)
    {
        if (hand_types.FULL_HOUSE)
        {
            hand_types.FLUSH_HOUSE = 1;
        }

        if (hand_types.FIVE_OF_A_KIND)
        {
            hand_types.FLUSH_FIVE = 1;
        }
    }

    return hand_types;
}

static enum HandType compute_hand_type(struct ContainedHandTypes contained_types)
{
    enum HandType ret;

    // test each pit see if it's set to 1, and return the first one
    for (ret = FLUSH_FIVE; ret > NONE; ret--)
    {
        // Shift the bit we want to check to the front and mask it with 1 to keep only that
        // Since the ContainedHandTypes is ordered the same way as the HandType enum, we
        // can shift right by ret-1 to have the bit we want at the front
        if ((contained_types.value >> (ret - 1)) & 0x1)
        {
            break;
        }
    }

    // If we broke early, ret contains the value of the HandType enum corresponding to
    // the position of the highest bit set to 1 in contained_types.value, which is the
    // most powerful poker hand contained in the current Hand
    // If not, then it contains NONE, which is what we're supposed to return when there
    // are no Hands contained in what we played
    return ret;
}
