#include "game.h"

#include "affine_background.h"
#include "affine_background_gfx.h"
#include "audio_utils.h"
#include "background_gfx.h"
#include "background_shop_gfx.h"
#include "bitset.h"
#include "blind.h"
#include "button.h"
#include "card.h"
#include "game/blind_select.h"
#include "game/common_ui.h"
#include "game/main_menu.h"
#include "game/palette.h"
#include "game/point.h"
#include "game/rect.h"
#include "game/round.h"
#include "game/round_end.h"
#include "game/selection.h"
#include "game/shop.h"
#include "game/timer.h"
#include "game/win_lose.h"
#include "graphic_utils.h"
#include "hand_analysis.h"
#include "joker.h"
#include "list.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "splash_screen.h"
#include "sprite.h"
#include "tonc_memdef.h"
#include "util.h"

#include <maxmod.h>
#include <stdint.h>
#include <stdlib.h>

#define STRAIGHT_AND_FLUSH_SIZE_FOUR_FINGERS 4
#define STRAIGHT_AND_FLUSH_SIZE_DEFAULT      5

// Pixel sizes
#define ITEM_SHOP_Y               71
#define ROUND_END_REWARD_AMOUNT_X 168
#define ROUND_END_REWARD_TEXT_X   88

// SE sizes
#define ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE 12

// TODO: Properly define and use
#define MENU_POP_OUT_ANIM_FRAMES 20
#define GAME_OVER_ANIM_FRAMES    15

#define SHOP_LIGHTS_1_CLR 0xFFFF
#define SHOP_LIGHTS_2_CLR 0x32BE
#define SHOP_LIGHTS_3_CLR 0x4B5F
#define SHOP_LIGHTS_4_CLR 0x5F9F

#define STARTING_ROUND 0
#define STARTING_ANTE  1
#define STARTING_MONEY 4
#define STARTING_SCORE 0

// Naming the stage where cards return from the discard pile to the deck "undiscard"

// Shop
#define REROLL_BASE_COST 5 // Base cost for rerolling the shop items

#define NEXT_ROUND_BTN_SEL_X 0

#define GAME_PLAYING_BUTTONS_SEL_Y   2
#define GAME_PLAYING_NUM_BOTTOM_BTNS 2

#define REROLL_BTN_FRAME_PAL_IDX 7
#define REROLL_BTN_PAL_IDX       3

#define EXPIRE_ANIMATION_FRAME_COUNT 3

enum GameShopStates
{
    GAME_SHOP_INTRO,
    GAME_SHOP_ACTIVE,
    GAME_SHOP_EXIT,
    GAME_SHOP_MAX
};

enum GameRoundEndStates
{
    ROUND_END_START,
    START_EXPAND_POPUP,
    DISPLAY_FINISHED_BLIND,
    DISPLAY_SCORE_MIN,
    UPDATE_BLIND_REWARD,
    BLIND_PANEL_EXIT,
    DISPLAY_REWARDS,
    DISPLAY_CASHOUT,
    DISMISS_ROUND_END_PANEL,
    ROUND_END_EXIT
};

typedef struct
{
    u32 chips;
    u32 mult;
    char* display_name;
} HandValues;

// Used as a No Operation for game states that have no init and/or exit function.
// ricfehr3 did the work of determining whether a noop or a NULL check was more
// efficient. Well, this is the answer.
// Thanks!
// https://github.com/cellos51/balatro-gba/issues/137#issuecomment-3322485129
static void noop(void)
{
}

// These functions need to be forward declared
// so they're visible to the state_info array,
// and the sub-state function tables.
// This could be done, and maybe should be done,
// with an X macro, but I'll leave that to the
// reviewer(s).
void set_hand(void);
int deck_get_size(void);
int deck_get_max_size(void);

static void game_playing_discard_on_pressed(void);
static void game_playing_play_hand_on_pressed(void);
int game_playing_button_row_get_size(void);
extern bool game_playing_button_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
extern void game_playing_button_row_on_key_hit(SelectionGrid* selection_grid, Selection* selection);

