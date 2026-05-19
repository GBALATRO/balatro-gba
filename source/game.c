#include "game.h"

#include "affine_background.h"
#include "affine_background_gfx.h"
#include "audio_utils.h"
#include "background_gfx.h"
#include "background_main_menu_gfx.h"
#include "button.h"
#include "card.h"
#include "game/blind_select.h"
#include "game/common_ui.h"
#include "game/game_over.h"
#include "game/joker_row.h"
#include "game/main_menu.h"
#include "game/options_menu.h"
#include "game/round_end.h"
#include "game/shop.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "hand_analysis.h"
#include "joker.h"
#include "layout.h"
#include "list.h"
#include "save.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "splash_screen.h"
#include "sprite.h"
#include "timer.h"
#include "tonc_memdef.h"
#include "util.h"

#include <maxmod.h>
#include <stdint.h>
#include <stdlib.h>

#define STRAIGHT_AND_FLUSH_SIZE_FOUR_FINGERS 4
#define STRAIGHT_AND_FLUSH_SIZE_DEFAULT      5

// Pixel sizes
#define SCORED_CARD_TEXT_Y 48

// SE sizes

#define PITCH_STEP_DISCARD_SFX   (-64)
#define PITCH_STEP_DRAW_SFX      24
#define PITCH_STEP_UNDISCARD_SFX 2 * PITCH_STEP_DRAW_SFX

#define STARTING_ROUND 0
#define STARTING_ANTE  1
#define STARTING_MONEY 4
#define STARTING_SCORE 0

#define CARD_FOCUSED_UNSEL_Y 10
#define CARD_UNFOCUSED_SEL_Y 15
#define CARD_FOCUSED_SEL_Y   20

// TODO: Rename "PID" to "PAL_IDX"
// Palette IDs

#define BLIND_BG_SHADOW_PAL_IDX     5
#define BLIND_BG_SECONDARY_PAL_IDX  18
#define BLIND_BG_PRIMARY_PAL_IDX    19
#define REWARD_PANEL_BORDER_PAL_IDX 19

#define PLAY_HAND_BTN_PAL_IDX           6
#define PLAY_HAND_BTN_BORDER_PAL_IDX    7
#define DISCARD_BTN_PAL_IDX             13
#define DISCARD_BTN_BORDER_PAL_IDX      8
#define SORT_BTNS_PAL_IDX               9
#define SORT_BY_RANK_BTN_BORDER_PAL_IDX 22
#define SORT_BY_SUIT_BTN_BORDER_PAL_IDX 23

// Naming the stage where cards return from the discard pile to the deck "undiscard"

/* This needs to stay a power of 2 and small enough
 * for the lerping to be done before the next hand is drawn.
 */
#define NUM_SCORE_LERP_STEPS   16
#define TM_SCORE_LERP_INTERVAL 2

#define GAME_PLAYING_HAND_SEL_Y      1
#define GAME_PLAYING_BUTTONS_SEL_Y   2
#define GAME_PLAYING_NUM_BOTTOM_BTNS 2

#define EXPIRE_ANIMATION_FRAME_COUNT 3

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
static void game_round_on_init(void);
static void game_playing_on_update(void);

static void sort_cards(void);
static void display_temp_score(u32 value);
static void check_flaming_score(void);
static void set_hand(void);
static int deck_get_size(void);
static int deck_get_max_size(void);
static bool check_and_score_joker_for_event(
    ListItr* starting_joker_itr,
    CardObject* card_object,
    enum JokerEvent joker_event
);

static void game_playing_discard_on_pressed(void);
static void game_playing_execute_discard(void);
static void game_playing_play_hand_on_pressed(void);
static void game_playing_execute_play_hand(void);
static void game_playing_sort_by_rank_on_pressed(void);
static void game_playing_sort_by_suit_on_pressed(void);

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

static int game_playing_hand_row_get_size(void);

static int hand_sel_idx_to_card_idx(int selection_index);
static void hand_select_card(int index);
static void hand_toggle_sort(void);
static void hand_change_sort(bool to_sort_by_suit);
static void hand_deselect_all_cards(void);
static bool can_play_hand(void);
static bool can_discard_hand(void);

// Consts

// clang-format off
// Rects                                       left     top     right   bottom

