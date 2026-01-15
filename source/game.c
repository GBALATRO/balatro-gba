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
#include "game/main_menu.h"
#include "game/palette.h"
#include "game/point.h"
#include "game/rect.h"
#include "game/round.h"
#include "game/selection.h"
#include "game/timer.h"
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
extern void game_round_on_init(void);
extern void game_playing_on_update(void);
static void game_round_end_on_update(void);
static void game_round_end_on_exit(void);
static void game_shop_on_update(void);
static void game_shop_on_exit(void);
static void game_lose_on_init(void);
static void game_lose_on_update(void);
static void game_over_on_exit(void);
static void game_win_on_init(void);
static void game_win_on_update(void);
static void game_shop_intro(void);
static void game_shop_process_user_input(void);
static void game_shop_outro(void);
static void game_round_end_start(void);
static void game_round_end_start_expand_popup(void);
static void game_round_end_display_finished_blind(void);
static void game_round_end_display_score_min(void);
static void game_round_end_update_blind_reward(void);
static void game_round_end_panel_exit(void);
static void game_round_end_display_rewards(void);
static void game_round_end_display_cashout(void);
static void game_round_end_dismiss_round_end_panel(void);

void change_background(enum BackgroundId id);
void display_temp_score(u32 value);
void display_score(u32 value);
static void check_flaming_score(void);
void display_round(int value);
static void display_hands(int value);
static void display_discards(int value);
void set_hand(void);
int deck_get_size(void);
int deck_get_max_size(void);
void increment_blind(enum BlindState increment_reason);
static void game_over_init(void);
static int calculate_interest_reward(void);
static void game_over_anim_frame(void);

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

static void shop_reroll_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool shop_reroll_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static int shop_reroll_row_get_size(void);
static bool shop_top_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void shop_top_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static int shop_top_row_get_size(void);
static void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static int jokers_sel_row_get_size(void);
static void game_shop_create_items(void);

static void erase_price_under_sprite_object(SpriteObject* sprite_object);
static void print_price_under_sprite_object(SpriteObject* sprite_object, int price);
static void game_round_end_extend_black_panel_down(int black_panel_bottom);

static void remove_owned_joker(int owned_joker_idx);

extern bool can_play_hand(void);
static bool can_discard_hand(void);

// Consts

// clang-format off
// disable clang-format here to preserve the organization here
// Flaming score animation frames
#define SCORE_FLAMES_ANIM_FREQ  5 // animation will run at 12FPS
#define NUM_SCORE_FLAMES_FRAMES 8 // Chips and Mult flame frames are next to one another
#define SCORE_FLAME_FRAME_WIDTH 3 // so we only need to offset to get the next ones
// clang-format on

uint rng_seed = 0;

typedef void (*SubStateActionFn)(void);

uint timer = 0; // This might already exist in libtonc but idk so i'm just making my own
// BY DEFAULT IS SET TO 1, but if changed to 2 or more, should speed up all (or most) of the game
// aspects that should be sped up by speed, as in the original game.
int game_speed = 1;
enum BackgroundId background = BG_NONE;

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

SelectionGridRow shop_selection_rows[] = {
    {0, jokers_sel_row_get_size,  jokers_sel_row_on_selection_changed,  jokers_sel_row_on_key_transit,  {.wrap = false}},
    {1, shop_top_row_get_size,    shop_top_row_on_selection_changed,    shop_top_row_on_key_transit,    {.wrap = false}},
    {2, shop_reroll_row_get_size, shop_reroll_row_on_selection_changed, shop_reroll_row_on_key_transit, {.wrap = false}}
};

static const Selection SHOP_INIT_SEL = {-1, 1};