extern void game_playing_hand_row_on_key_transit(
    SelectionGrid* selection_grid,
    Selection* selection
);

extern bool game_playing_hand_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

static int game_playing_hand_row_get_size(void);

extern void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
extern bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
extern int jokers_sel_row_get_size(void);
extern void game_shop_create_items(void);

void remove_owned_joker(int owned_joker_idx);

extern bool can_play_hand(void);
static bool can_discard_hand(void);

uint rng_seed = 0;

typedef void (*SubStateActionFn)(void);

uint timer = 0; // This might already exist in libtonc but idk so i'm just making my own

StateInfo state_info[] = {
#define DEF_STATE_INFO(stateEnum, init_fn, update_fn, exit_fn) \
    {.on_init = init_fn, .on_update = update_fn, .on_exit = exit_fn, .substate = 0},
#include "../include/def_state_info_table.h"
#undef DEF_STATE_INFO
};

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

SelectionGrid game_playing_selection_grid = {
    game_playing_selection_rows,
    NUM_ELEM_IN_ARR(game_playing_selection_rows),
    GAME_PLAYING_INIT_SEL
};

// Array of buttons by horizontal selection index (x)
Button game_playing_buttons[] = {
    {PLAY_HAND_BTN_BORDER_PID, PLAY_HAND_BTN_PID, game_playing_play_hand_on_pressed, can_play_hand   },
    {DISCARD_BTN_BORDER_PID,   DISCARD_BTN_PID,   game_playing_discard_on_pressed,   can_discard_hand},
};

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

// The current game state, this is used to determine what the game is doing at any given time
enum GameState game_state = GAME_STATE_UNDEFINED;
enum HandState hand_state = HAND_DRAW;
enum PlayState play_state = PLAY_STARTING;

enum HandType hand_type = NONE;

// The sprite that displays the blind when in "GAME_PLAYING/GAME_ROUND_END" state
Sprite* playing_blind_token = NULL;

// The sprite that displays the blind when in "GAME_ROUND_END" state
Sprite* round_end_blind_token = NULL;

// The sprites that display the blinds when in "GAME_BLIND_SELECT" state
Sprite* blind_select_tokens[BLIND_TYPE_MAX] = {NULL};

int current_blind = BLIND_TYPE_SMALL;

// The current state of the blinds, this is used to determine what the game is doing at any given
// time
enum BlindState blinds_states[BLIND_TYPE_MAX] = {
    BLIND_STATE_CURRENT,
    BLIND_STATE_UPCOMING,
    BLIND_STATE_UPCOMING
};

// Red deck default (can later be moved to a deck.h file or something)
int max_hands = 4;
int max_discards = 4;
// Set in game_init and game_round_init
int hands = 0;
int discards = 0;

int game_round = 0;
int ante = 0;
int money = 0;
u32 score = 0;
u32 temp_score = 0; // This is the score that shows in the same spot as the hand type.

FIXED lerped_score = 0;
FIXED lerped_temp_score = 0;

u32 chips = 0;
u32 mult = 0;
bool retrigger = false;

int hand_size = 8; // Default hand size is 8
int cards_drawn = 0;
int hand_selections = 0;

// Keeping track of cards scored
int scored_card_index = 0;

// discarded cards specific
bool sound_played = false;
bool discarded_card = false;

// Keeping track of what Jokers are scored at each step
ListItr _joker_scored_itr;
ListItr _joker_card_scored_end_itr;
ListItr _joker_round_end_itr;

int selection_x = 0;
int selection_y = 0;

List _owned_jokers_list;
List _discarded_jokers_list;
List _expired_jokers_list;

BITSET_DEFINE(_avail_jokers_bitset, MAX_DEFINABLE_JOKERS)
static List _shop_jokers_list;

// Stacks
CardObject* played[MAX_SELECTION_SIZE] = {NULL};
int played_top = -1;

CardObject* hand[MAX_HAND_SIZE] = {NULL};
int hand_top = -1;

Card* deck[MAX_DECK_SIZE] = {NULL};
int deck_top = -1;

