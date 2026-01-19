#include "game/round.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "background_gfx.h"
#include "blind.h"
#include "button.h"
#include "card.h"
#include "game.h"
#include "game/common_ui.h"
#include "game/palette.h"
#include "game/point.h"
#include "game/rect.h"
#include "game/shop.h"
#include "game/timer.h"
#include "graphic_utils.h"
#include "hand_analysis.h"
#include "joker.h"
#include "list.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "sprite.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <tonc.h>
#include <tonc_memdef.h>

// Sound pitch steps for SFX
#define PITCH_STEP_DISCARD_SFX   (-64)
#define PITCH_STEP_DRAW_SFX      24
#define PITCH_STEP_UNDISCARD_SFX (2 * PITCH_STEP_DRAW_SFX)

// Selection grid row indices
#define GAME_PLAYING_HAND_SEL_Y 1

// Flaming score animation frames
#define SCORE_FLAMES_ANIM_FREQ  5 // animation will run at 12FPS
#define NUM_SCORE_FLAMES_FRAMES 8 // Chips and Mult flame frames are next to one another
#define SCORE_FLAME_FRAME_WIDTH 3 // so we only need to offset to get the next ones

/* This needs to stay a power of 2 and small enough
 * for the lerping to be done before the next hand is drawn.
 */
#define NUM_SCORE_LERP_STEPS   16
#define TM_SCORE_LERP_INTERVAL 2

// Pixel sizes
#define SCORED_CARD_TEXT_Y 48

#define CARD_FOCUSED_UNSEL_Y 10
#define CARD_UNFOCUSED_SEL_Y 15
#define CARD_FOCUSED_SEL_Y   20

static const int HAND_SPACING_LUT[MAX_HAND_SIZE] =
    {28, 28, 28, 28, 27, 21, 18, 15, 13, 12, 10, 9, 9, 8, 8, 7};

static int hand_selections = 0;

typedef struct
{
    u32 chips;
    u32 mult;
    char* display_name;
} HandValues;

static const HandValues hand_base_values[] = {
    {.chips = 0,   .mult = 0,  .display_name = NULL     }, // NONE
    {.chips = 5,   .mult = 1,  .display_name = "HIGH C" }, // HIGH_CARD
    {.chips = 10,  .mult = 2,  .display_name = "PAIR"   }, // PAIR
    {.chips = 20,  .mult = 2,  .display_name = "2 PAIR" }, // TWO_PAIR
    {.chips = 30,  .mult = 3,  .display_name = "3 OAK"  }, // THREE_OF_A_KIND
    {.chips = 60,  .mult = 7,  .display_name = "4 OAK"  }, // FOUR_OF_A_KIND
    {.chips = 30,  .mult = 4,  .display_name = "STRT"   }, // STRAIGHT
    {.chips = 35,  .mult = 4,  .display_name = "FLUSH"  }, // FLUSH
    {.chips = 40,  .mult = 4,  .display_name = "FULL H" }, // FULL_HOUSE
    {.chips = 100, .mult = 8,  .display_name = "STRT F" }, // STRAIGHT_FLUSH
    {.chips = 100, .mult = 8,  .display_name = "ROYAL F"}, // ROYAL_FLUSH
    {.chips = 120, .mult = 12, .display_name = "5 OAK"  }, // FIVE_OF_A_KIND
    {.chips = 140, .mult = 14, .display_name = "FLUSH H"}, // FLUSH_HOUSE
    {.chips = 160, .mult = 16, .display_name = "FLUSH 5"}  // FLUSH_FIVE
};

static int hand_size = 8;
static int cards_drawn = 0;

u32 chips = 0;
u32 mult = 0;
bool retrigger = false;

// discarded cards specific
bool sound_played = false;
bool discarded_card = false;

// Keeping track of cards scored
static int scored_card_index = 0;

// The current game state
enum HandState hand_state = HAND_DRAW;
enum PlayState play_state = PLAY_STARTING;

enum HandType hand_type = NONE;

static bool sort_by_suit = false;

// Keeping track of what Jokers are scored at each step
ListItr _joker_scored_itr;
ListItr _joker_card_scored_end_itr;
ListItr _joker_round_end_itr;

// card moving logic

// true if and only if we are currently moving a card around
static bool moving_card = false;

// This will prevent us from moving cards around if we selected one
// by moving too fast after pressing the A button
static bool card_moved_too_fast = false;
static bool card_selected_instead_of_moved = false;

// After pressing A, if we press Left/Right too fast, we should select the card
// and change focus to the next one, instead of swapping them
// This should fix inputs sometimes not registering when quickly selecting cards
static const int card_swap_time_threshold = 6;
static uint selection_hit_timer = TM_ZERO;

// Getters/Setters

void set_retrigger(bool value)
{
    retrigger = value;
}

u32 get_chips(void)
{
    return chips;
}

void set_chips(u32 new_chips)
{
    chips = new_chips;
}

u32 get_mult(void)
{
    return mult;
}

void set_mult(u32 new_mult)
{
    mult = new_mult;
}

// Forward declarations for static functions defined later in this file
static void set_hand(void);
static void hand_deselect_all_cards(RoundProps* props);
static void hand_change_sort(RoundProps* props);
static void hand_select_card(RoundProps* props, int index);
static inline int hand_sel_idx_to_card_idx(int hand_top, int selection_index);

static void game_playing_play_hand_on_pressed(void* ctx);
static void game_playing_discard_on_pressed(void* ctx);
static bool can_play_hand(void* ctx);
static bool can_discard_hand(void* ctx);
// Array of buttons by horizontal selection index (x)
Button game_playing_buttons[] = {
    {PLAY_HAND_BTN_BORDER_PID, PLAY_HAND_BTN_PID, game_playing_play_hand_on_pressed, can_play_hand   },
    {DISCARD_BTN_BORDER_PID,   DISCARD_BTN_PID,   game_playing_discard_on_pressed,   can_discard_hand},
};

static int game_playing_hand_row_get_size(void*);
static bool game_playing_hand_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection,
    void* ctx
);
static void game_playing_hand_row_on_key_transit(
    SelectionGrid* selection_grid,
    Selection* selection,
    void* ctx
);

static int game_playing_button_row_get_size(void*);
static bool game_playing_button_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection,
    void* ctx
);
static void game_playing_button_row_on_key_hit(
    SelectionGrid* selection_grid,
    Selection* selection,
    void* ctx
);

// clang-format off
SelectionGridRow game_playing_selection_rows[] = {
    {
        0,
        jokers_sel_row_get_size,
        jokers_sel_row_on_selection_changed,
        jokers_sel_row_on_key_transit,
        {.wrap = false}
    },
    {
        1,
        game_playing_hand_row_get_size,
        game_playing_hand_row_on_selection_changed,
        game_playing_hand_row_on_key_transit,
        {.wrap = true}
    },
    {
        2,
        game_playing_button_row_get_size,
        game_playing_button_row_on_selection_changed,
        game_playing_button_row_on_key_hit,
        {.wrap = false}
    }
};
// clang-format on

static const Selection GAME_PLAYING_INIT_SEL = {0, 1};

SelectionGrid game_playing_selection_grid = {
    game_playing_selection_rows,
    NUM_ELEM_IN_ARR(game_playing_selection_rows),
    GAME_PLAYING_INIT_SEL
};

static int game_playing_hand_row_get_size(void* ctx)
{
    RoundProps* props = (RoundProps*)ctx;
    return props->hand_top + 1;
}

static void game_playing_play_hand_on_pressed(void* ctx)
{
    RoundProps* props = (RoundProps*)ctx;

    if (!can_play_hand(ctx))
        return;

    hand_state = HAND_PLAY;
    props->hands--;
    display_hands();

    selection_grid_move_selection_vert(&game_playing_selection_grid, -1, props);
}

// Playing state functions
static void game_playing_discard_on_pressed(void* ctx)
{
    RoundProps* props = (RoundProps*)ctx;
    if (!can_discard_hand(props))
        return;

    hand_state = HAND_DISCARD;
    props->discards--;
    display_hands();
    set_hand();
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d",
        DISCARDS_TEXT_RECT.left,
        DISCARDS_TEXT_RECT.top,
        TTE_RED_PB,
        props->discards
    );

    // Move back to hand selection
    selection_grid_move_selection_vert(&game_playing_selection_grid, -1, props);
}