SelectionGrid shop_selection_grid = {
    shop_selection_rows,
    NUM_ELEM_IN_ARR(shop_selection_rows),
    SHOP_INIT_SEL
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

static const SubStateActionFn shop_state_actions[] = {
    game_shop_intro,
    game_shop_process_user_input,
    game_shop_outro
};

static const SubStateActionFn round_end_state_actions[] = {
    game_round_end_start,
    game_round_end_start_expand_popup,
    game_round_end_display_finished_blind,
    game_round_end_display_score_min,
    game_round_end_update_blind_reward,
    game_round_end_panel_exit,
    game_round_end_display_rewards,
    game_round_end_display_cashout,
    game_round_end_dismiss_round_end_panel
};

static int reroll_cost = REROLL_BASE_COST;

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

static int blind_reward = 0;
static int hand_reward = 0;
static int interest_reward = 0;
static int interest_to_count = 0;
static int interest_start_time = UNDEFINED;

// Red deck default (can later be moved to a deck.h file or something)
static int max_hands = 4;
static int max_discards = 4;
// Set in game_init and game_round_init
int hands = 0;
static int discards = 0;

int game_round = 0;
int ante = 0;
static int money = 0;
u32 score = 0;
u32 temp_score = 0; // This is the score that shows in the same spot as the hand type.
bool score_flames_active = false;
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
static List _discarded_jokers_list;
static List _expired_jokers_list;

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
static int shortcut_joker_count = 0;

static int four_fingers_joker_count = 0;

GBAL_UNUSED
static inline bool is_shop_joker_avail(int joker_id)
{
    return bitset_get_idx(&_avail_jokers_bitset, joker_id);
}

static inline void set_shop_joker_avail(int joker_id, bool avail)
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

static inline bool no_avail_jokers(void)
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

bool card_is_face(Card* card)
{
    // Card is a face card, or Pareidolia is present
    return (
        card->rank == JACK || card->rank == QUEEN || card->rank == KING ||
        is_joker_owned(PAREIDOLIA_JOKER_ID)
    );
}

List* get_jokers_list(void)
{
    return &_owned_jokers_list;
}

List* get_expired_jokers_list(void)
{
    return &_expired_jokers_list;
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

static void add_joker(JokerObject* joker_object)
{
    list_push_back(&_owned_jokers_list, joker_object);

    // TODO: Extract to on_joker_added() callback
    // In case the player gets multiple Four Fingers Jokers,
    // only change size when the first one is added
    if (joker_object->joker->id == FOUR_FINGERS_JOKER_ID)
    {
        four_fingers_joker_count++;
    }

    if (joker_object->joker->id == SHORTCUT_JOKER_ID)
    {
        shortcut_joker_count++;
    }
}

static void remove_owned_joker(int owned_joker_idx)
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

int get_deck_top(void)
{
    return deck_top;
}

int get_num_discards_remaining(void)
{
    return discards;
}

int get_num_hands_remaining(void)
{
    return hands;
}

int get_game_speed(void)
{
    return game_speed;
}

// for the future when a menu actually lets this variable be changed.
void set_game_speed(int new_game_speed)
{
    game_speed = new_game_speed;
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

int get_money(void)
{
    return money;
}

void set_money(int new_money)
{
    money = new_money;
}

void set_retrigger(bool new_retrigger)
{
    retrigger = new_retrigger;
}

void display_money()
{
    Rect money_text_rect = MONEY_TEXT_RECT;
    tte_erase_rect_wrapper(MONEY_TEXT_RECT);

    char money_str_buff[INT_MAX_DIGITS + 2]; // + 2 for null terminator and "$" sign
    snprintf(money_str_buff, sizeof(money_str_buff), "$%d", money);

    // Bias left so the number is centered and the "$" sign is on the left
    update_text_rect_to_center_str(&money_text_rect, money_str_buff, SCREEN_LEFT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        money_text_rect.left,
        money_text_rect.top,
        TTE_YELLOW_PB,
        money_str_buff
    );
}

void display_chips(void)
{
    Rect chips_text_rect = CHIPS_TEXT_RECT;

    // In case of overflow, the rect overflow left by 1 char
    Rect chips_text_overflow_rect = chips_text_rect;
    chips_text_overflow_rect.left -= TTE_CHAR_SIZE;
    tte_erase_rect_wrapper(chips_text_overflow_rect);

    char chips_str_buff[UINT_MAX_DIGITS + 1];
    truncate_uint_to_suffixed_str(
        chips,
        rect_width(&chips_text_rect) / TTE_CHAR_SIZE,
        chips_str_buff
    );

    update_text_rect_to_right_align_str(&chips_text_rect, chips_str_buff, OVERFLOW_LEFT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000;}%s",
        chips_text_rect.left,
        chips_text_rect.top,
        TTE_WHITE_PB,
        chips_str_buff
    );
    check_flaming_score();
}

void display_mult(void)
{
    Rect mult_text_overflow_rect = MULT_TEXT_RECT;
    // In case of overflow the rect will overflow right by 1 char
    mult_text_overflow_rect.right += TTE_CHAR_SIZE;
    tte_erase_rect_wrapper(mult_text_overflow_rect);

    char mult_str_buff[UINT_MAX_DIGITS + 1];
    truncate_uint_to_suffixed_str(mult, rect_width(&MULT_TEXT_RECT) / TTE_CHAR_SIZE, mult_str_buff);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000;}%s",
        MULT_TEXT_RECT.left,
        MULT_TEXT_RECT.top,
        TTE_WHITE_PB,
        mult_str_buff
    );

    check_flaming_score();
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

/* Copies the appropriate item into the top left panel (blind/shop icon)
 * from where it was put outside the screenview
 */
static void bg_copy_current_item_to_top_left_panel(void)
{
    main_bg_se_copy_rect(TOP_LEFT_ITEM_SRC_RECT, TOP_LEFT_PANEL_POINT);
}

// Resets bottom row bg tiles of the top left panel (shop/blind) after
// it is dismissed to match the rest of the game panel background.
void reset_top_left_panel_bottom_row()
{
    BG_POINT top_left_panel_bottom_row_pos = TOP_LEFT_PANEL_POINT;
    // Use the source rect height to offset to the bottom row point
    top_left_panel_bottom_row_pos.y += rect_height(&TOP_LEFT_ITEM_SRC_RECT) - 1;
    main_bg_se_copy_rect(TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT, top_left_panel_bottom_row_pos);
}

void change_background(enum BackgroundId id)
{
    if (background == id)
    {
        return;
    }
    else if (id == BG_CARD_SELECTING)
    {
        tte_erase_rect_wrapper(HAND_SIZE_RECT_PLAYING);
        REG_WIN0V = (REG_WIN0V << 8) | 0x80; // Set window 0 top to 128

        if (background == BG_CARD_PLAYING)
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

                affine_background_set_color(
                    blind_get_color(BLIND_TYPE_BOSS, BLIND_SHADOW_COLOR_INDEX)
                );
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

            for (int i = 0; i < NUM_ELEM_IN_ARR(game_playing_buttons); i++)
            {
                button_set_highlight(&game_playing_buttons[i], false);
            }
        }
    }
    else if (id == BG_CARD_PLAYING)
    {
        if (background != BG_CARD_SELECTING)
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
    else if (id == BG_ROUND_END)
    {
        if (background != BG_CARD_SELECTING && background != BG_CARD_PLAYING)
        {
            change_background(BG_CARD_SELECTING);
            background = BG_ROUND_END;
        }

        // Disable window 0 so it doesn't make the cashout menu transparent
        toggle_windows(false, true);

        main_bg_se_clear_rect(ROUND_END_MENU_RECT);
        tte_erase_rect_wrapper(HAND_SIZE_RECT);
    }
    else if (id == BG_SHOP)
    {
        toggle_windows(false, true);

        GRIT_CPY(pal_bg_mem, background_shop_gfxPal);
        GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_shop_gfxTiles);
        GRIT_CPY(&se_mem[MAIN_BG_SBB], background_shop_gfxMap);

        // Set the outline colors for the shop background. This is used for the alternate shop
        // palettes when opening packs
        memset16(&pal_bg_mem[SHOP_BOTTOM_PANEL_BORDER_PID], 0x213D, 1);
        memset16(&pal_bg_mem[SHOP_PANEL_SHADOW_PID], 0x10B4, 1);

        // Reset the shop lights to correct colors
        memset16(&pal_bg_mem[SHOP_LIGHTS_2_PID], SHOP_LIGHTS_2_CLR, 1);
        memset16(&pal_bg_mem[SHOP_LIGHTS_3_PID], SHOP_LIGHTS_3_CLR, 1);
        memset16(&pal_bg_mem[SHOP_LIGHTS_4_PID], SHOP_LIGHTS_4_CLR, 1);
        memset16(&pal_bg_mem[SHOP_LIGHTS_1_PID], SHOP_LIGHTS_1_CLR, 1);

        // Disable the button highlight colors
        memcpy16(&pal_bg_mem[REROLL_BTN_SELECTED_BORDER_PID], &pal_bg_mem[REROLL_BTN_PID], 1);
        memcpy16(
            &pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[NEXT_ROUND_BTN_PID],
            1
        );
    }
    else if (id == BG_BLIND_SELECT)
    {
        game_blind_select_change_background();
    }
    else if (id == BG_MAIN_MENU)
    {
        game_main_menu_change_background();
    }
    else
    {
        return; // Invalid background ID
    }

    background = id;
}