Card* discard_pile[MAX_DECK_SIZE] = {NULL};
int discard_top = -1;

// Joker Special Variables
int shortcut_joker_count = 0;

int four_fingers_joker_count = 0;

GBAL_UNUSED
static inline bool is_shop_joker_avail(int joker_id)
{
    return bitset_get_idx(&_avail_jokers_bitset, joker_id);
}

void set_shop_joker_avail(int joker_id, bool avail)
{
    bitset_set_idx(&_avail_jokers_bitset, joker_id, avail);
}

static inline int get_num_shop_jokers_avail(void)
{
    return bitset_num_set_bits(&_avail_jokers_bitset);
}

static inline void reset_shop_jokers(void)
{
    int num_jokers = get_joker_registry_size();
    bitset_clear(&_avail_jokers_bitset);
    for (int i = 0; i < num_jokers; i++)
    {
        bitset_set_idx(&_avail_jokers_bitset, i, true);
    }
}

bool no_avail_jokers(void)
{
    return bitset_is_empty(&_avail_jokers_bitset);
}

void played_push(CardObject* card_object)
{
    if (played_top >= MAX_SELECTION_SIZE - 1)
        return;
    played[++played_top] = card_object;
}

static inline CardObject* played_pop()
{
    if (played_top < 0)
        return NULL;
    return played[played_top--];
}

void deck_push(Card* card)
{
    if (deck_top >= MAX_DECK_SIZE - 1)
        return;
    deck[++deck_top] = card;
}

Card* deck_pop()
{
    if (deck_top < 0)
        return NULL;
    return deck[deck_top--];
}

void discard_push(Card* card)
{
    if (discard_top >= MAX_DECK_SIZE - 1)
        return;
    discard_pile[++discard_top] = card;
}

Card* discard_pop()
{
    if (discard_top < 0)
        return NULL;
    return discard_pile[discard_top--];
}

static inline void jokers_available_to_shop_init(void)
{
    reset_shop_jokers();
}

void game_init()
{
    // Initialize all jokers list once
    _owned_jokers_list = list_create();
    _discarded_jokers_list = list_create();
    _expired_jokers_list = list_create();
    _shop_jokers_list = list_create();
    // TODO: Move this to an initialization of the play scoring states
    _joker_scored_itr = list_itr_create(&_owned_jokers_list);

    jokers_available_to_shop_init();

    hands = max_hands;
    discards = max_discards;
    timer = TM_ZERO;
    current_blind = BLIND_TYPE_SMALL;
    blinds_states[0] = BLIND_STATE_CURRENT;
    blinds_states[1] = BLIND_STATE_UPCOMING;
    blinds_states[2] = BLIND_STATE_UPCOMING;
    ante = STARTING_ANTE;
    money = STARTING_MONEY;
    score = STARTING_SCORE;

    blind_select_tokens[BLIND_TYPE_SMALL] = blind_token_new(
        BLIND_TYPE_SMALL,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        MAX_SELECTION_SIZE + MAX_HAND_SIZE + 3
    );
    blind_select_tokens[BLIND_TYPE_BIG] = blind_token_new(
        BLIND_TYPE_BIG,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        MAX_SELECTION_SIZE + MAX_HAND_SIZE + 4
    );
    blind_select_tokens[BLIND_TYPE_BOSS] = blind_token_new(
        BLIND_TYPE_BOSS,
        CUR_BLIND_TOKEN_POS.x,
        CUR_BLIND_TOKEN_POS.y,
        MAX_SELECTION_SIZE + MAX_HAND_SIZE + 5
    );

    obj_hide(blind_select_tokens[BLIND_TYPE_SMALL]->obj);
    obj_hide(blind_select_tokens[BLIND_TYPE_BIG]->obj);
    obj_hide(blind_select_tokens[BLIND_TYPE_BOSS]->obj);
}

