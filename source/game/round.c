#include "round.h"

#include "audio_utils.h"
#include "button.h"
#include "game_variables.h"
#include "hand.h"
#include "joker.h"
#include "joker_row.h"
#include "layout.h"
#include "list.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "timer.h"
#include "util.h"

#include "game.h"

#define MAX_DECK_SIZE        52
#define SCORE_FLAMES_ANIM_FREQ  5 // animation will run at 12FPS
#define NUM_SCORE_FLAMES_FRAMES 8 // Chips and Mult flame frames are next to one another
#define SCORE_FLAME_FRAME_WIDTH 3 // so we only need to offset to get the next ones

#define PLAY_HAND_BTN_BORDER_PAL_IDX    7
#define PLAY_HAND_BTN_PAL_IDX           6
#define DISCARD_BTN_PAL_IDX             13
#define DISCARD_BTN_BORDER_PAL_IDX      8
#define SORT_BTNS_PAL_IDX               9
#define SORT_BY_RANK_BTN_BORDER_PAL_IDX 22
#define SORT_BY_SUIT_BTN_BORDER_PAL_IDX 23

#define TM_SCORE_LERP_INTERVAL 2
#define NUM_SCORE_LERP_STEPS   16

#define PITCH_STEP_DISCARD_SFX   (-64)
#define PITCH_STEP_DRAW_SFX      24
#define PITCH_STEP_UNDISCARD_SFX 2 * PITCH_STEP_DRAW_SFX

#define GAME_PLAYING_HAND_SEL_Y      1

#define CARD_FOCUSED_UNSEL_Y 10
#define CARD_UNFOCUSED_SEL_Y 15
#define CARD_FOCUSED_SEL_Y   20

#define SCORED_CARD_TEXT_Y 48

// Flaming score animation frames
static const Rect SCORE_FLAME_RESET         = {26,      20,      28,     20};
static const Rect SCORE_FLAME_FRAMES_START  = {26,      21,      28,     21};
static const BG_POINT SCORE_FLAME_CHIPS_POS = {1,       9};
static const BG_POINT SCORE_FLAME_MULT_POS  = {5,       9};

static const Selection GAME_PLAYING_INIT_SEL = {0, 1};
static const Rect TEMP_SCORE_RECT           = {8,       64,     64,     72  }; 
static const BG_POINT CARD_DRAW_POS         = {208,     110};
static const Rect HAND_SIZE_RECT_SELECT     = {120,     128,    160,    136 };
static const Rect HAND_SIZE_RECT_PLAYING    = {120,     152,    160,    160 };
static const Rect DECK_SIZE_RECT            = {200,     152,    240,       160       };
static const BG_POINT CARD_DISCARD_PNT      = {240,     70};
static const BG_POINT HAND_START_POS        = {120,     90};
static const Rect PLAYED_CARDS_SCORES_RECT  = {72,      48,     240,    56  };
static const Rect HELD_CARDS_SCORES_RECT    = {72,      108,    240,    116 };
static const BG_POINT HAND_PLAY_POS         = {120,     70};

// This is a stupid way to do this but I don't care
static const int HAND_SPACING_LUT[MAX_HAND_SIZE] =
    {28, 28, 28, 28, 27, 21, 18, 15, 13, 12, 10, 9, 9, 8, 8, 7};


static int game_playing_hand_row_get_size(void);
static int game_playing_button_row_get_size(void);
static bool game_playing_button_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void game_playing_button_row_on_key_hit(SelectionGrid* selection_grid, Selection* selection);
static void game_playing_hand_row_on_key_transit(
    SelectionGrid* selection_grid,
    Selection* selection
);

static bool game_playing_hand_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void game_playing_play_hand_on_pressed(void);
static bool can_play_hand(void);
static void game_playing_sort_by_rank_on_pressed(void);
static void game_playing_sort_by_suit_on_pressed(void);
static void game_playing_discard_on_pressed(void);
static bool can_discard_hand(void);
static void game_playing_execute_play_hand(void);
static void game_playing_execute_discard(void);
static void display_temp_score(u32 value);
static int deck_get_size(void);
static int deck_get_max_size(void);
static int hand_sel_idx_to_card_idx(int selection_index);
static void check_flaming_score(void);
static bool check_and_score_joker_for_event(
    ListItr* starting_joker_itr,
    CardObject* card_object,
    enum JokerEvent joker_event
);