static int game_playing_button_row_get_size(void* _)
{
    return NUM_ELEM_IN_ARR(game_playing_buttons);
}

int get_scored_card_index(void)
{
    return scored_card_index;
}

// idx_a and idx_b are assumed to be valid indexes within the hand array
// no checks will be performed here for performance's sake
static void swap_cards_in_hand(RoundProps* props, int idx_a, int idx_b)
{
    CardObject* card_object_a = props->hand[idx_a];
    CardObject* card_object_b = props->hand[idx_b];
    props->hand[idx_a] = card_object_b;
    props->hand[idx_b] = card_object_a;
}

static bool shift_null_card_to_end(RoundProps* props, int null_card_idx)
{
    // Start by searching any non NULL cards after the NULL one
    // don't start at null_card_idx+1 to avoid potential illegal array access
    int non_null_card_idx = null_card_idx;
    int hand_top = props->hand_top;
    for (; non_null_card_idx <= hand_top; non_null_card_idx++)
    {
        if (props->hand[non_null_card_idx] != NULL)
        {
            break;
        }
    }

    // return false if there are no non-NULL cards left/there are no more sprites to destroy
    if (non_null_card_idx > hand_top)
    {
        return false;
    }

    // If there is one, shift it and all the cards that follow forward
    // This way we close the gap and ensure the next card is not NULL

    // Iterating up to `hand_top - non_null_card_idx + 1` should end up out of bounds
    // but for some reason it doesn't pose any issue, and taking out the +1 breaks
    // the code, so I'll be elaving it here until someone figures it out ^^'
    for (int j = 0; j <= hand_top - non_null_card_idx + 1; j++)
    {
        props->hand[non_null_card_idx + j] = props->hand[null_card_idx + j];
    }

    return true;
}

void deck_shuffle(RoundProps* props)
{
    for (int i = props->deck_top; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Card* card_i = props->deck[i];
        Card* card_j = props->deck[j];
        props->deck[i] = card_j;
        props->deck[j] = card_i;
    }
}

void sort_hand_by_suit(RoundProps* props)
{
    int hand_top = props->hand_top;
    for (int idx_a = 0; idx_a < hand_top; idx_a++)
    {
        for (int idx_b = idx_a + 1; idx_b <= hand_top; idx_b++)
        {
            CardObject* card_a = props->hand[idx_a];
            CardObject* card_b = props->hand[idx_b];
            if (card_a == NULL || (card_b != NULL && (card_a->card->suit > card_b->card->suit ||
                                                      (card_a->card->suit == card_b->card->suit &&
                                                       card_a->card->rank > card_b->card->rank))))
            {
                swap_cards_in_hand(props, idx_a, idx_b);
            }
        }
    }
}

void sort_hand_by_rank(RoundProps* props)
{
    int hand_top = props->hand_top;
    for (int idx_a = 0; idx_a < hand_top; idx_a++)
    {
        for (int idx_b = idx_a + 1; idx_b <= hand_top; idx_b++)
        {
            CardObject* card_a = props->hand[idx_a];
            CardObject* card_b = props->hand[idx_b];
            if (card_a == NULL || (card_b != NULL && card_a->card->rank > card_b->card->rank))
            {
                swap_cards_in_hand(props, idx_a, idx_b);
            }
        }
    }
}

static inline enum HandType hand_get_type(void)
{
    enum HandType res_hand_type = NONE;

    // Idk if this is how Balatro does it but this is how I'm doing it
    if (hand_selections == 0 || hand_state == HAND_DISCARD)
    {
        res_hand_type = NONE;
        return res_hand_type;
    }

    res_hand_type = HIGH_CARD;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_hand_distribution(ranks, suits);

    // Check for flush
    if (hand_contains_flush(suits))
        res_hand_type = FLUSH;

    // Check for straight
    if (hand_contains_straight(ranks))
    {
        if (res_hand_type == FLUSH)
            res_hand_type = STRAIGHT_FLUSH;
        else
            res_hand_type = STRAIGHT;
    }

    // The following can be optimized better but not sure how much it matters
    u8 n_of_a_kind = hand_contains_n_of_a_kind(ranks);

    if (n_of_a_kind >= 5)
    {
        if (res_hand_type == FLUSH)
        {
            return FLUSH_FIVE;
        }
        return FIVE_OF_A_KIND;
    }

    // Check for royal flush vs regular straight flush
    if (res_hand_type == STRAIGHT_FLUSH)
    {
        if (ranks[TEN] && ranks[JACK] && ranks[QUEEN] && ranks[KING] && ranks[ACE])
            return ROYAL_FLUSH;
        return STRAIGHT_FLUSH;
    }

    if (n_of_a_kind == 4)
    {
        return FOUR_OF_A_KIND;
    }

    if (n_of_a_kind == 3 && hand_contains_full_house(ranks))
    {
        return FULL_HOUSE;
    }

    // Flush and Straight are more valuable than the remaining hand types, so return them now
    if (res_hand_type == FLUSH)
    {
        if (n_of_a_kind >= 5)
        {
            return FLUSH_HOUSE;
        }
        return FLUSH;
    }
    if (res_hand_type == STRAIGHT)
    {
        return STRAIGHT;
    }

    if (n_of_a_kind == 3)
    {
        return THREE_OF_A_KIND;
    }

    if (n_of_a_kind == 2)
    {
        if (hand_contains_two_pair(ranks))
        {
            return TWO_PAIR;
        }
        return PAIR;
    }

    return res_hand_type; // should be HIGH_CARD
}

static void print_hand_type(const char* hand_type_str)
{
    if (hand_type_str == NULL)
        return; // NULL-checking paranoia
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        HAND_TYPE_RECT.left,
        HAND_TYPE_RECT.top,
        TTE_WHITE_PB,
        hand_type_str
    );
}

static void set_hand(void)
{
    tte_erase_rect_wrapper(HAND_TYPE_RECT);
    hand_type = hand_get_type();

    HandValues hand = hand_base_values[hand_type];

    chips = hand.chips;
    mult = hand.mult;

    print_hand_type(hand.display_name);
    display_chips();
    display_mult();
}

static void reorder_card_sprites_layers(RoundProps* props)
{
    // Update the sprites in the hand by destroying them and creating new ones in the correct order
    // (This feels like a diabolical solution but like literally how else would you do this)
    int hand_top = props->hand_top;
    for (int i = 0; i <= hand_top; i++)
    {
        // a NULL card will only happen if we rearrange the sprites without having sorted them
        // before. Any NULL CardObject will be sent to the end by shifting all elements forward
        CardObject* card_object = props->hand[i];
        if (card_object == NULL)
        {
            if (!shift_null_card_to_end(props, i))
            {
                break;
            }
        }

        // card_object_get_sprite() will not work here since we need the address
        sprite_destroy(&(card_object->sprite_object->sprite));
    }

    // Recreate the sprites for the remaining non NULL cards, in order
    for (int i = 0; i <= hand_top; i++)
    {
        CardObject* card_object = props->hand[i];
        if (card_object != NULL)
        {
            // Set the sprite for the card object
            card_object_set_sprite(card_object, i);
            sprite_position(
                card_object_get_sprite(card_object),
                fx2int(card_object->sprite_object->x),
                fx2int(card_object->sprite_object->y)
            );
        }
    }
}

static void sort_cards(RoundProps* props)
{
    if (sort_by_suit)
    {
        sort_hand_by_suit(props);
    }
    else
    {
        sort_hand_by_rank(props);
    }

    reorder_card_sprites_layers(props);
}

