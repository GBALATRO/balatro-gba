#ifndef GAME_H
#define GAME_H

#include "bitset.h"
#include "blind.h"
#include "game/blind_select.h"
#include "game/game_over.h"
#include "game/main_menu.h"
#include "game/round.h"
#include "game/round_end.h"
#include "game/shop.h"

#include <tonc.h>

#define MAX_HAND_SIZE        16
#define MAX_DECK_SIZE        52
#define MAX_JOKERS_HELD_SIZE 5 // This doesn't account for negatives right now.
#define MAX_SHOP_JOKERS      2 // TODO: Make this dynamic and allow for other items besides jokers
#define MAX_SELECTION_SIZE   5

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

typedef struct
{
    int substate;
    void (*on_init)(void*);
    void (*on_update)(void*);
    void (*on_exit)(void*);
    void* ctx;
} StateInfo;

typedef union
{
    MainMenuProps main_menu;
    BlindSelectProps blind_select;
    RoundProps playing;
    RoundEndProps round_end;
    ShopProps shop;
    GameOverProps game_over;
} GameStateCtx;

void set_game_state_ctx(enum GameState game_state);
void update_game_state_ctx(enum GameState game_state);
enum GameState get_ctx_game_state(void);

// ============================================================================
// Game Core Functions
// ============================================================================

void game_start(void);
void game_init(void);
void game_update(void);
void game_change_state(enum GameState new_game_state);
enum GameState get_game_state(void);

// ============================================================================
// Getters and Setters
// ============================================================================

uint get_game_speed(void);
uint logical_frames_to_real(uint logical_frames);

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
int get_money(void);
void set_money(int new_money);
void increase_money(int amount);
void decrease_money(int amount);
void display_money(void);

// Timer and RNG Functions

uint get_timer(void);
void set_timer(uint new_time);
void reset_timer(void);
void incr_rng_seed(void);
void mult_rng_seed(int factor);

// Hand and Deck

// Hand array
CardObject** get_hand_array(void);
CardObject* get_hand_card_at(int idx);
void set_hand_card_at(int idx, CardObject* card_object);
int get_hand_top(void);
int increment_hand_top(void);
int decrement_hand_top(void);

// Played array
CardObject** get_played_array(void);
CardObject* get_played_card_at(int idx);
void set_played_card_at(int idx, CardObject* card_object);
int get_played_top(void);
void reset_played_top(void);

// Deck Array
Card* get_deck_at(int idx);
void set_deck_at(int idx, Card* card);
int get_deck_top(void);

int get_discard_top(void);
int hand_get_size(void);
int deck_get_size(void);
int deck_get_max_size(int hand_top, int played_top, int deck_top, int discard_top);

// Hands, Discards, and Round

int get_ante(void);
int increment_ante(void);
int get_hands(void);
void reset_hands(void);
int decrement_hands(void);
int get_num_hands_remaining(void);
int get_discards(void);
void reset_discards(void);
int decrement_discards(void);
int get_round(void);
int increment_round(void);

// Score

u32 get_score(void);
void set_score(u32 new_score);
void reset_score(void);
u32 increase_score_by(u32 amount);

// Temp score

u32 get_temp_score(void);
void set_temp_score(u32 new_temp_score);
void reset_temp_score(void);
u32 mult_temp_score_by(u32 factor);

// Lerped score

FIXED get_lerped_score(void);
void set_lerped_score(FIXED new_lerped_score);
void reset_lerped_score(void);
FIXED increase_lerped_score_by(FIXED amount);

// Temp lerped score

FIXED get_lerped_temp_score(void);
void set_lerped_temp_score(FIXED new_lerped_temp_score);
void reset_lerped_temp_score(void);
FIXED decrease_lerped_temp_score_by(FIXED amount);

// Game substates
int get_substate(void);
void set_substate(int new_substate);

// ============================================================================
// Blind Management Functions
// ============================================================================

Sprite* get_blind_select_token(enum BlindType blind_type);
Sprite* get_playing_blind_token(void);
void set_playing_blind_token(Sprite* sprite);
bool playing_blind_token_exists(void);
void hide_playing_blind_token(void);
void unhide_playing_blind_token(void);

Sprite* get_round_end_blind_token(void);
void set_round_end_blind_token(Sprite* sprite);
bool round_end_blind_token_exists(void);
void hide_round_end_blind_token(void);
void unhide_round_end_blind_token(void);

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
void set_current_blind(int new_current_blind);
enum BlindState* get_blinds_states(void);
enum BlindState get_blinds_state(enum BlindType blind_type);
void increment_blind(enum BlindState* states, int* current_blind, enum BlindState increment_reason);

// Round.c helpers

void played_push(CardObject** played, int* played_top, CardObject* card_object);
CardObject* played_pop(CardObject** played, int* played_top);
void deck_push(Card** deck, int* deck_top, Card* card);
Card* deck_pop(Card** deck, int* deck_top);
void discard_push(Card** discard_pile, int* discard_top, Card* card);
Card* discard_pop(Card** discard_pile, int* discard_top);

#endif // GAME_H