static int cards_drawn = 0;
static u32 chips = 0;
static u32 mult = 0;
static enum PlayState play_state = PLAY_STARTING;
static u32 temp_score = 0; // This is the score that shows in the same spot as the hand type.
static FIXED lerped_temp_score = 0;
static FIXED lerped_score = 0;
static int deck_top = -1;
static int discard_top = -1;
static bool score_flames_active = false;
static bool discarded_card = false;
static bool sound_played = false;

static Card* deck[MAX_DECK_SIZE] = {NULL};
static Card* discard_pile[MAX_DECK_SIZE] = {NULL};

// Array of buttons by horizontal selection index (x)
static Button game_playing_buttons[] = {
    {PLAY_HAND_BTN_BORDER_PAL_IDX,    PLAY_HAND_BTN_PAL_IDX, game_playing_play_hand_on_pressed,    can_play_hand   },
    {SORT_BY_RANK_BTN_BORDER_PAL_IDX, SORT_BTNS_PAL_IDX,     game_playing_sort_by_rank_on_pressed, NULL            },
    {SORT_BY_SUIT_BTN_BORDER_PAL_IDX, SORT_BTNS_PAL_IDX,     game_playing_sort_by_suit_on_pressed, NULL            },
    {DISCARD_BTN_BORDER_PAL_IDX,      DISCARD_BTN_PAL_IDX,   game_playing_discard_on_pressed,      can_discard_hand},
};