void game_round_on_init(void* ctx)
{
    RoundProps* props = (RoundProps*)ctx;

    hand_state = HAND_DRAW;
    cards_drawn = 0;
    hand_selections = 0;

    int current_blind = props->current_blind;
    int ante = props->ante;

    Sprite* playing_blind_token_sprite = blind_token_new(
        current_blind,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        MAX_SELECTION_SIZE + MAX_HAND_SIZE + 1
    ); // Create the blind token sprite at the top left corner
    set_playing_blind_token(playing_blind_token_sprite);

    // TODO: Hide blind token and display it after sliding blind rect animation
    // hide_playing_blind_token();

    Sprite* round_end_blind_token_sprite = blind_token_new(
        current_blind,
        81,
        86,
        MAX_SELECTION_SIZE + MAX_HAND_SIZE + 2
    ); // Create the blind token sprite for round end
    set_round_end_blind_token(round_end_blind_token_sprite);
    hide_round_end_blind_token();

    Rect blind_req_text_rect = BLIND_REQ_TEXT_RECT;
    u32 blind_requirement = blind_get_requirement(current_blind, ante);

    char blind_req_str_buff[UINT_MAX_DIGITS + 1];

    truncate_uint_to_suffixed_str(
        blind_requirement,
        rect_width(&BLIND_REQ_TEXT_RECT) / TTE_CHAR_SIZE,
        blind_req_str_buff
    );

    // Update text rect for right alignment AFTER shortening the number
    update_text_rect_to_right_align_str(&blind_req_text_rect, blind_req_str_buff, OVERFLOW_RIGHT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        blind_req_text_rect.left,
        blind_req_text_rect.top,
        TTE_RED_PB,
        blind_req_str_buff
    );
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}$%d",
        BLIND_REWARD_RECT.left,
        BLIND_REWARD_RECT.top,
        TTE_YELLOW_PB,
        blind_get_reward(current_blind)
    ); // Blind reward

    deck_shuffle(props); // Shuffle the deck at the start of the round

    /* Note that since cards_in_hand_update_loop() handles card highlight there's no need
     * to call a selection changed callback to highlight the initial card, this wouldn't work
     * otherwise or for the buttons.
     */
    game_playing_selection_grid.selection = GAME_PLAYING_INIT_SEL;
}

static bool game_playing_hand_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection,
    void* ctx
)
{
    RoundProps* props = (RoundProps*)ctx;
    int prev_card_idx = UNDEFINED;
    int next_card_idx = UNDEFINED;

    // Do not use FRAMES(x) here as we are counting real frames ignoring game speed
    card_moved_too_fast = (props->timer - selection_hit_timer) < card_swap_time_threshold;

    if (prev_selection->y == GAME_PLAYING_HAND_SEL_Y)
    {
        prev_card_idx = hand_sel_idx_to_card_idx(props->hand_top, prev_selection->x);
    }

    if (new_selection->y == GAME_PLAYING_HAND_SEL_Y)
    {
        next_card_idx = hand_sel_idx_to_card_idx(props->hand_top, new_selection->x);
    }

    bool on_the_same_row = new_selection->y == prev_selection->y; // == GAME_PLAYING_HAND_SEL_Y

    if (on_the_same_row && key_is_down(SELECT_CARD) && !card_moved_too_fast &&
        !card_selected_instead_of_moved)
    {
        bool moved_by_one_tile = abs(new_selection->x - prev_selection->x) == 1;

        // Avoid swapping when selection wraps
        if (!moved_by_one_tile)
        {
            // Abort the selection if swapping so it doesn't wrap
            return false;
        }
        else
        {
            swap_cards_in_hand(props, prev_card_idx, next_card_idx);
            moving_card = true;
            reorder_card_sprites_layers(props);

            /* Not calling sprite_object_set_focus() because focus is handled by
             * cards_in_hand_update_loop() based on the selection grid value...
             */
            play_sfx(
                SFX_CARD_FOCUS,
                MM_BASE_PITCH_RATE + rand() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
                SFX_DEFAULT_VOLUME
            );
        }
    }
    else
    {
        // select current card if we tried moving it too fast
        if (key_released(SELECT_CARD) || (card_moved_too_fast && !moving_card))
        {
            hand_select_card(props, prev_card_idx);
            card_selected_instead_of_moved = true;
        }
        if (next_card_idx != UNDEFINED)
        {
            /* Not calling sprite_object_set_focus() because focus is handled by
             * cards_in_hand_update_loop() based on the selection grid value...
             */
            play_sfx(
                SFX_CARD_FOCUS,
                MM_BASE_PITCH_RATE + rand() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
                SFX_DEFAULT_VOLUME
            );
        }
    }

    return true;
}

static void game_playing_hand_row_on_key_transit(
    SelectionGrid* selection_grid,
    Selection* selection,
    void* ctx
)
{
    RoundProps* props = (RoundProps*)ctx;
    if (key_hit(SELECT_CARD))
    {
        selection_hit_timer = props->timer;
    }
    else if (key_released(SELECT_CARD))
    {
        if (!moving_card && !card_selected_instead_of_moved)
        {
            hand_select_card(props, hand_sel_idx_to_card_idx(props->hand_top, selection->x));
        }
        moving_card = false;
        card_moved_too_fast = false;
        card_selected_instead_of_moved = false;
        selection_hit_timer = TM_ZERO;
    }
    else if (key_hit(DESELECT_CARDS))
    {
        hand_deselect_all_cards(props);
        set_hand();
    }
    else if (key_hit(SORT_HAND))
    {
        hand_change_sort(props);
    }
}

static inline void game_playing_button_set_highlight(int btn_idx, bool highlight)
{
    button_set_highlight(&game_playing_buttons[btn_idx], highlight);
}

static bool game_playing_button_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection,
    void* ctx
)
{
    // The selection grid system only guarantees that the new selection is within bounds
    // but not the previous one...
    // As of writing (PR #348), this check is not strictly needed for this row but it is
    // left in, in case that ever changes. It can be reconsidered and removed.
    if (prev_selection->y == row_idx && prev_selection->x >= 0 &&
        prev_selection->x < game_playing_button_row_get_size(NULL))
    {
        game_playing_button_set_highlight(prev_selection->x, false);
    }

    if (new_selection->y == row_idx)
    {
        game_playing_button_set_highlight(new_selection->x, true);
    }

    return true;
}

static void game_playing_button_row_on_key_hit(
    SelectionGrid* selection_grid,
    Selection* selection,
    void* ctx
)
{
    if (key_hit(SELECT_CARD))
    {
        button_press(&game_playing_buttons[selection->x], ctx);
    }
}