static inline void discarded_jokers_update_loop(void)
{
    if (list_is_empty(&_discarded_jokers_list))
    {
        return;
    }

    ListItr itr = list_itr_create(&_discarded_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        joker_object_update(joker_object);
        if (joker_object->sprite_object->x == joker_object->sprite_object->tx &&
            joker_object->sprite_object->y == joker_object->sprite_object->ty)
        {
            list_itr_remove_current_node(&itr);
            joker_object_destroy(&joker_object);
        }
    }
}

static inline void held_jokers_update_loop(void)
{
    const int spacing_lut[MAX_JOKERS_HELD_SIZE][MAX_JOKERS_HELD_SIZE] = {
        {0,  0,   0,   0,   0  },
        {13, -13, 0,   0,   0  },
        {26, 0,   -26, 0,   0  },
        {39, 13,  -13, -39, 0  },
        {40, 20,  0,   -20, -40}
    };

    FIXED hand_x = int2fx(HELD_JOKERS_POS.x);

    ListItr itr = list_itr_create(&_owned_jokers_list);
    JokerObject* joker;
    int jokers_top = list_get_len(&_owned_jokers_list) - 1;
    int i = 0;
    while ((joker = list_itr_next(&itr)))
    {
        joker->sprite_object->tx = hand_x - int2fx(spacing_lut[jokers_top][i++]);

        joker_object_update(joker);
    }
}

static inline void expired_jokers_update_loop(void)
{
    if (list_is_empty(&_expired_jokers_list))
    {
        return;
    }

    ListItr itr = list_itr_create(&_expired_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        joker_object_update(joker_object);

        // let just enough frames pass that we see it rotating and shrinking
        if (timer % FRAMES(EXPIRE_ANIMATION_FRAME_COUNT) == 0)
        {
            // get joker idx
            int expired_joker_idx = 0;
            ListItr joker_itr = list_itr_create(&_owned_jokers_list);
            JokerObject* expired_joker;
            while ((expired_joker = list_itr_next(&joker_itr)) && expired_joker != joker_object)
            {
                expired_joker_idx++;
            }

            // Removing expired Jokers here, instead of immediately like ones we
            // sell or discard allow us to have a small shrink animation without
            // the other owned Jokers rearranging themselves to fill the newly
            // freed space, therefore obscuring the animation
            remove_owned_joker(expired_joker_idx);
            list_itr_remove_current_node(&itr);
            joker_object_destroy(&joker_object);
        }
    }
}

static inline void jokers_update_loop(void)
{
    held_jokers_update_loop();
    discarded_jokers_update_loop();
    expired_jokers_update_loop();
}

void game_update()
{
    timer++;

    jokers_update_loop();

    state_info[game_state].on_update();
}

void game_change_state(enum GameState new_game_state)
{
    timer = TM_ZERO; // Reset the timer

    if (game_state >= 0 && game_state < GAME_STATE_MAX)
    {
        state_info[game_state].substate = 0;
        state_info[game_state].on_exit();
    }

    if (new_game_state >= 0 && new_game_state < GAME_STATE_MAX)
    {
        state_info[new_game_state].on_init();

        game_state = new_game_state;
    }
}

// ============================================================================
// Getter and Setter Functions
// ============================================================================

// Hand and Deck Getters
CardObject** get_hand_array(void)
{
    return hand;
}

int get_hand_top(void)
{
    return hand_top;
}

int hand_get_size(void)
{
    return hand_top + 1;
}

CardObject** get_played_array(void)
{
    return played;
}

int get_played_top(void)
{
    return played_top;
}

int get_scored_card_index(void)
{
    return scored_card_index;
}

int get_deck_top(void)
{
    return deck_top;
}

int deck_get_size(void)
{
    return deck_top + 1;
}

int deck_get_max_size(void)
{
    // This is the max amount of cards that the player currently has in their possession
    return hand_top + played_top + deck_top + discard_top + 4;
}

// Joker List Getters
List* get_jokers_list(void)
{
    return &_owned_jokers_list;
}

List* get_expired_jokers_list(void)
{
    return &_expired_jokers_list;
}

// Game State Getters/Setters
enum GameState* get_game_state_ptr(void)
{
    return &game_state;
}

StateInfo* get_state_info_ptr(void)
{
    return &state_info[game_state];
}

