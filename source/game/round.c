/**
 * @file round.c
 * @brief Data structures and implementation of functions relative to the Rounds we play
 */

#include "game/round.h"

#include "audio_utils.h"
#include "background_gfx.h"
#include "button.h"
#include "game.h"
#include "game/common_ui.h"
#include "game/joker_row.h"
#include "graphic_utils.h"
#include "hand.h"
#include "joker.h"
#include "layout.h"
#include "list.h"
#include "selection_grid.h"
#include "skip_tag.h"
#include "soundbank.h"
#include "timer.h"
#include "util.h"

#include <stdlib.h>
#include <tonc.h>

/*******************************************************************************
 * CONSTS
 ******************************************************************************/

// Palette IDs
#define PLAY_HAND_BTN_PAL_IDX           6
#define PLAY_HAND_BTN_BORDER_PAL_IDX    7
#define DISCARD_BTN_PAL_IDX             13
#define DISCARD_BTN_BORDER_PAL_IDX      8
#define SORT_BTNS_PAL_IDX               9
#define SORT_BY_RANK_BTN_BORDER_PAL_IDX 22
#define SORT_BY_SUIT_BTN_BORDER_PAL_IDX 23
#define BLIND_BG_SHADOW_PAL_IDX         5
#define BLIND_BG_SECONDARY_PAL_IDX      18
#define BLIND_BG_PRIMARY_PAL_IDX        19

// Naming the stage where cards return from the discard pile to the deck "undiscard"
#define PITCH_STEP_DISCARD_SFX   (-64)
#define PITCH_STEP_DRAW_SFX      24
#define PITCH_STEP_UNDISCARD_SFX (2 * PITCH_STEP_DRAW_SFX)

/* This needs to stay a power of 2 and small enough
 * for the lerping to be done before the next hand is drawn.
 */
#define NUM_SCORE_LERP_STEPS   16
#define TM_SCORE_LERP_INTERVAL 2

#define GAME_PLAYING_HAND_SEL_Y 1

// Pixel sizes
#define CARD_FOCUSED_UNSEL_Y 10
#define CARD_UNFOCUSED_SEL_Y 15
#define CARD_FOCUSED_SEL_Y   20
#define SCORED_CARD_TEXT_Y   48

// clang-format off

// Flaming score animation frames
#define SCORE_FLAMES_ANIM_FREQ  5 // animation will run at 12FPS
#define NUM_SCORE_FLAMES_FRAMES 8 // Chips and Mult flame frames are next to one another
#define SCORE_FLAME_FRAME_WIDTH 3 // so we only need to offset to get the next ones
static const Rect     SCORE_FLAME_RESET          = {26,      20,      28,     20};
static const Rect     SCORE_FLAME_FRAMES_START   = {26,      21,      28,     21};
static const BG_POINT SCORE_FLAME_CHIPS_POS      = {1,       9};
static const BG_POINT SCORE_FLAME_MULT_POS       = {5,       9};

/* Contains the shop icon/current blind etc. 
 * The difference between TOP_LEFT_PANEL_ANIM_RECT and TOP_LEFT_PANEL_RECT 
 * is due to an overlap between the bottom of the top left panel
 * and the top of the score panel in the tiles connecting them.
 * TOP_LEFT_PANEL_ANIM_RECT should be used for animations, 
 * TOP_LEFT_PANEL_RECT for copies etc. but mind the overlap
 */
static const BG_POINT TOP_LEFT_BLIND_TITLE_POINT = {0,       21};
static const Rect     BIG_BLIND_TITLE_SRC_RECT   = {0,       26,     8,      26 };
static const Rect     BOSS_BLIND_TITLE_SRC_RECT  = {0,       27,     8,      27 };

// Rects for TTE (in pixels)

// The rect for popping menu animations (round end, shop, blinds) 
// - extends beyond the visible screen to the end of the screenblock
// It includes both the target and source position rects. 
// This is because when popping, the target position is blank so we just animate 
// the whole rect so we don't have to track its position
static const Rect     HAND_BG_RECT_SELECTING     = {9,       11,     24,     17  };
static const Rect     HAND_SIZE_RECT_SELECT      = {120,     128,    160,    136 };
static const Rect     HAND_SIZE_RECT_PLAYING     = {120,     152,    160,    160 };
static const Rect     PLAYED_CARDS_SCORES_RECT   = {72,      48,     240,    56  };
static const Rect     HELD_CARDS_SCORES_RECT     = {72,      108,    240,    116 };

static const BG_POINT CARD_DRAW_POS              = {208,     110};
static const BG_POINT CARD_DISCARD_PNT           = {240,     70};
static const BG_POINT HAND_START_POS             = {120,     90};
static const BG_POINT HAND_PLAY_POS              = {120,     70};
// clang-format on

/*******************************************************************************
 * ROUND SELECTIONGRID
 ******************************************************************************/

static int game_round_button_row_get_size(void);
static bool game_round_button_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void game_round_button_row_on_key_hit(SelectionGrid* selection_grid, Selection* selection);

static void game_round_hand_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);

static bool game_round_hand_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

static int game_round_hand_row_get_size(void);

static int hand_sel_idx_to_card_idx(int selection_index);

// clang-format off
static SelectionGridRow game_round_selection_rows[] = {
    {
        0,
        jokers_sel_row_get_size,
        jokers_sel_row_on_selection_changed,
        jokers_sel_row_on_key_transit,
        {.wrap = false}
    },
    {
        1,
        game_round_hand_row_get_size,
        game_round_hand_row_on_selection_changed,
        game_round_hand_row_on_key_transit,
        {.wrap = true}
    },
    {
        2,
        game_round_button_row_get_size,
        game_round_button_row_on_selection_changed,
        game_round_button_row_on_key_hit,
        {.wrap = false}
    }
};
// clang-format on

static const Selection GAME_PLAYING_INIT_SEL = {0, 1};

static SelectionGrid game_round_selection_grid = {
    game_round_selection_rows,
    NUM_ELEM_IN_ARR(game_round_selection_rows),
    GAME_PLAYING_INIT_SEL
};

/*******************************************************************************
 * ROUND BUTTONS
 ******************************************************************************/

static void game_round_discard_on_pressed(void);
static void game_round_play_hand_on_pressed(void);
static void game_round_sort_by_rank_on_pressed(void);
static void game_round_sort_by_suit_on_pressed(void);

static bool can_discard_hand(void);
static bool can_play_hand(void);

// clang-format off
// Array of buttons by horizontal selection index (x)
static Button game_round_buttons[] = {
    {PLAY_HAND_BTN_BORDER_PAL_IDX,    PLAY_HAND_BTN_PAL_IDX, game_round_play_hand_on_pressed,    can_play_hand   },
    {SORT_BY_RANK_BTN_BORDER_PAL_IDX, SORT_BTNS_PAL_IDX,     game_round_sort_by_rank_on_pressed, NULL            },
    {SORT_BY_SUIT_BTN_BORDER_PAL_IDX, SORT_BTNS_PAL_IDX,     game_round_sort_by_suit_on_pressed, NULL            },
    {DISCARD_BTN_BORDER_PAL_IDX,      DISCARD_BTN_PAL_IDX,   game_round_discard_on_pressed,      can_discard_hand},
};
// clang-format on