void display_temp_score(u32 value)
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

void display_score(u32 value)
{
    Rect score_rect = SCORE_RECT;
    // Clear the existing text before redrawing
    tte_erase_rect_wrapper(SCORE_RECT);

    char score_str_buff[UINT_MAX_DIGITS + 1];

    truncate_uint_to_suffixed_str(value, rect_width(&score_rect) / TTE_CHAR_SIZE, score_str_buff);
    update_text_rect_to_center_str(&score_rect, score_str_buff, SCREEN_RIGHT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        score_rect.left,
        score_rect.top,
        TTE_WHITE_PB,
        score_str_buff
    );
}

// Show/Hide flaming score effect if we will score
// more than the required amount or not
static void check_flaming_score(void)
{
    u32 curr_score = u32_protected_mult(chips, mult);
    u32 required_score = blind_get_requirement(current_blind, ante);
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

void display_round(int value)
{
    // tte_erase_rect_wrapper(ROUND_TEXT_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d",
        ROUND_TEXT_RECT.left,
        ROUND_TEXT_RECT.top,
        TTE_YELLOW_PB,
        game_round
    );
}

static void display_hands(int value)
{
    // tte_erase_rect_wrapper(HANDS_TEXT_RECT);
    tte_printf("#{P:%d,%d; cx:0xD000}%d", HANDS_TEXT_RECT.left, HANDS_TEXT_RECT.top, hands); // Hand
}

static void display_discards(int value)
{
    // tte_erase_rect_wrapper(DISCARDS_TEXT_RECT);
    // Discard
    tte_printf(
        "#{P:%d,%d; cx:0xE000}%d",
        DISCARDS_TEXT_RECT.left,
        DISCARDS_TEXT_RECT.top,
        discards
    );
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

int deck_get_size(void)
{
    return deck_top + 1;
}

int deck_get_max_size(void)
{
    // This is the max amount of cards that the player currently has in their possession
    return hand_top + played_top + deck_top + discard_top + 4;
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

static void game_over_init(void)
{
    // Clears the round end menu
    main_bg_se_clear_rect(POP_MENU_ANIM_RECT);
    main_bg_se_copy_expand_3x3_rect(GAME_OVER_DIALOG_DEST_RECT, GAME_OVER_SRC_RECT_3X3_POS);
    main_bg_se_copy_rect(NEW_RUN_BTN_SRC_RECT, NEW_RUN_BTN_DEST_POS);
}

static void game_lose_on_init()
{
    game_over_init();
    // Using the text color to match the "Game Over" text
    affine_background_set_color(TEXT_CLR_RED);
}

static void game_win_on_init()
{
    game_over_init();
    // Using the text color to match the "You Win" text
    affine_background_set_color(TEXT_CLR_BLUE);
}

// General functions
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

static int calculate_interest_reward(void)
{
    int reward = (money / 5) * INTEREST_PER_5;
    if (reward > MAX_INTEREST)
        reward = MAX_INTEREST;
    return reward;
}

static void game_round_end_on_exit()
{
    // Cleanup blind tokens from this round to avoid accumulating
    // allocated blind sprites each round
    blind_reward = 0;
    hand_reward = 0;
    interest_reward = 0;
    sprite_destroy(&playing_blind_token);
    sprite_destroy(&round_end_blind_token);
    // TODO: Reuse sprites for blind selection?
}

static void game_round_end_on_update()
{
    if (state_info[game_state].substate == ROUND_END_EXIT)
    {
        game_change_state(GAME_STATE_SHOP);
        return;
    }

    int substate = state_info[game_state].substate;
    round_end_state_actions[substate]();
}

static void game_round_end_start()
{
    // Reset static variables to default values upon re-entering the round end state
    if (timer == TM_RESET_STATIC_VARS)
    {
        change_background(BG_ROUND_END); // Change the background to the round end background
        state_info[game_state].substate = START_EXPAND_POPUP; // Change the state to the next one
        timer = TM_ZERO;                                      // Reset the timer
        blind_reward = blind_get_reward(current_blind);
        hand_reward = hands;
        interest_reward = calculate_interest_reward();
        interest_to_count = interest_reward;
        interest_start_time = UNDEFINED;
    }
}

static void game_round_end_start_expand_popup()
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (timer == TM_END_POP_MENU_ANIM)
    {
        state_info[game_state].substate = DISPLAY_FINISHED_BLIND;
        timer = TM_ZERO;
    }
}

static void game_round_end_extend_black_panel_down(int black_panel_bottom)
{
    Rect single_line_rect = ROUND_END_MENU_RECT;
    single_line_rect.bottom = black_panel_bottom;
    single_line_rect.top = single_line_rect.bottom - 1;
    main_bg_se_copy_rect_1_tile_vert(single_line_rect, SCREEN_DOWN);
}

static void game_round_end_display_finished_blind()
{
    obj_unhide(round_end_blind_token->obj, 0);

    int current_ante = ante;

    // Beating the boss blind increases the ante, so we need to display the previous ante value
    if (current_blind == BLIND_TYPE_BOSS)
        current_ante--;

    Rect blind_req_rect = ROUND_END_BLIND_REQ_RECT;
    u32 blind_req = blind_get_requirement(current_blind, current_ante);

    /* Not bothering to truncate here because there are 8 tiles
     * and the blind requirement will not increase past ante 8
     * so there's enough room for sure.
     */
    char blind_req_str_buff[UINT_MAX_DIGITS + 1];
    snprintf(blind_req_str_buff, sizeof(blind_req_str_buff), "%lu", blind_req);

    update_text_rect_to_right_align_str(&blind_req_rect, blind_req_str_buff, OVERFLOW_RIGHT);

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        blind_req_rect.left,
        blind_req_rect.top,
        TTE_RED_PB,
        blind_req_str_buff
    );

    if (timer == TM_START_ROUND_END_REWARDS_ANIM)
    {
        game_round_end_extend_black_panel_down(ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE);
    }

    if (timer >= TM_END_DISPLAY_FIN_BLIND)
    {
        state_info[game_state].substate = DISPLAY_SCORE_MIN;
        timer = TM_ZERO;
    }
}