int get_substate(void)
{
    return state_info[game_state].substate;
}

void set_substate(int new_substate)
{
    state_info[game_state].substate = new_substate;
}

// Timer Functions
uint get_timer(void)
{
    return timer;
}

void set_timer(uint new_time)
{
    timer = new_time;
}

void reset_timer(void)
{
    set_timer(TM_ZERO);
}

// Chips and Mult
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

void set_retrigger(bool new_retrigger)
{
    retrigger = new_retrigger;
}

// Money Functions
int get_money(void)
{
    return money;
}

void set_money(int new_money)
{
    money = new_money;
}

void increase_money(int amount)
{
    int _money = get_money();
    _money += amount;
    set_money(_money);
}

// Hands and Discards
int get_num_hands_remaining(void)
{
    return hands;
}

int get_hands(void)
{
    return hands;
}

void reset_hands(void)
{
    hands = max_hands;
}

int get_num_discards_remaining(void)
{
    return discards;
}

int get_discards(void)
{
    return discards;
}

void reset_discards(void)
{
    discards = max_discards;
}

// Round and Ante
int get_round(void)
{
    return game_round;
}

int increment_round(void)
{
    return ++game_round;
}

int get_ante(void)
{
    return ante;
}

// Score management
u32 get_score(void)
{
    return score;
}
void set_score(u32 new_score)
{
    score = new_score;
}
void reset_score(void)
{
    set_score(0);
}

// Blind Management
Sprite* get_blind_select_token(enum BlindType blind_type)
{
    if (blind_type < 0 || blind_type >= BLIND_TYPE_MAX)
    {
        return NULL;
    }
    return blind_select_tokens[blind_type];
}

Sprite* get_playing_blind_token(void)
{
    return playing_blind_token;
}

Sprite* get_round_end_blind_token(void)
{
    return round_end_blind_token;
}

void destroy_playing_and_round_end_blind_tokens(void)
{
    sprite_destroy(&playing_blind_token);
    sprite_destroy(&round_end_blind_token);
}

void hide_blind_select_token(enum BlindType blind_type)
{
    if (blind_type < 0 || blind_type >= BLIND_TYPE_MAX)
    {
        return;
    }
    Sprite* token_sprite = get_blind_select_token(blind_type);
    obj_hide(token_sprite->obj);
}

void hide_all_blind_select_tokens(void)
{
    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        hide_blind_select_token((enum BlindType)i);
    }
}

void unhide_blind_select_token(enum BlindType blind_type)
{
    if (blind_type < 0 || blind_type >= BLIND_TYPE_MAX)
    {
        return;
    }
    Sprite* token_sprite = get_blind_select_token(blind_type);
    obj_unhide(token_sprite->obj, 0);
}

void unhide_all_blind_select_tokens(void)
{
    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        unhide_blind_select_token((enum BlindType)i);
    }
}

void move_blind_select_token(enum BlindType blind_type, int x, int y)
{
    if (blind_type < 0 || blind_type >= BLIND_TYPE_MAX)
    {
        return;
    }
    Sprite* token_sprite = get_blind_select_token(blind_type);
    sprite_position(token_sprite, x, y);
}

void get_blind_select_token_pos(enum BlindType blind_type, int* x, int* y)
{
    if (!x || !y)
        return;

    if (blind_type < 0 || blind_type >= BLIND_TYPE_MAX)
    {
        *x = -1;
        *y = -1;
        return;
    }
    Sprite* token_sprite = get_blind_select_token(blind_type);
    *x = token_sprite->pos.x;
    *y = token_sprite->pos.y;
}

enum BlindState get_blinds_state(enum BlindType blind_type)
{
    if (blind_type < 0 || blind_type >= BLIND_TYPE_MAX)
    {
        return 0;
    }
    return blinds_states[blind_type];
}

int get_current_blind(void)
{
    return current_blind;
}