/*******************************************************************************
 * CARD PLAYING STATE
 ******************************************************************************/

enum PlayState
{
    PLAY_STARTING,
    PLAY_BEFORE_SCORING,
    PLAY_SCORING_CARDS,
    PLAY_SCORING_CARD_JOKERS,
    PLAY_SCORING_HELD_CARDS,
    PLAY_SCORING_INDEPENDENT_JOKERS,
    PLAY_SCORING_HAND_SCORED_END,
    PLAY_ENDING,
    PLAY_ENDED
};

static enum PlayState play_state = PLAY_STARTING;

/*******************************************************************************
 * INTERNAL VARIABLES
 ******************************************************************************/

// This is a stupid way to do this but I don't care
static const int HAND_SPACING_LUT[MAX_HAND_SIZE] =
    {28, 28, 28, 28, 27, 21, 18, 15, 13, 12, 10, 9, 9, 8, 8, 7};

// Stacks
static CardObject* s_played_hand[MAX_SELECTION_SIZE] = {NULL};
static int s_played_top = -1;

static Card* s_discard_pile[MAX_DECK_SIZE] = {NULL};
static int s_discard_top = -1;

// Keeping track of cards scored
static int s_scored_card_index = 0;
static bool s_retrigger = false;

// Keeping track of what Jokers are scored at each step
static ListItr s_joker_scored_itr;
static ListItr s_joker_card_scored_end_itr;
static ListItr s_joker_round_end_itr;

static u32 s_temp_score = 0; // This is the score that shows in the same spot as the hand type.
static bool s_score_flames_active = false;
static FIXED s_lerped_score = 0;
static FIXED s_lerped_temp_score = 0;

// discarded cards specific
static bool s_sound_played = false;
static bool s_discarded_card = false;

static int s_cards_drawn = 0;
static int s_cards_discarded = 0;

/*******************************************************************************
 * UTILS FUNCTIONS
 ******************************************************************************/

int get_played_top(void)
{
    return s_played_top;
}

int get_played_size(void)
{
    return s_played_top + 1;
}

/**
 * @brief Push a new card to play at the end of the associated array
 *
 * @param card_object the CardObject we just played
 */
static inline void played_push(CardObject* card_object)
{
    if (s_played_top >= MAX_SELECTION_SIZE - 1)
        return;
    s_played_hand[++s_played_top] = card_object;
}

int get_discard_top(void)
{
    return s_discard_top;
}

/**
 * @brief Push a new card to discard at the end of the associated array
 *
 * @param card_object the CardObject we just discarded
 *
 * @sa played_push
 */
static inline void discard_push(Card* card)
{
    if (s_discard_top >= MAX_DECK_SIZE - 1)
        return;
    s_discard_pile[++s_discard_top] = card;
}

/**
 * @brief Remove and recover the last discarded card
 *
 * @return pointer to the CardObject representing the last discarded card
 */
static inline Card* discard_pop()
{
    if (s_discard_top < 0)
        return NULL;
    return s_discard_pile[s_discard_top--];
}

int get_scored_card_index(void)
{
    return s_scored_card_index;
}

void set_retrigger(bool new_retrigger)
{
    s_retrigger = new_retrigger;
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

    for (int i = 0; i <= s_played_top; i++)
    {
        /* The difference from get_hand_distribution() (not checking if card is selected)
         * is in line Balatro behavior,
         * see https://github.com/GBALATRO/balatro-gba/issues/341#issuecomment-3691363488
         */
        if (!s_played_hand[i])
            continue;
        ranks_out[s_played_hand[i]->card->rank]++;
        suits_out[s_played_hand[i]->card->suit]++;
    }
}

/*******************************************************************************
 * BACKGROUND MANIPULATION
 ******************************************************************************/

void game_round_change_background_selecting(void)
{
    tte_erase_rect_wrapper(HAND_SIZE_RECT_PLAYING);
    REG_WIN0V = (REG_WIN0V << 8) | 0x80; // Set window 0 top to 128

    if (get_current_background() == BG_CARD_PLAYING)
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

        if (g_game_vars.current_blind ==
            BLIND_TYPE_BIG) // Change text and palette depending on blind type
        {
            main_bg_se_copy_rect(BIG_BLIND_TITLE_SRC_RECT, TOP_LEFT_BLIND_TITLE_POINT);
        }
        else if (g_game_vars.current_blind >= BLIND_TYPE_BOSS)
        {
            main_bg_se_copy_rect(BOSS_BLIND_TITLE_SRC_RECT, TOP_LEFT_BLIND_TITLE_POINT);
        }

        // Copies the Blind item into the top left panel
        main_bg_se_copy_rect(TOP_LEFT_ITEM_SRC_RECT, TOP_LEFT_PANEL_POINT);

        // This would change the palette of the background to match the blind, but the backgroun
        // doesn't use the blind token's exact colors so a different approach is required
        memset16(
            &pal_bg_mem[BLIND_BG_PRIMARY_PAL_IDX],
            blind_get_color(g_game_vars.current_blind, BLIND_BACKGROUND_MAIN_COLOR_INDEX),
            1
        );
        memset16(
            &pal_bg_mem[BLIND_BG_SECONDARY_PAL_IDX],
            blind_get_color(g_game_vars.current_blind, BLIND_BACKGROUND_SECONDARY_COLOR_INDEX),
            1
        );
        memset16(
            &pal_bg_mem[BLIND_BG_SHADOW_PAL_IDX],
            blind_get_color(g_game_vars.current_blind, BLIND_BACKGROUND_SHADOW_COLOR_INDEX),
            1
        );

        for (int i = 0; i < NUM_ELEM_IN_ARR(game_round_buttons); i++)
        {
            button_set_highlight(&game_round_buttons[i], false);
        }
    }
}

void game_round_change_background_playing(void)
{
    if (get_current_background() != BG_CARD_SELECTING)
    {
        change_background(BG_CARD_SELECTING, false);
    }

    REG_WIN0V = (REG_WIN0V << 8) | 0xA0; // Set window 0 bottom to 160
    toggle_windows(true, true);

    for (int i = 0; i <= 2; i++)
    {
        main_bg_se_move_rect_1_tile_vert(HAND_BG_RECT_SELECTING, SCREEN_DOWN);
    }

    tte_erase_rect_wrapper(HAND_SIZE_RECT_SELECT);
}

/*******************************************************************************
 * BUTTONS IMPLEMENTATION
 ******************************************************************************/

/**
 * @brief Determines whether the "Discard" button can be pressed or not
 *
 * @return true if the hand can be discarded, false otherwise
 */
static bool can_discard_hand(void)
{
    return (
        g_game_vars.discards > 0 && get_hand_state() == HAND_SELECT &&
        hand_get_nb_selected_cards() > 0
    );
}

/**
 * @brief Determines whether the "Play" button can be pressed or not
 *
 * @return true if the hand can be played, false otherwise
 */
static bool can_play_hand(void)
{
    return (
        g_game_vars.hands > 0 && get_hand_state() == HAND_SELECT && hand_get_nb_selected_cards() > 0
    );
}

/**
 * @brief Triggers the discard of the currently selected cards in hand
 */
static inline void game_round_execute_discard(void)
{
    if (!can_discard_hand())
        return;

    set_hand_state(HAND_DISCARD);
    --g_game_vars.discards;
    display_discards();
    compute_hand_value_info();
}