// The rect for popping menu animations (round end, shop, blinds) 
// - extends beyond the visible screen to the end of the screenblock
// It includes both the target and source position rects. 
// This is because when popping, the target position is blank so we just animate 
// the whole rect so we don't have to track its position

static const Rect HAND_BG_RECT_SELECTING    = {9,       11,     24,     17 };

/* Contains the shop icon/current blind etc. 
 * The difference between TOP_LEFT_PANEL_ANIM_RECT and TOP_LEFT_PANEL_RECT 
 * is due to an overlap between the bottom of the top left panel
 * and the top of the score panel in the tiles connecting them.
 * TOP_LEFT_PANEL_ANIM_RECT should be used for animations, 
 * TOP_LEFT_PANEL_RECT for copies etc. but mind the overlap
 */
static const BG_POINT TOP_LEFT_BLIND_TITLE_POINT = {0,  21, };
static const Rect BIG_BLIND_TITLE_SRC_RECT  = {0,       26,     8,      26 };
static const Rect BOSS_BLIND_TITLE_SRC_RECT = {0,       27,     8,      27 };

// Flaming score animation frames
#define SCORE_FLAMES_ANIM_FREQ  5 // animation will run at 12FPS
#define NUM_SCORE_FLAMES_FRAMES 8 // Chips and Mult flame frames are next to one another
#define SCORE_FLAME_FRAME_WIDTH 3 // so we only need to offset to get the next ones
static const Rect SCORE_FLAME_RESET         = {26,      20,      28,     20};
static const Rect SCORE_FLAME_FRAMES_START  = {26,      21,      28,     21};
static const BG_POINT SCORE_FLAME_CHIPS_POS = {1,       9};
static const BG_POINT SCORE_FLAME_MULT_POS  = {5,       9};

// Rects for TTE (in pixels)
static const Rect HAND_SIZE_RECT_SELECT     = {128,     128,    152,    136 };
static const Rect HAND_SIZE_RECT_PLAYING    = {128,     152,    152,    160 };
static const Rect HAND_TYPE_RECT            = {8,       64,     64,     72  };
// Score displayed in the same place as the hand type
static const Rect TEMP_SCORE_RECT           = {8,       64,     64,     72  }; 
static const Rect SCORE_RECT                = {24,      48,     64,     56  };

static const Rect PLAYED_CARDS_SCORES_RECT  = {72,      48,     240,    56  };
static const Rect HELD_CARDS_SCORES_RECT    = {72,      108,    240,    116 };
static const Rect MONEY_TEXT_RECT           = {8,       120,    64,     128 };
static const Rect CHIPS_TEXT_RECT           = {8,       80,     32,     88  };
static const Rect MULT_TEXT_RECT            = {40,      80,     64,     88  };

// Rects with UNDEFINED are only used in tte_printf, they need to be fully defined
// to be used with tte_erase_rect_wrapper()
static const Rect HANDS_TEXT_RECT           = {16,      104,    UNDEFINED, UNDEFINED };
static const Rect DISCARDS_TEXT_RECT        = {48,      104,    UNDEFINED, UNDEFINED };
static const Rect DECK_SIZE_RECT            = {200,     152,    240,       160       };
static const Rect ROUND_TEXT_RECT           = {48,      144,    UNDEFINED, UNDEFINED };
static const Rect ANTE_TEXT_RECT            = {8,       144,    UNDEFINED, UNDEFINED };

static const BG_POINT CARD_DRAW_POS         = {208,     110};
static const BG_POINT CARD_DISCARD_PNT      = {240,     70};
static const BG_POINT HAND_START_POS        = {120,     90};
static const BG_POINT HAND_PLAY_POS         = {120,     70};
// clang-format on

// NOTE: This is going to be removed in favor of the background
// variable and handling in common.c once the related refactor is finished
static enum BackgroundId background_legacy = BG_NONE;

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

// Array of buttons by horizontal selection index (x)
Button game_playing_buttons[] = {
    {PLAY_HAND_BTN_BORDER_PAL_IDX,    PLAY_HAND_BTN_PAL_IDX, game_playing_play_hand_on_pressed,    can_play_hand   },
    {SORT_BY_RANK_BTN_BORDER_PAL_IDX, SORT_BTNS_PAL_IDX,     game_playing_sort_by_rank_on_pressed, NULL            },
    {SORT_BY_SUIT_BTN_BORDER_PAL_IDX, SORT_BTNS_PAL_IDX,     game_playing_sort_by_suit_on_pressed, NULL            },
    {DISCARD_BTN_BORDER_PAL_IDX,      DISCARD_BTN_PAL_IDX,   game_playing_discard_on_pressed,      can_discard_hand},
};