static void hand_deselect_all_cards(RoundProps* props)
{
    bool any_cards_deselected = false;
    for (int i = 0; i <= props->hand_top; i++)
    {
        CardObject* card_object = props->hand[i];
        if (card_object_is_selected(card_object))
        {
            card_object_set_selected(card_object, false);
            hand_selections--;
            any_cards_deselected = true;
        }
    }

    if (any_cards_deselected)
    {
        play_sfx(SFX_CARD_DESELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
}

static void hand_change_sort(RoundProps* props)
{
    sort_by_suit = !sort_by_suit;
    sort_cards(props);
}

static bool can_play_hand(void* _)
{
    if (hand_state != HAND_SELECT || hand_selections == 0)
        return false;
    return true;
}

static bool can_discard_hand(void* ctx)
{
    RoundProps* props = (RoundProps*)ctx;
    return (props->discards > 0 && hand_state == HAND_SELECT && hand_selections > 0);
}

void reset_joker_scored_itr(List* jokers_list)
{
    _joker_scored_itr = list_itr_create(jokers_list);
}

/**
 * @brief Converts a selection index from the selection grid into a card index within the hand array
 * @param selection_index The selection index from the selection grid.
 * @return The index within the hand stack array.
 * Note that the result is not valid if hand size is 0.
 */
static inline int hand_sel_idx_to_card_idx(int hand_top, int selection_index)
{
    // This is because the hand is drawn from right to left.
    // There is no particular reason for why that was done, it's just how it was done.
    // Maybe one day it can be reverted and made consistent so this conversion is not needed.
    return hand_top - selection_index;
}

static void hand_select_card(RoundProps* props, int index)
{
    CardObject* card_object = props->hand[index];
    if (index < 0 || index >= (props->hand_top + 1) || hand_state != HAND_SELECT ||
        card_object == NULL)
        return;

    if (card_object_is_selected(card_object))
    {
        card_object_set_selected(card_object, false);
        hand_selections--;
        play_sfx(SFX_CARD_DESELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
    else if (hand_selections < MAX_SELECTION_SIZE)
    {
        card_object_set_selected(card_object, true);
        hand_selections++;
        play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
    set_hand();
}

static inline void game_playing_process_hand_select_input(RoundProps* props)
{
    selection_grid_process_input(&game_playing_selection_grid, props);
}

static inline void card_draw(RoundProps* props)
{
    int hand_top = props->hand_top, deck_top = props->deck_top;
    if (deck_top < 0 || hand_top >= hand_size - 1 || hand_top >= MAX_HAND_SIZE - 1)
        return;

    CardObject* card_object = card_object_new(deck_pop(props->deck, &props->deck_top));
    props->deck_top--;

    const FIXED deck_x = int2fx(CARD_DRAW_POS.x);
    const FIXED deck_y = int2fx(CARD_DRAW_POS.y);

    card_object->sprite_object->x = deck_x;
    card_object->sprite_object->y = deck_y;

    props->hand[++props->hand_top] = card_object;

    // Sort the hand after drawing a card
    sort_cards(props);

    play_sfx(
        SFX_CARD_DRAW,
        MM_BASE_PITCH_RATE + cards_drawn * PITCH_STEP_DRAW_SFX,
        SFX_DEFAULT_VOLUME
    );
}

static inline void game_playing_handle_round_over(RoundProps* props)
{
    enum GameState next_state = GAME_STATE_ROUND_END;
    int current_blind = props->current_blind;
    u32 score = props->score;

    if (score >= blind_get_requirement(current_blind, props->ante))
    {
        if (current_blind == BLIND_TYPE_BOSS)
        {
            if (props->ante < MAX_ANTE)
            {
                display_ante(++props->ante);
            }
            else
            {
                next_state = GAME_STATE_WIN;
            }
        }
    }
    else if (props->hands == 0)
    {
        next_state = GAME_STATE_LOSE;
    }

    game_change_state(next_state);
}

static inline void card_in_hand_loop_handle_discard_and_shuffling(
    RoundProps* props,
    int card_idx,
    FIXED* hand_x,
    FIXED* hand_y,
    bool* break_loop
)
{
    if (hand_state != HAND_DISCARD && hand_state != HAND_SHUFFLING)
    {
        // Assumes hand_state is one of these
        return;
    }

    *break_loop = false;
    CardObject* card_object = props->hand[card_idx];
    int hand_top = props->hand_top;

    if (card_object_is_selected(card_object) || hand_state == HAND_SHUFFLING)
    {
        if (!discarded_card)
        {
            *hand_x = int2fx(CARD_DISCARD_PNT.x);
            *hand_y = int2fx(CARD_DISCARD_PNT.y);

            if (!sound_played)
            {
                play_sfx(
                    SFX_CARD_DRAW,
                    MM_BASE_PITCH_RATE + cards_drawn * PITCH_STEP_DISCARD_SFX,
                    SFX_DEFAULT_VOLUME
                );
                sound_played = true;
            }

            if (card_object->sprite_object->x >= *hand_x)
            {
                discard_push(props->discard_pile, &props->discard_top, card_object->card);
                card_object_destroy(&card_object);
                props->hand[card_idx] = card_object;
                reorder_card_sprites_layers(props);

                props->hand_top--;
                // This technically isn't drawing cards, I'm just reusing the variable
                cards_drawn++;
                sound_played = false;
                props->timer = TM_ZERO;

                *hand_y = card_object->sprite_object->y;
                *hand_x = card_object->sprite_object->x;
            }

            discarded_card = true;
        }
        else
        {
            if (hand_state == HAND_DISCARD)
            {
                // Don't raise the card if we're mass discarding, it looks stupid.
                *hand_y -= int2fx(15);
            }
            else // hand_state == HAND_SHUFFLING
            {
                *hand_y += int2fx(24);
            }
            *hand_x =
                *hand_x + (int2fx(card_idx) - int2fx(hand_top) / 2) * -HAND_SPACING_LUT[hand_top];
        }
    }
    else
    {
        *hand_x = *hand_x + (int2fx(card_idx) - int2fx(hand_top) / 2) * -HAND_SPACING_LUT[hand_top];
    }

    if (card_idx == 0 && discarded_card == false && props->timer % FRAMES(10) == 0)
    {
        // This is never reached in the case of HAND_SHUFFLING. Not sure why but that's how it's
        // supposed to be.
        hand_state = HAND_DRAW;
        sound_played = false;
        cards_drawn = 0;
        hand_selections = 0;
        props->timer = TM_ZERO;
        *break_loop = true;
        return;
    };
}

static inline void select_flush_and_straight_cards_in_played_hand(RoundProps* props)
{
    CardObject** played = props->played;
    int played_top = props->played_top;

    // Special handling because Four Fingers might be active
    bool final_selection[MAX_SELECTION_SIZE] = {false};

    // Will be 4 if Four Fingers is in effect, otherwise 5
    int min_len = get_straight_and_flush_size();

    // if we have a flush in our hand
    if (hand_type == FLUSH || hand_type == STRAIGHT_FLUSH || hand_type == ROYAL_FLUSH)
    {
        bool flush_selection[MAX_HAND_SIZE] = {false};
        find_flush_in_played_cards(played, played_top, min_len, flush_selection);
        // Add the results into the final selection
        for (int i = 0; i <= played_top; i++)
        {
            final_selection[i] = flush_selection[i];
        }
    }

    // If we have a straight in our hand
    if (hand_type == STRAIGHT || hand_type == STRAIGHT_FLUSH || hand_type == ROYAL_FLUSH)
    {
        bool straight_selection[MAX_HAND_SIZE] = {false};
        find_straight_in_played_cards(
            played,
            played_top,
            is_shortcut_joker_active(),
            min_len,
            straight_selection
        );
        // Add the results into the final selection
        for (int i = 0; i <= played_top; i++)
        {
            final_selection[i] = final_selection[i] || straight_selection[i];
        }
        // If Four Fingers is active, pairs can happen in a valid straight
        // If Four Fingers is not active, pairs are impossible so this will not affect things
        select_paired_cards_in_hand(played, played_top, final_selection);
    }

    // Finally, set mark the cards as selected based final_selection
    for (int i = 0; i <= played_top; i++)
    {
        if (final_selection[i])
        {
            card_object_set_selected(played[i], true);
        }
    }
}

static inline void select_all_five_cards_in_played_hand(RoundProps* props)
{
    int played_top = props->played_top;
    for (int i = 0; i <= played_top; i++)
    {
        CardObject* card_object = props->played[i];
        card_object_set_selected(card_object, true);
    }
}

static inline void select_four_of_a_kind_cards_in_played_hand(RoundProps* props)
{
    // find four cards with the same rank
    // If there are 5 cards selected we just need to find the one card that doesn't match, and
    // select the others
    int played_top = props->played_top;
    if (played_top >= 3)
    {
        int unmatched_index = -1;

        for (int i = 0; i <= played_top; i++)
        {
            CardObject* card_a = props->played[i];
            CardObject* card_b = props->played[(i + 1) % (played_top + 1)];
            CardObject* card_c = props->played[(i + 2) % (played_top + 1)];
            if (card_a->card->rank != card_b->card->rank &&
                card_a->card->rank != card_c->card->rank)
            {
                unmatched_index = i;
                break;
            }
        }

        for (int i = 0; i <= played_top; i++)
        {
            if (i != unmatched_index)
            {
                CardObject* card_object = props->played[i];
                card_object_set_selected(card_object, true);
            }
        }
    }
    else // If there are only 4 cards selected we know they match
    {
        for (int i = 0; i <= played_top; i++)
        {
            CardObject* card_object = props->played[i];
            card_object_set_selected(card_object, true);
        }
    }
}

static inline void select_three_of_a_kind_cards_in_played_hand(RoundProps* props)
{
    // find three cards with the same rank
    int played_top = props->played_top;

    for (int i = 0; i <= played_top - 1; i++)
    {
        CardObject* card_a = props->played[i];
        for (int j = i + 1; j <= played_top; j++)
        {
            CardObject* card_b = props->played[j];
            if (card_a->card->rank == card_b->card->rank)
            {
                card_object_set_selected(card_a, true);
                card_object_set_selected(card_b, true);

                for (int k = j + 1; k <= played_top; k++)
                {
                    CardObject* card_c = get_played_card_at(k);
                    if (card_a->card->rank == card_c->card->rank &&
                        !card_object_is_selected(card_c))
                    {
                        card_object_set_selected(card_c, true);
                        break;
                    }
                }

                break;
            }
        }

        if (card_object_is_selected(card_a))
            break;
    }
}

static inline void select_two_pair_cards_in_played_hand(RoundProps* props)
{
    // find two pairs of cards with the same rank
    int i, played_top = props->played_top;

    for (i = 0; i <= played_top - 1; i++)
    {
        CardObject* card_a = props->played[i];
        for (int j = i + 1; j <= played_top; j++)
        {
            CardObject* card_b = props->played[j];
            if (card_a->card->rank == card_b->card->rank)
            {
                card_object_set_selected(card_a, true);
                card_object_set_selected(card_b, true);
                break;
            }
        }

        if (card_object_is_selected(card_a))
            break;
    }

    for (; i <= played_top - 1; i++) // Find second pair
    {
        for (int j = i + 1; j <= played_top; j++)
        {
            CardObject* card_a = props->played[i];
            CardObject* card_b = props->played[j];
            if (card_a->card->rank == card_b->card->rank && !card_object_is_selected(card_a) &&
                !card_object_is_selected(card_b))
            {
                card_object_set_selected(card_a, true);
                card_object_set_selected(card_b, true);
                break;
            }
        }
    }
}

static inline void select_pair_cards_in_played_hand(RoundProps* props)
{
    // find two cards with the same rank
    int played_top = props->played_top;
    for (int i = 0; i <= played_top - 1; i++)
    {
        CardObject* card_a = props->played[i];
        for (int j = i + 1; j <= played_top; j++)
        {
            CardObject* card_b = props->played[j];
            if (card_a->card->rank == card_b->card->rank)
            {
                card_object_set_selected(card_a, true);
                card_object_set_selected(card_b, true);
                break;
            }
        }

        if (card_object_is_selected(card_a))
            break;
    }
}

static inline void select_highcard_cards_in_played_hand(RoundProps* props)
{
    // find the card with the highest rank in the hand
    int highest_rank_index = 0;
    int played_top = props->played_top;

    for (int i = 0; i <= played_top; i++)
    {
        CardObject* card_object = props->played[i];
        CardObject* highest_rank_card_object = props->played[highest_rank_index];
        if (card_object->card->rank > highest_rank_card_object->card->rank)
        {
            highest_rank_index = i;
        }
    }

    card_object_set_selected(props->played[highest_rank_index], true);
}

// returns true if a joker was scored, false otherwise
static bool check_and_score_joker_for_event(
    ListItr* starting_joker_itr,
    CardObject* card_object,
    enum JokerEvent joker_event
)
{
    JokerObject* joker;

    while ((joker = list_itr_next(starting_joker_itr)))
    {
        if (joker_object_score(joker, card_object, joker_event))
        {
            return true;
        }
    }
    return false;
}

static inline bool game_round_is_over(RoundProps* props)
{
    int current_blind = props->current_blind;
    int ante = props->ante;
    u32 score = props->score;
    int hands = props->hands;

    return hands == 0 || score >= blind_get_requirement(current_blind, ante);
}

// Basically a copy of HAND_DISCARD
// returns true if the current card has been discarded
static bool play_ended_played_cards_update(RoundProps* props, int played_idx)
{
    if (!discarded_card && props->timer > FRAMES(40))
    {
        // play the sound only once per card, when it is pushed off-screen to the right
        if (!sound_played)
        {
            play_sfx(
                SFX_CARD_DRAW,
                MM_BASE_PITCH_RATE + cards_drawn * PITCH_STEP_DISCARD_SFX,
                SFX_DEFAULT_VOLUME
            );
            sound_played = true;
        }

        // card has exited the screen, now discard it and set it to NULL
        CardObject* card_object = props->played[played_idx];
        if (card_object->sprite_object->x >= int2fx(CARD_DISCARD_PNT.x))
        {
            discard_push(
                props->discard_pile,
                &props->discard_top,
                card_object->card
            ); // Push the card to the discard pile
            card_object_destroy(&card_object);
            props->played[played_idx] = card_object;

            // played_top--;
            cards_drawn++; // This technically isn't drawing cards, I'm just reusing the variable
            sound_played = false; // Allow for the sound for the next card to be played

            // we reached hand_top, all cards have been discarded
            if (played_idx == props->played_top)
            {
                if (game_round_is_over(props))
                {
                    hand_state = HAND_SHUFFLING;
                }
                else
                {
                    hand_state = HAND_DRAW;
                }

                play_state = PLAY_STARTING;
                cards_drawn = 0;
                hand_selections = 0;
                props->played_top = -1;
                scored_card_index = 0;
                _joker_scored_itr = list_itr_create(props->owned_jokers_list);
                props->timer = TM_ZERO;
            }

            return true; // return early to avoid accessing played[played_idx] == NULL
        }

        // put target X position off screen to the right
        card_object->sprite_object->tx = int2fx(CARD_DISCARD_PNT.x);
        discarded_card = true;
    }

    return false;
}

static inline void play_starting_played_cards_update(RoundProps* props, int played_idx)
{
    int played_top = props->played_top;
    uint timer = props->timer;
    CardObject** played = props->played;

    bool card_selected = card_object_is_selected(played[played_top - scored_card_index]);

    if (played_idx == played_top && (timer % FRAMES(10) == 0 || !card_selected) &&
        timer > FRAMES(40))
    {
        scored_card_index--;

        if (scored_card_index == 0)
        {
            _joker_scored_itr = list_itr_create(props->owned_jokers_list);
            props->timer = TM_ZERO;
            play_state = PLAY_BEFORE_SCORING;
        }
    }

    CardObject* card_object = played[played_idx];
    card_object->sprite_object->tx =
        int2fx(HAND_PLAY_POS.x) + (int2fx(played_top - played_idx) - int2fx(played_top) / 2) * -27;
    card_object->sprite_object->ty = int2fx(HAND_PLAY_POS.y);

    card_selected = card_object_is_selected(card_object);
    if (card_selected && played_top - played_idx >= scored_card_index)
    {
        card_object->sprite_object->ty -= int2fx(10);
    }
}

// returns true if the scoring loop has returned early
static inline bool play_before_scoring_cards_update(void)
{
    // Activate Jokers with an effect just before the hand is scored
    if (check_and_score_joker_for_event(&_joker_scored_itr, NULL, JOKER_EVENT_ON_HAND_PLAYED))
    {
        return true;
    }

    play_state = PLAY_SCORING_CARDS;
    return false;
}

// returns true if the scoring loop has returned early
static inline bool play_scoring_cards_update(RoundProps* props)
{
    uint timer = props->timer;
    int played_top = props->played_top;
    CardObject* scored_card_object = props->played[scored_card_index];

    if (timer % FRAMES(30) == 0 && timer > FRAMES(40))
    {
        // We are about to score played Cards.
        // Start from the current card index
        // and seek the next scoring card
        while (scored_card_index <= played_top && !card_object_is_selected(scored_card_object))
        {
            scored_card_object = props->played[++scored_card_index];
        }

        // go to the next state if there are no cards left to score
        if (scored_card_index > played_top)
        {
            // reuse these variables for held cards
            _joker_scored_itr = list_itr_create(props->owned_jokers_list);
            scored_card_index = props->hand_top;

            play_state = PLAY_SCORING_HELD_CARDS;

            return false;
        }

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        if (card_object_is_selected(scored_card_object))
        {
            // Offset of 1 tile to keep the text on the card
            tte_set_pos(
                fx2int(scored_card_object->sprite_object->x) + TILE_SIZE,
                SCORED_CARD_TEXT_Y
            );

            // Set text color to blue from background memory
            tte_set_special(TTE_BLUE_PB * TTE_SPECIAL_PB_MULT_OFFSET);

            u8 card_value = card_get_value(scored_card_object->card);

            // Write the score to a character buffer variable
            char score_buffer[INT_MAX_DIGITS + 2]; // for '+' and null terminator
            snprintf(score_buffer, sizeof(score_buffer), "+%hhu", card_value);
            tte_write(score_buffer);

            card_object_shake(scored_card_object, SFX_CHIPS_CARD);

            // Relocated card scoring logic here
            chips = u32_protected_add(chips, card_value);
            display_chips();

            // Allow Joker scoring
            _joker_scored_itr = list_itr_create(props->owned_jokers_list);
            _joker_card_scored_end_itr = list_itr_create(props->owned_jokers_list);
        }

        play_state = PLAY_SCORING_CARD_JOKERS;
        return true;
    }

    return false;
}

// Activate "on scored" Jokers for the previous scored card if any
// returns true if the scoring loop has returned early
static inline bool play_scoring_card_jokers_update(RoundProps* props)
{
    uint timer = props->timer;
    CardObject* scored_card_object = props->played[scored_card_index];

    if (timer % FRAMES(30) == 0 && timer > FRAMES(40))
    {
        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        // since we sought the next scoring card index in the previous state,
        // scored_card_index is guaranteed to be a scoring card
        if (check_and_score_joker_for_event(
                &_joker_scored_itr,
                scored_card_object,
                JOKER_EVENT_ON_CARD_SCORED
            ))
        {
            return true;
        }

        // Trigger all Jokers that have an effect when a card finishes scoring
        // (e.g. retriggers) after activating all the other scored_card Jokers normally
        if (check_and_score_joker_for_event(
                &_joker_card_scored_end_itr,
                scored_card_object,
                JOKER_EVENT_ON_CARD_SCORED_END
            ))
        {
            // If we just scored a retrigger, return early and go back to the
            // previous state score the same card again without incrementing
            // scored_card_index to score the current card again
            if (retrigger)
            {
                retrigger = false;
                play_state = PLAY_SCORING_CARDS;
            }
            return true;
        }

        // increment index to start seeking the next scoring card from the next card
        scored_card_object = props->played[++scored_card_index];
        play_state = PLAY_SCORING_CARDS;
    }

    return false;
}

// returns true if the scoring loop has returned early
static inline bool play_scoring_held_cards_update(RoundProps* props, int played_idx)
{
    uint timer = props->timer;
    if (played_idx == 0 && (timer % FRAMES(30) == 0) && timer > FRAMES(40))
    {
        tte_erase_rect_wrapper(HELD_CARDS_SCORES_RECT);

        // Go through all held cards and see if they activate Jokers
        for (; scored_card_index >= 0; scored_card_index--)
        {
            CardObject* card_object = props->hand[scored_card_index];
            if (check_and_score_joker_for_event(
                    &_joker_scored_itr,
                    card_object,
                    JOKER_EVENT_ON_CARD_HELD
                ))
            {
                card_object_shake(card_object, SFX_CARD_SELECT);
                return true;
            }
            _joker_scored_itr = list_itr_create(props->owned_jokers_list);
        }

        scored_card_index = 0;
        _joker_round_end_itr = list_itr_create(props->owned_jokers_list);
        play_state = PLAY_SCORING_INDEPENDENT_JOKERS;
    }

    return false;
}

// Score Jokers normally (independent)
// returns true if the scoring loop has returned early
static inline bool play_scoring_independent_jokers_update(RoundProps* props, int played_idx)
{
    uint timer = props->timer;
    if (played_idx == 0 && (timer % FRAMES(30) == 0) && timer > FRAMES(40))
    {

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        if (check_and_score_joker_for_event(&_joker_scored_itr, NULL, JOKER_EVENT_INDEPENDENT))
        {
            return true;
        }

        scored_card_index =
            props->played_top + 1; // Reset the scored card index to the top of the played stack

        play_state = PLAY_SCORING_HAND_SCORED_END;
    }

    return false;
}

// Trigger hand end effect for all jokers once they are done scoring
static inline bool play_scoring_hand_scored_end_update(RoundProps* props, int played_idx)
{
    uint timer = props->timer;
    if (played_idx == 0 && (timer % FRAMES(30) == 0) && timer > FRAMES(40))
    {

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        bool scored = check_and_score_joker_for_event(
            &_joker_round_end_itr,
            NULL,
            JOKER_EVENT_ON_HAND_SCORED_END
        );

        if (scored)
        {
            return true;
        }

        props->timer = TM_ZERO;
        play_state = PLAY_ENDING;
    }

    return false;
}

// This is the reverse of PLAY_STARTING. The cards get reset back to their neutral position
// sequentially
static inline void play_ending_played_cards_update(RoundProps* props, int played_idx)
{
    uint timer = props->timer;
    int played_top = props->played_top;
    bool card_selected = card_object_is_selected(props->played[played_top - scored_card_index]);

    if (played_idx == played_top && (timer % FRAMES(10) == 0 || !card_selected) &&
        timer > FRAMES(40))
    {
        scored_card_index--;

        /* SFX_CHIPS_ACCUM has been pitch shifted to perserve high frequencies in downsampling.
         * Now it needs to be pitch shifted back to the original frequency.
         */
        int static const CHIPS_ACCUM_SFX_PITCH_RATIO = 2;

        if (scored_card_index == 0)
        {
            play_sfx(
                SFX_CHIPS_ACCUM,
                CHIPS_ACCUM_SFX_PITCH_RATIO * MM_BASE_PITCH_RATE,
                SFX_DEFAULT_VOLUME
            );
            props->timer = TM_ZERO;
            play_state = PLAY_ENDED;
        }
    }

    CardObject* card_object = props->played[played_idx];
    if (card_object_is_selected(card_object) && played_top - played_idx >= scored_card_index)
    {
        card_object->sprite_object->ty = int2fx(HAND_PLAY_POS.y);
    }
}

static inline void played_cards_update_loop(RoundProps* props)
{
    // So this one is a bit fucking weird because I have to work kinda backwards for everything
    // because of the order of the pushed cards from the hand to the play stack (also crazy that the
    // company that published Balatro is called "Playstack" and this is a play stack, but I digress)
    for (int played_idx = 0; played_idx <= props->played_top; played_idx++)
    {
        CardObject* card_object = props->played[played_idx];
        if (card_object == NULL)
        {
            continue;
        }

        if (card_object_get_sprite(card_object) == NULL)
        {
            // Set the sprite for the played card object
            card_object_set_sprite(card_object, played_idx + MAX_HAND_SIZE);
        }

        switch (play_state)
        {
            case PLAY_STARTING:

                play_starting_played_cards_update(props, played_idx);
                break;

            case PLAY_BEFORE_SCORING:

                if (play_before_scoring_cards_update())
                {
                    return;
                }
                break;

            case PLAY_SCORING_CARDS:

                if (play_scoring_cards_update(props))
                {
                    return;
                }
                break;

            case PLAY_SCORING_CARD_JOKERS:

                if (play_scoring_card_jokers_update(props))
                {
                    return;
                }
                break;

            case PLAY_SCORING_HELD_CARDS:

                if (play_scoring_held_cards_update(props, played_idx))
                {
                    return;
                }
                break;

            case PLAY_SCORING_INDEPENDENT_JOKERS:

                if (play_scoring_independent_jokers_update(props, played_idx))
                {
                    return;
                }
                break;

            case PLAY_SCORING_HAND_SCORED_END:

                if (play_scoring_hand_scored_end_update(props, played_idx))
                {
                    return;
                }
                break;

            case PLAY_ENDING:

                play_ending_played_cards_update(props, played_idx);
                break;

            case PLAY_ENDED:

                if (play_ended_played_cards_update(props, played_idx))
                {
                    // we continue here instead of returning for performance
                    // to instantly go to the next card to discard at played_idx+1,
                    // instead of  starting over from index 0 and going up
                    // to that card again
                    continue;
                }
                break;
        }

        card_object->sprite_object->tscale = FIX_ONE;
        card_object_update(card_object);
    }
}

static inline int hand_get_max_size(void)
{
    return hand_size;
}

static inline void game_playing_process_input_and_state(RoundProps* props)
{
    if (hand_state == HAND_SELECT)
    {
        game_playing_process_hand_select_input(props);
    }
    else if (play_state == PLAY_ENDING)
    {
        if (mult > 0)
        {
            // protect against score overflow
            props->temp_score = u32_protected_mult(chips, mult);
            props->lerped_temp_score = int2fx(props->temp_score);
            props->lerped_score = int2fx(props->score);

            display_temp_score(props->temp_score);

            chips = 0;
            mult = 0;
            display_mult();
            display_chips();

            static const int SCORE_CALC_SFX_PITCH_SHIFT = -102; // -10% OF MM_BASE_PITCH_RATE
            static const int SCORE_CALC_SFX_VOLUME = 204;       // 80% MM_FULL_VOLUME

            // The chips calculation SFX is the same as button
            play_sfx(
                SFX_BUTTON,
                MM_BASE_PITCH_RATE + SCORE_CALC_SFX_PITCH_SHIFT,
                SCORE_CALC_SFX_VOLUME
            );
        }
    }
    else if (play_state == PLAY_ENDED && props->timer % FRAMES(TM_SCORE_LERP_INTERVAL) == 0)
    {
        /* Using fixed point in case the score is lower than NUM_SCORE_LERP_STEPS and then
         * then the division rounds it down to 0 and it's never added to the total.
         * The operation is equivalent to
         * fxdiv(int2fx(temp_score * GAME_SPEED), int2fx(NUM_SCORE_LERP_STEPS))
         */
        FIXED lerped_score_offset = int2fx(props->temp_score * GAME_SPEED) / NUM_SCORE_LERP_STEPS;
        props->lerped_temp_score -= lerped_score_offset;
        props->lerped_score += lerped_score_offset;

        if (props->lerped_temp_score > 0)
        {
            // Set the score display first because it's more important
            // in case there isn't enough time within the frame to display both
            display_score(fx2uint(props->lerped_score));
            display_temp_score(fx2uint(props->lerped_temp_score));
        }
        else
        {
            props->score += props->temp_score;
            props->temp_score = 0;
            props->lerped_temp_score = 0;
            props->lerped_score = 0;

            tte_erase_rect_wrapper(TEMP_SCORE_RECT); // Just erase the temp score

            display_score(props->score);
        }
    }
}

static inline void game_playing_process_card_draw(RoundProps* props)
{
    if (hand_state == HAND_DRAW && cards_drawn < hand_size)
    {
        if (props->timer % FRAMES(10) == 0) // Draw a card every 10 frames
        {
            cards_drawn++;
            card_draw(props);
        }
    }
    else if (hand_state == HAND_DRAW)
    {
        hand_state = HAND_SELECT; // Change the hand state to select after drawing all the cards
        cards_drawn = 0;
        props->timer = TM_ZERO;
    }
}

static inline void game_playing_discarded_cards_loop(RoundProps* props)
{
    // Discarded cards loop (mainly for shuffling)
    int hand_size = props->hand_top + 1;

    if (hand_size == 0 && hand_state == HAND_SHUFFLING && props->discard_top >= -1 &&
        props->timer > FRAMES(10))
    {
        // Change the background to the round end background. This is how it works in Balatro, so
        // I'm doing it this way too.
        change_background(BG_ROUND_END);

        // We take each discarded card and put it back into the deck with a short animation
        static CardObject* discarded_card_object = NULL;
        if (discarded_card_object == NULL)
        {
            discarded_card_object =
                card_object_new(discard_pop(props->discard_pile, &props->discard_top));
            // discarded_card_object->sprite = sprite_new(ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF,
            // ATTR1_SIZE_32,
            // card_sprite_lut[discarded_card_object->card->suit][discarded_card_object->card->rank],
            // 0, 0);
            // Set the sprite for the discarded card object
            card_object_set_sprite(discarded_card_object, 0);
            sprite_object_reset_transform(discarded_card_object->sprite_object);

            discarded_card_object->sprite_object->tx = int2fx(204);
            discarded_card_object->sprite_object->ty = int2fx(112);
            discarded_card_object->sprite_object->x = int2fx(240);
            discarded_card_object->sprite_object->y = int2fx(80);

            card_object_update(discarded_card_object);
        }
        else
        {
            card_object_update(discarded_card_object);

            if (discarded_card_object->sprite_object->y >= discarded_card_object->sprite_object->ty)
            {
                deck_push(
                    props->deck,
                    &props->deck_top,
                    discarded_card_object->card
                ); // Put the card back into the deck
                card_object_destroy(&discarded_card_object);

                play_sfx(
                    SFX_CARD_DRAW,
                    MM_BASE_PITCH_RATE + PITCH_STEP_UNDISCARD_SFX,
                    SFX_DEFAULT_VOLUME
                );
            }
        }

        // If there are no more discarded cards, stop shuffling
        if (props->discard_top == -1 && discarded_card_object == NULL)
        {
            // After HAND_SHUFFLING the round is over
            game_playing_handle_round_over(props);
        }
    }
}

static inline void select_cards_in_played_hand(RoundProps* props)
{
    switch (hand_type) // select the cards that apply to the hand type
    {
        case NONE:
            break;
        case HIGH_CARD:
            select_highcard_cards_in_played_hand(props);
            break;
        case PAIR:
            select_pair_cards_in_played_hand(props);
            break;
        case TWO_PAIR:
            select_two_pair_cards_in_played_hand(props);
            break;
        case THREE_OF_A_KIND:
            select_three_of_a_kind_cards_in_played_hand(props);
            break;
        case FOUR_OF_A_KIND:
            select_four_of_a_kind_cards_in_played_hand(props);
            break;
        case STRAIGHT:
            /* FALL THROUGH */
        case FLUSH:
            /* FALL THROUGH */
        case STRAIGHT_FLUSH:
            /* FALL THROUGH */
        case ROYAL_FLUSH:
            select_flush_and_straight_cards_in_played_hand(props);
            break;
        case FULL_HOUSE:
            /* FALL THROUGH */
        case FIVE_OF_A_KIND:
            /* FALL THROUGH */
        case FLUSH_HOUSE:
            /* FALL THROUGH */
        case FLUSH_FIVE: // Select all played cards in the hand
            select_all_five_cards_in_played_hand(props);
            break;
    }
}

static inline void cards_in_hand_update_loop(RoundProps* props)
{
    int selected_card_idx =
        hand_sel_idx_to_card_idx(props->hand_top, game_playing_selection_grid.selection.x);

    // TODO: Break this function up into smaller ones, Gods be good
    // Start from the end of the hand and work backwards because that's how Balatro does it
    CardObject** hand = props->hand;

    for (int i = props->hand_top; i >= 0; i--)
    {
        CardObject* card_object = hand[i];
        if (card_object != NULL)
        {
            FIXED hand_x = int2fx(HAND_START_POS.x);
            FIXED hand_y = int2fx(HAND_START_POS.y);

            switch (hand_state)
            {
                case HAND_DRAW:
                    hand_x = hand_x + (int2fx(i) - int2fx(props->hand_top) / 2) *
                                          -HAND_SPACING_LUT[props->hand_top];
                    break;
                case HAND_SELECT:
                    bool is_focused =
                        (i == selected_card_idx &&
                         game_playing_selection_grid.selection.y == GAME_PLAYING_HAND_SEL_Y);

                    bool is_selected = card_object_is_selected(card_object);
                    if (is_focused && !is_selected)
                    {
                        hand_y -= int2fx(CARD_FOCUSED_UNSEL_Y);
                    }
                    else if (!is_focused && is_selected)
                    {
                        hand_y -= int2fx(CARD_UNFOCUSED_SEL_Y);
                    }
                    else if (is_focused && is_selected)
                    {
                        hand_y -= int2fx(CARD_FOCUSED_SEL_Y);
                    }

                    if (i != selected_card_idx && card_object->sprite_object->y > hand_y)
                    {
                        card_object->sprite_object->y = hand_y;
                        card_object->sprite_object->vy = 0;
                    }

                    hand_x =
                        hand_x +
                        (int2fx(i) - int2fx(props->hand_top) / 2) *
                            -HAND_SPACING_LUT[props->hand_top]; // TODO: Change this later to
                                                                // reference a 2D LUT of positions
                    break;
                case HAND_SHUFFLING:
                    /* FALL THROUGH */
                case HAND_DISCARD: // TODO: Add sound
                    bool break_loop;
                    card_in_hand_loop_handle_discard_and_shuffling(
                        props,
                        i,
                        &hand_x,
                        &hand_y,
                        &break_loop
                    );
                    if (break_loop)
                        break;

                    break;
                case HAND_PLAY:
                    hand_x = hand_x + (int2fx(i) - int2fx(props->hand_top) / 2) *
                                          -HAND_SPACING_LUT[props->hand_top];
                    hand_y += int2fx(24);
                    uint timer = props->timer;

                    if (card_object_is_selected(card_object) && discarded_card == false &&
                        timer % FRAMES(10) == 0)
                    {
                        card_object_set_selected(card_object, false);
                        played_push(props->played, &props->played_top, card_object);
                        sprite_destroy(&card_object->sprite_object->sprite);
                        props->hand[i] = NULL; // Remove the card from the hand
                        reorder_card_sprites_layers(props);

                        play_sfx(
                            SFX_CARD_DRAW,
                            MM_BASE_PITCH_RATE + cards_drawn * PITCH_STEP_DISCARD_SFX,
                            SFX_DEFAULT_VOLUME
                        );

                        props->hand_top--;
                        hand_selections--;
                        cards_drawn++;

                        discarded_card = true;
                    }

                    if (i == 0 && discarded_card == false && timer % FRAMES(10) == 0)
                    {
                        hand_state = HAND_PLAYING;
                        cards_drawn = 0;
                        hand_selections = 0;
                        props->timer = TM_ZERO;
                        scored_card_index = props->played_top + 1;

                        select_cards_in_played_hand(props);
                    }

                    break;
                // Don't need to do anything here, just wait for the player to select cards
                case HAND_PLAYING:
                    hand_x = hand_x + (int2fx(i) - int2fx(props->hand_top) / 2) *
                                          -HAND_SPACING_LUT[props->hand_top];
                    hand_y += int2fx(24);
                    break;
            }

            card_object->sprite_object->tx = hand_x;
            card_object->sprite_object->ty = hand_y;
            card_object_update(card_object);
        }
    }
}

static inline void game_playing_ui_text_update(RoundProps* props)
{
    static int last_hand_size = 0;
    static int last_deck_size = 0;

    int _hand_size = props->hand_top + 1;
    int _deck_size = props->deck_top + 1;
    int deck_max_size =
        deck_get_max_size(props->hand_top, props->played_top, props->deck_top, props->discard_top);

    enum BackgroundId background = get_background();

    if (last_hand_size != _hand_size || last_deck_size != props->deck_top + 1)
    {
        if (background == BG_CARD_SELECTING)
        {
            // Hand size/max size
            tte_printf(
                "#{P:%d,%d; cx:0x%X000}%d/%d",
                HAND_SIZE_RECT_SELECT.left,
                HAND_SIZE_RECT_SELECT.top,
                TTE_WHITE_PB,
                _hand_size,
                hand_get_max_size()
            );
        }
        else if (background == BG_CARD_PLAYING)
        {
            // Hand size/max size
            tte_printf(
                "#{P:%d,%d; cx:0x%X000}%d/%d",
                HAND_SIZE_RECT_PLAYING.left,
                HAND_SIZE_RECT_PLAYING.top,
                TTE_WHITE_PB,
                _hand_size,
                hand_get_max_size()
            );
        }

        // Deck size/max size
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%d/%d",
            DECK_SIZE_RECT.left,
            DECK_SIZE_RECT.top,
            TTE_WHITE_PB,
            _deck_size,
            deck_max_size
        );

        last_hand_size = _hand_size;
        last_deck_size = _deck_size;
    }
}

static inline void game_playing_process_flaming_score(RoundProps* props)
{
    static u8 flame_score_frame = 0;

    if (are_score_flames_active())
    {
        if (props->timer % SCORE_FLAMES_ANIM_FREQ == 0)
        {
            Rect frame_rect = SCORE_FLAME_FRAMES_START;
            flame_score_frame = (flame_score_frame + 1) % NUM_SCORE_FLAMES_FRAMES;

            // chips flame (blue)
            frame_rect.top += flame_score_frame;
            frame_rect.bottom += flame_score_frame;
            main_bg_se_copy_rect(frame_rect, SCORE_FLAME_CHIPS_POS);

            // mult flame (red)
            frame_rect.left += SCORE_FLAME_FRAME_WIDTH;
            frame_rect.right += SCORE_FLAME_FRAME_WIDTH;
            main_bg_se_copy_rect(frame_rect, SCORE_FLAME_MULT_POS);
        }
    }
}

void game_playing_change_background(enum BackgroundId current_background)
{
    if (current_background != BG_CARD_SELECTING)
    {
        change_background(BG_CARD_SELECTING);
        set_background(BG_CARD_PLAYING
        ); // Not sure why this is set? I'm pretty sure at the end of the change_background function
           // it sets it to BG_CARD_SELECTING anyway. -emiyl
    }

    REG_WIN0V = (REG_WIN0V << 8) | 0xA0; // Set window 0 bottom to 160
    toggle_windows(true, true);

    for (int i = 0; i <= 2; i++)
    {
        main_bg_se_move_rect_1_tile_vert(HAND_BG_RECT_SELECTING, SCREEN_DOWN);
    }

    tte_erase_rect_wrapper(HAND_SIZE_RECT_SELECT);
}

void game_selecting_change_background(enum BackgroundId current_background)
{
    tte_erase_rect_wrapper(HAND_SIZE_RECT_PLAYING);
    REG_WIN0V = (REG_WIN0V << 8) | 0x80; // Set window 0 top to 128

    if (current_background == BG_CARD_PLAYING)
    {
        int offset = 11;
        memcpy16(
            &se_mem[MAIN_BG_SBB][SE_ROW_LEN * offset],
            &background_gfxMap[SE_ROW_LEN * offset],
            SE_ROW_LEN * 8
        );
    }
    else
    {
        toggle_windows(true, true); // Enable window 0 for the hand shadow

        // Load the tiles and palette
        // Background
        GRIT_CPY(pal_bg_mem, background_gfxPal);
        GRIT_CPY(&tile8_mem[MAIN_BG_CBB], background_gfxTiles);
        GRIT_CPY(&se_mem[MAIN_BG_SBB], background_gfxMap);

        int current_blind = get_current_blind();

        if (current_blind == BLIND_TYPE_BIG) // Change text and palette depending on blind type
        {
            main_bg_se_copy_rect(BIG_BLIND_TITLE_SRC_RECT, TOP_LEFT_BLIND_TITLE_POINT);
        }
        else if (current_blind == BLIND_TYPE_BOSS)
        {
            main_bg_se_copy_rect(BOSS_BLIND_TITLE_SRC_RECT, TOP_LEFT_BLIND_TITLE_POINT);

            affine_background_set_color(blind_get_color(BLIND_TYPE_BOSS, BLIND_SHADOW_COLOR_INDEX));
        }

        bg_copy_current_item_to_top_left_panel();

        // This would change the palette of the background to match the blind, but the backgroun
        // doesn't use the blind token's exact colors so a different approach is required
        memset16(
            &pal_bg_mem[BLIND_BG_PRIMARY_PID],
            blind_get_color(current_blind, BLIND_BACKGROUND_MAIN_COLOR_INDEX),
            1
        );
        memset16(
            &pal_bg_mem[BLIND_BG_SECONDARY_PID],
            blind_get_color(current_blind, BLIND_BACKGROUND_SECONDARY_COLOR_INDEX),
            1
        );
        memset16(
            &pal_bg_mem[BLIND_BG_SHADOW_PID],
            blind_get_color(current_blind, BLIND_BACKGROUND_SHADOW_COLOR_INDEX),
            1
        );

        for (int i = 0; i < game_playing_button_row_get_size(NULL); i++)
        {
            button_set_highlight(&game_playing_buttons[i], false);
        }
    }
}

void game_playing_on_update(void* ctx)
{
    // Background logic (thissss might be moved to the card'ssss logic later. I'm a sssssnake)
    if (hand_state == HAND_DRAW || hand_state == HAND_DISCARD || hand_state == HAND_SELECT)
    {
        change_background(BG_CARD_SELECTING);
    }
    else if (hand_state != HAND_SHUFFLING)
    {
        change_background(BG_CARD_PLAYING);
    }

    RoundProps* props = (RoundProps*)ctx;

    game_playing_process_input_and_state(props);

    // Card logic

    game_playing_process_card_draw(props);

    game_playing_discarded_cards_loop(props);

    discarded_card = false;

    cards_in_hand_update_loop(props);
    played_cards_update_loop(props);

    game_playing_ui_text_update(props);

    // animate score flames if we exceed the score requirement
    game_playing_process_flaming_score(props);
}