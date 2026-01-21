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
#include "game/game_over.h"
#include "game/main_menu.h"
#include "game/palette.h"
#include "game/point.h"
#include "game/rect.h"
#include "game/round.h"
#include "game/round_end.h"
#include "game/shop.h"
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

#define STARTING_ROUND 0
#define STARTING_ANTE  1
#define STARTING_MONEY 4
#define STARTING_SCORE 0

#define EXPIRE_ANIMATION_FRAME_COUNT 3

// Used as a No Operation for game states that have no init and/or exit function.
// ricfehr3 did the work of determining whether a noop or a NULL check was more
// efficient. Well, this is the answer.
// Thanks!
// https://github.com/cellos51/balatro-gba/issues/137#issuecomment-3322485129
static void noop(GameStateCtx* _)
{
}

uint rng_seed = 0;
uint timer = 0;     // This might already exist in libtonc but idk so i'm just making my own
int game_speed = 1; // Can be used to fast-forward the game

StateInfo state_info[] = {
#define DEF_STATE_INFO(stateEnum, init_fn, update_fn, exit_fn) \
    {.on_init = init_fn, .on_update = update_fn, .on_exit = exit_fn, .substate = 0, .ctx = {{0}}},
#include "../include/def_state_info_table.h"
#undef DEF_STATE_INFO
};

// The current game state, this is used to determine what the game is doing at any given time
enum GameState game_state = GAME_STATE_UNDEFINED;

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

Bitset* get_avail_jokers_bitset_ptr(void)
{
    return &_avail_jokers_bitset;
}

// ============================================================================
// Getter and Setter Functions
// ============================================================================

int get_game_speed(void)
{
    return game_speed;
}

int logical_frames_to_real(int logical_frames)
{
    int game_speed = get_game_speed();
    return (logical_frames + game_speed - 1) / game_speed;
}

// Hand and Deck Getters
CardObject** get_hand_array(void)
{
    return hand;
}

int get_hand_top(void)
{
    return hand_top;
}

CardObject** get_played_array(void)
{
    return played;
}

CardObject* get_played_card_at(int idx)
{
    if (idx < 0 || idx >= MAX_SELECTION_SIZE)
    {
        return NULL;
    }
    return played[idx];
}

int get_played_top(void)
{
    return played_top;
}

int get_deck_top(void)
{
    return deck_top;
}

int hand_get_size(void)
{
    return hand_top + 1;
}

int deck_get_size(void)
{
    return deck_top + 1;
}

int deck_get_max_size(int hand_top, int played_top, int deck_top, int discard_top)
{
    // This is the max amount of cards that the player currently has in their possession
    return hand_top + played_top + deck_top + discard_top + 4;
}

// Joker List Getters
List* get_jokers_list(void)
{
    return &_owned_jokers_list;
}

List* get_discarded_jokers_list(void)
{
    return &_discarded_jokers_list;
}

List* get_expired_jokers_list(void)
{
    return &_expired_jokers_list;
}