// clang-format off
static SelectionGridRow game_playing_selection_rows[] = {
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

static SelectionGrid game_playing_selection_grid = {
    game_playing_selection_rows,
    NUM_ELEM_IN_ARR(game_playing_selection_rows),
    GAME_PLAYING_INIT_SEL
};
    

static int game_playing_hand_row_get_size(void)
{
    return hand_nb_held_cards();
}

static int game_playing_button_row_get_size(void)
{
    return NUM_ELEM_IN_ARR(game_playing_buttons);
}

static inline void game_playing_button_set_highlight(int btn_idx, bool highlight)
{
    button_set_highlight(&game_playing_buttons[btn_idx], highlight);
}

static bool game_playing_button_row_on_selection_changed(
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

static void game_playing_button_row_on_key_hit(SelectionGrid* selection_grid, Selection* selection)
{
    if (key_hit(SELECT_CARD))
    {
        button_press(&game_playing_buttons[selection->x]);
    }
}

static void game_playing_play_hand_on_pressed(void)
{
    if (!can_play_hand())
        return;

    game_playing_execute_play_hand();

    // Move back to hand selection
    selection_grid_move_selection_vert(&game_playing_selection_grid, -1);
}

static bool can_play_hand(void)
{
    if (get_hand_state() != HAND_SELECT || hand_get_nb_selected_cards() == 0)
        return false;
    return true;
}

static void game_playing_sort_by_rank_on_pressed(void)
{
    hand_change_sort(false);
}

static void game_playing_sort_by_suit_on_pressed(void)
{
    hand_change_sort(true);
}

static bool can_discard_hand(void)
{
    return (
        g_game_vars.discards > 0 && get_hand_state() == HAND_SELECT &&
        hand_get_nb_selected_cards() > 0
    );
}

static void game_playing_execute_play_hand(void)
{
    if (!can_play_hand())
        return;

    set_hand_state(HAND_PLAY);
    --g_game_vars.hands;
    display_hands();
}

static void game_playing_discard_on_pressed(void)
{
    if (!can_discard_hand())
        return;

    game_playing_execute_discard();

    // Move back to hand selection
    selection_grid_move_selection_vert(&game_playing_selection_grid, -1);
}

static void game_playing_execute_discard(void)
{
    if (!can_discard_hand())
        return;

    set_hand_state(HAND_DISCARD);
    --g_game_vars.discards;
    display_discards();
    compute_hand_value_info();
}

static void display_temp_score(u32 value)
{
    char temp_score_str_buff[UINT_MAX_DIGITS + 1];
    Rect temp_score_rect = TEMP_SCORE_RECT;
    truncate_uint_to_suffixed_str(
        value,
        rect_width(&temp_score_rect) / TTE_CHAR_SIZE,
        temp_score_str_buff
    );
    update_text_rect_to_center_str(&temp_score_rect, temp_score_str_buff, SCREEN_RIGHT);

    tte_erase_rect_wrapper(TEMP_SCORE_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        temp_score_rect.left,
        temp_score_rect.top,
        TTE_WHITE_PB,
        temp_score_str_buff
    );
}

static int deck_get_size(void)
{
    return deck_top + 1;
}

static int deck_get_max_size(void)
{
    // This is the max amount of cards that the player currently has in their possession
    return get_hand_top() + g_game_vars.played_top + deck_top + discard_top + 4;
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

// Show/Hide flaming score effect if we will score
// more than the required amount or not
static void check_flaming_score(void)
{
    u32 curr_score = u32_protected_mult(chips, mult);
    u32 required_score = blind_get_requirement(g_game_vars.current_blind, g_game_vars.ante);
    if (curr_score >= required_score && !score_flames_active)
    {
        // start flaming score
        score_flames_active = true;
        return;
    }
    if (curr_score < required_score && score_flames_active)
    {
        // stop flaming score and clear rect
        score_flames_active = false;

        Rect reset_rect = SCORE_FLAME_RESET;
        main_bg_se_copy_rect(reset_rect, SCORE_FLAME_CHIPS_POS);
        reset_rect.left += SCORE_FLAME_FRAME_WIDTH;
        reset_rect.right += SCORE_FLAME_FRAME_WIDTH;
        main_bg_se_copy_rect(reset_rect, SCORE_FLAME_MULT_POS);
    }
}

static inline void deck_shuffle(void)
{
    for (int i = deck_top; i > 0; i--)
    {
        int j = rng_get_u32() % (i + 1);
        Card* temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void game_round_on_init(void)
{
    set_hand_state(HAND_DRAW);
    hand_set_nb_selected_cards(0);
    cards_drawn = 0;

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

    /* Note that since cards_in_hand_update_loop() handles card highlight there's no need
     * to call a selection changed callback to highlight the initial card, this wouldn't work
     * otherwise or for the buttons.
     */
    game_playing_selection_grid.selection = GAME_PLAYING_INIT_SEL;
}

static inline Card* deck_pop()
{
    if (deck_top < 0)
        return NULL;
    return deck[deck_top--];
}

static inline void deck_push(Card* card)
{
    if (deck_top >= MAX_DECK_SIZE - 1)
        return;
    deck[++deck_top] = card;
}

static inline void card_draw(void)
{
    if (deck_top < 0 || get_hand_top() >= g_game_vars.hand_size - 1 ||
        get_hand_top() >= MAX_HAND_SIZE - 1)
        return;

    CardObject* card_object = card_object_new(deck_pop());

    const FIXED deck_x = int2fx(CARD_DRAW_POS.x);
    const FIXED deck_y = int2fx(CARD_DRAW_POS.y);

    card_object->sprite_object->x = deck_x;
    card_object->sprite_object->y = deck_y;
    sprite_position(card_object->sprite_object->sprite, fx2int(deck_x), fx2int(deck_y));

    set_hand_top(get_hand_top() + 1);
    get_hand_array()[get_hand_top()] = card_object;

    // Sort the hand after drawing a card
    sort_cards();

    play_sfx(
        SFX_CARD_DRAW,
        MM_BASE_PITCH_RATE + cards_drawn * PITCH_STEP_DRAW_SFX,
        SFX_DEFAULT_VOLUME
    );
}

static inline void game_playing_process_card_draw()
{
    if (get_hand_state() == HAND_DRAW && cards_drawn < g_game_vars.hand_size)
    {
        if (g_game_vars.timer % FRAMES(10) == 0) // Draw a card every 10 frames
        {
            cards_drawn++;
            card_draw();
        }
    }
    else if (get_hand_state() == HAND_DRAW)
    {
        set_hand_state(HAND_SELECT); // Change the hand state to select after drawing all the cards
        cards_drawn = 0;
        g_game_vars.timer = TM_ZERO;
    }
}

static inline void game_playing_process_hand_select_input(void)
{
    selection_grid_process_input(&game_playing_selection_grid);
}

static inline void game_playing_process_input_and_state(void)
{
    if (get_hand_state() == HAND_SELECT)
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
            lerped_score = int2fx(g_game_vars.score);

            display_temp_score(temp_score);

            chips = 0;
            mult = 0;
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
        lerped_temp_score -= int2fx(temp_score * g_game_vars.game_speed) / NUM_SCORE_LERP_STEPS;
        lerped_score += int2fx(temp_score * g_game_vars.game_speed) / NUM_SCORE_LERP_STEPS;

        if (lerped_temp_score > 0)
        {
            // Set the score display first because it's more important
            // in case there isn't enough time within the frame to display both
            display_score(fx2uint(lerped_score));

            display_temp_score(fx2uint(lerped_temp_score));
        }
        else
        {
            g_game_vars.score = u32_protected_add(g_game_vars.score, temp_score);
            temp_score = 0;
            lerped_temp_score = 0;
            lerped_score = 0;

            tte_erase_rect_wrapper(TEMP_SCORE_RECT); // Just erase the temp score

            display_score(g_game_vars.score);
        }
    }
}

static inline Card* discard_pop()
{
    if (discard_top < 0)
        return NULL;
    return discard_pile[discard_top--];
}

static inline void display_ante(int value)
{
    tte_printf(
        "#{P:%d,%d; cx:0xC000}%d#{cx:0xF000}/%d",
        ANTE_TEXT_RECT.left,
        ANTE_TEXT_RECT.top,
        value,
        MAX_ANTE
    );
}

static inline void game_playing_handle_round_over(void)
{
    enum GameState next_state = GAME_STATE_ROUND_END;

    if (g_game_vars.score >= blind_get_requirement(g_game_vars.current_blind, g_game_vars.ante))
    {
        if (g_game_vars.current_blind > BLIND_TYPE_BIG)
        {
            if (g_game_vars.ante < MAX_ANTE)
            {
                display_ante(++g_game_vars.ante);

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

static inline void game_playing_discarded_cards_loop(void)
{
    // Discarded cards loop (mainly for shuffling)
    if (hand_nb_held_cards() == 0 && get_hand_state() == HAND_SHUFFLING && discard_top >= -1 &&
        g_game_vars.timer > FRAMES(10))
    {
        // Change the background to the round end background. This is how it works in Balatro, so
        // I'm doing it this way too.
        change_background(BG_ROUND_END, false);

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

static inline void game_playing_process_flaming_score(void)
{
    static u8 flame_score_frame = 0;

    if (score_flames_active)
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

static inline void game_playing_ui_text_update(void)
{
    static int last_hand_size = 0;
    static int last_deck_size = 0;

    if (last_hand_size != hand_nb_held_cards() || last_deck_size != deck_get_size())
    {
        if (background_legacy == BG_CARD_SELECTING)
        {
            // Hand size/max size
            tte_printf(
                "#{P:%d,%d; cx:0x%X000}%2d/%-2ld",
                HAND_SIZE_RECT_SELECT.left,
                HAND_SIZE_RECT_SELECT.top,
                TTE_WHITE_PB,
                hand_nb_held_cards(),
                g_game_vars.hand_size
            );
        }
        else if (background_legacy == BG_CARD_PLAYING)
        {
            // Hand size/max size
            tte_printf(
                "#{P:%d,%d; cx:0x%X000}%2d/%-2ld",
                HAND_SIZE_RECT_PLAYING.left,
                HAND_SIZE_RECT_PLAYING.top,
                TTE_WHITE_PB,
                hand_nb_held_cards(),
                g_game_vars.hand_size
            );
        }

        // Deck size/max size
        // TODO: the text will overflow if deck max size exceeds 99,
        // we will need a fix at some point for this
        tte_erase_rect_wrapper(DECK_SIZE_RECT);
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%d/%d",
            DECK_SIZE_RECT.left,
            DECK_SIZE_RECT.top,
            TTE_WHITE_PB,
            deck_get_size(),
            deck_get_max_size()
        );

        last_hand_size = hand_nb_held_cards();
        last_deck_size = deck_get_size();
    }
}

static inline void discard_push(Card* card)
{
    if (discard_top >= MAX_DECK_SIZE - 1)
        return;
    discard_pile[++discard_top] = card;
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

                set_hand_top(get_hand_top() - 1);
                // This technically isn't drawing cards, I'm just reusing the variable
                cards_drawn++;
                sound_played = false;
                g_game_vars.timer = TM_ZERO;

                *hand_y = hand[card_idx]->sprite_object->y;
                *hand_x = hand[card_idx]->sprite_object->x;
            }

            discarded_card = true;
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

    if (card_idx == 0 && discarded_card == false && g_game_vars.timer % FRAMES(10) == 0)
    {
        // This is never reached in the case of HAND_SHUFFLING. Not sure why but that's how it's
        // supposed to be.
        set_hand_state(HAND_DRAW);
        sound_played = false;
        cards_drawn = 0;
        hand_set_nb_selected_cards(0);
        g_game_vars.timer = TM_ZERO;
        *break_loop = true;
        return;
    };
}

static inline void played_push(CardObject* card_object)
{
    if (g_game_vars.played_top >= MAX_SELECTION_SIZE - 1)
        return;
    played[++g_game_vars.played_top] = card_object;
}

static inline CardObject* played_pop()
{
    if (g_game_vars.played_top < 0)
        return NULL;
    return played[g_game_vars.played_top--];
}

static inline void select_highcard_cards_in_played_hand(void)
{
    // find the card with the highest rank in the hand
    int highest_rank_index = 0;

    for (int i = 0; i <= g_game_vars.played_top; i++)
    {
        if (played[i]->card->rank > played[highest_rank_index]->card->rank)
        {
            highest_rank_index = i;
        }
    }

    card_object_set_selected(played[highest_rank_index], true);
}

static inline void select_pair_cards_in_played_hand(void)
{
    // find two cards with the same rank
    for (int i = 0; i <= g_game_vars.played_top - 1; i++)
    {
        for (int j = i + 1; j <= g_game_vars.played_top; j++)
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
        find_flush_in_played_cards(played, g_game_vars.played_top, min_len, flush_selection);
        // Add the results into the final selection
        for (int i = 0; i <= g_game_vars.played_top; i++)
        {
            final_selection[i] = flush_selection[i];
        }
    }

    // If we have a straight in our hand
    if (get_hand_type() == STRAIGHT || get_hand_type() == STRAIGHT_FLUSH ||
        get_hand_type() == ROYAL_FLUSH)
    {
        bool straight_selection[MAX_HAND_SIZE] = {false};
        find_straight_in_played_cards(
            played,
            g_game_vars.played_top,
            is_shortcut_joker_active(),
            min_len,
            straight_selection
        );
        // Add the results into the final selection
        for (int i = 0; i <= g_game_vars.played_top; i++)
        {
            final_selection[i] = final_selection[i] || straight_selection[i];
        }
        // If Four Fingers is active, pairs can happen in a valid straight
        // If Four Fingers is not active, pairs are impossible so this will not affect things
        select_paired_cards_in_hand(played, g_game_vars.played_top, final_selection);
    }

    // Finally, set mark the cards as selected based final_selection
    for (int i = 0; i <= g_game_vars.played_top; i++)
    {
        if (final_selection[i])
        {
            card_object_set_selected(played[i], true);
        }
    }
}

static inline void select_all_five_cards_in_played_hand(void)
{
    for (int i = 0; i <= g_game_vars.played_top; i++)
    {
        card_object_set_selected(played[i], true);
    }
}

static inline void select_four_of_a_kind_cards_in_played_hand(void)
{
    // find four cards with the same rank
    // If there are 5 cards selected we just need to find the one card that doesn't match, and
    // select the others
    if (g_game_vars.played_top >= 3)
    {
        int unmatched_index = -1;

        for (int i = 0; i <= g_game_vars.played_top; i++)
        {
            if (played[i]->card->rank != played[(i + 1) % g_game_vars.played_top]->card->rank &&
                played[i]->card->rank != played[(i + 2) % g_game_vars.played_top]->card->rank)
            {
                unmatched_index = i;
                break;
            }
        }

        for (int i = 0; i <= g_game_vars.played_top; i++)
        {
            if (i != unmatched_index)
            {
                card_object_set_selected(played[i], true);
            }
        }
    }
    else // If there are only 4 cards selected we know they match
    {
        for (int i = 0; i <= g_game_vars.played_top; i++)
        {
            card_object_set_selected(played[i], true);
        }
    }
}

static inline void select_three_of_a_kind_cards_in_played_hand(void)
{
    // find three cards with the same rank
    for (int i = 0; i <= g_game_vars.played_top - 1; i++)
    {
        for (int j = i + 1; j <= g_game_vars.played_top; j++)
        {
            if (played[i]->card->rank == played[j]->card->rank)
            {
                card_object_set_selected(played[i], true);
                card_object_set_selected(played[j], true);

                for (int k = j + 1; k <= g_game_vars.played_top; k++)
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

    for (i = 0; i <= g_game_vars.played_top - 1; i++)
    {
        for (int j = i + 1; j <= g_game_vars.played_top; j++)
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

    for (; i <= g_game_vars.played_top - 1; i++) // Find second pair
    {
        for (int j = i + 1; j <= g_game_vars.played_top; j++)
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

static inline void select_cards_in_played_hand()
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
    int selected_card_idx = hand_sel_idx_to_card_idx(game_playing_selection_grid.selection.x);

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
                case HAND_DRAW:
                    hand_x = hand_x + (int2fx(i) - int2fx(get_hand_top()) / 2) *
                                          -HAND_SPACING_LUT[get_hand_top()];
                    break;
                case HAND_SELECT:
                    bool is_focused =
                        (i == selected_card_idx &&
                         game_playing_selection_grid.selection.y == GAME_PLAYING_HAND_SEL_Y);

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
                    if (i != selected_card_idx && hand[i]->sprite_object->y > hand_y)
                    {
                        hand[i]->sprite_object->y = hand_y;
                        sprite_position(
                            hand[i]->sprite_object->sprite,
                            fx2int(hand[i]->sprite_object->x),
                            fx2int(hand_y)
                        );
                        // Set target y to match y. Ensures target is updated even when vy becomes
                        // 0, preventing immediate snap back.
                        hand[i]->sprite_object->ty = hand_y;
                        hand[i]->sprite_object->vy = 0;
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

                    if (card_object_is_selected(hand[i]) && discarded_card == false &&
                        g_game_vars.timer % FRAMES(10) == 0)
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

                        set_hand_top(get_hand_top() - 1);
                        hand_set_nb_selected_cards(hand_get_nb_selected_cards() - 1);
                        cards_drawn++;

                        discarded_card = true;
                    }

                    if (i == 0 && discarded_card == false && g_game_vars.timer % FRAMES(10) == 0)
                    {
                        set_hand_state(HAND_PLAYING);
                        cards_drawn = 0;
                        hand_set_nb_selected_cards(0);
                        g_game_vars.timer = TM_ZERO;
                        g_game_vars.scored_card_index = g_game_vars.played_top + 1;

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

            hand[i]->sprite_object->tx = hand_x;
            hand[i]->sprite_object->ty = hand_y;
            card_object_update(hand[i]);
        }
    }
}

static inline void play_starting_played_cards_update(int played_idx)
{
    bool card_selected = card_object_is_selected(played[g_game_vars.played_top - g_game_vars.scored_card_index]);
    if (played_idx == g_game_vars.played_top && (g_game_vars.timer % FRAMES(10) == 0 || !card_selected) &&
        g_game_vars.timer > FRAMES(40))
    {
        g_game_vars.scored_card_index--;

        if (g_game_vars.scored_card_index == 0)
        {
            g_game_vars.joker_scored_itr = list_itr_create(&g_game_vars.owned_jokers_list);
            g_game_vars.timer = TM_ZERO;
            play_state = PLAY_BEFORE_SCORING;
        }
    }

    played[played_idx]->sprite_object->tx =
        int2fx(HAND_PLAY_POS.x) + (int2fx(g_game_vars.played_top - played_idx) - int2fx(g_game_vars.played_top) / 2) * -27;
    played[played_idx]->sprite_object->ty = int2fx(HAND_PLAY_POS.y);

    card_selected = card_object_is_selected(played[played_idx]);
    if (card_selected && g_game_vars.played_top - played_idx >= g_game_vars.scored_card_index)
    {
        played[played_idx]->sprite_object->ty -= int2fx(10);
    }
}

// returns true if the scoring loop has returned early
static inline bool play_scoring_cards_update(void)
{
    if (g_game_vars.timer % FRAMES(30) == 0 && g_game_vars.timer > FRAMES(40))
    {
        // We are about to score played Cards.
        // Start from the current card index
        // and seek the next scoring card
        while (g_game_vars.scored_card_index <= g_game_vars.played_top &&
               !card_object_is_selected(played[g_game_vars.scored_card_index]))
        {
            g_game_vars.scored_card_index++;
        }

        // go to the next state if there are no cards left to score
        if (g_game_vars.scored_card_index > g_game_vars.played_top)
        {
            // reuse these variables for held cards
            g_game_vars.joker_scored_itr = list_itr_create(&g_game_vars.owned_jokers_list);
            g_game_vars.scored_card_index = get_hand_top();

            play_state = PLAY_SCORING_HELD_CARDS;

            return false;
        }

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        CardObject* scored_card_object = played[g_game_vars.scored_card_index];

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
            g_game_vars.joker_scored_itr = list_itr_create(&g_game_vars.owned_jokers_list);
            g_game_vars.joker_card_scored_end_itr = list_itr_create(&g_game_vars.owned_jokers_list);
        }

        play_state = PLAY_SCORING_CARD_JOKERS;
        return true;
    }

    return false;
}

// returns true if the scoring loop has returned early
static inline bool play_before_scoring_cards_update(void)
{
    // Activate Jokers with an effect just before the hand is scored
    if (check_and_score_joker_for_event(&g_game_vars.joker_scored_itr, NULL, JOKER_EVENT_ON_HAND_PLAYED))
    {
        return true;
    }

    play_state = PLAY_SCORING_CARDS;
    return false;
}

// returns true if the scoring loop has returned early
static inline bool play_scoring_held_cards_update(int played_idx)
{
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
    {
        tte_erase_rect_wrapper(HELD_CARDS_SCORES_RECT);

        CardObject** hand = get_hand_array();

        // Go through all held cards and see if they activate Jokers
        for (; g_game_vars.scored_card_index >= 0; g_game_vars.scored_card_index--)
        {
            if (check_and_score_joker_for_event(
                    &g_game_vars.joker_scored_itr,
                    hand[g_game_vars.scored_card_index],
                    JOKER_EVENT_ON_CARD_HELD
                ))
            {
                card_object_shake(hand[g_game_vars.scored_card_index], SFX_CARD_SELECT);
                return true;
            }
            g_game_vars.joker_scored_itr = list_itr_create(&g_game_vars.owned_jokers_list);
        }

        g_game_vars.scored_card_index = 0;
        g_game_vars.joker_round_end_itr = list_itr_create(&g_game_vars.owned_jokers_list);
        play_state = PLAY_SCORING_INDEPENDENT_JOKERS;
    }

    return false;
}

// Activate "on scored" Jokers for the previous scored card if any
// returns true if the scoring loop has returned early
static inline bool play_scoring_card_jokers_update(void)
{
    if (g_game_vars.timer % FRAMES(30) == 0 && g_game_vars.timer > FRAMES(40))
    {
        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        // since we sought the next scoring card index in the previous state,
        // g_game_vars.scored_card_index is guaranteed to be a scoring card
        if (check_and_score_joker_for_event(
                &g_game_vars.joker_scored_itr,
                played[g_game_vars.scored_card_index],
                JOKER_EVENT_ON_CARD_SCORED
            ))
        {
            return true;
        }

        // Trigger all Jokers that have an effect when a card finishes scoring
        // (e.g. retriggers) after activating all the other scored_card Jokers normally
        if (check_and_score_joker_for_event(
                &g_game_vars.joker_card_scored_end_itr,
                played[g_game_vars.scored_card_index],
                JOKER_EVENT_ON_CARD_SCORED_END
            ))
        {
            // If we just scored a retrigger, return early and go back to the
            // previous state score the same card again without incrementing
            // g_game_vars.scored_card_index to score the current card again
            if (retrigger)
            {
                retrigger = false;
                play_state = PLAY_SCORING_CARDS;
            }
            return true;
        }

        // increment index to start seeking the next scoring card from the next card
        g_game_vars.scored_card_index++;
        play_state = PLAY_SCORING_CARDS;
    }

    return false;
}

// Score Jokers normally (independent)
// returns true if the scoring loop has returned early
static inline bool play_scoring_independent_jokers_update(int played_idx)
{
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
    {

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        if (check_and_score_joker_for_event(&g_game_vars.joker_scored_itr, NULL, JOKER_EVENT_INDEPENDENT))
        {
            return true;
        }

        g_game_vars.scored_card_index =
            g_game_vars.played_top + 1; // Reset the scored card index to the top of the played stack

        play_state = PLAY_SCORING_HAND_SCORED_END;
    }

    return false;
}

// Trigger hand end effect for all jokers once they are done scoring
static inline bool play_scoring_hand_scored_end_update(int played_idx)
{
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
    {

        tte_erase_rect_wrapper(PLAYED_CARDS_SCORES_RECT);

        bool scored = check_and_score_joker_for_event(
            &g_game_vars.joker_round_end_itr,
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
// This is the reverse of PLAY_STARTING. The cards get reset back to their neutral position
// sequentially
static inline void play_ending_played_cards_update(int played_idx)
{
    bool card_selected = card_object_is_selected(played[g_game_vars.played_top - g_game_vars.scored_card_index]);
    if (played_idx == g_game_vars.played_top && (g_game_vars.timer % FRAMES(10) == 0 || !card_selected) &&
        g_game_vars.timer > FRAMES(40))
    {
        g_game_vars.scored_card_index--;

        /* SFX_CHIPS_ACCUM has been pitch shifted to perserve high frequencies in downsampling.
         * Now it needs to be pitch shifted back to the original frequency.
         */
        int static const CHIPS_ACCUM_SFX_PITCH_RATIO = 2;

        if (g_game_vars.scored_card_index == 0)
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

    if (card_object_is_selected(played[played_idx]) && g_game_vars.played_top - played_idx >= g_game_vars.scored_card_index)
    {
        played[played_idx]->sprite_object->ty = int2fx(HAND_PLAY_POS.y);
    }
}

static inline bool game_round_is_over(void)
{
    return g_game_vars.hands == 0 ||
           g_game_vars.score >= blind_get_requirement(g_game_vars.current_blind, g_game_vars.ante);
}

// Basically a copy of HAND_DISCARD
// returns true if the current card has been discarded
static bool play_ended_played_cards_update(int played_idx)
{
    if (!discarded_card && g_game_vars.timer > FRAMES(40))
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

            // g_game_vars.played_top--;
            cards_drawn++; // This technically isn't drawing cards, I'm just reusing the variable
            sound_played = false; // Allow for the sound for the next card to be played

            // we reached hand_top, all cards have been discarded
            if (played_idx == g_game_vars.played_top)
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
                cards_drawn = 0;
                hand_set_nb_selected_cards(0);
                g_game_vars.played_top = -1; // Reset the played stack
                g_game_vars.scored_card_index = 0;
                g_game_vars.joker_scored_itr = list_itr_create(&g_game_vars.owned_jokers_list);
                g_game_vars.timer = TM_ZERO;
            }

            return true; // return early to avoid accessing played[played_idx] == NULL
        }

        // put target X position off screen to the right
        played[played_idx]->sprite_object->tx = int2fx(CARD_DISCARD_PNT.x);
        discarded_card = true;
    }

    return false;
}

static inline void played_cards_update_loop(void)
{
    // So this one is a bit fucking weird because I have to work kinda backwards for everything
    // because of the order of the pushed cards from the hand to the play stack (also crazy that the
    // company that published Balatro is called "Playstack" and this is a play stack, but I digress)
    for (int played_idx = 0; played_idx <= g_game_vars.played_top; played_idx++)
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

void game_round_on_update(void)
{
    // Background logic (thissss might be moved to the card'ssss logic later. I'm a sssssnake)
    if (get_hand_state() == HAND_DRAW || get_hand_state() == HAND_DISCARD ||
        get_hand_state() == HAND_SELECT)
    {
        change_background(BG_CARD_SELECTING, false);
    }
    else if (get_hand_state() != HAND_SHUFFLING)
    {
        change_background(BG_CARD_PLAYING, false);
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