static void game_round_end_display_score_min()
{
    const int timer_offset = timer - 1;
    const int x_from = 0;
    const int y_from = 29;
    const int x_to = 13;
    const int y_to = 11;

    memcpy16(
        &se_mem[MAIN_BG_SBB][x_to + timer_offset + 32 * y_to],
        &se_mem[MAIN_BG_SBB][x_from + timer_offset + 32 * y_from],
        1
    );

    if (timer >= TM_END_DISPLAY_SCORE_MIN)
    {
        state_info[game_state].substate = UPDATE_BLIND_REWARD;
        timer = TM_ZERO;
    }
}

static void game_round_end_update_blind_reward()
{
    if (timer % FRAMES(20) != 0)
        return;

    // TODO: Add sound effect here

    if (blind_reward > 0)
    {
        blind_reward--;
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            BLIND_REWARD_RECT.left,
            BLIND_REWARD_RECT.top,
            TTE_YELLOW_PB,
            blind_reward
        );
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            ROUND_END_BLIND_REWARD_RECT.left,
            ROUND_END_BLIND_REWARD_RECT.top,
            TTE_YELLOW_PB,
            blind_get_reward(current_blind) - blind_reward
        );
    }
    else if (timer > FRAMES(20))
    {
        tte_erase_rect_wrapper(BLIND_REWARD_RECT);
        tte_erase_rect_wrapper(BLIND_REQ_TEXT_RECT);
        obj_hide(playing_blind_token->obj);
        affine_background_load_palette(affine_background_gfxPal);
        state_info[game_state].substate = BLIND_PANEL_EXIT;
        timer = TM_ZERO;
    }
}

static void game_round_end_panel_exit()
{
    // TODO: make heads or tails of what's going on here and replace
    // magic numbers.
    if (timer < 8)
    {
        main_bg_se_copy_rect_1_tile_vert(TOP_LEFT_PANEL_ANIM_RECT, SCREEN_UP);

        if (timer == 1) // Copied from shop. Feels slightly too niche of a function for me
                        // personally to make one.
        {
            reset_top_left_panel_bottom_row();
        }
        else if (timer == 2)
        {
            int y = 5;
            memset16(&se_mem[MAIN_BG_SBB][32 * (y - 1)], 0x0001, 1);
            memset16(&se_mem[MAIN_BG_SBB][1 + 32 * (y - 1)], 0x0002, 7);
            memset16(&se_mem[MAIN_BG_SBB][8 + 32 * (y - 1)], 0x0401, 1);
        }
    }
    else if (timer > FRAMES(20))
    {
        memset16(&pal_bg_mem[REWARD_PANEL_BORDER_PID], 0x1483, 1);
        state_info[game_state].substate = DISPLAY_REWARDS;
        timer = TM_ZERO;
    }
}

static inline void game_round_end_print_separator_ellipsis(void)
{
    int x =
        (ROUND_END_REWARDS_ELLIPSIS_POS.x + timer - TM_REWARDS_ELLIPSIS_PRINT_START) * TILE_SIZE;
    int y = (ROUND_END_REWARDS_ELLIPSIS_POS.y) * TILE_SIZE;

    tte_printf("#{P:%d,%d; cx:0x%X000}.", x, y, TTE_WHITE_PB);
}

// TODO: Allow for more generic rewards and consolidate with game_round_end_print_interest_reward()
static inline void game_round_end_print_hand_reward(int hand_y_offset)
{
    int hand_y = ROUND_END_REWARDS_ELLIPSIS_POS.y + hand_y_offset;
    if (timer == TM_DISPLAY_REWARDS_CONT_WAIT)
    {
        game_round_end_extend_black_panel_down(hand_y);

        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%d #{cx:0x%X000}Hands",
            ROUND_END_REWARD_TEXT_X,
            hand_y * TILE_SIZE,
            TTE_BLUE_PB,
            hand_reward,
            TTE_WHITE_PB
        );
    }
    // Increment the hand reward text until the hand reward variable is depleted
    else if (timer > TM_HAND_REWARD_INCR_WAIT && timer % FRAMES(TM_REWARD_INCREMENT_INTERVAL) == 0)
    {
        hand_reward--;
        tte_printf(
            "#{P:%d, %d; cx:0x%X000}$%d",
            ROUND_END_REWARD_AMOUNT_X,
            hand_y * TILE_SIZE,
            TTE_YELLOW_PB,
            hands - hand_reward
        );
        if (hand_reward == 0)
        {
            interest_start_time = timer + TM_REWARD_DISPLAY_INTERVAL;
        }
    }
}

