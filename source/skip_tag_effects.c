#include "game.h"
#include "game/blind_select.h"
#include "game/common_ui.h"
#include "game/shop.h"
#include "game_variables.h"
#include "hand.h"
#include "joker.h"
#include "list.h"
#include "round_end.h"
#include "skip_tag.h"
#include "util.h"

#include <stdlib.h>
#include <tonc.h>

#define DECLARE_SKIP_TAG_CONDITION_FUNC(tag_condition_name) static bool tag_condition_name(void);

#define DECLARE_SKIP_TAG_EFFECT_FUNC(tag_effect_name) static void tag_effect_name(void);

DECLARE_SKIP_TAG_CONDITION_FUNC(skip_tag_cond_true)
DECLARE_SKIP_TAG_CONDITION_FUNC(skip_tag_cond_double)
DECLARE_SKIP_TAG_CONDITION_FUNC(skip_tag_cond_juggle)
DECLARE_SKIP_TAG_CONDITION_FUNC(skip_tag_cond_d6)
DECLARE_SKIP_TAG_CONDITION_FUNC(skip_tag_cond_investment)

DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_noop)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_boss)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_handy)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_garbage)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_double)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_juggle)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_d6)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_top_up)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_speed)
DECLARE_SKIP_TAG_EFFECT_FUNC(skip_tag_effect_economy)

// The index of a skip tag in the registry matches its ID.
// clang-format off
const SkipTagInfo skip_tag_registry[MAX_SKIP_TAG_TYPES] = 
{
    [SKIP_TAG_TYPE_INVESTMENT] = { SKIP_TAG_EVENT_ON_ROUND_END,   skip_tag_cond_investment, skip_tag_effect_noop    },
    [SKIP_TAG_TYPE_BOSS]       = { SKIP_TAG_EVENT_IMMEDIATE,      skip_tag_cond_true,       skip_tag_effect_boss    },
    [SKIP_TAG_TYPE_HANDY]      = { SKIP_TAG_EVENT_IMMEDIATE,      skip_tag_cond_true,       skip_tag_effect_handy   },
    [SKIP_TAG_TYPE_GARBAGE]    = { SKIP_TAG_EVENT_IMMEDIATE,      skip_tag_cond_true,       skip_tag_effect_garbage },
    [SKIP_TAG_TYPE_DOUBLE]     = { SKIP_TAG_EVENT_IMMEDIATE,      skip_tag_cond_double,     skip_tag_effect_double  },
    [SKIP_TAG_TYPE_JUGGLE]     = { SKIP_TAG_EVENT_ON_ROUND_START, skip_tag_cond_juggle,     skip_tag_effect_juggle  },
    [SKIP_TAG_TYPE_D6]         = { SKIP_TAG_EVENT_ON_SHOP_INIT,   skip_tag_cond_d6,         skip_tag_effect_d6      },
    [SKIP_TAG_TYPE_TOP_UP]     = { SKIP_TAG_EVENT_IMMEDIATE,      skip_tag_cond_true,       skip_tag_effect_top_up  },
    [SKIP_TAG_TYPE_SPEED]      = { SKIP_TAG_EVENT_IMMEDIATE,      skip_tag_cond_true,       skip_tag_effect_speed   },
    [SKIP_TAG_TYPE_ECONOMY]    = { SKIP_TAG_EVENT_IMMEDIATE,      skip_tag_cond_true,       skip_tag_effect_economy },
};
// clang-format on

static const size_t skip_tag_registry_size = NUM_ELEM_IN_ARR(skip_tag_registry);

const SkipTagInfo* get_skip_tag_registry_entry(int tag_id)
{
    if (tag_id < 0 || (size_t)tag_id >= skip_tag_registry_size)
    {
        return NULL;
    }
    return &skip_tag_registry[tag_id];
}

// CONDITIONS IMPLEMENTATION

static bool skip_tag_cond_true(void)
{
    return true;
}

static bool skip_tag_cond_investment(void)
{
    return g_game_vars.current_blind >= BLIND_TYPE_BOSS;
}

static bool skip_tag_cond_double(void)
{
    SkipTag* latest_tag = get_owned_skip_tags()->tail->data;
    return (latest_tag != NULL) && (latest_tag->type != SKIP_TAG_TYPE_DOUBLE);
}

static bool skip_tag_cond_juggle(void)
{
    return g_game_vars.hand_size < MAX_HAND_SIZE;
}

static bool skip_tag_cond_d6(void)
{
    return game_shop_get_reroll_cost() > 0;
}

// EFFECTS IMPLEMENTATION

static void skip_tag_effect_noop(void)
{
}

static void skip_tag_effect_boss(void)
{
    game_blind_select_reroll_boss_from_menu();
}

static void skip_tag_effect_handy(void)
{
    for (int i = 0; i < HAND_TYPE_MAX; i++)
        g_game_vars.money += g_game_vars.nb_played_hands[i];
    display_money();
}

static void skip_tag_effect_garbage(void)
{
    g_game_vars.money += g_game_vars.nb_unused_discards;
    display_money();
}

static void skip_tag_effect_double(void)
{
    // If the double tag can activate, then we have already checked the latest
    // tag exists and isn't a double tag
    SkipTag* latest_tag = get_owned_skip_tags()->tail->data;
    SkipTag* latest_tag_copy = skip_tag_new(latest_tag->type);
    add_skip_tag(&latest_tag_copy);
}

static void skip_tag_effect_juggle(void)
{
    s32 new_hand_size = g_game_vars.hand_size + 3;
    g_game_vars.hand_size = (new_hand_size > MAX_HAND_SIZE) ? MAX_HAND_SIZE : new_hand_size;
}

static void skip_tag_effect_d6(void)
{
    game_shop_set_reroll_cost(0);
}

#define TOP_UP_TAG_JOKER_BONUS 2
static void skip_tag_effect_top_up(void)
{
    int available_joker_slots = MAX_JOKERS_HELD_SIZE - list_get_len(get_jokers_list());
    if (available_joker_slots > TOP_UP_TAG_JOKER_BONUS)
        available_joker_slots = TOP_UP_TAG_JOKER_BONUS;

    for (int i = 0; i < available_joker_slots; i++)
    {
        int joker_id = joker_roll_id_with_rarity(COMMON_JOKER);

        // Something went wrong, maybe there are no more Common Jokers available
        if (joker_id == UNDEFINED)
            break;

        joker_set_rollable(joker_id, false);
        add_joker(joker_object_new(joker_new(joker_id)));
    }
}

#define SPEED_TAG_MONEY_BONUS 5
static void skip_tag_effect_speed(void)
{
    g_game_vars.money += g_game_vars.nb_skipped_rounds * SPEED_TAG_MONEY_BONUS;
    display_money();
}

static void skip_tag_effect_economy(void)
{
    int added_money = 0;
    if (g_game_vars.money > 0)
        added_money = min(g_game_vars.money, 40);

    g_game_vars.money += added_money;
    display_money();
}
