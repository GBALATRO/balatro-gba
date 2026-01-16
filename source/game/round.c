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
#include "game/selection.h"
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

#define HIGHLIGHT_COLOR 0xFFFF

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

// Forward declarations - extern functions and constants from game.c
extern void game_change_state(enum GameState new_game_state);
extern int game_playing_button_row_get_size(void);
extern void change_background(enum BackgroundId id);
extern void deck_shuffle(void);
extern void sort_hand_by_suit(void);
extern void sort_hand_by_rank(void);
extern void set_hand(void);
extern void display_score(u32 value);
extern void display_temp_score(u32 value);
extern void display_chips(void);
extern void display_mult(void);
extern void display_round(int value);
extern bool shift_null_card_to_end(int idx);
extern Card* deck_pop(void);
extern void deck_push(Card* card);
extern void discard_push(Card* card);
extern Card* discard_pop(void);
extern int deck_get_size(void);
extern int deck_get_max_size(void);
extern int hand_get_size(void);
extern int get_hand_top(void);
extern int get_straight_and_flush_size(void);
extern bool is_shortcut_joker_active(void);
extern void swap_cards_in_hand(int idx1, int idx2);
extern void increment_blind(enum BlindState increment_reason);
extern void played_push(CardObject* card);
extern int find_flush_in_played_cards(
    CardObject** played,
    int top,
    int min_len,
    bool* out_selection
);
extern int find_straight_in_played_cards(
    CardObject** played,
    int top,
    bool shortcut,
    int min_len,
    bool* out_selection
);
extern void select_paired_cards_in_hand(CardObject* played[], int played_top, bool out[]);
extern void main_bg_se_copy_rect(Rect src_rect, BG_POINT dest_pos);
extern void main_bg_se_copy_expand_3x3_rect(Rect dest_rect, BG_POINT src_pos);
extern void main_bg_se_clear_rect(Rect rect);
extern void tte_erase_rect_wrapper(Rect rect);
extern void tte_erase_screen(void);
extern void play_sfx(mm_word id, mm_word rate, mm_byte volume);
extern u32 u32_protected_add(u32 a, u32 b);
extern u32 u32_protected_mult(u32 a, u32 b);
extern void card_object_shake(CardObject* card_object, mm_word sound_id);
extern void button_press(Button* button);

extern const Rect BLIND_REQ_TEXT_RECT;
extern const Rect DISCARDS_TEXT_RECT;
extern const Rect DECK_SIZE_RECT;
extern const Rect ANTE_TEXT_RECT;
extern const Rect TEMP_SCORE_RECT;
extern const Rect HAND_SIZE_RECT_SELECT;
extern const Rect HAND_SIZE_RECT_PLAYING;
extern const Rect SCORE_FLAME_FRAMES_START;
extern const BG_POINT SCORE_FLAME_CHIPS_POS;
extern const BG_POINT SCORE_FLAME_MULT_POS;

// Global variables from game.c
extern enum BackgroundId background;
extern uint timer;
extern CardObject* hand[MAX_HAND_SIZE];
extern int hand_top;
extern CardObject* played[MAX_SELECTION_SIZE];
extern int played_top;
extern Card* deck[MAX_DECK_SIZE];
extern int deck_top;
extern Card* discard_pile[MAX_DECK_SIZE];
extern int discard_top;
extern SelectionGrid game_playing_selection_grid;
extern Button game_playing_buttons[];
extern int current_blind;
extern int ante;
extern u32 score;
extern u32 temp_score;
extern u32 chips;
extern u32 mult;
extern FIXED lerped_score;
extern FIXED lerped_temp_score;
extern bool retrigger;
extern int hand_size;
extern int cards_drawn;
extern int hand_selections;
extern int scored_card_index;
extern bool sound_played;
extern bool discarded_card;
extern bool score_flames_active;
extern enum HandState hand_state;
extern enum PlayState play_state;
extern enum HandType hand_type;
extern Sprite* playing_blind_token;
extern Sprite* round_end_blind_token;
extern int hands;
extern int discards;
extern int max_hands;
extern int max_discards;
extern int shortcut_joker_count;
extern int four_fingers_joker_count;
extern List _owned_jokers_list;
extern ListItr _joker_scored_itr;
extern ListItr _joker_card_scored_end_itr;
extern ListItr _joker_round_end_itr;