void increment_blind(enum BlindState increment_reason)
{
    current_blind++;
    if (current_blind >= BLIND_TYPE_MAX)
    {
        current_blind = 0;
        blinds_states[0] = BLIND_STATE_CURRENT;  // Reset the blinds to the first one
        blinds_states[1] = BLIND_STATE_UPCOMING; // Set the next blind to upcoming
        blinds_states[2] = BLIND_STATE_UPCOMING; // Set the next blind to upcoming
    }
    else
    {
        blinds_states[current_blind] = BLIND_STATE_CURRENT;
        blinds_states[current_blind - 1] = increment_reason;
    }
}

// RNG Functions
void incr_rng_seed(void)
{
    rng_seed++;
}

void mult_rng_seed(int factor)
{
    rng_seed *= factor;
}

// End of Getter and Setter Functions
// ============================================================================

// ============================================================================
// Joker Query Functions
// ============================================================================

bool is_joker_owned(int joker_id)
{
    ListItr itr = list_itr_create(&_owned_jokers_list);
    JokerObject* joker;

    while ((joker = list_itr_next(&itr)))
    {
        if (joker->joker->id == joker_id)
        {
            return true;
        }
    }
    return false;
}

bool is_shortcut_joker_active(void)
{
    return shortcut_joker_count > 0;
}

int get_straight_and_flush_size(void)
{
    return four_fingers_joker_count > 0 ? STRAIGHT_AND_FLUSH_SIZE_FOUR_FINGERS
                                        : STRAIGHT_AND_FLUSH_SIZE_DEFAULT;
}

bool card_is_face(Card* card)
{
    // Card is a face card, or Pareidolia is present
    return (
        card->rank == JACK || card->rank == QUEEN || card->rank == KING ||
        is_joker_owned(PAREIDOLIA_JOKER_ID)
    );
}

void remove_owned_joker(int owned_joker_idx)
{
    // TODO: Extract to on_joker_removed() callback
    JokerObject* joker_object = list_get_at_idx(&_owned_jokers_list, owned_joker_idx);
    // In case the player gets multiple Four Fingers Jokers,
    // and only reset the size when all of them have been removed
    if (joker_object->joker->id == FOUR_FINGERS_JOKER_ID)
    {
        four_fingers_joker_count--;
    }

    if (joker_object->joker->id == SHORTCUT_JOKER_ID)
    {
        shortcut_joker_count--;
    }

    set_shop_joker_avail(joker_object->joker->id, true);
    list_remove_at_idx(&_owned_jokers_list, owned_joker_idx);
}

// ============================================================================
// Hand Management Functions
// ============================================================================

// idx_a and idx_b are assumed to be valid indexes within the hand array
// no checks will be performed here for performance's sake
void swap_cards_in_hand(int idx_a, int idx_b)
{
    CardObject* temp = hand[idx_a];
    hand[idx_a] = hand[idx_b];
    hand[idx_b] = temp;
}