static inline void game_round_end_print_interest_reward(int interest_y_offset)
{
    int interest_y = ROUND_END_REWARDS_ELLIPSIS_POS.y + interest_y_offset;

    if (timer == interest_start_time)
    {
        game_round_end_extend_black_panel_down(interest_y);

        tte_printf(
            "#{P:%d,%d; cx:0x%X000}%d #{cx:0x%X000}Interest",
            ROUND_END_REWARD_TEXT_X,
            interest_y * TILE_SIZE,
            TTE_YELLOW_PB,
            interest_reward,
            TTE_WHITE_PB
        );
    }
    // Increment the interest reward text until the interest reward variable is depleted
    else if (timer > interest_start_time + TM_REWARD_DISPLAY_INTERVAL &&
             timer % FRAMES(TM_REWARD_INCREMENT_INTERVAL) == 0)
    {
        interest_to_count--;
        tte_printf(
            "#{P:%d, %d; cx:0x%X000}$%d",
            ROUND_END_REWARD_AMOUNT_X,
            interest_y * TILE_SIZE,
            TTE_YELLOW_PB,
            interest_reward - interest_to_count
        );
    }
}

static void game_round_end_display_rewards()
{
    int hand_y_offset = 0;
    int interest_y_offset = 0;

    if (hands > 0)
    {
        hand_y_offset = 1;
    }
    else
    {
        interest_start_time = TM_DISPLAY_REWARDS_CONT_WAIT;
    }

    if (interest_reward > 0)
    {
        interest_y_offset = hand_y_offset + 1;
    }

    // Once all rewards are accounted for go to the next state
    if (hand_reward <= 0 && interest_to_count <= 0)
    {
        timer = TM_ZERO;
        state_info[game_state].substate = DISPLAY_CASHOUT;
    }
    else if (timer == TM_START_ROUND_END_REWARDS_ANIM)
    {
        game_round_end_extend_black_panel_down(ROUND_END_REWARDS_ELLIPSIS_POS.y);
    }
    else if (timer < TM_REWARDS_ELLIPSIS_PRINT_END)
    {
        game_round_end_print_separator_ellipsis();
    }
    else if (timer >= TM_DISPLAY_REWARDS_CONT_WAIT && hand_reward > 0)
    {
        game_round_end_print_hand_reward(hand_y_offset);
    }
    else if (interest_start_time != UNDEFINED && timer >= interest_start_time &&
             interest_to_count > 0)
    {
        game_round_end_print_interest_reward(interest_y_offset);
    }
}

static inline void game_round_end_cashout(void)
{
    // Reward the player
    money += hands + blind_get_reward(current_blind) + calculate_interest_reward();
    display_money();

    hands = max_hands;          // Reset the hands to the maximum
    discards = max_discards;    // Reset the discards to the maximum
    display_hands(hands);       // Set the hands display
    display_discards(discards); // Set the discards display

    score = 0;
    display_score(score); // Set the score display
}

static void game_round_end_display_cashout()
{
    if (timer == FRAMES(40))
    {
        // Put the "cash out" button onto the round end panel
        main_bg_se_copy_expand_3x3_rect(CASHOUT_DEST_RECT, CASHOUT_SRC_3X3_RECT_POS);

        int cashout_amount = hands + blind_get_reward(current_blind) + calculate_interest_reward();

        bool omit_space = cashout_amount >= 10;
        tte_printf(
            "#{P:%d, %d; cx:0x%X000}Cash Out:%s$%d",
            CASHOUT_TEXT_RECT.left,
            CASHOUT_TEXT_RECT.top,
            TTE_WHITE_PB,
            omit_space ? "" : " ",
            cashout_amount
        );
    }

    // Wait until the player presses A to cash out
    else if (timer > FRAMES(40) && key_hit(SELECT_CARD))
    {
        game_round_end_cashout();

        state_info[game_state].substate = DISMISS_ROUND_END_PANEL; // Go to the next state
        timer = TM_ZERO;

        obj_hide(round_end_blind_token->obj);          // Hide the blind token object
        tte_erase_rect_wrapper(BLIND_TOKEN_TEXT_RECT); // Erase the blind token text
    }
}

static void game_round_end_dismiss_round_end_panel()
{
    Rect round_end_down = ROUND_END_MENU_RECT;
    round_end_down.top--;
    main_bg_se_copy_rect_1_tile_vert(round_end_down, SCREEN_DOWN);

    if (timer >= TM_DISMISS_ROUND_END_TM)
    {
        timer = TM_ZERO;
        state_info[game_state].substate = ROUND_END_EXIT;
    }
}

static Rect get_text_rect_under_sprite_object(SpriteObject* sprite_object)
{
    int height = 0;
    int width = 0;

    if (sprite_object_get_dimensions(sprite_object, &width, &height) == false)
    {
        // fallback
        height = CARD_SPRITE_SIZE;
        width = CARD_SPRITE_SIZE;
    }

    Rect ret_rect = {0};

    ret_rect.left = fx2int(sprite_object->tx);
    ret_rect.top = fx2int(sprite_object->ty) + height + TILE_SIZE;
    ret_rect.right = ret_rect.left + width;
    ret_rect.bottom = ret_rect.top + TTE_CHAR_SIZE;

    return ret_rect;
}

static void print_price_under_sprite_object(SpriteObject* sprite_object, int price)
{
    Rect price_rect = get_text_rect_under_sprite_object(sprite_object);

    char price_str_buff[INT_MAX_DIGITS + 2]; // + 2 for null-terminator and "$"

    snprintf(price_str_buff, sizeof(price_str_buff), "$%d", price);

    update_text_rect_to_center_str(&price_rect, price_str_buff, SCREEN_LEFT);

    tte_printf("#{P:%d,%d; cx:0x%X000}$%d", price_rect.left, price_rect.top, TTE_YELLOW_PB, price);
}

static void erase_price_under_sprite_object(SpriteObject* sprite_object)
{
    Rect price_rect = get_text_rect_under_sprite_object(sprite_object);

    // Add SPRITE_FOCUS_RAISE_PX to cover the focused case
    price_rect.bottom = price_rect.bottom + SPRITE_FOCUS_RAISE_PX;

    tte_erase_rect_wrapper(price_rect);
}

static inline int game_shop_get_rand_available_joker_id(void)
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

