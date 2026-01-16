#ifndef GAME_H
#define GAME_H

#include "bitset.h"
#include "blind.h"

#include <tonc.h>

#define MAX_HAND_SIZE        16
#define MAX_DECK_SIZE        52
#define MAX_JOKERS_HELD_SIZE 5 // This doesn't account for negatives right now.
#define MAX_SHOP_JOKERS      2 // TODO: Make this dynamic and allow for other items besides jokers
#define MAX_SELECTION_SIZE   5
#define GAME_SPEED           1
#define FRAMES(x)            (((x) + GAME_SPEED - 1) / GAME_SPEED)

// TODO: Can make these dynamic to support interest-related jokers and vouchers
#define MAX_INTEREST   5
#define INTEREST_PER_5 1

// Input bindings
#define SELECT_CARD    KEY_A
#define DESELECT_CARDS KEY_B
#define PEEK_DECK      KEY_L // Not implemented
#define SORT_HAND      KEY_R
#define PAUSE_GAME     KEY_START // Not implemented
#define SELL_KEY       KEY_L

struct List;
typedef struct List List;

// Utility functions for other files
typedef struct CardObject CardObject;
typedef struct Card Card;
typedef struct JokerObject JokerObject;

// Enum value names in ../include/def_state_info_table.h
enum GameState
{
#define DEF_STATE_INFO(stateEnum, on_init, on_update, on_exit) stateEnum,
#include "def_state_info_table.h"
#undef DEF_STATE_INFO
    GAME_STATE_MAX,
    GAME_STATE_UNDEFINED
};

enum HandState
{
    HAND_DRAW,
    HAND_SELECT,
    // This is actually a misnomer because it's used for the deck
    // but it mechanically makes sense to be a state of the hand
    HAND_SHUFFLING,
    HAND_DISCARD,
    HAND_PLAY,
    HAND_PLAYING
};

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

// Hand types
enum HandType
{
    NONE,
    HIGH_CARD,
    PAIR,
    TWO_PAIR,
    THREE_OF_A_KIND,
    FOUR_OF_A_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HOUSE,
    STRAIGHT_FLUSH,
    ROYAL_FLUSH,
    FIVE_OF_A_KIND,
    FLUSH_HOUSE,
    FLUSH_FIVE
};

typedef struct
{
    int substate;
    void (*on_init)();
    void (*on_update)();
    void (*on_exit)();
} StateInfo;

// ============================================================================
// Game Core Functions
// ============================================================================

void game_start(void);
void game_init(void);
void game_update(void);
void game_change_state(enum GameState new_game_state);

// ============================================================================
// Getters and Setters
// ============================================================================

// Joker and Card Query Functions

Bitset* get_avail_jokers_bitset_ptr(void);
bool is_joker_owned(int joker_id);
bool card_is_face(Card* card);
List* get_jokers_list(void);
List* get_discarded_jokers_list(void);
List* get_expired_jokers_list(void);
bool is_shortcut_joker_active(void);
int get_straight_and_flush_size(void);
void clear_joker_lists(void);
void remove_owned_joker(int owned_joker_idx);
void increment_four_fingers_joker_count(void);
void increment_shortcut_joker_count(void);

// Chips, Mult, and Money Functions

u32 get_chips(void);
void set_chips(u32 new_chips);
void display_chips(void);
u32 get_mult(void);
void set_mult(u32 new_mult);
void display_mult(void);
int get_money(void);
void set_money(int new_money);
void increase_money(int amount);
void decrease_money(int amount);
void display_money(void);
void set_retrigger(bool new_retrigger);

// Timer and RNG Functions

uint get_timer(void);
void set_timer(uint new_time);
void reset_timer(void);
void incr_rng_seed(void);
void mult_rng_seed(int factor);

// Hand and Deck

CardObject** get_hand_array(void);
CardObject* get_hand_card_at(int idx);
void set_hand_card_at(int idx, CardObject* card_object);
int get_hand_top(void);
int increment_hand_top(void);
int hand_get_size(void);
int deck_get_size(void);
int deck_get_max_size(void);
CardObject** get_played_array(void);
int get_played_top(void);
int get_scored_card_index(void);
int get_deck_top(void);
int increment_deck_top(void);
int get_played_top(void);
int increment_played_top(void);
void set_played_card_at(int idx, CardObject* card_object);
CardObject* get_played_card_at(int idx);

// Hands, Discards, and Round

int get_ante(void);
int get_hands(void);
void reset_hands(void);
int decrement_hands(void);
int get_num_hands_remaining(void);
int get_discards(void);
void reset_discards(void);
int decrement_discards(void);
int get_num_discards_remaining(void);
int get_round(void);
int increment_round(void);

// Score

u32 get_score(void);
void set_score(u32 new_score);
void reset_score(void);

// Game State

enum GameState* get_game_state_ptr(void);
StateInfo* get_state_info_ptr(void);
int get_substate(void);
void set_substate(int new_substate);

// Hand state

void set_hand_state(enum HandState new_hand_state);
enum HandState get_hand_state(void);

// ============================================================================
// Blind Management Functions
// ============================================================================

Sprite* get_blind_select_token(enum BlindType blind_type);
Sprite* get_playing_blind_token(void);
Sprite* get_round_end_blind_token(void);
void destroy_playing_and_round_end_blind_tokens(void);
void destroy_blind_select_token(enum BlindType blind_type);
void destroy_all_blind_select_tokens(void);
void hide_blind_select_token(enum BlindType blind_type);
void hide_all_blind_select_tokens(void);
void unhide_blind_select_token(enum BlindType blind_type);
void unhide_all_blind_select_tokens(void);
void move_blind_select_token(enum BlindType blind_type, int x, int y);
void get_blind_select_token_pos(enum BlindType blind_type, int* x, int* y);
int get_current_blind(void);
enum BlindState get_blinds_state(enum BlindType blind_type);
void increment_blind(enum BlindState increment_reason);

// Round.c helpers

void played_push(CardObject* card_object);
CardObject* played_pop(void);
void deck_push(Card* card);
Card* deck_pop(void);
void discard_push(Card* card);
Card* discard_pop(void);

#endif // GAME_H