void sort_hand_by_suit(void)
{
    for (int idx_a = 0; idx_a < hand_top; idx_a++)
    {
        for (int idx_b = idx_a + 1; idx_b <= hand_top; idx_b++)
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

void sort_hand_by_rank(void)
{
    for (int idx_a = 0; idx_a < hand_top; idx_a++)
    {
        for (int idx_b = idx_a + 1; idx_b <= hand_top; idx_b++)
        {
            if (hand[idx_a] == NULL ||
                (hand[idx_b] != NULL && hand[idx_a]->card->rank > hand[idx_b]->card->rank))
            {
                swap_cards_in_hand(idx_a, idx_b);
            }
        }
    }
}

bool shift_null_card_to_end(int null_card_idx)
{
    // Start by searching any non NULL cards after the NULL one
    // don't start at null_card_idx+1 to avoid potential illegal array access
    int non_null_card_idx = null_card_idx;
    for (; non_null_card_idx <= hand_top; non_null_card_idx++)
    {
        if (hand[non_null_card_idx] != NULL)
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
        hand[null_card_idx + j] = hand[non_null_card_idx + j];
    }

    return true;
}

// ============================================================================
// Hand Type Detection and Card Evaluation
// ============================================================================

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

void set_hand(void)
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

static bool can_discard_hand(void)
{
    return (discards > 0 && hand_state == HAND_SELECT && hand_selections > 0);
}

// ============================================================================
// Deck Management Functions
// ============================================================================

void deck_shuffle(void)
{
    for (int i = deck_top; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Card* temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

// ============================================================================
// Game Initialization and Core Functions
// ============================================================================

static inline void set_seed(int seed)
{
    rng_seed = seed;
    srand(rng_seed);
}

// Playing state functions
static void game_playing_discard_on_pressed(void)
{
    if (!can_discard_hand())
        return;

    hand_state = HAND_DISCARD;
    display_hands(--discards);
    set_hand();
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d",
        DISCARDS_TEXT_RECT.left,
        DISCARDS_TEXT_RECT.top,
        TTE_RED_PB,
        discards
    );

    // Move back to hand selection
    selection_grid_move_selection_vert(&game_playing_selection_grid, -1);
}

static void game_playing_play_hand_on_pressed(void)
{
    if (!can_play_hand())
        return;

    hand_state = HAND_PLAY;
    display_hands(--hands);

    // Move back to hand selection
    selection_grid_move_selection_vert(&game_playing_selection_grid, -1);
}

static int game_playing_hand_row_get_size(void)
{
    return hand_get_size();
}

int game_playing_button_row_get_size(void)
{
    return NUM_ELEM_IN_ARR(game_playing_buttons);
}

int game_shop_get_rand_available_joker_id(void)
{
    // Roll for what rarity the joker will be
    int joker_rarity = joker_get_random_rarity();

    // Now determine how many jokers are available based on the rarity
    int jokers_avail_size = get_num_shop_jokers_avail();

    if (jokers_avail_size == 0)
        return UNDEFINED;

    int matching_joker_ids[jokers_avail_size];
    int fallback_random_idx = random() % jokers_avail_size;
    int fallback_random_joker_id = UNDEFINED;
    int match_count = 0;

    BitsetItr itr = bitset_itr_create(&_avail_jokers_bitset);

    int i = 0;
    int joker_id = UNDEFINED;
    while ((joker_id = bitset_itr_next(&itr)) != UNDEFINED)
    {
        if (i++ == fallback_random_idx)
            fallback_random_joker_id = joker_id;
        const JokerInfo* info = get_joker_registry_entry(joker_id);
        if (info->rarity == joker_rarity)
        {
            matching_joker_ids[match_count++] = joker_id;
        }
    }

    int selected_joker_id =
        (match_count > 0) ? matching_joker_ids[random() % match_count] : fallback_random_joker_id;

    return selected_joker_id;
}

void game_start(void)
{
    game_main_menu_cleanup();

    set_seed(rng_seed);
    // set_seed(9); // 9 is a full house

    affine_background_change_background(AFFINE_BG_GAME);

    hands = max_hands;
    discards = max_discards;

    // Fill the deck with all the cards. Later on this can be replaced with a more dynamic system
    // that allows for different decks and card types.
    for (int suit = 0; suit < NUM_SUITS; suit++)
    {
        for (int rank = 0; rank < NUM_RANKS; rank++)
        {
            Card* card = card_new(suit, rank);
            deck_push(card);
        }
    }

    change_background(BG_BLIND_SELECT);

    // Deck size/max size
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d/%d",
        DECK_SIZE_RECT.left,
        DECK_SIZE_RECT.top,
        TTE_WHITE_PB,
        deck_get_size(),
        deck_get_max_size()
    );

    display_round(game_round); // Set the round display
    display_score(score);      // Set the score display

    display_chips(); // Set the chips display
    display_mult();  // Set the multiplier display

    display_hands(hands);       // Hand
    display_discards(discards); // Discard

    display_money(); // Set the money display

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d#{cx:0x%X000}/%d",
        ANTE_TEXT_RECT.left,
        ANTE_TEXT_RECT.top,
        TTE_YELLOW_PB,
        ante,
        TTE_WHITE_PB,
        MAX_ANTE
    ); // Ante

    game_change_state(GAME_STATE_BLIND_SELECT);
}