// This is a stupid way to do this but I don't care
static const int HAND_SPACING_LUT[MAX_HAND_SIZE] =
    {28, 28, 28, 28, 27, 21, 18, 15, 13, 12, 10, 9, 9, 8, 8, 7};

static const HandValues hand_base_values[] = {
    {.chips = 0,   .mult = 0,  .display_name = NULL     }, // NONE
    {.chips = 5,   .mult = 1,  .display_name = "Hi-Card"}, // HIGH_CARD
    {.chips = 10,  .mult = 2,  .display_name = "Pair"   }, // PAIR
    {.chips = 20,  .mult = 2,  .display_name = "2 Pair" }, // TWO_PAIR
    {.chips = 30,  .mult = 3,  .display_name = "3 OAK"  }, // THREE_OF_A_KIND
    {.chips = 30,  .mult = 4,  .display_name = "Strt"   }, // STRAIGHT
    {.chips = 35,  .mult = 4,  .display_name = "Flush"  }, // FLUSH
    {.chips = 40,  .mult = 4,  .display_name = "Full H" }, // FULL_HOUSE
    {.chips = 60,  .mult = 7,  .display_name = "4 OAK"  }, // FOUR_OF_A_KIND
    {.chips = 100, .mult = 8,  .display_name = "Strt F" }, // STRAIGHT_FLUSH
    {.chips = 100, .mult = 8,  .display_name = "Royal F"}, // ROYAL_FLUSH
    {.chips = 120, .mult = 12, .display_name = "5 OAK"  }, // FIVE_OF_A_KIND
    {.chips = 140, .mult = 14, .display_name = "Flush H"}, // FLUSH_HOUSE
    {.chips = 160, .mult = 16, .display_name = "Flush 5"}  // FLUSH_FIVE
};

// The current game state, this is used to determine what the game is doing at any given time
static enum GameState game_state = GAME_STATE_UNDEFINED;
static enum HandState hand_state = HAND_DRAW;
static enum PlayState play_state = PLAY_STARTING;

static enum HandType hand_type = NONE;
static ContainedHandTypes _contained_hands = {0};

// Initialization of the global vars
// clang-format off
GameVariables g_game_vars = {
    .timer = 0, .rng_seed = 0, .rng_step = 0,

    .round = 0, .ante = 0, .money = 0,

    .current_blind = BLIND_TYPE_SMALL,
    .next_boss_blind = BLIND_TYPE_BIG,
    .blinds_states =
    {
        BLIND_STATE_CURRENT,
        BLIND_STATE_UPCOMING,
        BLIND_STATE_UPCOMING
    },

    .hands = 0,
    .discards = 0,
    .score = 0,

    .playing_blind_token = NULL,
    .round_end_blind_token = NULL,

    .game_speed = DEFAULT_GAME_SPEED,
    .high_contrast = DEFAULT_HIGH_CONTRAST,
    .more_readable = DEFAULT_MORE_READABLE,
    .music_volume = DEFAULT_MUSIC_VOLUME,
    .sound_volume = DEFAULT_SOUND_VOLUME,
};
// clang-format on

typedef struct
{
    int substate;
    GameStateCallback on_init;
    GameStateCallback on_update;
    GameStateCallback on_exit;
} StateInfo;

StateInfo state_info[] = {
#define DEF_STATE_INFO(stateEnum, init_fn, update_fn, exit_fn) \
    {.on_init = init_fn, .on_update = update_fn, .on_exit = exit_fn, .substate = 0},
#include "../include/def_state_info_table.h"
#undef DEF_STATE_INFO
};

// The sprite that displays the blind when in "GAME_PLAYING/GAME_ROUND_END" state

// The sprite that displays the blind when in "GAME_ROUND_END" state

// Red deck default (can later be moved to a deck.h file or something)
// Set in game_init and game_round_init

static u32 temp_score = 0; // This is the score that shows in the same spot as the hand type.
static bool score_flames_active = false;
static FIXED lerped_score = 0;
static FIXED lerped_temp_score = 0;

static u32 chips = 0;
static u32 mult = 0;
static bool retrigger = false;

