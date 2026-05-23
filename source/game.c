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
#include "game/round.h"
#include "game/round_end.h"
#include "game/shop.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "hand.h"
#include "joker.h"
#include "layout.h"
#include "list.h"
#include "random.h"
#include "save.h"
#include "selection_grid.h"
#include "soundbank.h"
#include "splash_screen.h"
#include "sprite.h"
#include "state_machine.h"
#include "timer.h"
#include "tonc_memdef.h"
#include "util.h"

#include <maxmod.h>
#include <stdint.h>
#include <stdlib.h>

#define STRAIGHT_AND_FLUSH_SIZE_FOUR_FINGERS 4
#define STRAIGHT_AND_FLUSH_SIZE_DEFAULT      5

// Pixel sizes

#define STARTING_ROUND 0
#define STARTING_ANTE  1
#define STARTING_MONEY 4
#define STARTING_SCORE 0


// TODO: Rename "PID" to "PAL_IDX"
// Palette IDs

#define BLIND_BG_SHADOW_PAL_IDX     5
#define BLIND_BG_SECONDARY_PAL_IDX  18
#define BLIND_BG_PRIMARY_PAL_IDX    19
#define REWARD_PANEL_BORDER_PAL_IDX 19

// Naming the stage where cards return from the discard pile to the deck "undiscard"

/* This needs to stay a power of 2 and small enough
 * for the lerping to be done before the next hand is drawn.
 */

#define GAME_PLAYING_BUTTONS_SEL_Y   2
#define GAME_PLAYING_NUM_BOTTOM_BTNS 2

#define EXPIRE_ANIMATION_FRAME_COUNT 3

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

// Rects for TTE (in pixels)
// Score displayed in the same place as the hand type
static const Rect SCORE_RECT                = {24,      48,     64,     56  };

static const Rect HELD_CARDS_SCORES_RECT    = {72,      108,    240,    116 };
static const Rect MONEY_TEXT_RECT           = {8,       120,    64,     128 };
static const Rect CHIPS_TEXT_RECT           = {8,       80,     32,     88  };
static const Rect MULT_TEXT_RECT            = {40,      80,     64,     88  };

// Rects with UNDEFINED are only used in tte_printf, they need to be fully defined
// to be used with tte_erase_rect_wrapper()
static const Rect HANDS_TEXT_RECT           = {16,      104,    UNDEFINED, UNDEFINED };
static const Rect DISCARDS_TEXT_RECT        = {48,      104,    UNDEFINED, UNDEFINED };
static const Rect ROUND_TEXT_RECT           = {48,      144,    UNDEFINED, UNDEFINED };

// clang-format on

// NOTE: This is going to be removed in favor of the background
// variable and handling in common.c once the related refactor is finished
static enum BackgroundId background_legacy = BG_NONE;

static StateInfo state_info[] = {
#define DEF_STATE_INFO(stateEnum, init_fn, update_fn, exit_fn) \
    {.on_init = init_fn, .on_update = update_fn, .on_exit = exit_fn},
#include "../include/def_state_info_table.h"
#undef DEF_STATE_INFO
};

static StateMachine game_sm = {
    .state_infos = &state_info[0],
    .num_infos = GAME_STATE_MAX,
};

// Initialization of the global vars
// clang-format off
GameVariables g_game_vars = {
    .timer = 0, .rng_info = {0, 0},

    .round = 0, .ante = 0, .money = 0,
    .hand_size = DEFAULT_HAND_SIZE,

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
    .music_volume = DEFAULT_MUSIC_VOLUME,
    .sound_volume = DEFAULT_SOUND_VOLUME,
};
// clang-format on

static bool retrigger = false;

// Keeping track of cards scored

// Keeping track of what Jokers are scored at each step
static ListItr _joker_scored_itr;
static ListItr _joker_card_scored_end_itr;
static ListItr _joker_round_end_itr;

static List _owned_jokers_list;
static List _discarded_jokers_list;
static List _expired_jokers_list;

BITSET_DEFINE(_avail_jokers_bitset, MAX_DEFINABLE_JOKERS)
static List _shop_jokers_list;

// Stacks

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

static inline void jokers_available_to_shop_init(void)
{
    reset_shop_jokers();
}

void game_init()
{
    state_machine_remove(&game_sm);
    state_machine_register(&game_sm);
    // Initialize all jokers list once
    _owned_jokers_list = list_init();
    _discarded_jokers_list = list_init();
    _expired_jokers_list = list_init();
    _shop_jokers_list = list_init();
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
    rng_update();

    g_game_vars.timer++;

    jokers_update_loop();

    state_machine_update();
}

void game_change_state(enum GameState new_game_state)
{
    g_game_vars.timer = TM_ZERO; // Reset the timer

    state_machine_change_state(&game_sm, new_game_state);
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
            else if (g_game_vars.current_blind >= BLIND_TYPE_BOSS)
            {
                main_bg_se_copy_rect(BOSS_BLIND_TITLE_SRC_RECT, TOP_LEFT_BLIND_TITLE_POINT);
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
                MM_BASE_PITCH_RATE + rng_get_u32() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
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
                MM_BASE_PITCH_RATE + rng_get_u32() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
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
        compute_hand_value_info();
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


void game_start(void)
{
    rng_shuffle_seed();

    affine_background_change_background(AFFINE_BG_GAME);

    g_game_vars.hands = MAX_HANDS;
    g_game_vars.discards = MAX_DISCARDS;

    // Activate high contrast palette for cards if loaded settings tell us to
    toggle_high_contrast_cards(g_game_vars.high_contrast);

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