/**
 * @brief "Discard" button implementation.
 */
static void game_round_discard_on_pressed(void)
{
    if (!can_discard_hand())
        return;

    game_round_execute_discard();

    // Move back to hand selection
    selection_grid_move_selection_vert(&game_round_selection_grid, -1);
}

/**
 * @brief "Rank" sorting button implementation.
 */
static void game_round_sort_by_rank_on_pressed(void)
{
    hand_change_sort(false);
}

/**
 * @brief "Suit" sorting button implementation.
 */
static void game_round_sort_by_suit_on_pressed(void)
{
    hand_change_sort(true);
}

/**
 * @brief Triggers the playing of the currently selected cards in hand
 */
static inline void game_round_execute_play_hand(void)
{
    if (!can_play_hand())
        return;

    set_hand_state(HAND_PLAY);

    --g_game_vars.hands;
    display_hands();

    g_game_vars.nb_played_hands[get_hand_type() - 1]++;
}

/**
 * @brief "Play" button implementation.
 */
static void game_round_play_hand_on_pressed(void)
{
    if (!can_play_hand())
        return;

    game_round_execute_play_hand();

    // Move back to hand selection
    selection_grid_move_selection_vert(&game_round_selection_grid, -1);
}

static int game_round_hand_row_get_size(void)
{
    return hand_nb_held_cards();
}

/*******************************************************************************
 * CARD MOVING LOGIC
 ******************************************************************************/

// true if and only if we are currently moving a card around
static bool s_moving_card = false;

// This will prevent us from moving cards around if we selected one
// by moving too fast after pressing the A button
static bool s_card_moved_too_fast = false;
static bool s_card_selected_instead_of_moved = false;

// After pressing A, if we press Left/Right too fast, we should select the card
// and change focus to the next one, instead of swapping them
// This should fix inputs sometimes not registering when quickly selecting cards
static const int CARD_SWAP_TIME_THRESHOLD = 6;
static int s_selection_hit_timer = UNDEFINED;

static bool game_round_hand_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    int prev_card_idx = UNDEFINED;
    int next_card_idx = UNDEFINED;

    // Do not use FRAMES(x) here as we are counting real frames ignoring game speed
    s_card_moved_too_fast = (s_selection_hit_timer != UNDEFINED) &&
                            (g_game_vars.timer - s_selection_hit_timer) < CARD_SWAP_TIME_THRESHOLD;

    if (prev_selection->y == GAME_PLAYING_HAND_SEL_Y)
    {
        prev_card_idx = hand_sel_idx_to_card_idx(prev_selection->x);
    }

    if (new_selection->y == GAME_PLAYING_HAND_SEL_Y)
    {
        next_card_idx = hand_sel_idx_to_card_idx(new_selection->x);
    }

    bool on_the_same_row = new_selection->y == prev_selection->y; // == GAME_PLAYING_HAND_SEL_Y

    if (on_the_same_row && key_is_down(SELECT_CARD) && !s_card_moved_too_fast &&
        !s_card_selected_instead_of_moved)
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
            s_moving_card = true;

            /* Not calling sprite_object_set_focus() because focus is handled by
             * cards_in_hand_update_loop() based on the selection grid value...
             */
            play_sfx(
                SFX_CARD_FOCUS,
                MM_BASE_PITCH_RATE + rng_get_u32() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
                SFX_DEFAULT_VOLUME
            );
        }
    }
    else
    {
        // select current card if we tried moving it too fast
        if (key_released(SELECT_CARD) || (s_card_moved_too_fast && !s_moving_card))
        {
            hand_select_card(prev_card_idx);
            s_card_selected_instead_of_moved = true;
        }
        if (next_card_idx != UNDEFINED)
        {
            /* Not calling sprite_object_set_focus() because focus is handled by
             * cards_in_hand_update_loop() based on the selection grid value...
             */
            play_sfx(
                SFX_CARD_FOCUS,
                MM_BASE_PITCH_RATE + rng_get_u32() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
                SFX_DEFAULT_VOLUME
            );
        }
    }

    return true;
}

static void game_round_hand_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
    {
        s_selection_hit_timer = g_game_vars.timer;
    }
    else if (key_released(SELECT_CARD))
    {
        if (!s_moving_card && !s_card_selected_instead_of_moved)
        {
            hand_select_card(hand_sel_idx_to_card_idx(selection->x));
        }
        s_moving_card = false;
        s_card_moved_too_fast = false;
        s_card_selected_instead_of_moved = false;
        s_selection_hit_timer = UNDEFINED;
    }
    else if (key_hit(DESELECT_CARDS))
    {
        hand_deselect_all_cards();
        compute_hand_value_info();
    }
    else if (key_hit(PLAY_HAND_KEY))
    {
        game_round_execute_play_hand();
    }
    else if (key_hit(DISCARD_HAND_KEY))
    {
        game_round_execute_discard();
    }
}

/*******************************************************************************
 * OTHER SELECTIONGRID IMPLEMENTATION
 ******************************************************************************/

static int game_round_button_row_get_size(void)
{
    return NUM_ELEM_IN_ARR(game_round_buttons);
}

static inline void game_round_button_set_highlight(int btn_idx, bool highlight)
{
    button_set_highlight(&game_round_buttons[btn_idx], highlight);
}

static bool game_round_button_row_on_selection_changed(
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
        prev_selection->x < game_round_button_row_get_size())
    {
        game_round_button_set_highlight(prev_selection->x, false);
    }

    if (new_selection->y == row_idx)
    {
        game_round_button_set_highlight(new_selection->x, true);
    }

    return true;
}

static void game_round_button_row_on_key_hit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
    {
        button_press(&game_round_buttons[selection->x]);
    }
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
    return hand_nb_held_cards() - selection_index - 1;
}

static inline void game_round_process_hand_select_input(void)
{
    selection_grid_process_input(&game_round_selection_grid);
}

/**
 * @brief Evaluates if we have won or lost when we can no longer play so that we land on the correct
 *         Game Over screen
 */