// Forward declarations for static functions defined later in this file
static void hand_deselect_all_cards(void);
static void hand_change_sort(void);
static void hand_select_card(int index);
static inline int hand_sel_idx_to_card_idx(int selection_index);

static bool sort_by_suit = false;

static void reorder_card_sprites_layers(void)
{
    // Update the sprites in the hand by destroying them and creating new ones in the correct order
    // (This feels like a diabolical solution but like literally how else would you do this)
    for (int i = 0; i <= hand_top; i++)
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
    for (int i = 0; i <= hand_top; i++)
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

static void sort_cards(void)
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

void game_round_on_init()
{
    hand_state = HAND_DRAW;
    cards_drawn = 0;
    hand_selections = 0;

    playing_blind_token = blind_token_new(
        current_blind,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        MAX_SELECTION_SIZE + MAX_HAND_SIZE + 1
    ); // Create the blind token sprite at the top left corner
    // TODO: Hide blind token and display it after sliding blind rect animation
    // if (playing_blind_token != NULL)
    //{
    //    obj_hide(playing_blind_token->obj); // Hide the blind token sprite for now
    //}
    round_end_blind_token = blind_token_new(
        current_blind,
        81,
        86,
        MAX_SELECTION_SIZE + MAX_HAND_SIZE + 2
    ); // Create the blind token sprite for round end

    if (round_end_blind_token != NULL)
    {
        obj_hide(round_end_blind_token->obj); // Hide the blind token sprite for now
    }

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

    deck_shuffle(); // Shuffle the deck at the start of the round

    /* Note that since cards_in_hand_update_loop() handles card highlight there's no need
     * to call a selection changed callback to highlight the initial card, this wouldn't work
     * otherwise or for the buttons.
     */
    game_playing_selection_grid.selection = GAME_PLAYING_INIT_SEL;
}

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

bool game_playing_hand_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    int prev_card_idx = UNDEFINED;
    int next_card_idx = UNDEFINED;

    // Do not use FRAMES(x) here as we are counting real frames ignoring game speed
    card_moved_too_fast = (timer - selection_hit_timer) < card_swap_time_threshold;

    if (prev_selection->y == GAME_PLAYING_HAND_SEL_Y)
    {
        prev_card_idx = hand_sel_idx_to_card_idx(prev_selection->x);
    }

    if (new_selection->y == GAME_PLAYING_HAND_SEL_Y)
    {
        next_card_idx = hand_sel_idx_to_card_idx(new_selection->x);
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
            swap_cards_in_hand(prev_card_idx, next_card_idx);
            moving_card = true;
            reorder_card_sprites_layers();

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
            hand_select_card(prev_card_idx);
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

void game_playing_hand_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
    {
        selection_hit_timer = timer;
    }
    else if (key_released(SELECT_CARD))
    {
        if (!moving_card && !card_selected_instead_of_moved)
        {
            hand_select_card(hand_sel_idx_to_card_idx(selection->x));
        }
        moving_card = false;
        card_moved_too_fast = false;
        card_selected_instead_of_moved = false;
        selection_hit_timer = TM_ZERO;
    }
    else if (key_hit(DESELECT_CARDS))
    {
        hand_deselect_all_cards();
        set_hand();
    }
    else if (key_hit(SORT_HAND))
    {
        hand_change_sort();
    }
}

static inline void game_playing_button_set_highlight(int btn_idx, bool highlight)
{
    button_set_highlight(&game_playing_buttons[btn_idx], highlight);
}

bool game_playing_button_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    // The selection grid system only guarantees that the new selection is within bounds
    // but not the previous one...
    // As of writing (PR #348), this check is not strictly needed for this row but it is
    // left in, in case that ever changes. It can be reconsidered and removed.
    if (prev_selection->y == row_idx && prev_selection->x >= 0 &&
        prev_selection->x < game_playing_button_row_get_size())
    {
        game_playing_button_set_highlight(prev_selection->x, false);
    }

    if (new_selection->y == row_idx)
    {
        game_playing_button_set_highlight(new_selection->x, true);
    }

    return true;
}

void game_playing_button_row_on_key_hit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
    {
        button_press(&game_playing_buttons[selection->x]);
    }
}