static void game_shop_create_items(void)
{
    tte_erase_rect_wrapper(SHOP_PRICES_TEXT_RECT);

    if (no_avail_jokers())
        return;

    list_clear(&_shop_jokers_list);
    _shop_jokers_list = list_create();

    for (int i = 0; i < MAX_SHOP_JOKERS; i++)
    {
        int joker_id = 0;
#ifdef TEST_JOKER_ID0 // Allow defining an ID for a joker to always appear in shop and be tested
        if (is_shop_joker_avail(TEST_JOKER_ID0))
        {
            joker_id = TEST_JOKER_ID0;
        }
        else
#endif
#ifdef TEST_JOKER_ID1
            if (is_shop_joker_avail(TEST_JOKER_ID1))
        {
            joker_id = TEST_JOKER_ID1;
        }
        else
#endif
        {
            joker_id = game_shop_get_rand_available_joker_id();
        }

        // If for some reason only no joker is left, don't make another
        if (joker_id == UNDEFINED)
            break;

        set_shop_joker_avail(joker_id, false);

        JokerObject* joker_object = joker_object_new(joker_new(joker_id));

        joker_object->sprite_object->x = int2fx(120 + i * CARD_SPRITE_SIZE);
        joker_object->sprite_object->y = int2fx(160);
        joker_object->sprite_object->tx = joker_object->sprite_object->x;
        joker_object->sprite_object->ty = int2fx(ITEM_SHOP_Y);

        print_price_under_sprite_object(joker_object->sprite_object, joker_object->joker->value);

        sprite_position(
            joker_object_get_sprite(joker_object),
            fx2int(joker_object->sprite_object->x),
            fx2int(joker_object->sprite_object->y)
        );

        list_push_back(&_shop_jokers_list, joker_object);
    }
}

// Intro sequence (menu and shop icon coming into frame)
static void game_shop_intro()
{
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    if (timer == TM_CREATE_SHOP_ITEMS_WAIT)
    {
        game_shop_create_items();
    }

    if (timer >= TM_SHIFT_SHOP_ICON_WAIT) // Shift the shop icon
    {
        int timer_offset = timer - 6;

        // TODO: Extract to generic function?
        for (int y = 0; y < timer_offset; y++)
        {
            int y_from = 26 + y - timer_offset;
            int y_to = 0 + y;

            Rect from = {0, y_from, 8, y_from};
            BG_POINT to = {0, y_to};

            main_bg_se_copy_rect(from, to);
        }
    }

    if (timer == TM_END_GAME_SHOP_INTRO)
    {
        state_info[game_state].substate = GAME_SHOP_ACTIVE;
        timer = TM_ZERO; // Reset the timer
    }
}

static int jokers_sel_row_get_size(void)
{
    return list_get_len(&_owned_jokers_list);
}

static bool jokers_sel_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    // swap Jokers if the A button is held down and all Jokers are on the same row
    bool swapping =
        key_is_down(SELECT_CARD) && new_selection->y == row_idx && prev_selection->y == row_idx;

    if (prev_selection->y == row_idx)
    {
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(&_owned_jokers_list, prev_selection->x);
        // Don't change focus from current Joker if swapping
        if (joker_object != NULL && !swapping)
        {
            erase_price_under_sprite_object(joker_object->sprite_object);
            sprite_object_set_focus(joker_object->sprite_object, false);
        }
    }

    if (new_selection->y == row_idx)
    {
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(&_owned_jokers_list, new_selection->x);
        if (joker_object != NULL)
        {
            if (!swapping)
            {
                sprite_object_set_focus(joker_object->sprite_object, true);
            }
            // If we land on this row while the A button is being held, we are in swapping mode
            // This means that we need to hide the price, whether we were already
            // on this row or if we come from another
            if (!key_is_down(SELECT_CARD))
            {
                print_price_under_sprite_object(
                    joker_object->sprite_object,
                    joker_get_sell_value(joker_object->joker)
                );
            }
        }
    }

    if (swapping)
    {
        list_swap(
            &_owned_jokers_list,
            (unsigned int)prev_selection->x,
            (unsigned int)new_selection->x
        );
    }

    return true;
}

static inline void joker_start_discard_animation(JokerObject* joker_object)
{
    joker_object->sprite_object->tx = int2fx(JOKER_DISCARD_TARGET.x);
    joker_object->sprite_object->ty = int2fx(JOKER_DISCARD_TARGET.y);
    list_push_back(&_discarded_jokers_list, joker_object);
}

static inline void game_sell_joker(int joker_idx)
{
    if (joker_idx < 0 || joker_idx >= list_get_len(&_owned_jokers_list))
        return;

    JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_owned_jokers_list, joker_idx);
    money += joker_get_sell_value(joker_object->joker);
    display_money();
    erase_price_under_sprite_object(joker_object->sprite_object);

    remove_owned_joker(joker_idx);

    joker_start_discard_animation(joker_object);
}

static void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_owned_jokers_list, selection->x);
    if (joker_object != NULL)
    {
        if (key_hit(SELECT_CARD))
        {
            erase_price_under_sprite_object(joker_object->sprite_object);
        }
        else if (key_released(SELECT_CARD))
        {
            print_price_under_sprite_object(
                joker_object->sprite_object,
                joker_get_sell_value(joker_object->joker)
            );
        }
    }

    if (key_hit(SELL_KEY))
    {
        int sold_joker_idx = selection->x;

        // Move the selection away from the jokers so it doesn't point to an invalid place
        // Do this before selling the joker so valid row sizes are used
        selection_grid_move_selection_vert(selection_grid, SCREEN_DOWN);

        game_sell_joker(sold_joker_idx);
    }
}

// Shop input
static int shop_top_row_get_size(void)
{
    // + 1 to account for next round button
    return list_get_len(&_shop_jokers_list) + 1;
}

static inline void add_to_held_jokers(JokerObject* joker_object)
{
    joker_object->sprite_object->ty = int2fx(HELD_JOKERS_POS.y);
    add_joker(joker_object);
}