void clear_joker_lists(void)
{
    list_clear(&_owned_jokers_list);
    list_clear(&_discarded_jokers_list);
    list_clear(&_expired_jokers_list);
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

// Hands and Discards
int get_num_hands_remaining(void)
{
    return hands;
}

int get_discards(void)
{
    return discards;
}

// Round and Ante

int get_ante(void)
{
    return ante;
}

// Blind token management

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

void set_playing_blind_token(Sprite* sprite)
{
    playing_blind_token = sprite;
}

bool playing_blind_token_exists(void)
{
    return playing_blind_token != NULL;
}

void hide_playing_blind_token(void)
{
    if (playing_blind_token_exists())
    {
        obj_hide(playing_blind_token->obj);
    }
}

void unhide_playing_blind_token(void)
{
    if (playing_blind_token_exists())
    {
        obj_unhide(playing_blind_token->obj, 0);
    }
}

Sprite* get_round_end_blind_token(void)
{
    return round_end_blind_token;
}

void set_round_end_blind_token(Sprite* sprite)
{
    round_end_blind_token = sprite;
}

bool round_end_blind_token_exists(void)
{
    return round_end_blind_token != NULL;
}

void hide_round_end_blind_token(void)
{
    if (round_end_blind_token_exists())
    {
        obj_hide(round_end_blind_token->obj);
    }
}

void unhide_round_end_blind_token(void)
{
    if (round_end_blind_token_exists())
    {
        obj_unhide(round_end_blind_token->obj, 0);
    }
}

void destroy_playing_and_round_end_blind_tokens(void)
{
    sprite_destroy(&playing_blind_token);
    sprite_destroy(&round_end_blind_token);
}

void destroy_blind_select_token(enum BlindType blind_type)
{
    if (blind_type < 0 || blind_type >= BLIND_TYPE_MAX)
    {
        return;
    }
    sprite_destroy(&blind_select_tokens[blind_type]);
}

void destroy_all_blind_select_tokens(void)
{
    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        destroy_blind_select_token((enum BlindType)i);
    }
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

// Blind states

enum BlindState* get_blinds_states(void)
{
    return blinds_states;
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

void increment_blind(enum BlindState* states, int* current_blind, enum BlindState increment_reason)
{
    (*current_blind)++;
    if (*current_blind >= BLIND_TYPE_MAX)
    {
        *current_blind = 0;
        states[0] = BLIND_STATE_CURRENT;  // Reset the blinds to the first one
        states[1] = BLIND_STATE_UPCOMING; // Set the next blind to upcoming
        states[2] = BLIND_STATE_UPCOMING; // Set the next blind to upcoming
    }
    else
    {
        states[*current_blind] = BLIND_STATE_CURRENT;
        states[*current_blind - 1] = increment_reason;
    }
}

// ============================================================================
// Deck helpers
// ============================================================================

void played_push(CardObject** played, int* played_top, CardObject* card_object)
{
    if (*played_top >= MAX_SELECTION_SIZE - 1)
        return;
    played[++(*played_top)] = card_object;
}

CardObject* played_pop(CardObject** played, int* played_top)
{
    if (*played_top < 0)
        return NULL;
    return played[(*played_top)--];
}

void deck_push(Card** deck, int* deck_top, Card* card)
{
    if (*deck_top >= MAX_DECK_SIZE - 1)
        return;
    deck[++(*deck_top)] = card;
}

Card* deck_pop(Card** deck, int* deck_top)
{
    if (*deck_top < 0)
        return NULL;
    return deck[(*deck_top)--];
}

void discard_push(Card** discard_pile, int* discard_top, Card* card)
{
    if (*discard_top >= MAX_DECK_SIZE - 1)
        return;
    discard_pile[++(*discard_top)] = card;
}

Card* discard_pop(Card** discard_pile, int* discard_top)
{
    if (*discard_top < 0)
        return NULL;
    return discard_pile[(*discard_top)--];
}

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
// Game Initialization and Core Functions
// ============================================================================

void game_init()
{
    // Initialize all jokers list once
    _owned_jokers_list = list_create();
    _discarded_jokers_list = list_create();
    _expired_jokers_list = list_create();
    _shop_jokers_list = list_create();
    // TODO: Move this to an initialization of the play scoring states

    reset_joker_scored_itr(&_owned_jokers_list);
    reset_shop_jokers();

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

    hide_all_blind_select_tokens();
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
        if (timer % logical_frames_to_real(EXPIRE_ANIMATION_FRAME_COUNT) == 0)
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

GameStateCtx ctx = {0};
enum GameState ctx_game_state = UNDEFINED;

enum GameState get_ctx_game_state(void)
{
    return ctx_game_state;
}

void set_game_state_ctx(enum GameState game_state)
{
    ctx_game_state = game_state;
    switch (game_state)
    {
        case GAME_STATE_MAIN_MENU:
        {
            MainMenuProps props = ctx.main_menu;
            props.timer = timer;
            props.rng_seed = rng_seed;
            ctx.main_menu = props;
            break;
        }
        case GAME_STATE_BLIND_SELECT:
        {
            BlindSelectProps props = ctx.blind_select;
            props.timer = timer;
            props.substate = state_info[game_state].substate;
            props.game_round = game_round;
            props.current_blind = current_blind;
            for (int i = 0; i < BLIND_TYPE_MAX; i++)
                props.blinds_states[i] = blinds_states[i];
            props.ante = ante;
            ctx.blind_select = props;
            break;
        }
        case GAME_STATE_PLAYING:
        {
            RoundProps props = ctx.round;
            props.timer = timer;
            props.substate = state_info[game_state].substate;
            props.money = money;
            props.ante = ante;
            props.current_blind = current_blind;
            props.score = score;
            props.temp_score = temp_score;
            props.lerped_score = lerped_score;
            props.lerped_temp_score = lerped_temp_score;
            props.hands = hands;
            props.discards = discards;
            props.owned_jokers_list = &_owned_jokers_list;
            props.played = played;
            props.played_top = played_top;
            props.hand = hand;
            props.hand_top = hand_top;
            props.deck = deck;
            props.deck_top = deck_top;
            props.discard_pile = discard_pile;
            props.discard_top = discard_top;
            ctx.round = props;
            break;
        }
        case GAME_STATE_ROUND_END:
        {
            RoundEndProps props = ctx.round_end;
            props.timer = timer;
            props.substate = state_info[game_state].substate;
            props.money = money;
            props.hands = hands;
            props.max_hands = max_hands;
            props.discards = discards;
            props.max_discards = max_discards;
            props.ante = ante;
            props.current_blind = current_blind;
            props.score = score;
            ctx.round_end = props;
            break;
        }
        case GAME_STATE_SHOP:
        {
            ShopProps props = ctx.shop;
            props.timer = timer;
            props.substate = state_info[game_state].substate;
            props.money = money;
            for (int i = 0; i < BLIND_TYPE_MAX; i++)
                props.blinds_states[i] = blinds_states[i];
            props.current_blind = current_blind;
            props.shortcut_joker_count = shortcut_joker_count;
            props.four_fingers_joker_count = four_fingers_joker_count;
            props.owned_jokers_list = &_owned_jokers_list;
            props.discarded_jokers_list = &_discarded_jokers_list;
            ctx.shop = props;
            break;
        }
        case GAME_STATE_LOSE:
        case GAME_STATE_WIN:
        {
            GameOverProps props = ctx.game_over;
            props.timer = timer;
            props.game_round = game_round;
            props.score = score;
            props.owned_jokers_list = &_owned_jokers_list;
            ctx.game_over = props;
            break;
        }
        default:
            break;
    }
    state_info[game_state].ctx = ctx;
}

void update_game_state_ctx(enum GameState game_state)
{
    GameStateCtx ctx = state_info[game_state].ctx;
    ctx_game_state = game_state;
    switch (game_state)
    {
        case GAME_STATE_MAIN_MENU:
        {
            MainMenuProps props = ctx.main_menu;
            timer = props.timer;
            rng_seed = props.rng_seed;
            break;
        }
        case GAME_STATE_BLIND_SELECT:
        {
            BlindSelectProps props = ctx.blind_select;
            timer = props.timer;
            state_info[game_state].substate = props.substate;
            game_round = props.game_round;
            current_blind = props.current_blind;
            for (int i = 0; i < BLIND_TYPE_MAX; i++)
                blinds_states[i] = props.blinds_states[i];
            ante = props.ante;
            break;
        }
        case GAME_STATE_PLAYING:
        {
            RoundProps props = ctx.round;
            timer = props.timer;
            state_info[game_state].substate = props.substate;
            money = props.money;
            ante = props.ante;
            current_blind = props.current_blind;
            score = props.score;
            temp_score = props.temp_score;
            lerped_score = props.lerped_score;
            lerped_temp_score = props.lerped_temp_score;
            hands = props.hands;
            discards = props.discards;
            // Joker lists are not updated here since they are pointers to the global lists
            // played array is not updated here since it is a pointer to the global array
            played_top = props.played_top;
            // hand array is not updated here since it is a pointer to the global array
            hand_top = props.hand_top;
            // deck array is not updated here since it is a pointer to the global array
            deck_top = props.deck_top;
            discard_top = props.discard_top;
            break;
        }
        case GAME_STATE_ROUND_END:
        {
            RoundEndProps props = ctx.round_end;
            timer = props.timer;
            state_info[game_state].substate = props.substate;
            money = props.money;
            hands = props.hands;
            max_hands = props.max_hands;
            discards = props.discards;
            max_discards = props.max_discards;
            ante = props.ante;
            current_blind = props.current_blind;
            score = props.score;
            break;
        }
        case GAME_STATE_SHOP:
        {
            ShopProps props = ctx.shop;
            timer = props.timer;
            state_info[game_state].substate = props.substate;
            money = props.money;
            for (int i = 0; i < BLIND_TYPE_MAX; i++)
                blinds_states[i] = props.blinds_states[i];
            current_blind = props.current_blind;
            shortcut_joker_count = props.shortcut_joker_count;
            four_fingers_joker_count = props.four_fingers_joker_count;
            // Joker lists are not updated here since they are pointers to the global lists
            break;
        }
        case GAME_STATE_LOSE:
        case GAME_STATE_WIN:
        {
            GameOverProps props = ctx.game_over;
            timer = props.timer;
            game_round = props.game_round;
            score = props.score;
            // Joker lists are not updated here since they are pointers to the global lists
            break;
        }
        default:
            break;
    }
}

void game_update()
{
    timer++;

    jokers_update_loop();

    set_game_state_ctx(game_state);
    state_info[game_state].on_update(&state_info[game_state].ctx);
    update_game_state_ctx(game_state);
}

void game_change_state(enum GameState new_game_state)
{
    timer = TM_ZERO; // Reset the timer

    if (game_state >= 0 && game_state < GAME_STATE_MAX)
    {
        state_info[game_state].substate = 0;
        state_info[game_state].on_exit(&state_info[game_state].ctx);
    }

    if (new_game_state >= 0 && new_game_state < GAME_STATE_MAX)
    {
        // Sync global state to the new state's context before on_init
        // so that on_init can read any updates made in the previous state's on_exit
        set_game_state_ctx(new_game_state);
        state_info[new_game_state].on_init(&state_info[new_game_state].ctx);

        game_state = new_game_state;
    }
}

enum GameState get_game_state(void)
{
    return game_state;
}

static inline void set_seed(int seed)
{
    rng_seed = seed;
    srand(rng_seed);
}

void game_start(void)
{
    game_main_menu_cleanup();

    set_seed(rng_seed);
    // set_seed(9); // 9 is a full house

    affine_background_change_background(AFFINE_BG_GAME);

    hands = max_hands;
    discards = max_discards;

    // Fill the deck with all the cards. Later on this can be replaced with a more dynamic
    // system that allows for different decks and card types.
    for (int suit = 0; suit < NUM_SUITS; suit++)
    {
        for (int rank = 0; rank < NUM_RANKS; rank++)
        {
            Card* card = card_new(suit, rank);
            deck_push(deck, &deck_top, card);
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
        deck_get_max_size(hand_top, played_top, deck_top, discard_top)
    );

    display_round(game_round); // Set the round display
    display_score(score);      // Set the score display

    display_chips(); // Set the chips display
    display_mult();  // Set the multiplier display

    display_hands();    // Hand
    display_discards(); // Discard

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