static int hand_size = 8; // Default hand size is 8
static int cards_drawn = 0;
static int hand_selections = 0;

// Keeping track of cards scored
static int scored_card_index = 0;

// discarded cards specific
static bool sound_played = false;
static bool discarded_card = false;

// Keeping track of what Jokers are scored at each step
static ListItr _joker_scored_itr;
static ListItr _joker_card_scored_end_itr;
static ListItr _joker_round_end_itr;

static bool sort_by_suit = false;

static List _owned_jokers_list;
static List _discarded_jokers_list;
static List _expired_jokers_list;

BITSET_DEFINE(_avail_jokers_bitset, MAX_DEFINABLE_JOKERS)
static List _shop_jokers_list;

// Stacks
static CardObject* played[MAX_SELECTION_SIZE] = {NULL};
static int played_top = -1;

static CardObject* hand[MAX_HAND_SIZE] = {NULL};
static int hand_top = -1;

static Card* deck[MAX_DECK_SIZE] = {NULL};
static int deck_top = -1;

static Card* discard_pile[MAX_DECK_SIZE] = {NULL};
static int discard_top = -1;

// Joker Special Variables
static int shortcut_joker_count = 0;

static int four_fingers_joker_count = 0;

GBAL_UNUSED
static inline bool is_shop_joker_avail(int joker_id)
{
    return bitset_get_idx(&_avail_jokers_bitset, joker_id);
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

static inline void played_push(CardObject* card_object)
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

static inline void deck_push(Card* card)
{
    if (deck_top >= MAX_DECK_SIZE - 1)
        return;
    deck[++deck_top] = card;
}

static inline Card* deck_pop()
{
    if (deck_top < 0)
        return NULL;
    return deck[deck_top--];
}

static inline void discard_push(Card* card)
{
    if (discard_top >= MAX_DECK_SIZE - 1)
        return;
    discard_pile[++discard_top] = card;
}

static inline Card* discard_pop()
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

    g_game_vars.hands = MAX_HANDS;
    g_game_vars.discards = MAX_DISCARDS;
    g_game_vars.timer = TM_ZERO;
    g_game_vars.current_blind = BLIND_TYPE_SMALL;
    g_game_vars.blinds_states[0] = BLIND_STATE_CURRENT;
    g_game_vars.blinds_states[1] = BLIND_STATE_UPCOMING;
    g_game_vars.blinds_states[2] = BLIND_STATE_UPCOMING;
    g_game_vars.ante = STARTING_ANTE;
    g_game_vars.money = STARTING_MONEY;
    g_game_vars.score = STARTING_SCORE;
    g_game_vars.round = 0;

    // Initialize/reset unbeaten Boss/Showdown Blinds so they are all available
    init_unbeaten_blinds_list(false);
    init_unbeaten_blinds_list(true);
}

void game_reset()
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
    sprite_destroy(&g_game_vars.playing_blind_token);
    sprite_destroy(&g_game_vars.round_end_blind_token);

    list_clear(&_owned_jokers_list);
    list_clear(&_discarded_jokers_list);
    list_clear(&_expired_jokers_list);
    list_clear(&_shop_jokers_list);

    game_init();

    display_round();
    display_score(g_game_vars.score);
    display_chips();
    display_mult();
    display_hands();
    display_discards();
    display_money();
    // Ante
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%ld#{cx:0x%X000}/%d",
        ANTE_TEXT_RECT.left,
        ANTE_TEXT_RECT.top,
        TTE_YELLOW_PB,
        g_game_vars.ante,
        TTE_WHITE_PB,
        MAX_ANTE
    );

    affine_background_load_palette(affine_background_gfxPal);
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
    static const int spacing_lut[MAX_JOKERS_HELD_SIZE][MAX_JOKERS_HELD_SIZE] = {
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
        if (g_game_vars.timer % FRAMES(EXPIRE_ANIMATION_FRAME_COUNT) == 0)
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
    g_game_vars.timer++;

    jokers_update_loop();

    state_info[game_state].on_update();
}