static inline void game_shop_buy_joker(int shop_joker_idx)
{
    JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_shop_jokers_list, shop_joker_idx);

    money -= joker_object->joker->value; // Deduct the money spent on the joker
    display_money();                     // Update the money display
    erase_price_under_sprite_object(joker_object->sprite_object);
    sprite_object_set_focus(joker_object->sprite_object, false);
    add_to_held_jokers(joker_object);
    list_remove_at_idx(&_shop_jokers_list, shop_joker_idx); // Remove the joker from the shop
}

static void shop_top_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (!key_hit(SELECT_CARD))
        return;

    if (selection->x == NEXT_ROUND_BTN_SEL_X)
    {
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);

        // Go to next blind selection game state
        state_info[game_state].substate = GAME_SHOP_EXIT; // Go to the outro sequence state
        timer = TM_ZERO;                                  // Reset the timer
        reroll_cost = REROLL_BASE_COST;

        memcpy16(
            &pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[SHOP_PANEL_SHADOW_PID],
            1
        );

        // memcpy16(&pal_bg_mem[16], &pal_bg_mem[6], 1);
        // This changes the color of the button to a dark red.
        // However, it shares a palette with the shop icon, so it will change the color of the shop
        // icon as well. And I don't care enough to fix it right now.
    }
    else
    {
        int shop_joker_idx = selection->x - 1; // - 1 to account for next round button
        JokerObject* joker_object =
            (JokerObject*)list_get_at_idx(&_shop_jokers_list, shop_joker_idx);
        if (joker_object == NULL || list_get_len(&_owned_jokers_list) >= MAX_JOKERS_HELD_SIZE ||
            money < joker_object->joker->value)
        {
            return;
        }

        game_shop_buy_joker(shop_joker_idx);

        // In Balatro the selection actually stays on the purchased joker it's easier to just move
        // it left
        selection_grid_move_selection_horz(selection_grid, -1);
    }
}

static bool shop_top_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    // The selection grid system only guarantees that the new selection is within bounds
    // but not the previous one...
    // This allows using INIT_SEL = {-1, 1} and move to set the initial selection in a hacky way...
    if (prev_selection->y == row_idx && prev_selection->x >= 0 &&
        prev_selection->x < shop_top_row_get_size())
    {
        if (prev_selection->x == NEXT_ROUND_BTN_SEL_X)
        {
            // Remove next round button highlight
            memcpy16(
                &pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID],
                &pal_bg_mem[NEXT_ROUND_BTN_PID],
                1
            );
        }
        else
        {
            int idx = prev_selection->x - 1; // -1 to account for next round button
            JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_shop_jokers_list, idx);
            sprite_object_set_focus(joker_object->sprite_object, false);
        }
    }

    if (new_selection->y == row_idx)
    {
        if (new_selection->x == NEXT_ROUND_BTN_SEL_X)
        {
            // Highlight next round button
            memset16(&pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID], BTN_HIGHLIGHT_COLOR, 1);
        }
        else
        {
            int idx = new_selection->x - 1; // -1 to account for next round button
            JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_shop_jokers_list, idx);
            sprite_object_set_focus(joker_object->sprite_object, true);
        }
    }

    return true;
}

static int shop_reroll_row_get_size()
{
    return 1; // Only the reroll button
}

static bool shop_reroll_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    if (row_idx == prev_selection->y)
    {
        // Remove highlight
        memcpy16(&pal_bg_mem[REROLL_BTN_SELECTED_BORDER_PID], &pal_bg_mem[REROLL_BTN_PID], 1);
    }
    else if (row_idx == new_selection->y)
    {
        memset16(&pal_bg_mem[REROLL_BTN_SELECTED_BORDER_PID], BTN_HIGHLIGHT_COLOR, 1);
    }

    return true;
}

static inline void game_shop_reroll(int* reroll_cost)
{
    money -= *reroll_cost;
    display_money(); // Update the money display

    ListItr itr = list_itr_create(&_shop_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL)
        {
            set_shop_joker_avail(joker_object->joker->id, true);
            joker_object_destroy(&joker_object); // Destroy the joker object if it exists
        }
    }

    list_clear(&_shop_jokers_list);
    _shop_jokers_list = list_create();

    game_shop_create_items();

    itr = list_itr_create(&_shop_jokers_list);

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL)
        {
            // Set the y position to the target position
            joker_object->sprite_object->y = joker_object->sprite_object->ty;

            // Give the joker a little wiggle animation
            joker_object_shake(joker_object, UNDEFINED);
        }
    }

    (*reroll_cost)++;
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}$%d",
        SHOP_REROLL_RECT.left,
        SHOP_REROLL_RECT.top,
        TTE_WHITE_PB,
        *reroll_cost
    );
}

static void shop_reroll_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection)
{
    if (!key_hit(SELECT_CARD))
    {
        return;
    }

    if (money >= reroll_cost)
    {
        // TODO: Add money sound effect
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
        game_shop_reroll(&reroll_cost);
    }
}

// Shop menu input and selection
static void game_shop_process_user_input()
{
    if (timer == TM_SHOP_PRC_INPUT_START)
    {
        // TODO: Move to on_init?
        // The selection grid is initialized outside of bounds and moved
        // to trigger the selection change so the initial selection is visible
        shop_selection_grid.selection = SHOP_INIT_SEL;
        selection_grid_move_selection_horz(&shop_selection_grid, 1);
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}$%d",
            SHOP_REROLL_RECT.left,
            SHOP_REROLL_RECT.top,
            TTE_WHITE_PB,
            reroll_cost
        );
    }

    // Shop input logic
    selection_grid_process_input(&shop_selection_grid);
}

