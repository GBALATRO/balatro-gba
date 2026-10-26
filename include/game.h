/**
 * @file game.h
 *
 * @brief Functions relative to the global management of the Game's state and common data
 */

#ifndef GAME_H
#define GAME_H

#include "game/common_ui.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "item.h"

#include <tonc.h>

#define MAX_DECK_SIZE        52
#define MAX_JOKERS_HELD_SIZE 5 // This doesn't account for negatives right now.
#define MAX_SHOP_ITEMS       2 // TODO: Make this dynamic
#define FRAMES(x)            (((x) + (g_game_vars.game_speed) - 1) / (g_game_vars.game_speed))

// TODO: Can make these dynamic to support interest-related jokers and vouchers
#define MAX_INTEREST   5
#define INTEREST_PER_5 1

// Input bindings

#define KEY_CONFIRM          KEY_A // Not using tonc's KEY_ACCEPT because it includes SELECT
#define KEY_CARD_SELECT      KEY_CONFIRM
#define KEY_CARDS_DESELECT   KEY_CANCEL // From tonc
#define KEY_PEEK_DECK        KEY_L      // TODO: Not implemented, remove?
#define KEY_PAUSE_GAME       KEY_START  // TODO: Not implemented, remove?
#define KEY_SHOW_DESCRIPTION KEY_B
#define KEY_SELL             KEY_L
#define KEY_TAB_LEFT         KEY_L
#define KEY_TAB_RIGHT        KEY_R

// Using these buttons for play and discard hand to match the position of the on-screen buttons
#define KEY_PLAY_HAND KEY_L
// Same value as KEY_SELL - okay because they are activated on different rows

#define KEY_DISCARD_HAND KEY_R

struct List;
typedef struct List List;

// Utility functions for other files
typedef struct CardObject CardObject;
typedef struct Card Card;
typedef struct JokerObject JokerObject;

// clang-format off
/**
 * @brief Enum value names defined in ../include/def_state_info_table.h
 *
 * These are the game main state machine's states, which is organized as shown below:
 * ```
 *                   ╔══════════════╗
 *                   ║ OPTIONS_MENU ║
 *                   ╚════╤═════════╝
 *                        │   🡹
 *                        🡻   │
 * ╔═══════════════╗  ╔═══════╧═══╗
 * ║ SPLASH_SCREEN ╟─🡺║ MAIN_MENU ║🡸────────┐
 * ╚═══════════════╝  ╚═════╤═════╝         │
 *                          │               │
 *                          🡻               │ No
 *    ╔════════════╗  ╔═══════════╗    Yes🯐─┴──🯓
 *    ║ GAME_START ║🡸─╢ RUN_SETUP ║🡸─────🯝RETRY?🯟
 *    ╚═════╤══════╝  ╚═══════════╝       🯒────🯑
 *          │                               🡹
 *          │                          ╔══════════╗              Setup/Menuing
 * ╴╴╴╴╴╴╴╴╴│╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴║ LOSE/WIN ║╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴╴
 *          │                          ╚══════════╝               Run/Gameplay
 *          🡻                               │
 *   ╔══════════════╗    ╔═══════╗    ╔═════╧═════╗    ╔══════╗
 *   ║ BLIND_SELECT ╟───🡺║ ROUND ╟───🡺║ ROUND_END ╟───🡺║ SHOP ║
 *   ╚══════════════╝    ╚═══════╝    ╚═══════════╝    ╚══╤═══╝
 *          🡹                                             │
 *          └─────────────────────────────────────────────┘
 * ```
 */
// clang-format on
enum GameState
{
#define DEF_STATE_INFO(stateEnum, on_init, on_update, on_exit) stateEnum,
#include "def_state_info_table.h"
#undef DEF_STATE_INFO
    GAME_STATE_MAX,
    GAME_STATE_UNDEFINED
};

/**
 * @brief Initialize all data structures, assuming they are not present and need to be created from
 * scratch
 */
void game_init(void);

/**
 * @brief Called when exiting the Game Over screen (both win or lose) to reset game variables
 *         and start a fresh new run.
 *
 * WARNING: This function is currently only meant to be called from the "GAME_OVER" state
 * and shouldn't be called from other states, otherwise some data such as shop jokers
 * may not be properly reset.
 */
void game_reset(void);

/**
 * @brief Called from the program's main loop to advance the state of the game, mainly by processing
 *         all the registered State Machines
 */
void game_update(void);

/**
 * @brief Change the game's main state machine state for the specified state.
 * @param new_game_state the new valid GameState
 */