void game_change_state(enum GameState new_game_state)
{
    g_game_vars.timer = TM_ZERO; // Reset the timer

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

u32 get_rand()
{
    g_game_vars.rng_step++;
    return rand();
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

List* get_jokers_list(void)
{
    return &_owned_jokers_list;
}

List* get_expired_jokers_list(void)
{
    return &_expired_jokers_list;
}

List* get_discarded_jokers_list(void)
{
    return &_discarded_jokers_list;
}

List* get_shop_jokers_list(void)
{
    return &_shop_jokers_list;
}

Bitset* get_avail_jokers_bitset(void)
{
    return &_avail_jokers_bitset;
}

void set_shop_joker_avail(int joker_id, bool avail)
{
    bitset_set_idx(&_avail_jokers_bitset, joker_id, avail);
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

void add_joker(JokerObject* joker_object)
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

int get_deck_top(void)
{
    return deck_top;
}

int get_num_discards_remaining(void)
{
    return g_game_vars.discards;
}

int get_num_hands_remaining(void)
{
    return g_game_vars.hands;
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

void set_retrigger(bool new_retrigger)
{
    retrigger = new_retrigger;
}

void display_money()
{
    Rect money_text_rect = MONEY_TEXT_RECT;
    tte_erase_rect_wrapper(MONEY_TEXT_RECT);

    char money_str_buff[INT_MAX_DIGITS + 2]; // + 2 for null terminator and "$" sign
    snprintf(money_str_buff, sizeof(money_str_buff), "$%ld", g_game_vars.money);

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
static inline void swap_cards_in_hand(int idx_a, int idx_b)
{
    CardObject* temp = hand[idx_a];
    hand[idx_a] = hand[idx_b];
    hand[idx_b] = temp;
}

static inline void sort_hand_by_suit(void)
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

static inline void sort_hand_by_rank(void)
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

static inline bool shift_null_card_to_end(int null_card_idx)
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

static ContainedHandTypes compute_contained_hand_types(void)
{
    ContainedHandTypes hand_types = {0};

    // Idk if this is how Balatro does it but this is how I'm doing it
    if (hand_selections == 0 || hand_state == HAND_DISCARD)
    {
        return hand_types;
    }

    hand_types.HIGH_CARD = 1;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_hand_distribution(ranks, suits);

    // The following can be optimized better but not sure how much it matters
    u8 n_of_a_kind = hand_contains_n_of_a_kind(ranks);

    // Pair and 2 Pair
    if (n_of_a_kind >= 2)
    {
        hand_types.PAIR = 1;

        if (hand_contains_two_pair(ranks))
        {
            hand_types.TWO_PAIR = 1;
        }
    }

    // 3 OAK
    if (n_of_a_kind >= 3)
    {
        hand_types.THREE_OF_A_KIND = 1;
    }

    // Straight
    if (hand_contains_straight(ranks))
    {
        hand_types.STRAIGHT = 1;
    }

    // Flush
    if (hand_contains_flush(suits))
    {
        hand_types.FLUSH = 1;
    }

    // Full House
    if (n_of_a_kind >= 3 && hand_contains_full_house(ranks))
    {
        hand_types.FULL_HOUSE = 1;
    }

    // 4 OAK
    if (n_of_a_kind >= 4)
    {
        hand_types.FOUR_OF_A_KIND = 1;
    }

    // Straight Flush
    if (hand_types.STRAIGHT && hand_types.FLUSH)
    {
        hand_types.STRAIGHT_FLUSH = 1;
    }

    // Royal Flush
    if (hand_types.STRAIGHT_FLUSH)
    {
        if (ranks[TEN] && ranks[JACK] && ranks[QUEEN] && ranks[KING] && ranks[ACE])
        {
            hand_types.ROYAL_FLUSH = 1;
        }
    }

    // 5 OAK
    if (n_of_a_kind >= 5)
    {
        hand_types.FIVE_OF_A_KIND = 1;
    }

    // Flush House and Five
    if (hand_types.FLUSH)
    {
        if (hand_types.FULL_HOUSE)
        {
            hand_types.FLUSH_HOUSE = 1;
        }

        if (hand_types.FIVE_OF_A_KIND)
        {
            hand_types.FLUSH_FIVE = 1;
        }
    }

    return hand_types;
}

ContainedHandTypes* get_contained_hands(void)
{
    return &_contained_hands;
}

enum HandType compute_hand_type(struct ContainedHandTypes contained_types)
{
    enum HandType ret;

    // test each pit see if it's set to 1, and return the first one
    for (ret = FLUSH_FIVE; ret > NONE; ret--)
    {
        // Shift the bit we want to check to the front and mask it with 1 to keep only that
        // Since the ContainedHandTypes is ordered the same way as the HandType enum, we
        // can shift right by ret-1 to have the bit we want at the front
        if ((contained_types.value >> (ret - 1)) & 0x1)
        {
            break;
        }
    }

    // If we broke early, ret contains the value of the HandType enum corresponding to
    // the position of the highest bit set to 1 in contained_types.value, which is the
    // most powerful poker hand contained in the current Hand
    // If not, then it contains NONE, which is what we're supposed to return when there
    // are no Hands contained in what we played
    return ret;
}

enum HandType* get_hand_type(void)
{
    return &hand_type;
}

// Returns true if the card is *considered* a face card
bool card_is_face(Card* card)
{
    // Card is a face card, or Pareidolia is present
    return (
        card->rank == JACK || card->rank == QUEEN || card->rank == KING ||
        is_joker_owned(PAREIDOLIA_JOKER_ID)
    );
}

/* Copies the appropriate item into the top left panel (blind/shop icon)
 * from where it was put outside the screenview
 */
static void bg_copy_current_item_to_top_left_panel(void)
{
    main_bg_se_copy_rect(TOP_LEFT_ITEM_SRC_RECT, TOP_LEFT_PANEL_POINT);
}

void change_background_legacy(enum BackgroundId id)
{
    if (background_legacy == id)
    {
        return;
    }
    else if (id == BG_CARD_SELECTING)
    {
        tte_erase_rect_wrapper(HAND_SIZE_RECT_PLAYING);
        REG_WIN0V = (REG_WIN0V << 8) | 0x80; // Set window 0 top to 128

        if (background_legacy == BG_CARD_PLAYING)
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
            else if (g_game_vars.current_blind > BLIND_TYPE_BIG)
            {
                main_bg_se_copy_rect(BOSS_BLIND_TITLE_SRC_RECT, TOP_LEFT_BLIND_TITLE_POINT);
                affine_background_set_color(
                    blind_get_color(g_game_vars.current_blind, BLIND_SHADOW_COLOR_INDEX)
                );
            }

            bg_copy_current_item_to_top_left_panel();

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

            for (int i = 0; i < NUM_ELEM_IN_ARR(game_playing_buttons); i++)
            {
                button_set_highlight(&game_playing_buttons[i], false);
            }
        }
    }
    else if (id == BG_CARD_PLAYING)
    {
        if (background_legacy != BG_CARD_SELECTING)
        {
            change_background(BG_CARD_SELECTING, false);
            background_legacy = BG_CARD_PLAYING;
        }

        REG_WIN0V = (REG_WIN0V << 8) | 0xA0; // Set window 0 bottom to 160
        toggle_windows(true, true);

        for (int i = 0; i <= 2; i++)
        {
            main_bg_se_move_rect_1_tile_vert(HAND_BG_RECT_SELECTING, SCREEN_DOWN);
        }

        tte_erase_rect_wrapper(HAND_SIZE_RECT_SELECT);
    }
    else if (id == BG_MAIN_MENU || id == BG_BLIND_SELECT || id == BG_SHOP || id == BG_ROUND_END)
    {
        // do nothing, just don't return early!
    }
    else
    {
        return; // Invalid background ID
    }

    background_legacy = id;
}

void reset_background(void)
{
    background_legacy = BG_NONE;
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

void display_round(void)
{
    // tte_erase_rect_wrapper(ROUND_TEXT_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%ld",
        ROUND_TEXT_RECT.left,
        ROUND_TEXT_RECT.top,
        TTE_YELLOW_PB,
        g_game_vars.round
    );
}

void display_hands(void)
{
    tte_printf(
        "#{P:%d,%d; cx:0xD000}%ld",
        HANDS_TEXT_RECT.left,
        HANDS_TEXT_RECT.top,
        g_game_vars.hands
    );
}

void display_discards(void)
{
    tte_printf(
        "#{P:%d,%d; cx:0xE000}%ld",
        DISCARDS_TEXT_RECT.left,
        DISCARDS_TEXT_RECT.top,
        g_game_vars.discards
    );
}

static void print_hand_type(const char* hand_type_str)
{
    if (hand_type_str == NULL)
        return; // NULL-checking paranoia

    Rect hand_type_rect = HAND_TYPE_RECT;
    update_text_rect_to_center_str(&hand_type_rect, hand_type_str, SCREEN_LEFT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%s",
        hand_type_rect.left,
        hand_type_rect.top,
        TTE_WHITE_PB,
        hand_type_str
    );
}

static void set_hand(void)
{
    tte_erase_rect_wrapper(HAND_TYPE_RECT);
    _contained_hands = compute_contained_hand_types();
    hand_type = compute_hand_type(_contained_hands);

    HandValues hand = hand_base_values[hand_type];

    chips = hand.chips;
    mult = hand.mult;

    print_hand_type(hand.display_name);
    display_chips();
    display_mult();
}

static bool can_discard_hand(void)
{
    return (g_game_vars.discards > 0 && hand_state == HAND_SELECT && hand_selections > 0);
}

static int deck_get_size(void)
{
    return deck_top + 1;
}

static int deck_get_max_size(void)
{
    // This is the max amount of cards that the player currently has in their possession
    return hand_top + played_top + deck_top + discard_top + 4;
}

static inline void deck_shuffle(void)
{
    for (int i = deck_top; i > 0; i--)
    {
        int j = get_rand() % (i + 1);
        Card* temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

static void game_round_on_init(void)
{
    hand_state = HAND_DRAW;
    cards_drawn = 0;
    hand_selections = 0;

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

// General functions
static inline void set_seed(int seed)
{
    srand(seed);
    g_game_vars.rng_seed = seed;
    g_game_vars.rng_step = 0;
}

// Playing state functions
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

    hand_state = HAND_DISCARD;
    --g_game_vars.discards;
    display_discards();
    set_hand();
}

static void game_playing_sort_by_rank_on_pressed(void)
{
    hand_change_sort(false);
}

static void game_playing_sort_by_suit_on_pressed(void)
{
    hand_change_sort(true);
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

static inline void hand_toggle_sort(void)
{
    sort_by_suit = !sort_by_suit;
    sort_cards();
}

static inline void hand_change_sort(bool to_sort_by_suit)
{
    if (to_sort_by_suit != sort_by_suit)
    {
        sort_by_suit = to_sort_by_suit;
        sort_cards();
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

static void game_playing_execute_play_hand(void)
{
    if (!can_play_hand())
        return;

    hand_state = HAND_PLAY;
    --g_game_vars.hands;
    display_hands();
}

static int game_playing_hand_row_get_size(void)
{
    return hand_get_size();
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
static int selection_hit_timer = UNDEFINED;

static bool game_playing_hand_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    int prev_card_idx = UNDEFINED;
    int next_card_idx = UNDEFINED;

    // Do not use FRAMES(x) here as we are counting real frames ignoring game speed
    card_moved_too_fast = (selection_hit_timer != UNDEFINED) &&
                          (g_game_vars.timer - selection_hit_timer) < card_swap_time_threshold;

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
                MM_BASE_PITCH_RATE + get_rand() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
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
                MM_BASE_PITCH_RATE + get_rand() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
                SFX_DEFAULT_VOLUME
            );
        }
    }

    return true;
}

static void game_playing_hand_row_on_key_transit(
    SelectionGrid* selection_grid,
    Selection* selection
)
{
    if (key_hit(SELECT_CARD))
    {
        selection_hit_timer = g_game_vars.timer;
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
        selection_hit_timer = UNDEFINED;
    }
    else if (key_hit(DESELECT_CARDS))
    {
        hand_deselect_all_cards();
        set_hand();
    }
    else if (key_hit(PLAY_HAND_KEY))
    {
        game_playing_execute_play_hand();
    }
    else if (key_hit(DISCARD_HAND_KEY))
    {
        game_playing_execute_discard();
    }
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

static bool can_play_hand(void)
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
    sprite_position(card_object->sprite_object->sprite, fx2int(deck_x), fx2int(deck_y));

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
                g_game_vars.timer = TM_ZERO;

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

    if (card_idx == 0 && discarded_card == false && g_game_vars.timer % FRAMES(10) == 0)
    {
        // This is never reached in the case of HAND_SHUFFLING. Not sure why but that's how it's
        // supposed to be.
        hand_state = HAND_DRAW;
        sound_played = false;
        cards_drawn = 0;
        hand_selections = 0;
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

static inline void play_starting_played_cards_update(int played_idx)
{
    bool card_selected = card_object_is_selected(played[played_top - scored_card_index]);
    if (played_idx == played_top && (g_game_vars.timer % FRAMES(10) == 0 || !card_selected) &&
        g_game_vars.timer > FRAMES(40))
    {
        scored_card_index--;

        if (scored_card_index == 0)
        {
            _joker_scored_itr = list_itr_create(&_owned_jokers_list);
            g_game_vars.timer = TM_ZERO;
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
    if (g_game_vars.timer % FRAMES(30) == 0 && g_game_vars.timer > FRAMES(40))
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
    if (g_game_vars.timer % FRAMES(30) == 0 && g_game_vars.timer > FRAMES(40))
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
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
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
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
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
    if (played_idx == 0 && (g_game_vars.timer % FRAMES(30) == 0) && g_game_vars.timer > FRAMES(40))
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

        g_game_vars.timer = TM_ZERO;
        play_state = PLAY_ENDING;
    }

    return false;
}

// This is the reverse of PLAY_STARTING. The cards get reset back to their neutral position
// sequentially
static inline void play_ending_played_cards_update(int played_idx)
{
    bool card_selected = card_object_is_selected(played[played_top - scored_card_index]);
    if (played_idx == played_top && (g_game_vars.timer % FRAMES(10) == 0 || !card_selected) &&
        g_game_vars.timer > FRAMES(40))
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
            g_game_vars.timer = TM_ZERO;
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

static inline void game_playing_process_card_draw()
{
    if (hand_state == HAND_DRAW && cards_drawn < hand_size)
    {
        if (g_game_vars.timer % FRAMES(10) == 0) // Draw a card every 10 frames
        {
            cards_drawn++;
            card_draw();
        }
    }
    else if (hand_state == HAND_DRAW)
    {
        hand_state = HAND_SELECT; // Change the hand state to select after drawing all the cards
        cards_drawn = 0;
        g_game_vars.timer = TM_ZERO;
    }
}

static inline void game_playing_discarded_cards_loop(void)
{
    // Discarded cards loop (mainly for shuffling)
    if (hand_get_size() == 0 && hand_state == HAND_SHUFFLING && discard_top >= -1 &&
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

                        hand_top--;
                        hand_selections--;
                        cards_drawn++;

                        discarded_card = true;
                    }

                    if (i == 0 && discarded_card == false && g_game_vars.timer % FRAMES(10) == 0)
                    {
                        hand_state = HAND_PLAYING;
                        cards_drawn = 0;
                        hand_selections = 0;
                        g_game_vars.timer = TM_ZERO;
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
        if (background_legacy == BG_CARD_SELECTING)
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
        else if (background_legacy == BG_CARD_PLAYING)
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

        last_hand_size = hand_get_size();
        last_deck_size = deck_get_size();
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

static void game_playing_on_update(void)
{
    // Background logic (thissss might be moved to the card'ssss logic later. I'm a sssssnake)
    if (hand_state == HAND_DRAW || hand_state == HAND_DISCARD || hand_state == HAND_SELECT)
    {
        change_background(BG_CARD_SELECTING, false);
    }
    else if (hand_state != HAND_SHUFFLING)
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

void game_start(void)
{
    // set_seed(9); // 9 is a full house
    set_seed(g_game_vars.rng_seed);

    affine_background_change_background(AFFINE_BG_GAME);

    g_game_vars.hands = MAX_HANDS;
    g_game_vars.discards = MAX_DISCARDS;

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

    change_background(BG_BLIND_SELECT, false);

    // Deck size/max size
    tte_erase_rect_wrapper(DECK_SIZE_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d/%d",
        DECK_SIZE_RECT.left,
        DECK_SIZE_RECT.top,
        TTE_WHITE_PB,
        deck_get_size(),
        deck_get_max_size()
    );

    display_round();                  // Set the round display
    display_score(g_game_vars.score); // Set the score display

    display_chips(); // Set the chips display
    display_mult();  // Set the multiplier display

    display_hands();    // Hand
    display_discards(); // Discard

    display_money(); // Set the money display

    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%ld#{cx:0x%X000}/%d",
        ANTE_TEXT_RECT.left,
        ANTE_TEXT_RECT.top,
        TTE_YELLOW_PB,
        g_game_vars.ante,
        TTE_WHITE_PB,
        MAX_ANTE
    ); // Ante

    game_change_state(GAME_STATE_BLIND_SELECT);
}
