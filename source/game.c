#include "game.h"

#include "affine_background.h"
#include "affine_background_gfx.h"
#include "audio_utils.h"
#include "background_blind_select_gfx.h"
#include "background_gfx.h"
#include "background_main_menu_gfx.h"
#include "background_shop_gfx.h"
#include "bitset.h"
#include "blind.h"
#include "card.h"
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
#define SCORED_CARD_TEXT_Y        48

// SE sizes
#define ROUND_END_BLACK_PANEL_INIT_BOTTOM_SE 12

#define MAIN_MENU_BUTTONS             2
#define MAIN_MENU_IMPLEMENTED_BUTTONS 1 // Remove this once all buttons are implemented
#define MAIN_MENU_PLAY_BTN_IDX        0

// TODO: Properly define and use
#define MENU_POP_OUT_ANIM_FRAMES 20
#define GAME_OVER_ANIM_FRAMES    15

#define HIGHLIGHT_COLOR   0xFFFF
#define SHOP_LIGHTS_1_CLR 0xFFFF
#define SHOP_LIGHTS_2_CLR 0x32BE
#define SHOP_LIGHTS_3_CLR 0x4B5F
#define SHOP_LIGHTS_4_CLR 0x5F9F

#define PITCH_STEP_DISCARD_SFX   (-64)
#define PITCH_STEP_DRAW_SFX      24
#define PITCH_STEP_UNDISCARD_SFX 2 * PITCH_STEP_DRAW_SFX

#define BUTTON_SFX_VOLUME 154 // 60% of MM_FULL_VOLUME

#define STARTING_ROUND 0
#define STARTING_ANTE  1
#define STARTING_MONEY 4
#define STARTING_SCORE 0

#define CARD_FOCUSED_UNSEL_Y 10
#define CARD_UNFOCUSED_SEL_Y 15
#define CARD_FOCUSED_SEL_Y   20

// Timer defs
#define TM_ZERO                         0
#define TM_RESET_STATIC_VARS            30
#define TM_END_POP_MENU_ANIM            13
#define TM_START_ROUND_END_REWARDS_ANIM 1
#define TM_END_DISPLAY_FIN_BLIND        30
#define TM_END_DISPLAY_SCORE_MIN        4
#define TM_REWARDS_ELLIPSIS_PRINT_START 2
#define TM_REWARDS_ELLIPSIS_PRINT_END   16
#define TM_REWARD_DISPLAY_INTERVAL      15
#define TM_DISPLAY_REWARDS_CONT_WAIT    TM_REWARDS_ELLIPSIS_PRINT_END + TM_REWARD_DISPLAY_INTERVAL
#define TM_HAND_REWARD_INCR_WAIT        TM_DISPLAY_REWARDS_CONT_WAIT + TM_REWARD_DISPLAY_INTERVAL
#define TM_REWARD_INCREMENT_INTERVAL    20
#define TM_DISMISS_ROUND_END_TM         20
#define TM_CREATE_SHOP_ITEMS_WAIT       1
#define TM_SHIFT_SHOP_ICON_WAIT         7
#define TM_END_GAME_SHOP_INTRO          12
#define TM_SHOP_PRC_INPUT_START         1
#define TM_DISP_BLIND_PANEL_FINISH      7
#define TM_DISP_BLIND_PANEL_START       1
#define TM_BLIND_SELECT_START           1
#define TM_END_ANIM_SEQ                 12

// Palette IDs
#define BOSS_BLIND_PRIMARY_PID               1
#define MAIN_MENU_PLAY_BUTTON_OUTLINE_PID    2
#define REROLL_BTN_PID                       3
#define BLIND_SKIP_BTN_PID                   5
#define MAIN_MENU_PLAY_BUTTON_MAIN_COLOR_PID 5
#define NEXT_ROUND_BTN_SELECTED_BORDER_PID   5
#define BLIND_BG_SHADOW_PID                  5
#define SHOP_PANEL_SHADOW_PID                6
#define PLAY_HAND_BTN_PID                    6
#define BOSS_BLIND_SHADOW_PID                7
#define PLAY_HAND_BTN_BORDER_PID             7
#define REROLL_BTN_SELECTED_BORDER_PID       7
#define SHOP_LIGHTS_1_PID                    8
#define DISCARD_BTN_BORDER_PID               8
#define BLIND_SKIP_BTN_SELECTED_BORDER_PID   10
#define DISCARD_BTN_PID                      13
#define SHOP_LIGHTS_2_PID                    14
#define BLIND_SELECT_BTN_PID                 15
#define NEXT_ROUND_BTN_PID                   16
#define SHOP_LIGHTS_3_PID                    17
#define BLIND_SELECT_BTN_SELECTED_BORDER_PID 18
#define BLIND_BG_SECONDARY_PID               18
#define BLIND_BG_PRIMARY_PID                 19
#define REWARD_PANEL_BORDER_PID              19
#define SHOP_LIGHTS_4_PID                    22
#define SHOP_BOTTOM_PANEL_BORDER_PID         26
// Naming the stage where cards return from the discard pile to the deck "undiscard"

#define NUM_SCORE_LERP_STEPS 32

// Shop
#define REROLL_BASE_COST 5 // Base cost for rerolling the shop items

#define NEXT_ROUND_BTN_SEL_X 0

#define REROLL_BTN_FRAME_PAL_IDX 7
#define REROLL_BTN_PAL_IDX       3

#define EXPIRE_ANIMATION_FRAME_COUNT 3

#define CARD_FOCUSED_UNSEL_Y 10
#define CARD_UNFOCUSED_SEL_Y 15
#define CARD_FOCUSED_SEL_Y   20

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