static inline void game_round_handle_round_over(void)
{
    enum GameState next_state = GAME_STATE_ROUND_END;

    if (g_game_vars.score >= blind_get_requirement(g_game_vars.current_blind, g_game_vars.ante))
    {
        if (g_game_vars.current_blind > BLIND_TYPE_BIG)
        {
            if (g_game_vars.ante < MAX_ANTE)
            {
                g_game_vars.ante++;
                display_ante();

                // mark current boss blind as beaten and allow for reroll
                set_blind_beaten(g_game_vars.next_boss_blind);
            }
            else
            {
                next_state = GAME_STATE_WIN;
            }
        }
    }
    else if (g_game_vars.hands == 0)
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
    if (get_hand_state() != HAND_DISCARD && get_hand_state() != HAND_SHUFFLING)
    {
        // Assumes hand_state is one of these
        return;
    }

    CardObject** hand = get_hand_array();

    *break_loop = false;
    if (card_object_is_selected(hand[card_idx]) || get_hand_state() == HAND_SHUFFLING)
    {
        if (!s_discarded_card)
        {
            *hand_x = int2fx(CARD_DISCARD_PNT.x);
            *hand_y = int2fx(CARD_DISCARD_PNT.y);

            if (!s_sound_played)
            {
                play_sfx(
                    SFX_CARD_DRAW,
                    MM_BASE_PITCH_RATE + s_cards_discarded * PITCH_STEP_DISCARD_SFX,
                    SFX_DEFAULT_VOLUME
                );
                s_sound_played = true;
            }

            if (hand[card_idx]->x >= *hand_x)
            {
                discard_push(hand[card_idx]->card);

                // Remove discarded card from hand and shift the ones after it
                card_object_destroy(&hand[card_idx]);
                shift_null_card_to_end(card_idx);
                set_hand_top(get_hand_top() - 1);

                s_cards_discarded++;
                s_sound_played = false;
                g_game_vars.timer = TM_ZERO;

                // Protect against an edge case where `card_idx == hand_top` before discarding. In
                // that case, calling `reorder_card_sprites_layers` cannot shift the discarded card,
                // and we end up with an element at `card_idx` that is still NULL.
                if (hand[card_idx] != NULL)
                {
                    *hand_y = hand[card_idx]->y;
                    *hand_x = hand[card_idx]->x;
                }
            }

            s_discarded_card = true;
        }
        else
        {
            if (get_hand_state() == HAND_DISCARD)
            {
                // Don't raise the card if we're mass discarding, it looks stupid.
                *hand_y -= int2fx(15);
            }
            else // hand_state == HAND_SHUFFLING
            {
                *hand_y += int2fx(24);
            }
            *hand_x = *hand_x + (int2fx(card_idx) - int2fx(get_hand_top()) / 2) *
                                    -HAND_SPACING_LUT[get_hand_top()];
        }
    }
    else
    {
        *hand_x = *hand_x + (int2fx(card_idx) - int2fx(get_hand_top()) / 2) *
                                -HAND_SPACING_LUT[get_hand_top()];
    }

    if (card_idx == 0 && s_discarded_card == false && g_game_vars.timer % FRAMES(10) == 0)
    {
        // This is never reached in the case of HAND_SHUFFLING. Not sure why but that's how it's
        // supposed to be.
        set_hand_state(HAND_DRAW);
        s_sound_played = false;
        s_cards_discarded = 0;
        hand_set_nb_selected_cards(0);
        g_game_vars.timer = TM_ZERO;
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
    if (get_hand_type() == FLUSH || get_hand_type() == STRAIGHT_FLUSH ||
        get_hand_type() == ROYAL_FLUSH)
    {
        bool flush_selection[MAX_HAND_SIZE] = {false};
        find_flush_in_played_cards(s_played_hand, s_played_top, min_len, flush_selection);
        // Add the results into the final selection
        for (int i = 0; i <= s_played_top; i++)
        {
            final_selection[i] = flush_selection[i];
        }
    }

    // If we have a straight in our hand
    if (get_hand_type() == STRAIGHT || get_hand_type() == STRAIGHT_FLUSH ||
        get_hand_type() == ROYAL_FLUSH)
    {
        bool straight_selection[MAX_HAND_SIZE] = {false};
        find_straight_in_played_cards(s_played_hand, s_played_top, min_len, straight_selection);
        // Add the results into the final selection
        for (int i = 0; i <= s_played_top; i++)
        {
            final_selection[i] = final_selection[i] || straight_selection[i];
        }
        // If Four Fingers is active, pairs can happen in a valid straight
        // If Four Fingers is not active, pairs are impossible so this will not affect things
        select_paired_cards_in_hand(s_played_hand, s_played_top, final_selection);
    }

    // Finally, set mark the cards as selected based final_selection
    for (int i = 0; i <= s_played_top; i++)
    {
        if (final_selection[i])
        {
            card_object_set_selected(s_played_hand[i], true);
        }
    }
}

static inline void select_all_five_cards_in_played_hand(void)
{
    for (int i = 0; i <= s_played_top; i++)
    {
        card_object_set_selected(s_played_hand[i], true);
    }
}

static inline void select_four_of_a_kind_cards_in_played_hand(void)
{
    int played_size = get_played_size();

    // find four cards with the same rank
    // If there are 5 cards selected we just need to find the one card that doesn't match, and
    // select the others
    if (played_size >= 5)
    {
        int unmatched_index = -1;

        for (int i = 0; i <= played_size; i++)
        {
            if (s_played_hand[i]->card->rank != s_played_hand[(i + 1) % played_size]->card->rank &&
                s_played_hand[i]->card->rank != s_played_hand[(i + 2) % played_size]->card->rank)
            {
                unmatched_index = i;
                break;
            }
        }

        for (int i = 0; i <= played_size; i++)
        {
            if (i != unmatched_index)
            {
                card_object_set_selected(s_played_hand[i], true);
            }
        }
    }
    else // If there are only 4 cards selected we know they match
    {
        for (int i = 0; i <= played_size; i++)
        {
            card_object_set_selected(s_played_hand[i], true);
        }
    }
}

static inline void select_three_of_a_kind_cards_in_played_hand(void)
{
    // find three cards with the same rank
    for (int i = 0; i <= s_played_top - 1; i++)
    {
        for (int j = i + 1; j <= s_played_top; j++)
        {
            if (s_played_hand[i]->card->rank == s_played_hand[j]->card->rank)
            {
                card_object_set_selected(s_played_hand[i], true);
                card_object_set_selected(s_played_hand[j], true);

                for (int k = j + 1; k <= s_played_top; k++)
                {
                    if (s_played_hand[i]->card->rank == s_played_hand[k]->card->rank &&
                        !card_object_is_selected(s_played_hand[k]))
                    {
                        card_object_set_selected(s_played_hand[k], true);
                        break;
                    }
                }

                break;
            }
        }

        if (card_object_is_selected(s_played_hand[i]))
            break;
    }
}

static inline void select_two_pair_cards_in_played_hand(void)
{
    // find two pairs of cards with the same rank
    int i;

    for (i = 0; i <= s_played_top - 1; i++)
    {
        for (int j = i + 1; j <= s_played_top; j++)
        {
            if (s_played_hand[i]->card->rank == s_played_hand[j]->card->rank)
            {
                card_object_set_selected(s_played_hand[i], true);
                card_object_set_selected(s_played_hand[j], true);

                break;
            }
        }

        if (card_object_is_selected(s_played_hand[i]))
            break;
    }

    for (; i <= s_played_top - 1; i++) // Find second pair
    {
        for (int j = i + 1; j <= s_played_top; j++)
        {
            if (s_played_hand[i]->card->rank == s_played_hand[j]->card->rank &&
                !card_object_is_selected(s_played_hand[i]) &&
                !card_object_is_selected(s_played_hand[j]))
            {
                card_object_set_selected(s_played_hand[i], true);
                card_object_set_selected(s_played_hand[j], true);
                break;
            }
        }
    }
}

static inline void select_pair_cards_in_played_hand(void)
{
    // find two cards with the same rank
    for (int i = 0; i <= s_played_top - 1; i++)
    {
        for (int j = i + 1; j <= s_played_top; j++)
        {
            if (s_played_hand[i]->card->rank == s_played_hand[j]->card->rank)
            {
                card_object_set_selected(s_played_hand[i], true);
                card_object_set_selected(s_played_hand[j], true);
                break;
            }
        }

        if (card_object_is_selected(s_played_hand[i]))
            break;
    }
}

static inline void select_highcard_cards_in_played_hand(void)
{
    // find the card with the highest rank in the hand
    int highest_rank_index = 0;

    for (int i = 0; i <= s_played_top; i++)
    {
        if (s_played_hand[i]->card->rank > s_played_hand[highest_rank_index]->card->rank)
        {
            highest_rank_index = i;
        }
    }

    card_object_set_selected(s_played_hand[highest_rank_index], true);
}

/**
 * @brief Determines if the round is over, be it because we lost or won the round.
 *
 * @return true if the round is over, false if we can still play.
 */
static inline bool game_round_is_over(void)
{
    return g_game_vars.hands == 0 ||
           g_game_vars.score >= blind_get_requirement(g_game_vars.current_blind, g_game_vars.ante);
}

static inline void game_round_process_input_and_state(void)
{
    if (get_hand_state() == HAND_TAGS)
    {
        // We're checking for scoring Tags in a general way here, but only the Juggler can
        // really apply on round start
        if (skip_tag_process_get_effect() == SKIP_TAG_EFFECT_END)
        {
            set_hand_state(HAND_DRAW);
            g_game_vars.timer = TM_ZERO;
        }
    }
    else if (get_hand_state() == HAND_SELECT)
    {
        game_round_process_hand_select_input();
    }
    else if (play_state == PLAY_ENDING)
    {
        if (g_game_vars.mult > 0)
        {
            // protect against score overflow
            s_temp_score = u32_protected_mult(g_game_vars.chips, g_game_vars.mult);
            s_lerped_temp_score = int2fx(s_temp_score);
            s_lerped_score = int2fx(g_game_vars.score);

            if (s_temp_score > g_game_vars.best_hand_score)
                g_game_vars.best_hand_score = s_temp_score;

            display_temp_score(s_temp_score);

            g_game_vars.chips = 0;
            g_game_vars.mult = 0;
            display_mult();
            display_chips();

            static const int SCORE_CALC_SFX_PITCH_SHIFT = -102; // -10% OF MM_BASE_PITCH_RATE
            static const int SCORE_CALC_SFX_VOLUME = 204;       // 80% MM_SFX_FULL_VOLUME

            // The chips calculation SFX is the same as button
            play_sfx(
                SFX_BUTTON,
                MM_BASE_PITCH_RATE + SCORE_CALC_SFX_PITCH_SHIFT,
                SCORE_CALC_SFX_VOLUME
            );
        }
    }
    else if (play_state == PLAY_ENDED && g_game_vars.timer % FRAMES(TM_SCORE_LERP_INTERVAL) == 0)
    {
        /* Using fixed point in case the score is lower than NUM_SCORE_LERP_STEPS and then
         * then the division rounds it down to 0 and it's never added to the total.
         * The operation is equivalent to
         * fxdiv(int2fx(temp_score * g_game_vars.game_speed), int2fx(NUM_SCORE_LERP_STEPS))
         */
        s_lerped_temp_score -= int2fx(s_temp_score * g_game_vars.game_speed) / NUM_SCORE_LERP_STEPS;
        s_lerped_score += int2fx(s_temp_score * g_game_vars.game_speed) / NUM_SCORE_LERP_STEPS;

        if (s_lerped_temp_score > 0)
        {
            // Set the score display first because it's more important
            // in case there isn't enough time within the frame to display both
            display_score(fx2uint(s_lerped_score));
            display_temp_score(fx2uint(s_lerped_temp_score));
        }
        else
        {
            g_game_vars.score = u32_protected_add(g_game_vars.score, s_temp_score);
            s_temp_score = 0;
            s_lerped_temp_score = 0;
            s_lerped_score = 0;

            erase_temp_score();
            display_score(g_game_vars.score);
        }
    }
}

/**
 * @brief Draw the next card at the top of the Deck and play a little Sprite animation to position
 *         it in our hand.
 */
static inline void card_draw(void)
{
    if (get_deck_top() < 0 || get_hand_top() >= g_game_vars.hand_size - 1 ||
        get_hand_top() >= MAX_HAND_SIZE - 1)
        return;

    CardObject* card_object = card_object_new(deck_pop());

    const FIXED deck_x = int2fx(CARD_DRAW_POS.x);
    const FIXED deck_y = int2fx(CARD_DRAW_POS.y);

    card_object->x = deck_x;
    card_object->y = deck_y;

    set_hand_top(get_hand_top() + 1);
    get_hand_array()[get_hand_top()] = card_object;

    card_object_set_sprite(card_object, CARD_SPRITE, get_hand_top());

    // Sort the hand after drawing a card
    sort_cards();

    play_sfx(
        SFX_CARD_DRAW,
        MM_BASE_PITCH_RATE + s_cards_drawn * PITCH_STEP_DRAW_SFX,
        SFX_DEFAULT_VOLUME
    );
}

static inline void game_round_process_card_draw(void)
{
    if (get_hand_state() == HAND_DRAW && s_cards_drawn < g_game_vars.hand_size)
    {
        if (g_game_vars.timer % FRAMES(10) == 0) // Draw a card every 10 frames
        {
            s_cards_drawn++;
            card_draw();
        }
    }
    else if (get_hand_state() == HAND_DRAW)
    {
        set_hand_state(HAND_SELECT); // Change the hand state to select after drawing all the cards
        s_cards_drawn = 0;
        g_game_vars.timer = TM_ZERO;
    }
}

static inline void game_round_discarded_cards_loop(void)
{
    // Discarded cards loop (mainly for shuffling)
    if (hand_nb_held_cards() == 0 && get_hand_state() == HAND_SHUFFLING && s_discard_top >= -1 &&
        g_game_vars.timer > FRAMES(10))
    {
        // Change the background to the round end background. This is how it works in Balatro, so
        // I'm doing it this way too.
        change_background(BG_ROUND_END, false);

        // We take each discarded card and put it back into the deck with a short animation
        static CardObject* discarded_card_object = NULL;
        if (s_discard_top >= 0 && discarded_card_object == NULL)
        {
            discarded_card_object = card_object_new(discard_pop());

            // Set the sprite for the discarded card object
            card_object_set_sprite(discarded_card_object, CARD_PLAYED_SPRITE, 0);
            sprite_object_reset_transform((SpriteObject*)discarded_card_object);

            discarded_card_object->tx = int2fx(204);
            discarded_card_object->ty = int2fx(112);
            discarded_card_object->x = int2fx(240);
            discarded_card_object->y = int2fx(80);
        }
        else
        {
            if (discarded_card_object->y >= discarded_card_object->ty)
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
        if (s_discard_top == -1 && discarded_card_object == NULL)
        {
            // After HAND_SHUFFLING the round is over
            game_round_handle_round_over();
        }
    }
}

static inline void select_cards_in_played_hand(void)
{
    switch (get_hand_type()) // select the cards that apply to the hand type
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
    int selected_card_idx = hand_sel_idx_to_card_idx(game_round_selection_grid.selection.x);

    // TODO: Break this function up into smaller ones, Gods be good
    // Start from the end of the hand and work backwards because that's how Balatro does it
    CardObject** hand = get_hand_array();

    for (int i = get_hand_top(); i >= 0; i--)
    {
        if (hand[i] != NULL)
        {
            FIXED hand_x = int2fx(HAND_START_POS.x);
            FIXED hand_y = int2fx(HAND_START_POS.y);

            switch (get_hand_state())
            {
                // Nothing to do here
                case HAND_TAGS:
                    break;
                case HAND_DRAW:
                    hand_x = hand_x + (int2fx(i) - int2fx(get_hand_top()) / 2) *
                                          -HAND_SPACING_LUT[get_hand_top()];
                    break;
                case HAND_SELECT:
                    bool is_focused =
                        (i == selected_card_idx &&
                         game_round_selection_grid.selection.y == GAME_PLAYING_HAND_SEL_Y);

                    if (is_focused && !card_object_is_selected(hand[i]))
                    {
                        hand_y -= int2fx(CARD_FOCUSED_UNSEL_Y);
                    }
                    else if (!is_focused && card_object_is_selected(hand[i]))
                    {
                        hand_y -= int2fx(CARD_UNFOCUSED_SEL_Y);
                    }
                    else if (is_focused && card_object_is_selected(hand[i]))
                    {
                        hand_y -= int2fx(CARD_FOCUSED_SEL_Y);
                    }
                    if (i != selected_card_idx && hand[i]->y > hand_y)
                    {
                        hand[i]->y = hand_y;
                        // Set target y to match y. Ensures target is updated even when vy becomes
                        // 0, preventing immediate snap back.
                        hand[i]->ty = hand_y;
                        hand[i]->vy = 0;
                    }

                    hand_x = hand_x + (int2fx(i) - int2fx(get_hand_top()) / 2) *
                                          -HAND_SPACING_LUT[get_hand_top()]; // TODO: Change this
                                                                             // later to reference a
                                                                             // 2D LUT of positions
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
                    hand_x = hand_x + (int2fx(i) - int2fx(get_hand_top()) / 2) *
                                          -HAND_SPACING_LUT[get_hand_top()];
                    hand_y += int2fx(24);

                    if (card_object_is_selected(hand[i]) && s_discarded_card == false &&
                        g_game_vars.timer % FRAMES(10) == 0)
                    {
                        card_object_set_selected(hand[i], false);
                        played_push(hand[i]);
                        sprite_destroy(&hand[i]->sprite);
                        hand[i] = NULL;
                        reorder_card_sprites_layers();

                        play_sfx(
                            SFX_CARD_DRAW,
                            MM_BASE_PITCH_RATE + s_cards_drawn * PITCH_STEP_DISCARD_SFX,
                            SFX_DEFAULT_VOLUME
                        );

                        set_hand_top(get_hand_top() - 1);
                        hand_set_nb_selected_cards(hand_get_nb_selected_cards() - 1);
                        s_cards_drawn++;

                        s_discarded_card = true;
                    }

                    if (i == 0 && s_discarded_card == false && g_game_vars.timer % FRAMES(10) == 0)
                    {
                        set_hand_state(HAND_PLAYING);
                        s_cards_drawn = 0;
                        hand_set_nb_selected_cards(0);
                        g_game_vars.timer = TM_ZERO;
                        s_scored_card_index = get_played_size();

                        select_cards_in_played_hand();
                    }

                    break;
                // Don't need to do anything here, just wait for the player to select cards
                case HAND_PLAYING:
                    hand_x = hand_x + (int2fx(i) - int2fx(get_hand_top()) / 2) *
                                          -HAND_SPACING_LUT[get_hand_top()];
                    hand_y += int2fx(24);
                    break;
            }

            hand[i]->tx = hand_x;
            hand[i]->ty = hand_y;
        }
    }
}

static inline void game_round_ui_text_update(void)
{
    static int s_last_hand_size = UNDEFINED;
    static int s_last_hand_max_size = UNDEFINED;
    static int s_last_deck_size = UNDEFINED;

    if (g_game_vars.timer == 1 || s_last_hand_size != hand_nb_held_cards() ||
        s_last_hand_max_size != g_game_vars.hand_size || s_last_deck_size != deck_get_size())
    {
        // Print hand size/max size at correct height depending on background state
        switch (get_current_background())
        {
            case BG_CARD_SELECTING:
                tte_printf(
                    "#{P:%d,%d; cx:0x%X000}%2d/%-2ld",
                    HAND_SIZE_RECT_SELECT.left,
                    HAND_SIZE_RECT_SELECT.top,
                    TTE_WHITE_PB,
                    hand_nb_held_cards(),
                    g_game_vars.hand_size
                );
                break;

            case BG_CARD_PLAYING:
                tte_printf(
                    "#{P:%d,%d; cx:0x%X000}%2d/%-2ld",
                    HAND_SIZE_RECT_PLAYING.left,
                    HAND_SIZE_RECT_PLAYING.top,
                    TTE_WHITE_PB,
                    hand_nb_held_cards(),
                    g_game_vars.hand_size
                );
                break;

            default:
                break;
        }

        // Deck size/max size
        display_deck_size_max();

        s_last_hand_size = hand_nb_held_cards();
        s_last_hand_max_size = g_game_vars.hand_size;
        s_last_deck_size = deck_get_size();
    }
}

void toggle_flaming_score(void)
{
    u32 curr_score = u32_protected_mult(g_game_vars.chips, g_game_vars.mult);
    u32 required_score = blind_get_requirement(g_game_vars.current_blind, g_game_vars.ante);
    if (curr_score >= required_score && !s_score_flames_active)
    {
        // start flaming score
        s_score_flames_active = true;
        return;
    }
    if (curr_score < required_score && s_score_flames_active)
    {
        // stop flaming score and clear rect
        s_score_flames_active = false;

        Rect reset_rect = SCORE_FLAME_RESET;
        main_bg_se_copy_rect(reset_rect, SCORE_FLAME_CHIPS_POS);
        reset_rect.left += SCORE_FLAME_FRAME_WIDTH;
        reset_rect.right += SCORE_FLAME_FRAME_WIDTH;
        main_bg_se_copy_rect(reset_rect, SCORE_FLAME_MULT_POS);
    }
}

static inline void game_round_process_flaming_score(void)
{
    static u8 flame_score_frame = 0;

    if (s_score_flames_active)
    {
        if (g_game_vars.timer % SCORE_FLAMES_ANIM_FREQ == 0)
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

/*******************************************************************************
 * CARD/JOKER SCORING LOGIC
 ******************************************************************************/

/**
 * @brief Iterate over the Jokers List until we encounter one that scores for the specified event.
 *
 * @param starting_joker_itr iterator for the owned Jokers List
 * @param card_object card currently scored, can be NULL for some joker_event
 * @param joker_event the event we are scoring the Jokers for
 *
 * @return true if a scoring Joker was encountered, false if not
 * @sa JokerEvent
 */
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

static inline void play_starting_played_cards_update(int played_idx)
{
    // Protect against out of bounds read in `s_played_hand` array
    bool card_selected =
        (s_played_top < s_scored_card_index)
            ? false
            : card_object_is_selected(s_played_hand[s_played_top - s_scored_card_index]);

    if (played_idx == s_played_top && (g_game_vars.timer % FRAMES(10) == 0 || !card_selected) &&
        g_game_vars.timer > FRAMES(40))
    {
        s_scored_card_index--;

        if (s_scored_card_index == 0)
        {
            s_joker_scored_itr = list_itr_create(get_jokers_list());
            g_game_vars.timer = TM_ZERO;
            play_state = PLAY_BEFORE_SCORING;
        }
    }

    s_played_hand[played_idx]->tx =
        int2fx(HAND_PLAY_POS.x) +
        (int2fx(s_played_top - played_idx) - int2fx(s_played_top) / 2) * -27;
    s_played_hand[played_idx]->ty = int2fx(HAND_PLAY_POS.y);

    card_selected = card_object_is_selected(s_played_hand[played_idx]);
    if (card_selected && s_played_top - played_idx >= s_scored_card_index)
    {
        s_played_hand[played_idx]->ty -= int2fx(10);
    }
}

/**
 * @brief Returns true if the Jokers scoring loop has returned early for event
 *         JOKER_EVENT_ON_HAND_PLAYED.
 *
 * @return bool
 * @sa check_and_score_joker_for_event
 */
static inline bool play_before_scoring_cards_update(void)
{
    // Activate Jokers with an effect just before the hand is scored
    if (check_and_score_joker_for_event(&s_joker_scored_itr, NULL, JOKER_EVENT_ON_HAND_PLAYED))
    {
        return true;
    }

    play_state = PLAY_SCORING_CARDS;
    return false;
}

/**
 * @brief Score all played cards in order, then Jokers after each one.
 *
 * @return true if the Cards scoring loop has returned early.
 */
static inline bool play_scoring_cards_update(void)
{
    if (g_game_vars.timer % FRAMES(30) == 0 && g_game_vars.timer > FRAMES(40))
    {
        // We are about to score played Cards.
        // Start from the current card index
        // and seek the next scoring card
        while (s_scored_card_index <= s_played_top &&
               !card_object_is_selected(s_played_hand[s_scored_card_index]))
        {
            s_scored_card_index++;
        }

        // go to the next state if there are no cards left to score
        if (s_scored_card_index > s_played_top)
        {
            // reuse these variables for held cards
            s_joker_scored_itr = list_itr_create(get_jokers_list());
            s_scored_card_index = get_hand_top();

            play_state = PLAY_SCORING_HELD_CARDS;

            return false;
        }

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        CardObject* scored_card_object = s_played_hand[s_scored_card_index];

        if (card_object_is_selected(scored_card_object))
        {
            // Offset of 1 tile to keep the text on the card
            tte_set_pos(fx2int(scored_card_object->x) + TILE_SIZE, SCORED_CARD_TEXT_Y);

            // Set text color to blue from background memory
            tte_set_special(TTE_BLUE_PB * TTE_SPECIAL_PB_MULT_OFFSET);

            u8 card_value = card_get_value(scored_card_object->card);

            // Write the score to a character buffer variable
            char score_buffer[INT_MAX_DIGITS + 2]; // for '+' and null terminator
            snprintf(score_buffer, sizeof(score_buffer), "+%hhu", card_value);
            tte_write(score_buffer);

            card_object_shake(scored_card_object, SFX_CHIPS_CARD);

            // Relocated card scoring logic here
            g_game_vars.chips = u32_protected_add(g_game_vars.chips, card_value);
            display_chips();

            // Allow Joker scoring
            s_joker_scored_itr = list_itr_create(get_jokers_list());
            s_joker_card_scored_end_itr = list_itr_create(get_jokers_list());
        }

        play_state = PLAY_SCORING_CARD_JOKERS;
        return true;
    }

    return false;
}

/**
 * @brief Activate Jokers for event JOKER_EVENT_ON_CARD_SCORED_END for the previous scored card, if
 *         any.
 *
 * @return true if the Joker scoring loop has returned early
 * @sa check_and_score_joker_for_event
 */
static inline bool play_scoring_card_jokers_update(void)
{
    if (g_game_vars.timer % FRAMES(30) == 0 && g_game_vars.timer > FRAMES(40))
    {
        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        // since we sought the next scoring card index in the previous state,
        // scored_card_index is guaranteed to be a scoring card
        if (check_and_score_joker_for_event(
                &s_joker_scored_itr,
                s_played_hand[s_scored_card_index],
                JOKER_EVENT_ON_CARD_SCORED
            ))
        {
            return true;
        }

        // Trigger all Jokers that have an effect when a card finishes scoring
        // (e.g. retriggers) after activating all the other scored_card Jokers normally
        if (check_and_score_joker_for_event(
                &s_joker_card_scored_end_itr,
                s_played_hand[s_scored_card_index],
                JOKER_EVENT_ON_CARD_SCORED_END
            ))
        {
            // If we just scored a retrigger, return early and go back to the
            // previous state score the same card again without incrementing
            // scored_card_index to score the current card again
            if (s_retrigger)
            {
                s_retrigger = false;
                play_state = PLAY_SCORING_CARDS;
            }
            return true;
        }

        // increment index to start seeking the next scoring card from the next card
        s_scored_card_index++;
        play_state = PLAY_SCORING_CARDS;
    }

    return false;
}

/**
 * @brief Activate Jokers for event JOKER_EVENT_ON_CARD_HELD for the previous scored card, if any.
 *
 * @return true if the Joker scoring loop has returned early
 * @sa check_and_score_joker_for_event
 */
static inline bool play_scoring_held_cards_update(int played_idx)
{
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
    {
        tte_erase_rect_wrapper(HELD_CARDS_SCORES_RECT);

        CardObject** hand = get_hand_array();

        // Go through all held cards and see if they activate Jokers
        for (; s_scored_card_index >= 0; s_scored_card_index--)
        {
            if (check_and_score_joker_for_event(
                    &s_joker_scored_itr,
                    hand[s_scored_card_index],
                    JOKER_EVENT_ON_CARD_HELD
                ))
            {
                card_object_shake(hand[s_scored_card_index], SFX_CARD_SELECT);
                return true;
            }
            s_joker_scored_itr = list_itr_create(get_jokers_list());
        }

        s_scored_card_index = 0;
        s_joker_round_end_itr = list_itr_create(get_jokers_list());
        play_state = PLAY_SCORING_INDEPENDENT_JOKERS;
    }

    return false;
}

/**
 * @brief Score Jokers normally for event JOKER_EVENT_INDEPENDENT.
 *
 * @return true if the Joker scoring loop has returned early
 * @sa check_and_score_joker_for_event
 */
static inline bool play_scoring_independent_jokers_update(int played_idx)
{
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
    {

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        if (check_and_score_joker_for_event(&s_joker_scored_itr, NULL, JOKER_EVENT_INDEPENDENT))
        {
            return true;
        }

        // Reset the scored card index to past the top of the played stack
        s_scored_card_index = get_played_size();

        play_state = PLAY_SCORING_HAND_SCORED_END;
    }

    return false;
}

/**
 * @brief Trigger all Jokers for event JOKER_EVENT_ON_HAND_SCORED_END once they are done scoring.
 *
 * @return true if the Joker scoring loop has returned early
 * @sa check_and_score_joker_for_event
 */
static inline bool play_scoring_hand_scored_end_update(int played_idx)
{
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
    {

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        bool scored = check_and_score_joker_for_event(
            &s_joker_round_end_itr,
            NULL,
            JOKER_EVENT_ON_HAND_SCORED_END
        );

        if (scored)
        {
            return true;
        }

        g_game_vars.timer = TM_ZERO;
        play_state = PLAY_ENDING;
    }

    return false;
}

/**
 * @brief This is the reverse of PLAY_STARTING. The cards get reset back to their neutral position
 *         sequentially.
 *
 * @param played_idx index of the card currently considered
 */
static inline void play_ending_played_cards_update(int played_idx)
{
    // Same protection against out of bounds access as `play_starting_played_cards_update`
    bool card_selected =
        (s_played_top < s_scored_card_index)
            ? false
            : card_object_is_selected(s_played_hand[s_played_top - s_scored_card_index]);

    if (played_idx == s_played_top && (g_game_vars.timer % FRAMES(10) == 0 || !card_selected) &&
        g_game_vars.timer > FRAMES(40))
    {
        s_scored_card_index--;

        /* SFX_CHIPS_ACCUM has been pitch shifted to perserve high frequencies in downsampling.
         * Now it needs to be pitch shifted back to the original frequency.
         */
        int static const CHIPS_ACCUM_SFX_PITCH_RATIO = 2;

        if (s_scored_card_index == 0)
        {
            play_sfx(
                SFX_CHIPS_ACCUM,
                CHIPS_ACCUM_SFX_PITCH_RATIO * MM_BASE_PITCH_RATE,
                SFX_DEFAULT_VOLUME
            );
            g_game_vars.timer = TM_ZERO;
            play_state = PLAY_ENDED;
        }
    }

    if (card_object_is_selected(s_played_hand[played_idx]) &&
        s_played_top - played_idx >= s_scored_card_index)
    {
        s_played_hand[played_idx]->ty = int2fx(HAND_PLAY_POS.y);
    }
}

/**
 * @brief Returns true if the card at index played_idx has been discarded. Basically a copy of
 *         HAND_DISCARD.
 *
 * @param played_idx the index of the played card being considered.
 *
 * @return bool
 */
static bool play_ended_played_cards_update(int played_idx)
{
    if (!s_discarded_card && g_game_vars.timer > FRAMES(40))
    {
        // play the sound only once per card, when it is pushed off-screen to the right
        if (!s_sound_played)
        {
            play_sfx(
                SFX_CARD_DRAW,
                MM_BASE_PITCH_RATE + s_cards_drawn * PITCH_STEP_DISCARD_SFX,
                SFX_DEFAULT_VOLUME
            );
            s_sound_played = true;
        }

        // card has exited the screen, now discard it and set it to NULL
        if (s_played_hand[played_idx]->x >= int2fx(CARD_DISCARD_PNT.x))
        {
            discard_push(s_played_hand[played_idx]->card); // Push the card to the discard pile
            card_object_destroy(&s_played_hand[played_idx]);

            s_cards_drawn++; // This technically isn't drawing cards, I'm just reusing the variable
            s_sound_played = false; // Allow for the sound for the next card to be played

            // we reached hand_top, all cards have been discarded
            if (played_idx == s_played_top)
            {
                if (game_round_is_over())
                {
                    set_hand_state(HAND_SHUFFLING);
                }
                else
                {
                    set_hand_state(HAND_DRAW);
                }

                play_state = PLAY_STARTING;
                s_cards_drawn = 0;
                hand_set_nb_selected_cards(0);
                s_played_top = -1; // Reset the played stack
                s_scored_card_index = 0;
                s_joker_scored_itr = list_itr_create(get_jokers_list());
                g_game_vars.timer = TM_ZERO;
            }

            return true; // return early to avoid accessing played[played_idx] == NULL
        }

        // put target X position off screen to the right
        s_played_hand[played_idx]->tx = int2fx(CARD_DISCARD_PNT.x);
        s_discarded_card = true;
    }

    return false;
}

static inline void played_cards_update_loop(void)
{
    // So this one is a bit fucking weird because I have to work kinda backwards for everything
    // because of the order of the pushed cards from the hand to the play stack (also crazy that the
    // company that published Balatro is called "Playstack" and this is a play stack, but I digress)
    for (int played_idx = 0; played_idx <= s_played_top; played_idx++)
    {
        if (s_played_hand[played_idx] == NULL)
        {
            continue;
        }

        if (card_object_get_sprite(s_played_hand[played_idx]) == NULL)
        {
            // Set the sprite for the played card object
            card_object_set_sprite(s_played_hand[played_idx], CARD_PLAYED_SPRITE, played_idx);
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

        s_played_hand[played_idx]->tscale = FIX_ONE;
    }
}

/*******************************************************************************
 * FOUND STATE FUNCTIONS
 ******************************************************************************/

void game_round_on_init(void)
{
    s_joker_scored_itr = list_itr_create(get_jokers_list());

    g_game_vars.timer = TM_ZERO;
    set_hand_state(HAND_TAGS);

    hand_set_nb_selected_cards(0);
    s_cards_drawn = 0;

    sprite_destroy(&g_game_vars.playing_blind_token);
    g_game_vars.playing_blind_token = blind_token_new(
        g_game_vars.current_blind,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        PLAYING_BLIND_TOKEN_LAYER
    ); // Create the blind token sprite at the top left corner
    // TODO: Hide blind token and display it after sliding blind rect animation
    // if (g_game_vars.playing_blind_token != NULL)
    //{
    //    obj_hide(g_game_vars.playing_blind_token->obj); // Hide the blind token sprite for now
    //}
    sprite_destroy(&g_game_vars.round_end_blind_token);
    g_game_vars.round_end_blind_token = blind_token_new(
        g_game_vars.current_blind,
        81,
        86,
        ROUND_END_BLIND_TOKEN_LAYER
    ); // Create the blind token sprite for round end

    if (g_game_vars.round_end_blind_token != NULL)
    {
        obj_hide(g_game_vars.round_end_blind_token->obj); // Hide the blind token sprite for now
    }

    Rect blind_req_text_rect = BLIND_REQ_TEXT_RECT;
    u32 blind_requirement = blind_get_requirement(g_game_vars.current_blind, g_game_vars.ante);

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
        blind_get_reward(g_game_vars.current_blind)
    ); // Blind reward

    deck_shuffle(); // Shuffle the deck at the start of the round

    skip_tag_process_init(SKIP_TAG_EVENT_ON_ROUND_START);

    /* Note that since cards_in_hand_update_loop() handles card highlight there's no need
     * to call a selection changed callback to highlight the initial card, this wouldn't work
     * otherwise or for the buttons.
     */
    game_round_selection_grid.selection = GAME_PLAYING_INIT_SEL;
}

void game_round_on_update(void)
{
    // Background logic (thissss might be moved to the card'ssss logic later. I'm a sssssnake)
    if (get_hand_state() == HAND_TAGS || get_hand_state() == HAND_DRAW ||
        get_hand_state() == HAND_DISCARD || get_hand_state() == HAND_SELECT)
    {
        change_background(BG_CARD_SELECTING, false);
    }
    else if (get_hand_state() != HAND_SHUFFLING)
    {
        change_background(BG_CARD_PLAYING, false);
    }

    game_round_process_input_and_state();

    // Card logic

    game_round_process_card_draw();

    game_round_discarded_cards_loop();

    s_discarded_card = false;

    cards_in_hand_update_loop();
    played_cards_update_loop();

    game_round_ui_text_update();

    // animate score flames if we exceed the score requirement
    game_round_process_flaming_score();
}