static void hand_deselect_all_cards(void)
{
    bool any_cards_deselected = false;
    for (int i = 0; i <= get_hand_top(); i++)
    {
        if (card_object_is_selected(hand[i]))
        {
            card_object_set_selected(hand[i], false);
            hand_selections--;
            any_cards_deselected = true;
        }
    }

    if (any_cards_deselected)
    {
        play_sfx(SFX_CARD_DESELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
}

static void hand_change_sort(void)
{
    sort_by_suit = !sort_by_suit;
    sort_cards();
}

extern bool can_play_hand(void)
{
    if (hand_state != HAND_SELECT || hand_selections == 0)
        return false;
    return true;
}

/**
 * @brief Converts a selection index from the selection grid into a card index within the hand array
 * @param selection_index The selection index from the selection grid.
 * @return The index within the hand stack array.
 * Note that the result is not valid if hand size is 0.
 */
static inline int hand_sel_idx_to_card_idx(int selection_index)
{
    // This is because the hand is drawn from right to left.
    // There is no particular reason for why that was done, it's just how it was done.
    // Maybe one day it can be reverted and made consistent so this conversion is not needed.
    return hand_get_size() - selection_index - 1;
}

static void hand_select_card(int index)
{
    if (index < 0 || index >= hand_get_size() || hand_state != HAND_SELECT || hand[index] == NULL)
        return;

    if (card_object_is_selected(hand[index]))
    {
        card_object_set_selected(hand[index], false);
        hand_selections--;
        play_sfx(SFX_CARD_DESELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
    else if (hand_selections < MAX_SELECTION_SIZE)
    {
        card_object_set_selected(hand[index], true);
        hand_selections++;
        play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
    set_hand();
}

static inline void game_playing_process_hand_select_input(void)
{
    selection_grid_process_input(&game_playing_selection_grid);
}

static inline void card_draw(void)
{
    if (deck_top < 0 || hand_top >= hand_size - 1 || hand_top >= MAX_HAND_SIZE - 1)
        return;

    CardObject* card_object = card_object_new(deck_pop());

    const FIXED deck_x = int2fx(CARD_DRAW_POS.x);
    const FIXED deck_y = int2fx(CARD_DRAW_POS.y);

    card_object->sprite_object->x = deck_x;
    card_object->sprite_object->y = deck_y;

    hand[++hand_top] = card_object;

    // Sort the hand after drawing a card
    sort_cards();

    play_sfx(
        SFX_CARD_DRAW,
        MM_BASE_PITCH_RATE + cards_drawn * PITCH_STEP_DRAW_SFX,
        SFX_DEFAULT_VOLUME
    );
}

static inline void game_playing_handle_round_over(void)
{
    enum GameState next_state = GAME_STATE_ROUND_END;

    if (score >= blind_get_requirement(current_blind, ante))
    {
        if (current_blind == BLIND_TYPE_BOSS)
        {
            if (ante < MAX_ANTE)
            {
                display_ante(++ante);
            }
            else
            {
                next_state = GAME_STATE_WIN;
            }
        }
    }
    else if (hands == 0)
    {
        next_state = GAME_STATE_LOSE;
    }

    game_change_state(next_state);
}

static inline void card_in_hand_loop_handle_discard_and_shuffling(
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
    if (card_object_is_selected(hand[card_idx]) || hand_state == HAND_SHUFFLING)
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

            if (hand[card_idx]->sprite_object->x >= *hand_x)
            {
                discard_push(hand[card_idx]->card);
                card_object_destroy(&hand[card_idx]);
                reorder_card_sprites_layers();

                hand_top--;
                // This technically isn't drawing cards, I'm just reusing the variable
                cards_drawn++;
                sound_played = false;
                timer = TM_ZERO;

                *hand_y = hand[card_idx]->sprite_object->y;
                *hand_x = hand[card_idx]->sprite_object->x;
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

    if (card_idx == 0 && discarded_card == false && timer % FRAMES(10) == 0)
    {
        // This is never reached in the case of HAND_SHUFFLING. Not sure why but that's how it's
        // supposed to be.
        hand_state = HAND_DRAW;
        sound_played = false;
        cards_drawn = 0;
        hand_selections = 0;
        timer = TM_ZERO;
        *break_loop = true;
        return;
    };
}

static inline void select_flush_and_straight_cards_in_played_hand(void)
{
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

static inline void select_all_five_cards_in_played_hand(void)
{
    for (int i = 0; i <= played_top; i++)
    {
        card_object_set_selected(played[i], true);
    }
}

static inline void select_four_of_a_kind_cards_in_played_hand(void)
{
    // find four cards with the same rank
    // If there are 5 cards selected we just need to find the one card that doesn't match, and
    // select the others
    if (played_top >= 3)
    {
        int unmatched_index = -1;

        for (int i = 0; i <= played_top; i++)
        {
            if (played[i]->card->rank != played[(i + 1) % played_top]->card->rank &&
                played[i]->card->rank != played[(i + 2) % played_top]->card->rank)
            {
                unmatched_index = i;
                break;
            }
        }

        for (int i = 0; i <= played_top; i++)
        {
            if (i != unmatched_index)
            {
                card_object_set_selected(played[i], true);
            }
        }
    }
    else // If there are only 4 cards selected we know they match
    {
        for (int i = 0; i <= played_top; i++)
        {
            card_object_set_selected(played[i], true);
        }
    }
}

static inline void select_three_of_a_kind_cards_in_played_hand(void)
{
    // find three cards with the same rank
    for (int i = 0; i <= played_top - 1; i++)
    {
        for (int j = i + 1; j <= played_top; j++)
        {
            if (played[i]->card->rank == played[j]->card->rank)
            {
                card_object_set_selected(played[i], true);
                card_object_set_selected(played[j], true);

                for (int k = j + 1; k <= played_top; k++)
                {
                    if (played[i]->card->rank == played[k]->card->rank &&
                        !card_object_is_selected(played[k]))
                    {
                        card_object_set_selected(played[k], true);
                        break;
                    }
                }

                break;
            }
        }

        if (card_object_is_selected(played[i]))
            break;
    }
}

static inline void select_two_pair_cards_in_played_hand(void)
{
    // find two pairs of cards with the same rank
    int i;

    for (i = 0; i <= played_top - 1; i++)
    {
        for (int j = i + 1; j <= played_top; j++)
        {
            if (played[i]->card->rank == played[j]->card->rank)
            {
                card_object_set_selected(played[i], true);
                card_object_set_selected(played[j], true);

                break;
            }
        }

        if (card_object_is_selected(played[i]))
            break;
    }

    for (; i <= played_top - 1; i++) // Find second pair
    {
        for (int j = i + 1; j <= played_top; j++)
        {
            if (played[i]->card->rank == played[j]->card->rank &&
                !card_object_is_selected(played[i]) && !card_object_is_selected(played[j]))
            {
                card_object_set_selected(played[i], true);
                card_object_set_selected(played[j], true);
                break;
            }
        }
    }
}

static inline void select_pair_cards_in_played_hand(void)
{
    // find two cards with the same rank
    for (int i = 0; i <= played_top - 1; i++)
    {
        for (int j = i + 1; j <= played_top; j++)
        {
            if (played[i]->card->rank == played[j]->card->rank)
            {
                card_object_set_selected(played[i], true);
                card_object_set_selected(played[j], true);
                break;
            }
        }

        if (card_object_is_selected(played[i]))
            break;
    }
}

static inline void select_highcard_cards_in_played_hand(void)
{
    // find the card with the highest rank in the hand
    int highest_rank_index = 0;

    for (int i = 0; i <= played_top; i++)
    {
        if (played[i]->card->rank > played[highest_rank_index]->card->rank)
        {
            highest_rank_index = i;
        }
    }

    card_object_set_selected(played[highest_rank_index], true);
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

static inline bool game_round_is_over(void)
{
    return hands == 0 || score >= blind_get_requirement(current_blind, ante);
}

// Basically a copy of HAND_DISCARD
// returns true if the current card has been discarded
static bool play_ended_played_cards_update(int played_idx)
{
    if (!discarded_card && timer > FRAMES(40))
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
        if (played[played_idx]->sprite_object->x >= int2fx(CARD_DISCARD_PNT.x))
        {
            discard_push(played[played_idx]->card); // Push the card to the discard pile
            card_object_destroy(&played[played_idx]);

            // played_top--;
            cards_drawn++; // This technically isn't drawing cards, I'm just reusing the variable
            sound_played = false; // Allow for the sound for the next card to be played

            // we reached hand_top, all cards have been discarded
            if (played_idx == played_top)
            {
                if (game_round_is_over())
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
                played_top = -1; // Reset the played stack
                scored_card_index = 0;
                _joker_scored_itr = list_itr_create(&_owned_jokers_list);
                timer = TM_ZERO;
            }

            return true; // return early to avoid accessing played[played_idx] == NULL
        }

        // put target X position off screen to the right
        played[played_idx]->sprite_object->tx = int2fx(CARD_DISCARD_PNT.x);
        discarded_card = true;
    }

    return false;
}

static inline void play_starting_played_cards_update(int played_idx)
{
    bool card_selected = card_object_is_selected(played[played_top - scored_card_index]);
    if (played_idx == played_top && (timer % FRAMES(10) == 0 || !card_selected) &&
        timer > FRAMES(40))
    {
        scored_card_index--;

        if (scored_card_index == 0)
        {
            _joker_scored_itr = list_itr_create(&_owned_jokers_list);
            timer = TM_ZERO;
            play_state = PLAY_BEFORE_SCORING;
        }
    }

    played[played_idx]->sprite_object->tx =
        int2fx(HAND_PLAY_POS.x) + (int2fx(played_top - played_idx) - int2fx(played_top) / 2) * -27;
    played[played_idx]->sprite_object->ty = int2fx(HAND_PLAY_POS.y);

    card_selected = card_object_is_selected(played[played_idx]);
    if (card_selected && played_top - played_idx >= scored_card_index)
    {
        played[played_idx]->sprite_object->ty -= int2fx(10);
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
static inline bool play_scoring_cards_update(void)
{
    if (timer % FRAMES(30) == 0 && timer > FRAMES(40))
    {
        // We are about to score played Cards.
        // Start from the current card index
        // and seek the next scoring card
        while (scored_card_index <= played_top &&
               !card_object_is_selected(played[scored_card_index]))
        {
            scored_card_index++;
        }

        // go to the next state if there are no cards left to score
        if (scored_card_index > played_top)
        {
            // reuse these variables for held cards
            _joker_scored_itr = list_itr_create(&_owned_jokers_list);
            scored_card_index = hand_top;

            play_state = PLAY_SCORING_HELD_CARDS;

            return false;
        }

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        CardObject* scored_card_object = played[scored_card_index];

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
            _joker_scored_itr = list_itr_create(&_owned_jokers_list);
            _joker_card_scored_end_itr = list_itr_create(&_owned_jokers_list);
        }

        play_state = PLAY_SCORING_CARD_JOKERS;
        return true;
    }

    return false;
}

// Activate "on scored" Jokers for the previous scored card if any
// returns true if the scoring loop has returned early
static inline bool play_scoring_card_jokers_update(void)
{
    if (timer % FRAMES(30) == 0 && timer > FRAMES(40))
    {
        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        // since we sought the next scoring card index in the previous state,
        // scored_card_index is guaranteed to be a scoring card
        if (check_and_score_joker_for_event(
                &_joker_scored_itr,
                played[scored_card_index],
                JOKER_EVENT_ON_CARD_SCORED
            ))
        {
            return true;
        }

        // Trigger all Jokers that have an effect when a card finishes scoring
        // (e.g. retriggers) after activating all the other scored_card Jokers normally
        if (check_and_score_joker_for_event(
                &_joker_card_scored_end_itr,
                played[scored_card_index],
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
        scored_card_index++;
        play_state = PLAY_SCORING_CARDS;
    }

    return false;
}

// returns true if the scoring loop has returned early
static inline bool play_scoring_held_cards_update(int played_idx)
{
    if (played_idx == 0 && (timer % FRAMES(30) == 0) && timer > FRAMES(40))
    {
        tte_erase_rect_wrapper(HELD_CARDS_SCORES_RECT);

        // Go through all held cards and see if they activate Jokers
        for (; scored_card_index >= 0; scored_card_index--)
        {
            if (check_and_score_joker_for_event(
                    &_joker_scored_itr,
                    hand[scored_card_index],
                    JOKER_EVENT_ON_CARD_HELD
                ))
            {
                card_object_shake(hand[scored_card_index], SFX_CARD_SELECT);
                return true;
            }
            _joker_scored_itr = list_itr_create(&_owned_jokers_list);
        }

        scored_card_index = 0;
        _joker_round_end_itr = list_itr_create(&_owned_jokers_list);
        play_state = PLAY_SCORING_INDEPENDENT_JOKERS;
    }

    return false;
}

// Score Jokers normally (independent)
// returns true if the scoring loop has returned early
static inline bool play_scoring_independent_jokers_update(int played_idx)
{
    if (played_idx == 0 && (timer % FRAMES(30) == 0) && timer > FRAMES(40))
    {

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        if (check_and_score_joker_for_event(&_joker_scored_itr, NULL, JOKER_EVENT_INDEPENDENT))
        {
            return true;
        }

        scored_card_index =
            played_top + 1; // Reset the scored card index to the top of the played stack

        play_state = PLAY_SCORING_HAND_SCORED_END;
    }

    return false;
}

// Trigger hand end effect for all jokers once they are done scoring
static inline bool play_scoring_hand_scored_end_update(int played_idx)
{
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

        timer = TM_ZERO;
        play_state = PLAY_ENDING;
    }

    return false;
}

// This is the reverse of PLAY_STARTING. The cards get reset back to their neutral position
// sequentially
static inline void play_ending_played_cards_update(int played_idx)
{
    bool card_selected = card_object_is_selected(played[played_top - scored_card_index]);
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
            timer = TM_ZERO;
            play_state = PLAY_ENDED;
        }
    }

    if (card_object_is_selected(played[played_idx]) && played_top - played_idx >= scored_card_index)
    {
        played[played_idx]->sprite_object->ty = int2fx(HAND_PLAY_POS.y);
    }
}

static inline void played_cards_update_loop(void)
{
    // So this one is a bit fucking weird because I have to work kinda backwards for everything
    // because of the order of the pushed cards from the hand to the play stack (also crazy that the
    // company that published Balatro is called "Playstack" and this is a play stack, but I digress)
    for (int played_idx = 0; played_idx <= played_top; played_idx++)
    {
        if (played[played_idx] == NULL)
        {
            continue;
        }

        if (card_object_get_sprite(played[played_idx]) == NULL)
        {
            // Set the sprite for the played card object
            card_object_set_sprite(played[played_idx], played_idx + MAX_HAND_SIZE);
        }

        switch (play_state)
        {
            case PLAY_STARTING:

                play_starting_played_cards_update(played_idx);
                break;

            case PLAY_BEFORE_SCORING:

                if (play_before_scoring_cards_update())
                {
                    return;
                }
                break;

            case PLAY_SCORING_CARDS:

                if (play_scoring_cards_update())
                {
                    return;
                }
                break;

            case PLAY_SCORING_CARD_JOKERS:

                if (play_scoring_card_jokers_update())
                {
                    return;
                }
                break;

            case PLAY_SCORING_HELD_CARDS:

                if (play_scoring_held_cards_update(played_idx))
                {
                    return;
                }
                break;

            case PLAY_SCORING_INDEPENDENT_JOKERS:

                if (play_scoring_independent_jokers_update(played_idx))
                {
                    return;
                }
                break;

            case PLAY_SCORING_HAND_SCORED_END:

                if (play_scoring_hand_scored_end_update(played_idx))
                {
                    return;
                }
                break;

            case PLAY_ENDING:

                play_ending_played_cards_update(played_idx);
                break;

            case PLAY_ENDED:

                if (play_ended_played_cards_update(played_idx))
                {
                    // we continue here instead of returning for performance
                    // to instantly go to the next card to discard at played_idx+1,
                    // instead of  starting over from index 0 and going up
                    // to that card again
                    continue;
                }
                break;
        }

        played[played_idx]->sprite_object->tscale = FIX_ONE;
        card_object_update(played[played_idx]);
    }
}

static inline int hand_get_max_size(void)
{
    return hand_size;
}

static inline void game_playing_process_input_and_state(void)
{
    if (hand_state == HAND_SELECT)
    {
        game_playing_process_hand_select_input();
    }
    else if (play_state == PLAY_ENDING)
    {
        if (mult > 0)
        {
            // protect against score overflow
            temp_score = u32_protected_mult(chips, mult);
            lerped_temp_score = int2fx(temp_score);
            lerped_score = int2fx(score);

            display_temp_score(temp_score);

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
    else if (play_state == PLAY_ENDED && timer % FRAMES(TM_SCORE_LERP_INTERVAL) == 0)
    {
        /* Using fixed point in case the score is lower than NUM_SCORE_LERP_STEPS and then
         * then the division rounds it down to 0 and it's never added to the total.
         * The operation is equivalent to
         * fxdiv(int2fx(temp_score * GAME_SPEED), int2fx(NUM_SCORE_LERP_STEPS))
         */
        lerped_temp_score -= int2fx(temp_score * GAME_SPEED) / NUM_SCORE_LERP_STEPS;
        lerped_score += int2fx(temp_score * GAME_SPEED) / NUM_SCORE_LERP_STEPS;

        if (lerped_temp_score > 0)
        {
            // Set the score display first because it's more important
            // in case there isn't enough time within the frame to display both
            display_score(fx2uint(lerped_score));

            display_temp_score(fx2uint(lerped_temp_score));
        }
        else
        {
            score = u32_protected_add(score, temp_score);
            temp_score = 0;
            lerped_temp_score = 0;
            lerped_score = 0;

            tte_erase_rect_wrapper(TEMP_SCORE_RECT); // Just erase the temp score

            display_score(score);
        }
    }
}

static inline void game_playing_process_card_draw()
{
    if (hand_state == HAND_DRAW && cards_drawn < hand_size)
    {
        if (timer % FRAMES(10) == 0) // Draw a card every 10 frames
        {
            cards_drawn++;
            card_draw();
        }
    }
    else if (hand_state == HAND_DRAW)
    {
        hand_state = HAND_SELECT; // Change the hand state to select after drawing all the cards
        cards_drawn = 0;
        timer = TM_ZERO;
    }
}

static inline void game_playing_discarded_cards_loop(void)
{
    // Discarded cards loop (mainly for shuffling)
    if (hand_get_size() == 0 && hand_state == HAND_SHUFFLING && discard_top >= -1 &&
        timer > FRAMES(10))
    {
        // Change the background to the round end background. This is how it works in Balatro, so
        // I'm doing it this way too.
        change_background(BG_ROUND_END);

        // We take each discarded card and put it back into the deck with a short animation
        static CardObject* discarded_card_object = NULL;
        if (discarded_card_object == NULL)
        {
            discarded_card_object = card_object_new(discard_pop());
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
                deck_push(discarded_card_object->card); // Put the card back into the deck
                card_object_destroy(&discarded_card_object);

                play_sfx(
                    SFX_CARD_DRAW,
                    MM_BASE_PITCH_RATE + PITCH_STEP_UNDISCARD_SFX,
                    SFX_DEFAULT_VOLUME
                );
            }
        }

        // If there are no more discarded cards, stop shuffling
        if (discard_top == -1 && discarded_card_object == NULL)
        {
            // After HAND_SHUFFLING the round is over
            game_playing_handle_round_over();
        }
    }
}

static inline void select_cards_in_played_hand()
{
    switch (hand_type) // select the cards that apply to the hand type
    {
        case NONE:
            break;
        case HIGH_CARD:
            select_highcard_cards_in_played_hand();
            break;
        case PAIR:
            select_pair_cards_in_played_hand();
            break;
        case TWO_PAIR:
            select_two_pair_cards_in_played_hand();
            break;
        case THREE_OF_A_KIND:
            select_three_of_a_kind_cards_in_played_hand();
            break;
        case FOUR_OF_A_KIND:
            select_four_of_a_kind_cards_in_played_hand();
            break;
        case STRAIGHT:
            /* FALL THROUGH */
        case FLUSH:
            /* FALL THROUGH */
        case STRAIGHT_FLUSH:
            /* FALL THROUGH */
        case ROYAL_FLUSH:
            select_flush_and_straight_cards_in_played_hand();
            break;
        case FULL_HOUSE:
            /* FALL THROUGH */
        case FIVE_OF_A_KIND:
            /* FALL THROUGH */
        case FLUSH_HOUSE:
            /* FALL THROUGH */
        case FLUSH_FIVE: // Select all played cards in the hand
            select_all_five_cards_in_played_hand();
            break;
    }
}

static inline void cards_in_hand_update_loop(void)
{
    int selected_card_idx = hand_sel_idx_to_card_idx(game_playing_selection_grid.selection.x);

    // TODO: Break this function up into smaller ones, Gods be good
    // Start from the end of the hand and work backwards because that's how Balatro does it
    for (int i = hand_top + 1; i >= 0; i--)
    {
        if (hand[i] != NULL)
        {
            FIXED hand_x = int2fx(HAND_START_POS.x);
            FIXED hand_y = int2fx(HAND_START_POS.y);

            switch (hand_state)
            {
                case HAND_DRAW:
                    hand_x =
                        hand_x + (int2fx(i) - int2fx(hand_top) / 2) * -HAND_SPACING_LUT[hand_top];
                    break;
                case HAND_SELECT:
                    bool is_focused =
                        (i == selected_card_idx &&
                         game_playing_selection_grid.selection.y == GAME_PLAYING_HAND_SEL_Y);

                    bool is_selected = card_object_is_selected(hand[i]);
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

                    if (i != selected_card_idx && hand[i]->sprite_object->y > hand_y)
                    {
                        hand[i]->sprite_object->y = hand_y;
                        hand[i]->sprite_object->vy = 0;
                    }

                    hand_x =
                        hand_x + (int2fx(i) - int2fx(hand_top) / 2) *
                                     -HAND_SPACING_LUT[hand_top]; // TODO: Change this later to
                                                                  // reference a 2D LUT of positions
                    break;
                case HAND_SHUFFLING:
                    /* FALL THROUGH */
                case HAND_DISCARD: // TODO: Add sound
                    bool break_loop;
                    card_in_hand_loop_handle_discard_and_shuffling(
                        i,
                        &hand_x,
                        &hand_y,
                        &break_loop
                    );
                    if (break_loop)
                        break;

                    break;
                case HAND_PLAY:
                    hand_x =
                        hand_x + (int2fx(i) - int2fx(hand_top) / 2) * -HAND_SPACING_LUT[hand_top];
                    hand_y += int2fx(24);

                    if (card_object_is_selected(hand[i]) && discarded_card == false &&
                        timer % FRAMES(10) == 0)
                    {
                        card_object_set_selected(hand[i], false);
                        played_push(hand[i]);
                        sprite_destroy(&hand[i]->sprite_object->sprite);
                        hand[i] = NULL;
                        reorder_card_sprites_layers();

                        play_sfx(
                            SFX_CARD_DRAW,
                            MM_BASE_PITCH_RATE + cards_drawn * PITCH_STEP_DISCARD_SFX,
                            SFX_DEFAULT_VOLUME
                        );

                        hand_top--;
                        hand_selections--;
                        cards_drawn++;

                        discarded_card = true;
                    }

                    if (i == 0 && discarded_card == false && timer % FRAMES(10) == 0)
                    {
                        hand_state = HAND_PLAYING;
                        cards_drawn = 0;
                        hand_selections = 0;
                        timer = TM_ZERO;
                        scored_card_index = played_top + 1;

                        select_cards_in_played_hand();
                    }

                    break;
                // Don't need to do anything here, just wait for the player to select cards
                case HAND_PLAYING:
                    hand_x =
                        hand_x + (int2fx(i) - int2fx(hand_top) / 2) * -HAND_SPACING_LUT[hand_top];
                    hand_y += int2fx(24);
                    break;
            }

            hand[i]->sprite_object->tx = hand_x;
            hand[i]->sprite_object->ty = hand_y;
            card_object_update(hand[i]);
        }
    }
}

static inline void game_playing_ui_text_update(void)
{
    static int last_hand_size = 0;
    static int last_deck_size = 0;

    if (last_hand_size != hand_get_size() || last_deck_size != deck_get_size())
    {
        if (background == BG_CARD_SELECTING)
        {
            // Hand size/max size
            tte_printf(
                "#{P:%d,%d; cx:0x%X000}%d/%d",
                HAND_SIZE_RECT_SELECT.left,
                HAND_SIZE_RECT_SELECT.top,
                TTE_WHITE_PB,
                hand_get_size(),
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
                hand_get_size(),
                hand_get_max_size()
            );
        }

        // Deck size/max size
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%d/%d",
            DECK_SIZE_RECT.left,
            DECK_SIZE_RECT.top,
            TTE_WHITE_PB,
            deck_get_size(),
            deck_get_max_size()
        );

        last_hand_size = hand_get_size();
        last_deck_size = deck_get_size();
    }
}

static inline void game_playing_process_flaming_score(void)
{
    static u8 flame_score_frame = 0;

    if (score_flames_active)
    {
        if (timer % SCORE_FLAMES_ANIM_FREQ == 0)
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
        background = BG_CARD_PLAYING;
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

        for (int i = 0; i < game_playing_button_row_get_size(); i++)
        {
            button_set_highlight(&game_playing_buttons[i], false);
        }
    }
}

void game_playing_on_update(void)
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

    game_playing_process_input_and_state();

    // Card logic

    game_playing_process_card_draw();

    game_playing_discarded_cards_loop();

    discarded_card = false;

    cards_in_hand_update_loop();
    played_cards_update_loop();

    game_playing_ui_text_update();

    // animate score flames if we exceed the score requirement
    game_playing_process_flaming_score();
}