void game_change_state(enum GameState new_game_state);

/**
 * @brief Get the current state of teh game's main state machine
 *
 * @return GameState state value
 *
 * @sa GameState
 */
enum GameState game_get_state(void);

// TODO
/**
 * @brief Callback for the `GAME_STATE_GAME_START` state. Sets up the Deck and displays various
 *         initial values.
 *
 * This was extracted in its own transition state to avoid interfering with `GAME_STATE_RUN_SETUP`'s
 * @p _on_exit and `GAME_STATE_BLIND_SELECT`'s @p _on_init callbacks.
 *
 * TODO: refactor to eliminate the `GAME_STATE_GAME_START` state?
 */
void game_start(void);

/**
 * @brief Get the actual, current size of the Deck of Cards.
 *
 * Is exactly one more than the Deck top.
 *
 * @return int
 *
 * @sa get_deck_top
 */
int deck_get_size(void);

/**
 * @brief Get the index of the Card at the top of the Deck (the next one to be drawn).
 *
 * Is exactly one less than the Deck current size
 *
 * @return int
 *
 * @sa deck_get_size
 */
int get_deck_top(void);

/**
 * @brief Add a new Card at the top of the Deck.
 *
 * @param card pointer to the Card to push to the Deck
 */
void deck_push(Card* card);

/**
 * @brief Draw the Card at the top of the Deck.
 *
 * @return pointer to the newly drawn Card
 */
Card* deck_pop(void);

/**
 * @brief Shuffle the Cards inside the Deck
 */
void deck_shuffle(void);

/**
 * @brief Get the number of Hands that can still be discarded for the current Round
 *
 * @return int
 */
int get_num_discards_remaining(void);

/**
 * @brief Get the number of Hands that can still be played for the current Round
 *
 * @return int
 */
int get_num_hands_remaining(void);

/**
 * @brief Display the number of cards remaining in the Deck vs. the total number of cards in our
 * possession, in the bottom-left corner of the screen.
 */
void display_deck_size_max(void);

/**
 * @brief Display the base Chips for the currently selected Hand.
 */
void display_chips(void);

/**
 * @brief Display the base Mult for the currently selected Hand.
 */
void display_mult(void);

/**
 * @brief Display the amount of money currently in our possession.
 */
void display_money(void);

/**
 * @brief Display the current Ante
 */
void display_ante(void);

void display_round(void);
void display_hands(void);
void display_discards(void);
void display_temp_score(u32 value);
void erase_temp_score(void);
void display_score(u32 value);

// Item specific functions

/**
 * @brief Determines if a certain Joker ID is among the Jokers currently owned
 * @param joker_id the ID we are looking for
 *
 * @return true if a Joker in the player's possession is of the given ID, false otherwise
 */
bool is_joker_owned(int joker_id);

/**
 * @brief Determines if the given Joker can be acquired and added to the List of owned Jokers
 * @param item pointer to a JokerObject, cast as an Item pointer to matche @ref
 *             ItemFuncs.can_acquire()
 *
 * @return true if the given Joker can be acquired, false otherwise
 */
bool joker_object_can_acquire(Item* item);

/**
 * @brief Add a Joker to the List of owned Jokers
 * @param joker_object pointer to a Joker to acquire
 */
void add_joker(JokerObject* joker_object);

/**
 * @brief Remove a Joker located at a certain index in the List of owned Jokers
 * @param owned_joker_idx index of the Joker to remove
 */
void remove_owned_joker(int owned_joker_idx);

/**
 * @brief Starts the discard animation for the item, destroying it when it ends
 * @param item The discarded item
 */
void item_start_discard_animation(Item* item);

/**
 * @brief Get a pointer to the List of owned jokers
 *
 * @return List*
 */
List* get_jokers_list(void);

/**
 * @brief Get a pointer to the List of expired jokers
 *
 * @return List*
 */
List* get_expired_jokers_list(void);

/**
 * @brief Returns wether a given Card is a face card or not, taking Pareidolia into account
 * @param card pointer to a Card
 *
 * @return true if the Card is a face card, false otherwise
 */
bool card_is_face(Card* card);

/**
 * @brief Determines the Shortcut Joker is owned, applying its effect
 *
 * @return true if at least one copy of Shortcut is owned, false otherwise
 */
bool is_shortcut_joker_active(void);

/**
 * @brief Determines the Four Fingers Joker is owned, applying its effect
 *
 * @return true if at least one copy of Four Fingers is owned, false otherwise
 */
int get_straight_and_flush_size(void);

#endif // GAME_H