enum BlindSelectStates
{
    START_ANIM_SEQ,
    BLIND_SELECT,
    BLIND_SELECTED_ANIM_SEQ,
    DISPLAY_BLIND_PANEL,
    BLIND_SELECT_MAX
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
static void game_main_menu_on_init(void);
static void game_main_menu_on_update(void);
static void game_round_on_init(void);
static void game_playing_on_update(void);
static void game_round_end_on_update(void);
static void game_round_end_on_exit(void);
static void game_shop_on_update(void);
static void game_shop_on_exit(void);
static void game_blind_select_on_update(void);
static void game_blind_select_on_exit(void);
static void game_lose_on_init(void);
static void game_lose_on_update(void);
static void game_over_on_exit(void);
static void game_win_on_init(void);
static void game_win_on_update(void);
static void game_shop_intro(void);
static void game_shop_process_user_input(void);
static void game_shop_outro(void);
static void game_blind_select_start_anim_seq(void);
static void game_blind_select_handle_input(void);
static void game_blind_select_selected_anim_seq(void);
static void game_blind_select_display_blind_panel(void);
static void game_round_end_start(void);
static void game_round_end_start_expand_popup(void);
static void game_round_end_display_finished_blind(void);
static void game_round_end_display_score_min(void);
static void game_round_end_update_blind_reward(void);
static void game_round_end_panel_exit(void);
static void game_round_end_display_rewards(void);
static void game_round_end_display_cashout(void);
static void game_round_end_dismiss_round_end_panel(void);

static void sort_cards(void);
static void change_background(enum BackgroundId id);
static void display_temp_score(u32 value);
static void display_score(u32 value);
static void check_flaming_score(void);
static void display_round(int value);
static void display_hands(int value);
static void display_discards(int value);
static void set_hand(void);
static void hand_set_focus(int index);
static bool hand_discard(void);
static int deck_get_size(void);
static int deck_get_max_size(void);
static void increment_blind(enum BlindState increment_reason);
static void game_over_init(void);
static bool check_and_score_joker_for_event(
    ListItr* starting_joker_itr,
    CardObject* card_object,
    enum JokerEvent joker_event
);
static int calculate_interest_reward(void);
static void game_over_anim_frame(void);
static void shop_reroll_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static void shop_reroll_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static int shop_reroll_row_get_size(void);
static void shop_top_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);
static void shop_top_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static int shop_top_row_get_size(void);
static void jokers_sel_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection);
static void jokers_sel_row_on_selection_changed(
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
// Consts

// clang-format off
// disable clang-format here to preserve the organization here
// Rects                                       left     top     right   bottom
// Screenblock rects
static const Rect ROUND_END_MENU_RECT       = {9,       7,      24,     20 }; 

static const Rect POP_MENU_ANIM_RECT        = {9,       7,      24,     31 };
// The rect for popping menu animations (round end, shop, blinds) 
// - extends beyond the visible screen to the end of the screenblock
// It includes both the target and source position rects. 
// This is because when popping, the target position is blank so we just animate 
// the whole rect so we don't have to track its position

static const Rect SINGLE_BLIND_SELECT_RECT  = {9,       7,      13,     31 };
static const Rect BLIND_SKIP_BTN_GRAY_RECT  = {0,       24,     4,      27 };
static const Rect BLIND_SKIP_BTN_PREANIM_DEST_RECT = {9,29,     19,     31 };
// preanim - pre-animation rects for before the pop-up animation

static const Rect HAND_BG_RECT_SELECTING    = {9,       11,     24,     17 };
// TODO: Currently unused, remove?
//static const Rect HAND_BG_RECT_PLAYING      = {9,       14,     24,     18 };

static const Rect TOP_LEFT_ITEM_SRC_RECT    = {0,       20,     8,      25 };
static const BG_POINT TOP_LEFT_PANEL_POINT  = {0,       0, };
static const Rect TOP_LEFT_PANEL_ANIM_RECT  = {0,       0,      8,      4  };
/* Contains the shop icon/current blind etc. 
 * The difference between TOP_LEFT_PANEL_ANIM_RECT and TOP_LEFT_PANEL_RECT 
 * is due to an overlap between the bottom of the top left panel
 * and the top of the score panel in the tiles connecting them.
 * TOP_LEFT_PANEL_ANIM_RECT should be used for animations, 
 * TOP_LEFT_PANEL_RECT for copies etc. but mind the overlap
 */
static const Rect TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT = {0, 28, 8,     28 };
static const BG_POINT TOP_LEFT_BLIND_TITLE_POINT = {0,  21, };
static const Rect BIG_BLIND_TITLE_SRC_RECT  = {0,       26,     8,      26 };
static const Rect BOSS_BLIND_TITLE_SRC_RECT = {0,       27,     8,      27 };
static const Rect CASHOUT_DEST_RECT =         {10,      8,      23,     10 };
static const BG_POINT CASHOUT_SRC_3X3_RECT_POS =   {5,  29};
static const BG_POINT GAME_OVER_SRC_RECT_3X3_POS = {25, 29};
static const Rect GAME_OVER_DIALOG_DEST_RECT= {11,      21,      23,     28};
static const Rect GAME_OVER_ANIM_RECT       = {11,      8,       23,     28};
static const BG_POINT NEW_RUN_BTN_DEST_POS  = {15,      26};
static const Rect NEW_RUN_BTN_SRC_RECT      = {0,       30,      4,      31};
static const BG_POINT ROUND_END_REWARDS_ELLIPSIS_POS = {10, 13};

// Flaming score animation frames
#define SCORE_FLAMES_ANIM_FREQ  5 // animation will run at 12FPS
#define NUM_SCORE_FLAMES_FRAMES 8 // Chips and Mult flame frames are next to one another
#define SCORE_FLAME_FRAME_WIDTH 3 // so we only need to offset to get the next ones
static const Rect SCORE_FLAME_RESET         = {26,      20,      28,     20};
static const Rect SCORE_FLAME_FRAMES_START  = {26,      21,      28,     21};
static const BG_POINT SCORE_FLAME_CHIPS_POS = {1,       9};
static const BG_POINT SCORE_FLAME_MULT_POS  = {5,       9};

// Rects for TTE (in pixels)
static const Rect HAND_SIZE_RECT            = {128,     128,    152,    160 }; // Seems to include both SELECT and PLAYING
static const Rect HAND_SIZE_RECT_SELECT     = {128,     128,    152,    136 };
static const Rect HAND_SIZE_RECT_PLAYING    = {128,     152,    152,    160 };
static const Rect HAND_TYPE_RECT            = {8,       64,     64,     72  };
// Score displayed in the same place as the hand type
static const Rect TEMP_SCORE_RECT           = {8,       64,     64,     72  }; 
static const Rect SCORE_RECT                = {24,      48,     64,     56  };

static const Rect PLAYED_CARDS_SCORES_RECT  = {72,      48,     240,    56  };
static const Rect HELD_CARDS_SCORES_RECT    = {72,      108,    240,    116 };
static const Rect BLIND_TOKEN_TEXT_RECT     = {80,      72,     200,    160 };
static const Rect MONEY_TEXT_RECT           = {8,       120,    64,     128 };
static const Rect CHIPS_TEXT_RECT           = {8,       80,     32,     88  };
static const Rect MULT_TEXT_RECT            = {40,      80,     64,     88  };
static const Rect BLIND_REWARD_RECT         = {40,      32,     64,     40  };
static const Rect BLIND_REQ_TEXT_RECT       = {32,      24,     64,     32  };
static const Rect SHOP_PRICES_TEXT_RECT     = {72,      56,     192,    160 };

// Rects with UNDEFINED are only used in tte_printf, they need to be fully defined
// to be used with tte_erase_rect_wrapper()
static const Rect HANDS_TEXT_RECT           = {16,      104,    UNDEFINED, UNDEFINED };
static const Rect DISCARDS_TEXT_RECT        = {48,      104,    UNDEFINED, UNDEFINED };
static const Rect DECK_SIZE_RECT            = {200,     152,    UNDEFINED, UNDEFINED };
static const Rect ROUND_TEXT_RECT           = {48,      144,    UNDEFINED, UNDEFINED };
static const Rect ANTE_TEXT_RECT            = {8,       144,    UNDEFINED, UNDEFINED };
static const Rect ROUND_END_BLIND_REQ_RECT  = {104,     96,     136,       UNDEFINED };
static const Rect ROUND_END_BLIND_REWARD_RECT = { 168,  96,     UNDEFINED, UNDEFINED };
static const Rect CASHOUT_TEXT_RECT         = {88,      72,     UNDEFINED, UNDEFINED };
static const Rect SHOP_REROLL_RECT          = {88,      96,     UNDEFINED, UNDEFINED };
static const Rect GAME_LOSE_MSG_TEXT_RECT   = {104,     72,     UNDEFINED, UNDEFINED};
// 1 character to the right of GAME_LOSE
static const Rect GAME_WIN_MSG_TEXT_RECT    = {112,      72,     UNDEFINED, UNDEFINED};

static const BG_POINT HELD_JOKERS_POS       = {108,     10};
static const BG_POINT JOKER_DISCARD_TARGET  = {240,     30};
static const BG_POINT CARD_DRAW_POS         = {208,     110};
static const BG_POINT CUR_BLIND_TOKEN_POS   = {8,       18};
static const BG_POINT CARD_DISCARD_PNT      = {240,     70};
static const BG_POINT HAND_START_POS        = {120,     90};
static const BG_POINT HAND_PLAY_POS         = {120,     70};
static const BG_POINT MAIN_MENU_ACE_T       = {88,      26};
// clang-format on

static uint rng_seed = 0;

typedef void (*SubStateActionFn)(void);

static uint timer = 0; // This might already exist in libtonc but idk so i'm just making my own
// BY DEFAULT IS SET TO 1, but if changed to 2 or more, should speed up all (or most) of the game
// aspects that should be sped up by speed, as in the original game.
static int game_speed = 1;
static enum BackgroundId background = BG_NONE;

static StateInfo state_info[] = {
#define DEF_STATE_INFO(stateEnum, init_fn, update_fn, exit_fn) \
    {.on_init = init_fn, .on_update = update_fn, .on_exit = exit_fn, .substate = 0},
#include "../include/def_state_info_table.h"
#undef DEF_STATE_INFO
};

SelectionGridRow shop_selection_rows[] = {
    {0, jokers_sel_row_get_size,  jokers_sel_row_on_selection_changed,  jokers_sel_row_on_key_transit },
    {1, shop_top_row_get_size,    shop_top_row_on_selection_changed,    shop_top_row_on_key_transit   },
    {2, shop_reroll_row_get_size, shop_reroll_row_on_selection_changed, shop_reroll_row_on_key_transit}
};

static const Selection SHOP_INIT_SEL = {-1, 1};

SelectionGrid shop_selection_grid = {
    shop_selection_rows,
    NUM_ELEM_IN_ARR(shop_selection_rows),
    SHOP_INIT_SEL
};

// This is a stupid way to do this but I don't care
static const int HAND_SPACING_LUT[MAX_HAND_SIZE] =
    {28, 28, 28, 28, 27, 21, 18, 15, 13, 12, 10, 9, 9, 8, 8, 7};

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

static const SubStateActionFn blind_select_state_actions[] = {
    game_blind_select_start_anim_seq,
    game_blind_select_handle_input,
    game_blind_select_selected_anim_seq,
    game_blind_select_display_blind_panel
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
static enum GameState game_state = GAME_STATE_UNDEFINED;
static enum HandState hand_state = HAND_DRAW;
static enum PlayState play_state = PLAY_STARTING;

static enum HandType hand_type = NONE;

static CardObject* main_menu_ace = NULL;

// The sprite that displays the blind when in "GAME_PLAYING/GAME_ROUND_END" state
static Sprite* playing_blind_token = NULL;

// The sprite that displays the blind when in "GAME_ROUND_END" state
static Sprite* round_end_blind_token = NULL;

// The sprites that display the blinds when in "GAME_BLIND_SELECT" state
static Sprite* blind_select_tokens[BLIND_TYPE_MAX] = {NULL};

static int current_blind = BLIND_TYPE_SMALL;

// The current state of the blinds, this is used to determine what the game is doing at any given
// time
static enum BlindState blinds[BLIND_TYPE_MAX] = {
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
static int hands = 0;
static int discards = 0;

static int round = 0;
static int ante = 0;
static int money = 0;
static u32 score = 0;
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

static int selection_x = 0;
static int selection_y = 0;

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
static int straight_and_flush_size = STRAIGHT_AND_FLUSH_SIZE_DEFAULT;

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

    hands = max_hands;
    discards = max_discards;
    timer = TM_ZERO;
    current_blind = BLIND_TYPE_SMALL;
    blinds[0] = BLIND_STATE_CURRENT;
    blinds[1] = BLIND_STATE_UPCOMING;
    blinds[2] = BLIND_STATE_UPCOMING;
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
    return straight_and_flush_size;
}

static void add_joker(JokerObject* joker_object)
{
    list_push_back(&_owned_jokers_list, joker_object);

    // TODO: Extract to on_joker_added() callback
    // In case the player gets multiple Four Fingers Jokers,
    // only change size when the first one is added
    if (joker_object->joker->id == FOUR_FINGERS_JOKER_ID)
    {
        if (four_fingers_joker_count == 0)
        {
            straight_and_flush_size = STRAIGHT_AND_FLUSH_SIZE_FOUR_FINGERS;
        }
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
        if (four_fingers_joker_count == 0)
        {
            straight_and_flush_size = STRAIGHT_AND_FLUSH_SIZE_DEFAULT;
        }
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
    tte_erase_rect_wrapper(MULT_TEXT_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000;}%lu",
        MULT_TEXT_RECT.left,
        MULT_TEXT_RECT.top,
        TTE_WHITE_PB,
        mult
    );
    check_flaming_score();
}

static inline void sort_hand_by_suit(void)
{
    for (int a = 0; a < hand_top; a++)
    {
        for (int b = a + 1; b <= hand_top; b++)
        {
            if (hand[a] == NULL ||
                (hand[b] != NULL && (hand[a]->card->suit > hand[b]->card->suit ||
                                     (hand[a]->card->suit == hand[b]->card->suit &&
                                      hand[a]->card->rank > hand[b]->card->rank))))
            {
                CardObject* temp = hand[a];
                hand[a] = hand[b];
                hand[b] = temp;
            }
        }
    }
}

static inline void sort_hand_by_rank(void)
{
    for (int a = 0; a < hand_top; a++)
    {
        for (int b = a + 1; b <= hand_top; b++)
        {
            if (hand[a] == NULL || (hand[b] != NULL && hand[a]->card->rank > hand[b]->card->rank))
            {
                CardObject* temp = hand[a];
                hand[a] = hand[b];
                hand[b] = temp;
            }
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

    // Update the sprites in the hand by destroying them and creating new ones in the correct order
    // (This is feels like a diabolical solution but like literally how else would you do this)
    for (int i = 0; i <= hand_top; i++)
    {
        if (hand[i] != NULL)
        {
            // card_object_get_sprite() will not work here since we need the address
            sprite_destroy(&(hand[i]->sprite_object->sprite));
        }
    }

    for (int i = 0; i <= hand_top; i++)
    {
        if (hand[i] != NULL)
        {
            // hand[i]->sprite = sprite_new(ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF, ATTR1_SIZE_32,
            // card_sprite_lut[hand[i]->card->suit][hand[i]->card->rank], 0, i);
            card_object_set_sprite(hand[i], i); // Set the sprite for the card object
            sprite_position(
                card_object_get_sprite(hand[i]),
                fx2int(hand[i]->sprite_object->x),
                fx2int(hand[i]->sprite_object->y)
            );
        }
    }
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
static inline void reset_top_left_panel_bottom_row()
{
    BG_POINT top_left_panel_bottom_row_pos = TOP_LEFT_PANEL_POINT;
    // Use the source rect height to offset to the bottom row point
    top_left_panel_bottom_row_pos.y += rect_height(&TOP_LEFT_ITEM_SRC_RECT) - 1;
    main_bg_se_copy_rect(TOP_LEFT_PANEL_BOTTOM_ROW_RESET_RECT, top_left_panel_bottom_row_pos);
}

static void change_background(enum BackgroundId id)
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

            // Copy the Play Hand and Discard button colors to their selection highlights
            memcpy16(&pal_bg_mem[PLAY_HAND_BTN_BORDER_PID], &pal_bg_mem[PLAY_HAND_BTN_PID], 1);
            memcpy16(&pal_bg_mem[DISCARD_BTN_BORDER_PID], &pal_bg_mem[DISCARD_BTN_PID], 1);
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
        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            obj_unhide(blind_select_tokens[i]->obj, 0);
        }

        // Default y position for the blind select tokens. 12 is the amount of tiles the background
        // is shifted down by
        const int default_y = 89 + (TILE_SIZE * 12);
        // TODO refactor magic numbers '80/120/160' into a map to loop with
        sprite_position(blind_select_tokens[BLIND_TYPE_SMALL], 80, default_y);
        sprite_position(blind_select_tokens[BLIND_TYPE_BIG], 120, default_y);
        sprite_position(blind_select_tokens[BLIND_TYPE_BOSS], 160, default_y);

        toggle_windows(false, true);

        GRIT_CPY(pal_bg_mem, background_blind_select_gfxPal);
        GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_blind_select_gfxTiles);
        GRIT_CPY(&se_mem[MAIN_BG_SBB], background_blind_select_gfxMap);

        // Copy boss blind colors to blind select palette
        memset16(
            &pal_bg_mem[1],
            blind_get_color(BLIND_TYPE_BOSS, BLIND_BACKGROUND_MAIN_COLOR_INDEX),
            1
        );
        memset16(
            &pal_bg_mem[7],
            blind_get_color(BLIND_TYPE_BOSS, BLIND_BACKGROUND_SHADOW_COLOR_INDEX),
            1
        );

        // Disable the button highlight colors
        // Select button PID is 15 and the outline is 18
        memcpy16(
            &pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[BLIND_SELECT_BTN_PID],
            1
        );
        // It seems the skip button (and score multiplier and deck) PB idx is
        // actually 5, not 10. 10 is the selected border color
        // Setting this palette value though doesn't seem to have an
        // effect.
        memcpy16(
            &pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[BLIND_SKIP_BTN_PID],
            1
        );

        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            Rect curr_blind_rect = SINGLE_BLIND_SELECT_RECT;

            // There's no gap between them
            curr_blind_rect.left += i * rect_width(&SINGLE_BLIND_SELECT_RECT);
            curr_blind_rect.right += i * rect_width(&SINGLE_BLIND_SELECT_RECT);

            if (blinds[i] != BLIND_STATE_CURRENT &&
                (i == BLIND_TYPE_SMALL || i == BLIND_TYPE_BIG)) // Make the skip button gray
            {
                BG_POINT skip_blind_btn_pos_dest = {
                    BLIND_SKIP_BTN_PREANIM_DEST_RECT.left,
                    BLIND_SKIP_BTN_PREANIM_DEST_RECT.top
                };
                skip_blind_btn_pos_dest.x = curr_blind_rect.left;

                Rect skip_blind_btn_rect_src = BLIND_SKIP_BTN_GRAY_RECT;
                skip_blind_btn_rect_src.top += i * rect_height(&BLIND_SKIP_BTN_GRAY_RECT);
                skip_blind_btn_rect_src.bottom += i * rect_height(&BLIND_SKIP_BTN_GRAY_RECT);

                main_bg_se_copy_rect(skip_blind_btn_rect_src, skip_blind_btn_pos_dest);
            }

            switch (blinds[i])
            {
                case BLIND_STATE_CURRENT: // Raise the blind panel up a bit
                {
                    // TODO: Replace copies with main_bg_se_copy_rect() of named rects
                    int x_from = 0;
                    int y_from = 27;

                    main_bg_se_copy_rect_1_tile_vert(curr_blind_rect, SCREEN_UP);

                    int x_to = curr_blind_rect.left;
                    int y_to = 31;

                    if (i == BLIND_TYPE_BIG)
                    {
                        y_from = 31;
                    }
                    else if (i == BLIND_TYPE_BOSS)
                    {
                        x_from = x_to;
                        y_from = 30;
                    }

                    // Copy plain tiles onto the bottom of the raised blind panel to fill the gap
                    // created by the raise
                    Rect gap_fill_rect = {
                        x_from,
                        y_from,
                        x_from + rect_width(&SINGLE_BLIND_SELECT_RECT) - 1,
                        y_from
                    };
                    BG_POINT gap_fill_point = {x_to, y_to};
                    main_bg_se_copy_rect(gap_fill_rect, gap_fill_point);

                    // Move token up by a tile
                    sprite_position(
                        blind_select_tokens[i],
                        blind_select_tokens[i]->pos.x,
                        blind_select_tokens[i]->pos.y - TILE_SIZE
                    );
                    break;
                }
                case BLIND_STATE_UPCOMING: // Change the select icon to "NEXT"
                {
                    int x_from = 0;
                    int y_from = 20;

                    int x_to = 10 + (i * rect_width(&SINGLE_BLIND_SELECT_RECT));
                    int y_to = 20;

                    memcpy16(
                        &se_mem[MAIN_BG_SBB][x_to + 32 * y_to],
                        &se_mem[MAIN_BG_SBB][x_from + 32 * y_from],
                        3
                    );
                    break;
                }
                case BLIND_STATE_SKIPPED: // Change the select icon to "SKIP"
                {
                    int x_from = 3;
                    int y_from = 20;

                    int x_to = 10 + (i * 5);
                    int y_to = 20;

                    memcpy16(
                        &se_mem[MAIN_BG_SBB][x_to + 32 * y_to],
                        &se_mem[MAIN_BG_SBB][x_from + 32 * y_from],
                        3
                    );
                    break;
                }
                case BLIND_STATE_DEFEATED: // Change the select icon to "DEFEATED"
                {
                    int x_from = 6;
                    int y_from = 20;

                    int x_to = 10 + (i * 5);
                    int y_to = 20;

                    memcpy16(
                        &se_mem[MAIN_BG_SBB][x_to + 32 * y_to],
                        &se_mem[MAIN_BG_SBB][x_from + 32 * y_from],
                        3
                    );
                    break;
                }
                default:
                    break;
            }
        }
    }
    else if (id == BG_MAIN_MENU)
    {
        toggle_windows(false, false);

        tte_erase_screen();
        GRIT_CPY(pal_bg_mem, background_main_menu_gfxPal);
        GRIT_CPY(&tile_mem[MAIN_BG_CBB], background_main_menu_gfxTiles);
        GRIT_CPY(&se_mem[MAIN_BG_SBB], background_main_menu_gfxMap);

        // Disable the button highlight colors
        memcpy16(
            &pal_bg_mem[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID],
            &pal_bg_mem[MAIN_MENU_PLAY_BUTTON_MAIN_COLOR_PID],
            1
        );
    }
    else
    {
        return; // Invalid background ID
    }

    background = id;
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

static void display_score(u32 value)
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

static void display_round(int value)
{
    // tte_erase_rect_wrapper(ROUND_TEXT_RECT);
    tte_printf(
        "#{P:%d,%d; cx:0x%X000}%d",
        ROUND_TEXT_RECT.left,
        ROUND_TEXT_RECT.top,
        TTE_YELLOW_PB,
        round
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

static void set_hand(void)
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

static void hand_set_focus(int index)
{
    if (hand_state != HAND_SELECT)
        return;

    // Wrap around to the other side of the hand when going out of bounds on either side
    if (index < 0)
    {
        selection_x = hand_top;
    }
    else if (index > hand_top)
    {
        selection_x = 0;
    }
    else
    {
        selection_x = index;
    }

    play_sfx(
        SFX_CARD_FOCUS,
        MM_BASE_PITCH_RATE + rand() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
        SFX_DEFAULT_VOLUME
    );
}

static bool hand_discard(void)
{
    if (hand_state != HAND_SELECT || hand_selections == 0)
        return false;
    return true;
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

static void increment_blind(enum BlindState increment_reason)
{
    current_blind++;
    if (current_blind >= BLIND_TYPE_MAX)
    {
        current_blind = 0;
        blinds[0] = BLIND_STATE_CURRENT;  // Reset the blinds to the first one
        blinds[1] = BLIND_STATE_UPCOMING; // Set the next blind to upcoming
        blinds[2] = BLIND_STATE_UPCOMING; // Set the next blind to upcoming
    }
    else
    {
        blinds[current_blind] = BLIND_STATE_CURRENT;
        blinds[current_blind - 1] = increment_reason;
    }
}

static inline void deck_shuffle(void)
{
    for (int i = deck_top; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Card* temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

static void game_round_on_init()
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
}

static void game_main_menu_on_init()
{
    affine_background_change_background(AFFINE_BG_MAIN_MENU);
    change_background(BG_MAIN_MENU);
    main_menu_ace = card_object_new(card_new(SPADES, ACE));
    card_object_set_sprite(main_menu_ace, 0); // Set the sprite for the ace of spades
    main_menu_ace->sprite_object->sprite->obj->attr0 |=
        ATTR0_AFF_DBL; // Make the sprite double sized
    main_menu_ace->sprite_object->tx = int2fx(MAIN_MENU_ACE_T.x);
    main_menu_ace->sprite_object->x = main_menu_ace->sprite_object->tx;
    main_menu_ace->sprite_object->ty = int2fx(MAIN_MENU_ACE_T.y);
    main_menu_ace->sprite_object->y = main_menu_ace->sprite_object->ty;
    main_menu_ace->sprite_object->tscale = float2fx(0.8f);
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

static inline void hand_toggle_card_selection(void)
{
    if (hand_state != HAND_SELECT || hand[selection_x] == NULL)
        return;

    if (card_object_is_selected(hand[selection_x]))
    {
        card_object_set_selected(hand[selection_x], false);
        hand_selections--;
        play_sfx(SFX_CARD_DESELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
    }
    else if (hand_selections < MAX_SELECTION_SIZE)
    {
        card_object_set_selected(hand[selection_x], true);
        hand_selections++;
        play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
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

static inline void hand_change_sort(void)
{
    sort_by_suit = !sort_by_suit;
    sort_cards();
}

static inline bool hand_play(void)
{
    if (hand_state != HAND_SELECT || hand_selections == 0)
        return false;
    return true;
}

static inline void game_playing_process_hand_select_input(void)
{
    static bool discard_button_highlighted =
        false; // true = play button highlighted, false = discard button highlighted

    if (key_hit(KEY_LEFT))
    {
        if (selection_y == 0)
        {
            // The reason why this adds 1 is because the hand is drawn from right to left. There is
            // no particular reason for this, it's just how I did it.
            hand_set_focus(selection_x + 1);
        }
        else
        {
            discard_button_highlighted = false; // Play button
        }
    }
    else if (key_hit(KEY_RIGHT))
    {
        if (selection_y == 0)
        {
            hand_set_focus(selection_x - 1);
        }
        else
        {
            discard_button_highlighted = true; // Discard button
        }
    }
    else if (key_hit(KEY_UP) && selection_y != 0)
    {
        selection_y = 0;
    }
    else if (key_hit(KEY_DOWN) && selection_y != 1)
    {
        selection_y = 1;

        if (selection_x > hand_top / 2)
        {
            discard_button_highlighted = false; // Play button
        }
        else
        {
            discard_button_highlighted = true; // Discard button
        }
    }
    else if (selection_y == 1) // On row of play/discard buttons
    {
        if (discard_button_highlighted == false) // Play button logic
        {
            memset16(&pal_bg_mem[PLAY_HAND_BTN_BORDER_PID], HIGHLIGHT_COLOR, 1);
            memcpy16(&pal_bg_mem[DISCARD_BTN_BORDER_PID], &pal_bg_mem[DISCARD_BTN_PID], 1);

            if (key_hit(SELECT_CARD) && hands > 0 && hand_play())
            {
                play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);

                hand_state = HAND_PLAY;
                selection_x = 0;
                selection_y = 0;
                display_hands(--hands);
            }
        }
        else // Discard button logic
        {
            memcpy16(&pal_bg_mem[PLAY_HAND_BTN_BORDER_PID], &pal_bg_mem[PLAY_HAND_BTN_PID], 1);
            memset16(&pal_bg_mem[DISCARD_BTN_BORDER_PID], HIGHLIGHT_COLOR, 1);

            if (key_hit(SELECT_CARD) && discards > 0 && hand_discard())
            {
                play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);

                hand_state = HAND_DISCARD;
                selection_x = 0;
                selection_y = 0;
                display_hands(--discards);
                set_hand();
                tte_printf(
                    "#{P:%d,%d; cx:0x%X000}%d",
                    DISCARDS_TEXT_RECT.left,
                    DISCARDS_TEXT_RECT.top,
                    TTE_RED_PB,
                    discards
                );
            }
        }
    }
    else if (selection_y == 0) // On row of cards
    {
        // Play button highlight color
        memcpy16(&pal_bg_mem[PLAY_HAND_BTN_BORDER_PID], &pal_bg_mem[PLAY_HAND_BTN_PID], 1);
        // Discard button highlight color
        memcpy16(&pal_bg_mem[DISCARD_BTN_BORDER_PID], &pal_bg_mem[DISCARD_BTN_PID], 1);

        if (key_hit(SELECT_CARD))
        {
            hand_toggle_card_selection();
            set_hand();
        }

        if (key_hit(DESELECT_CARDS))
        {
            hand_deselect_all_cards();
            set_hand();
        }
    }

    if (key_hit(SORT_HAND))
    {
        hand_change_sort();
    }
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
                sort_cards();

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

        if (scored_card_index == 0)
        {
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

// TODO: Help this comment find its way back to its variable
/* This needs to stay a power of 2 and small enough
 * for the lerping to be done before the next hand is drawn.
 */

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
        }
    }
    else if (play_state == PLAY_ENDED)
    {
        /* Using fixed point in case the score is lower than NUM_SCORE_LERP_STEPS and then
         * then the division rounds it down to 0 and it's never added to the total.
         * The operation is equivalent to
         * fxdiv(int2fx(temp_score * get_game_speed()), int2fx(NUM_SCORE_LERP_STEPS))
         */
        lerped_temp_score -= int2fx(temp_score * get_game_speed()) / NUM_SCORE_LERP_STEPS;
        lerped_score += int2fx(temp_score * get_game_speed()) / NUM_SCORE_LERP_STEPS;

        if (lerped_temp_score > 0)
        {
            display_temp_score(fx2uint(lerped_temp_score));

            // We actually don't need to erase this because the score only increases
            display_score(fx2uint(lerped_score)); // Set the score display
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

static inline void cards_in_hand_update_loop(void)
{
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
                    bool is_focused = (i == selection_x && selection_y == 0);

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

                    if (i != selection_x && hand[i]->sprite_object->y > hand_y)
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
                        sort_cards();

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
                                // ELSE FALL THROUGH
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

static void game_playing_on_update(void)
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

static void jokers_sel_row_on_selection_changed(
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
        game_sell_joker(selection->x);
        // Move the selection away from the jokers so it doesn't point to an invalid place
        selection_grid_move_selection_vert(selection_grid, SCREEN_DOWN);
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

static void shop_top_row_on_selection_changed(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
)
{
    // The selection grid system only guarantees that the new selection is within bounds
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
            // -1 to account for next round button
        }
    }

    if (new_selection->y == row_idx)
    {
        if (new_selection->x == NEXT_ROUND_BTN_SEL_X)
        {
            // Highlight next round button
            memset16(&pal_bg_mem[NEXT_ROUND_BTN_SELECTED_BORDER_PID], HIGHLIGHT_COLOR, 1);
        }
        else
        {
            int idx = new_selection->x - 1; // -1 to account for next round button
            JokerObject* joker_object = (JokerObject*)list_get_at_idx(&_shop_jokers_list, idx);
            sprite_object_set_focus(joker_object->sprite_object, true);
        }
    }
}

static int shop_reroll_row_get_size()
{
    return 1; // Only the reroll button
}

static void shop_reroll_row_on_selection_changed(
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
        memset16(&pal_bg_mem[REROLL_BTN_SELECTED_BORDER_PID], HIGHLIGHT_COLOR, 1);
    }
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

static void game_blind_select_on_update()
{
    if (state_info[game_state].substate == BLIND_SELECT_MAX)
    {
        game_change_state(GAME_STATE_PLAYING);
        return;
    }

    int substate = state_info[game_state].substate;
    blind_select_state_actions[substate]();
}

static void game_blind_select_start_anim_seq()
{
    change_background(BG_BLIND_SELECT);
    main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

    for (int i = 0; i < BLIND_TYPE_MAX; i++)
    {
        sprite_position(
            blind_select_tokens[i],
            blind_select_tokens[i]->pos.x,
            blind_select_tokens[i]->pos.y - TILE_SIZE
        );
    }

    if (timer == TM_END_ANIM_SEQ)
    {
        state_info[game_state].substate = BLIND_SELECT;
        timer = TM_ZERO; // Reset the timer
    }
}

static void game_blind_select_handle_input()
{
    if (timer == TM_BLIND_SELECT_START && current_blind == BLIND_TYPE_BOSS)
    {
        selection_y = 0;
    }

    // Blind select input logic
    if (key_hit(KEY_UP))
    {
        selection_y = 0;
    }
    else if (key_hit(KEY_DOWN) && current_blind != BLIND_TYPE_BOSS)
    {
        selection_y = 1;
    }
    else if (key_hit(SELECT_CARD))
    {
        if (selection_y == 0) // Blind selected
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            state_info[game_state].substate = BLIND_SELECTED_ANIM_SEQ;
            timer = TM_ZERO;
            display_round(++round);
        }
        else if (current_blind != BLIND_TYPE_BOSS)
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            increment_blind(BLIND_STATE_SKIPPED);

            background = UNDEFINED; // Force refresh of the background
            change_background(BG_BLIND_SELECT);

            // TODO: Create a generic vertical move by any number of tiles to avoid for loops?
            for (int i = 0; i < 12; i++)
            {
                main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);
            }

            for (int i = 0; i < BLIND_TYPE_MAX; i++)
            {
                sprite_position(
                    blind_select_tokens[i],
                    blind_select_tokens[i]->pos.x,
                    blind_select_tokens[i]->pos.y - (TILE_SIZE * 12)
                );
            }

            timer = TM_ZERO;
        }
    }

    if (selection_y == 0)
    {
        // 5 is the multiplier palette color and the skip button color
        memset16(&pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID], 0xFFFF, 1);
        memcpy16(
            &pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[BLIND_SKIP_BTN_PID],
            1
        );
    }
    else
    {
        // 15 is the select button color
        memcpy16(
            &pal_bg_mem[BLIND_SELECT_BTN_SELECTED_BORDER_PID],
            &pal_bg_mem[BLIND_SELECT_BTN_PID],
            1
        );
        memset16(&pal_bg_mem[BLIND_SKIP_BTN_SELECTED_BORDER_PID], 0xFFFF, 1);
    }
}

static void game_blind_select_selected_anim_seq()
{
    if (timer < 15)
    {
        Rect blinds_rect = POP_MENU_ANIM_RECT;
        blinds_rect.top -= 1; // Because of the raised blind
        main_bg_se_move_rect_1_tile_vert(blinds_rect, SCREEN_DOWN);

        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            sprite_position(
                blind_select_tokens[i],
                blind_select_tokens[i]->pos.x,
                blind_select_tokens[i]->pos.y + TILE_SIZE
            );
        }
    }
    else if (timer >= MENU_POP_OUT_ANIM_FRAMES)
    {
        for (int i = 0; i < BLIND_TYPE_MAX; i++)
        {
            obj_hide(blind_select_tokens[i]->obj);
        }

        state_info[game_state].substate = DISPLAY_BLIND_PANEL; // Reset the state
        timer = TM_ZERO;                                       // Reset the timer
    }
}

static void game_blind_select_display_blind_panel()
{
    if (timer >= TM_DISP_BLIND_PANEL_FINISH)
    {
        state_info[game_state].substate = BLIND_SELECT_MAX;
        return;
    }

    // Switches to the selecting background and clears the blind panel area
    if (timer == TM_DISP_BLIND_PANEL_START)
    {
        change_background(BG_CARD_SELECTING);

        main_bg_se_clear_rect(ROUND_END_MENU_RECT);

        for (int y = 0; y < 5; y++)
        {
            int y_from = 28;
            int y_to = 0 + y;

            Rect from = {0, y_from, 8, y_from + 1};
            BG_POINT to = {0, y_to};

            main_bg_se_copy_rect(from, to);
        }

        reset_top_left_panel_bottom_row();
    }

    // Shift the blind panel down onto screen
    for (int y = 0; y < timer; y++)
    {
        int y_from = 26 + y - timer;
        int y_to = 0 + y;

        Rect from = {0, y_from, 8, y_from};
        BG_POINT to = {0, y_to};

        main_bg_se_copy_rect(from, to);
    }
}

static void game_blind_select_on_exit()
{
    selection_y = 0;
    background = UNDEFINED;
}

static inline void game_start(void)
{
    set_seed(rng_seed);
    // set_seed(9); // 9 is a full house

    affine_background_change_background(AFFINE_BG_GAME);

    // Normally I would just cache these and hide/unhide but I didn't feel like dealing with
    // defining a layer for it
    card_destroy(&main_menu_ace->card);
    card_object_destroy(&main_menu_ace);

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

    display_round(round); // Set the round display
    display_score(score); // Set the score display

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

static void game_main_menu_on_update()
{
    change_background(BG_MAIN_MENU);

    card_object_update(main_menu_ace);
    main_menu_ace->sprite_object->trotation = lu_sin((timer << 8) / 2) / 3;
    main_menu_ace->sprite_object->rotation = main_menu_ace->sprite_object->trotation;

    // Seed randomization
    rng_seed++;
    // If the keys have changed, make it more pseudo-random
    if (key_curr_state() != key_prev_state())
    {
        rng_seed *= 2;
    }

    if (key_hit(KEY_LEFT))
    {
        if (selection_x > 0)
        {
            selection_x--;
        }
    }
    else if (key_hit(KEY_RIGHT))
    {
        if (selection_x < MAIN_MENU_IMPLEMENTED_BUTTONS - 1)
        {
            selection_x++;
        }
    }

    if (selection_x == MAIN_MENU_PLAY_BTN_IDX)
    {
        memset16(&pal_bg_mem[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID], HIGHLIGHT_COLOR, 1);

        if (key_hit(SELECT_CARD))
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            game_start();
        }
    }
    else
    {
        memcpy16(
            &pal_bg_mem[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID],
            &pal_bg_mem[MAIN_MENU_PLAY_BUTTON_MAIN_COLOR_PID],
            1
        );
    }
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
    ListItr itr = list_itr_create(&_owned_jokers_list);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
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

    display_round(round);
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