// Outro sequence (menu and shop icon going out of frame)
static void game_shop_outro()
{
    // Shift the shop panel
    main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_DOWN);

    main_bg_se_copy_rect_1_tile_vert(TOP_LEFT_PANEL_ANIM_RECT, SCREEN_UP);

    // TODO: make heads or tails of what's going on here and replace
    // magic numbers.
    if (timer == 1)
    {
        tte_erase_rect_wrapper(SHOP_PRICES_TEXT_RECT); // Erase the shop prices text

        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* joker_object;
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != NULL)
            {
                joker_object->sprite_object->ty = int2fx(160);
            }
        }

        reset_top_left_panel_bottom_row();
    }
    else if (timer == 2)
    {
        int y = 5;
        memset16(&se_mat[MAIN_BG_SBB][y - 1][0], 0x0001, 1);
        memset16(&se_mat[MAIN_BG_SBB][y - 1][1], 0x0002, 7);
        memset16(&se_mat[MAIN_BG_SBB][y - 1][8], SE_HFLIP | 0x0001, 1);
    }

    if (timer >= MENU_POP_OUT_ANIM_FRAMES)
    {
        state_info[game_state].substate = GAME_SHOP_MAX; // Go to the next state
        timer = TM_ZERO;                                 // Reset the timer
    }
}

static inline void game_shop_lights_anim_frame(void)
{
    // Shift palette around the border of the shop icon
    COLOR shifted_palette[4];
    memcpy16(&shifted_palette[0], &pal_bg_mem[SHOP_LIGHTS_2_PID], 1);
    memcpy16(&shifted_palette[1], &pal_bg_mem[SHOP_LIGHTS_3_PID], 1);
    memcpy16(&shifted_palette[2], &pal_bg_mem[SHOP_LIGHTS_4_PID], 1);
    memcpy16(&shifted_palette[3], &pal_bg_mem[SHOP_LIGHTS_1_PID], 1);

    // Circularly shift the palette
    int last = shifted_palette[3];

    for (int i = 3; i > 0; --i)
    {
        shifted_palette[i] = shifted_palette[i - 1];
    }

    shifted_palette[0] = last;

    // Copy the shifted palette to the next 4 slots
    memcpy16(&pal_bg_mem[SHOP_LIGHTS_2_PID], &shifted_palette[0], 1);
    memcpy16(&pal_bg_mem[SHOP_LIGHTS_3_PID], &shifted_palette[1], 1);
    memcpy16(&pal_bg_mem[SHOP_LIGHTS_4_PID], &shifted_palette[2], 1);
    memcpy16(&pal_bg_mem[SHOP_LIGHTS_1_PID], &shifted_palette[3], 1);
}

static void game_shop_on_update()
{
    change_background(BG_SHOP);

    if (!list_is_empty(&_shop_jokers_list))
    {
        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* joker_object;
        while ((joker_object = list_itr_next(&itr)))
        {
            if (joker_object != NULL)
            {
                joker_object_update(joker_object);
            }
        }
    }

    if (timer % 20 == 0)
    {
        game_shop_lights_anim_frame();
    }

    if (state_info[game_state].substate == GAME_SHOP_MAX)
    {
        game_change_state(GAME_STATE_BLIND_SELECT);
        return;
    }

    int substate = state_info[game_state].substate;

    shop_state_actions[substate]();
}

static void game_shop_on_exit()
{
    ListItr itr = list_itr_create(&_shop_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object != NULL)
        {
            // Make the joker available back to shop
            set_shop_joker_avail(joker_object->joker->id, true);
        }
        joker_object_destroy(&joker_object); // Destroy the joker objects
    }

    list_clear(&_shop_jokers_list);

    increment_blind(BLIND_STATE_DEFEATED); // TODO: Move to game_round_end()?
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

static void game_over_anim_frame(void)
{
    main_bg_se_move_rect_1_tile_vert(GAME_OVER_ANIM_RECT, SCREEN_UP);
}

static inline void game_over_process_user_input()
{
    if (key_hit(SELECT_CARD))
    {
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
        game_change_state(GAME_STATE_BLIND_SELECT);
    }
}

static void game_lose_on_update()
{
    if (timer < GAME_OVER_ANIM_FRAMES)
    {
        game_over_anim_frame();
    }
    else if (timer == GAME_OVER_ANIM_FRAMES)
    {
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}GAME OVER",
            GAME_LOSE_MSG_TEXT_RECT.left,
            GAME_LOSE_MSG_TEXT_RECT.top,
            TTE_RED_PB
        );
    }

    game_over_process_user_input();
}

// This function isn't set in stone. This is just a placeholder
// allowing the player to restart the game. Thought it would be nice to have
// util we decide what we want to do after a game over.
static void game_over_on_exit()
{
    while (list_get_len(&_owned_jokers_list) > 0)
    {
        JokerObject* joker_object = list_get_at_idx(&_owned_jokers_list, 0);
        remove_owned_joker(0);
        joker_object_destroy(&joker_object);
    }

    tte_erase_screen();

    // For some reason that I haven't figured out yet,
    // if I don't destroy the blind tokens they won't
    // show up on the next run.
    sprite_destroy(&playing_blind_token);
    sprite_destroy(&round_end_blind_token);
    sprite_destroy(&blind_select_tokens[BLIND_TYPE_SMALL]);
    sprite_destroy(&blind_select_tokens[BLIND_TYPE_BIG]);
    sprite_destroy(&blind_select_tokens[BLIND_TYPE_BOSS]);

    list_clear(&_owned_jokers_list);
    list_clear(&_discarded_jokers_list);
    list_clear(&_expired_jokers_list);
    list_clear(&_shop_jokers_list);

    game_init();

    display_round(game_round);
    display_score(score);
    display_chips();
    display_mult();
    display_hands(hands);
    display_discards(discards);
    display_money();
    // Ante
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d#{cx:0x%X000}/%d",
        ANTE_TEXT_RECT.left,
        ANTE_TEXT_RECT.top,
        TTE_YELLOW_PB,
        ante,
        TTE_WHITE_PB,
        MAX_ANTE
    );

    affine_background_load_palette(affine_background_gfxPal);
}

static void game_win_on_update()
{
    if (timer < GAME_OVER_ANIM_FRAMES)
    {
        game_over_anim_frame();
    }
    else if (timer == GAME_OVER_ANIM_FRAMES)
    {
        tte_printf(
            "#{P:%d,%d; cx:0x%X000}YOU WIN",
            GAME_WIN_MSG_TEXT_RECT.left,
            GAME_WIN_MSG_TEXT_RECT.top,
            TTE_BLUE_PB
        );
    }

    game_over_process_user_input();
}
