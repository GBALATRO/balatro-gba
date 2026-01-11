#include "game.h"
#include "hand_analysis.h"
#include "joker.h"
#include "list.h"
#include "pool.h"
#include "util.h"

#include <stdlib.h>

#define MISPRINT_MAX_MULT 23

#define SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, restricted_event, checked_event) \
    if (checked_event != restricted_event || scored_card == NULL)                   \
    {                                                                               \
        return JOKER_EFFECT_FLAG_NONE;                                              \
    }
#define SCORE_ON_EVENT_ONLY(restricted_event, checked_event) \
    if (checked_event != restricted_event)                   \
    {                                                        \
        return JOKER_EFFECT_FLAG_NONE;                       \
    }

static JokerEffect shared_joker_effect = {0};

// Joker Effect functions
static u32 joker_effect_noop(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 default_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 sinful_joker_effect(
    Card* scored_card,
    u8 sinful_suit,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 greedy_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 lusty_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 wrathful_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 gluttonous_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 jolly_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 zany_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 mad_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 crazy_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 droll_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 sly_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 wily_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 clever_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 devious_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 crafty_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 half_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 stencil_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 misprint_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 walkie_talkie_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 fibonnaci_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 banner_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 mystic_summit_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 blackboard_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 blue_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 raised_fist_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 reserved_parking_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 business_card_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 scholar_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 scary_face_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 abstract_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 bull_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 smiley_face_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 even_steven_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 odd_todd_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 acrobat_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 hanging_chad_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 the_duo_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 the_trio_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 the_family_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 the_order_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 the_tribe_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 bootstraps_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 shoot_the_moon_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 photograph_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 dusk_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 blueprint_brainstorm_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 hack_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 seltzer_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);
static u32 sock_and_buskin_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

// clang-format off
/* The index of a joker in the registry matches its ID.
 * The joker sprites are matched by ID so the position in the registry
 * determines the joker's sprite.
 * Each consecutive NUM_JOKERS_PER_SPRITESHEET (defined in joker.c) jokers
 * share a spritesheet and thus a color palette.
 * To make better use of color palettes jokers may be rearranged here
 * (and put together in the matching spritesheet) to share a color palette.
 * Otherwise the order is similar to the wiki.
 */
// clang-format off
const JokerInfo joker_registry[] = 
{
    // Spritesheet 0
    { COMMON_JOKER,    2, default_joker_effect              }, // DEFAULT_JOKER_ID = 0
    { COMMON_JOKER,    4, abstract_joker_effect             }, // 1
    { COMMON_JOKER,    5, half_joker_effect                 }, // 2
    { COMMON_JOKER,    4, misprint_joker_effect             }, // 3
    { COMMON_JOKER,    4, scary_face_joker_effect           }, // 4
    { UNCOMMON_JOKER,  6, sock_and_buskin_joker_effect      }, // 5
    { UNCOMMON_JOKER,  6, acrobat_joker_effect              }, // 6
    { UNCOMMON_JOKER,  8, fibonnaci_joker_effect            }, // 7
    { COMMON_JOKER,    4, scholar_joker_effect              }, // 8
    { COMMON_JOKER,    4, droll_joker_effect                }, // 9
    { COMMON_JOKER,    4, crafty_joker_effect               }, // 10
    { COMMON_JOKER,    5, raised_fist_joker_effect          }, // 11
    { COMMON_JOKER,    6, reserved_parking_joker_effect     }, // 12
    { COMMON_JOKER,    4, business_card_joker_effect        }, // 13
    { COMMON_JOKER,    4, hanging_chad_joker_effect         }, // 14
    { UNCOMMON_JOKER,  8, stencil_joker_effect              }, // 15
    { COMMON_JOKER,    5, banner_joker_effect               }, // 16
    { COMMON_JOKER,    5, shoot_the_moon_joker_effect,      }, // 17
    // Spritesheet 1 
    { COMMON_JOKER,    5, greedy_joker_effect               }, // 18
    { COMMON_JOKER,    5, lusty_joker_effect                }, // 19
    // Spritesheet 2
    { COMMON_JOKER,    5, wrathful_joker_effect             }, // 20
    { COMMON_JOKER,    5, gluttonous_joker_effect           }, // 21
    // Spritesheet 3
    { COMMON_JOKER,    4, crazy_joker_effect                }, // 22
    { COMMON_JOKER,    4, mad_joker_effect                  }, // 23
    { COMMON_JOKER,    4, clever_joker_effect               }, // 24
    { COMMON_JOKER,    4, devious_joker_effect              }, // 25
    { COMMON_JOKER,    4, even_steven_joker_effect          }, // 26
    // Spritesheet 4
    { UNCOMMON_JOKER,  6, blackboard_joker_effect           }, // 27
    { COMMON_JOKER,    5, mystic_summit_joker_effect        }, // 28
    { COMMON_JOKER,    4, walkie_talkie_joker_effect        }, // 29
    { COMMON_JOKER,    4, zany_joker_effect                 }, // 30
    { COMMON_JOKER,    4, wily_joker_effect                 }, // 31
    // Spritesheet 5
    { COMMON_JOKER,    3, sly_joker_effect                  }, // 32
    { COMMON_JOKER,    3, jolly_joker_effect                }, // 33
    { COMMON_JOKER,    5, blue_joker_effect                 }, // 34
    { COMMON_JOKER,    4, odd_todd_joker_effect             }, // 35
    // Spritesheet 6
    { RARE_JOKER,      8, the_duo_joker_effect              }, // 36
    { RARE_JOKER,      8, the_trio_joker_effect             }, // 37
    { RARE_JOKER,      8, the_order_joker_effect            }, // 38
    { RARE_JOKER,      8, the_tribe_joker_effect            }, // 39
    // Spritesheet 7
    { RARE_JOKER,      8, the_family_joker_effect           }, // 40
    { RARE_JOKER,     10, blueprint_brainstorm_joker_effect }, // 41 Brainstorm
    // Spritesheet 8
    { COMMON_JOKER,    4, smiley_face_joker_effect          }, // 42
    { UNCOMMON_JOKER,  6, bull_joker_effect                 }, // 43
    // Individual Jokers (for now :3)
    { COMMON_JOKER,    5, photograph_joker_effect,          }, // 44
    { UNCOMMON_JOKER,  6, hack_joker_effect                 }, // 45
    { UNCOMMON_JOKER,  5, joker_effect_noop                 }, // 46 Pareidolia
    { UNCOMMON_JOKER,  7, bootstraps_joker_effect           }, // 47
    { UNCOMMON_JOKER,  7, joker_effect_noop,                }, // 48 Shortcut
    { UNCOMMON_JOKER,  5, dusk_joker_effect                 }, // 49
    { UNCOMMON_JOKER,  7, joker_effect_noop,                }, // 50 Four Fingers
    { UNCOMMON_JOKER,  6, seltzer_joker_effect,             }, // 51
    { RARE_JOKER,     10, blueprint_brainstorm_joker_effect }, // 52 Blueprint

    // The following jokers don't have sprites yet,
    // uncomment them when their sprites are added.
#if 0
#endif
};
// clang-format on

static const size_t joker_registry_size = NUM_ELEM_IN_ARR(joker_registry);

const JokerInfo* get_joker_registry_entry(int joker_id)
{
    if (joker_id < 0 || (size_t)joker_id >= joker_registry_size)
    {
        return NULL;
    }
    return &joker_registry[joker_id];
}

size_t get_joker_registry_size(void)
{
    return joker_registry_size;
}

static u32 joker_effect_noop(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return JOKER_EFFECT_FLAG_NONE;
}

static u32 default_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)
    *joker_effect = &shared_joker_effect;

    (*joker_effect)->mult = 4;

    return JOKER_EFFECT_FLAG_MULT;
}

static u32 sinful_joker_effect(
    Card* scored_card,
    u8 sinful_suit,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (scored_card->suit == sinful_suit)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }
    return effect_flags_ret;
}

static u32 greedy_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return sinful_joker_effect(scored_card, DIAMONDS, joker_event, joker_effect);
}

static u32 lusty_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return sinful_joker_effect(scored_card, HEARTS, joker_event, joker_effect);
}

static u32 wrathful_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return sinful_joker_effect(scored_card, SPADES, joker_event, joker_effect);
}

static u32 gluttonous_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    return sinful_joker_effect(scored_card, CLUBS, joker_event, joker_effect);
}

static u32 jolly_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    // This is really inefficient but the only way at the moment to check for whole-hand conditions
    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_n_of_a_kind(ranks) >= 2)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 8;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 zany_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // This is really inefficient but the only way at the moment to check for whole-hand conditions
    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_n_of_a_kind(ranks) >= 3)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 12;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 mad_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_two_pair(ranks))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 10;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 crazy_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_straight(ranks))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 12;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 droll_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_flush(suits))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 10;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 sly_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_n_of_a_kind(ranks) >= 2)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 50;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 wily_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_n_of_a_kind(ranks) >= 3)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 100;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 clever_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_two_pair(ranks))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 80;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 devious_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_straight(ranks))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 100;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 crafty_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_flush(suits))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 80;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 half_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    int played_size = get_played_top() + 1;
    if (played_size <= 3)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 20;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 stencil_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    *joker_effect = &shared_joker_effect;

    List* jokers = get_jokers_list();

    // +1 xmult per empty joker slot...
    int num_jokers = list_get_len(jokers);

    (*joker_effect)->xmult = (MAX_JOKERS_HELD_SIZE)-num_jokers;

    // ...and also each stencil_joker adds +1 xmult
    ListItr itr = list_itr_create(jokers);
    JokerObject* joker_object;

    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object->joker->id == STENCIL_JOKER_ID)
            (*joker_effect)->xmult++;
    }

    return JOKER_EFFECT_FLAG_XMULT;
}

#define MISPRINT_MAX_MULT 23
static u32 misprint_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    *joker_effect = &shared_joker_effect;

    (*joker_effect)->mult = random() % (MISPRINT_MAX_MULT + 1);

    return JOKER_EFFECT_FLAG_MULT;
}

static u32 walkie_talkie_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (scored_card->rank == TEN || scored_card->rank == FOUR)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 10;
        (*joker_effect)->mult = 4;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS | JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 fibonnaci_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    switch (scored_card->rank)
    {
        case ACE:
        case TWO:
        case THREE:
        case FIVE:
        case EIGHT:
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->mult = 8;
            effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
            break;
        default:
            break;
    }

    return effect_flags_ret;
}

static u32 banner_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_num_discards_remaining() > 0)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 30 * get_num_discards_remaining();
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 mystic_summit_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (get_num_discards_remaining() == 0)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 15;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 blackboard_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    bool all_cards_are_spades_or_clubs = true;
    CardObject** hand = get_hand_array();
    int hand_size = hand_get_size();
    for (int i = 0; i < hand_size; i++)
    {
        u8 suit = hand[i]->card->suit;
        if (suit == HEARTS || suit == DIAMONDS)
        {
            all_cards_are_spades_or_clubs = false;
            break;
        }
    }

    if (all_cards_are_spades_or_clubs)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->xmult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 blue_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    *joker_effect = &shared_joker_effect;

    (*joker_effect)->chips = (get_deck_top() + 1) * 2;

    return JOKER_EFFECT_FLAG_CHIPS;
}

static u32 raised_fist_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    s32* p_lowest_value_index = &(joker->scoring_state);

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    switch (joker_event)
    {
        // Use this event to compute the index of the lowest value card only once.
        // Aces are always considered high value, even in an ace-low straight
        case JOKER_EVENT_ON_HAND_PLAYED:
            // index initialized at 0 but accessed only if
            // hand_size > 0 so we're never out of bounds
            *p_lowest_value_index = 0;
            u8 lowest_value = IMPOSSIBLY_HIGH_CARD_VALUE;
            CardObject** hand = get_hand_array();
            int hand_size = hand_get_size();
            for (int i = 0; i < hand_size; i++)
            {
                u8 value = card_get_value(hand[i]->card);
                if (lowest_value > value)
                {
                    *p_lowest_value_index = i;
                    lowest_value = value;
                }
            }
            break;

        case JOKER_EVENT_ON_CARD_HELD:
            if (get_scored_card_index() == *p_lowest_value_index)
            {
                *joker_effect = &shared_joker_effect;

                (*joker_effect)->mult = 2 * card_get_value(scored_card);
                effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 reserved_parking_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_ON_CARD_HELD, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if ((random() % 2 == 0) && card_is_face(scored_card))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->money = 1;
        effect_flags_ret = JOKER_EFFECT_FLAG_MONEY;
    }

    return effect_flags_ret;
};

static u32 business_card_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if ((random() % 2 == 0) && card_is_face(scored_card))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->money = 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_MONEY;
    }

    return effect_flags_ret;
}

static u32 scholar_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (scored_card->rank == ACE)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 20;
        (*joker_effect)->mult = 4;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS | JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 scary_face_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (card_is_face(scored_card))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 30;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 abstract_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    *joker_effect = &shared_joker_effect;

    // +1 xmult per occupied joker slot
    int num_jokers = list_get_len(get_jokers_list());

    (*joker_effect)->mult = num_jokers * 3;

    return JOKER_EFFECT_FLAG_MULT;
}

static u32 bull_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // The wiki says it does nothing if money is 0 or below
    // This allows us to avoid scoring negative Chips
    if (get_money() > 0)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = get_money() * 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 smiley_face_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (card_is_face(scored_card))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 5;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 even_steven_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    switch (scored_card->rank)
    {
        case KING:
        case QUEEN:
        case JACK:
            break;
        default:
            if (card_get_value(scored_card) % 2 == 0)
            {
                *joker_effect = &shared_joker_effect;

                (*joker_effect)->mult = 4;
                effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
            }
            break;
    }

    return effect_flags_ret;
}

static u32 odd_todd_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY_WITH_CARD(scored_card, JOKER_EVENT_ON_CARD_SCORED, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (card_get_value(scored_card) % 2 == 1) // todo test ace
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->chips = 31;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }

    return effect_flags_ret;
}

static u32 acrobat_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // 0 remaining hands mean we're scoring the last hand
    if (get_num_hands_remaining() == 0)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->xmult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 hanging_chad_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    s32* p_remaining_retriggers = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_remaining_retriggers = 2;
            break;

        // No need to check if this is the first card scored or not
        // p_remaining_retriggers will always reach 0 on the first card, then retrigger
        // will be false and scoring will go onto the next card
        case JOKER_EVENT_ON_CARD_SCORED_END:
            *joker_effect = &shared_joker_effect;

            (*joker_effect)->retrigger = (*p_remaining_retriggers > 0);
            if ((*joker_effect)->retrigger)
            {
                *p_remaining_retriggers -= 1;
                (*joker_effect)->message = "Again!";
                effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 the_duo_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // This is really inefficient but the only way at the moment to check for whole-hand conditions
    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_n_of_a_kind(ranks) >= 2)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->xmult = 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 the_trio_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // This is really inefficient but the only way at the moment to check for whole-hand conditions
    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_n_of_a_kind(ranks) >= 3)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->xmult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 the_family_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // This is really inefficient but the only way at the moment to check for whole-hand conditions
    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_n_of_a_kind(ranks) >= 4)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->xmult = 4;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 the_order_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_straight(ranks))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->xmult = 3;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 the_tribe_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    u8 suits[NUM_SUITS];
    u8 ranks[NUM_RANKS];
    get_played_distribution(ranks, suits);

    if (hand_contains_flush(suits))
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->xmult = 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }

    return effect_flags_ret;
}

static u32 bootstraps_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_INDEPENDENT, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // Same protection as the Bull Joker
    if (get_money() > 0)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = (get_money() / 5) * 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 shoot_the_moon_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    SCORE_ON_EVENT_ONLY(JOKER_EVENT_ON_CARD_HELD, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (scored_card->rank == QUEEN)
    {
        *joker_effect = &shared_joker_effect;

        (*joker_effect)->mult = 13;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    return effect_flags_ret;
}

static u32 photograph_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_first_face_index = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_first_face_index = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED:
            // has a face card been encountered already, and if not, is the current scoring card a
            // face card?
            if (*p_first_face_index == UNDEFINED && card_is_face(scored_card))
            {
                *p_first_face_index = get_scored_card_index();
            }
            // if we have a face card index saved, check against it and give mult accordingly
            // Doing this now will trigger the effect the first time we encounter the face card,
            // and we will catch potential retriggers
            if (*p_first_face_index == get_scored_card_index())
            {
                *joker_effect = &shared_joker_effect;

                (*joker_effect)->xmult = 2;
                effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
            }
            break;
        default:
            break;
    }

    return effect_flags_ret;
}

static u32 dusk_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_index = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            // start at -1 so that a first index of 0 can satisfy the retrigger condition below
            *p_last_retriggered_index = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED_END:
            // Only retrigger current card if it's strictly after the last one we retriggered
            if (get_num_hands_remaining() == 0)
            {
                *joker_effect = &shared_joker_effect;

                (*joker_effect)->retrigger = (*p_last_retriggered_index < get_scored_card_index());
                if ((*joker_effect)->retrigger)
                {
                    *p_last_retriggered_index = get_scored_card_index();
                    (*joker_effect)->message = "Again!";
                    effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
                }
            }

            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 blueprint_brainstorm_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    // No need for this kind of init since these Jokers
    // will have their data copied when needed
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED ||
        joker_event == JOKER_EVENT_ON_HAND_SCORED_END || joker_event == JOKER_EVENT_ON_ROUND_END)
    {
        return effect_flags_ret;
    }

    // find ourselves in the Jokers list
    List* jokers = get_jokers_list();
    ListItr itr = list_itr_create(jokers);
    JokerObject* copied_joker_object;
    while ((copied_joker_object = list_itr_next(&itr)))
    {
        if (copied_joker_object->joker == joker)
        {
            break;
        }
    }

    // This shouldn't happen since if we are a scoring Joker, we should always
    // be part of the Jokers list, but being extra careful doesn't cost much
    if (copied_joker_object == NULL)
    {
        return effect_flags_ret;
    }

    // find the copied Joker, may need to bounce around Blueprints and a Brainstorm
    // If we encounter NULL, we have a Blueprint at the end of the list that can't copy anything.
    // If we go through a Brainstorms twice, we will be in a loop and need to exit
    u8 brainstorm_counter = 0;
    do
    {
        switch (copied_joker_object->joker->id)
        {
            // get the next Joker for Blueprint
            case BLUEPRINT_JOKER_ID:
                copied_joker_object = list_itr_next(&itr);
                break;

            // Get the first (leftmost) Joker for Brainstorm
            case BRAINSTORM_JOKER_ID:
                brainstorm_counter++;
                itr = list_itr_create(jokers);
                copied_joker_object = list_itr_next(&itr);
                break;

            // We encountered a Joker that isn't a Copying Joker and copy it now
            // but how we copy it depends on this Joker's ID because they don't
            // all handle data the same way.
            default:
                u8 copied_joker_id = copied_joker_object->joker->id;
                const JokerInfo* copied_joker_info = get_joker_registry_entry(copied_joker_id);

                // Copy the persistent data
                joker->persistent_state = copied_joker_object->joker->persistent_state;

                // Then regardless of if we copied the data above, apply the
                // copied JokerEffect function to the local data
                effect_flags_ret =
                    copied_joker_info
                        ->joker_effect_func(joker, scored_card, joker_event, joker_effect);

                // make also sure we don't expire
                effect_flags_ret &= ~JOKER_EFFECT_FLAG_EXPIRE;

                // exit the loop
                copied_joker_object = NULL;

                break;
        }
    } while (copied_joker_object != NULL && brainstorm_counter < 2);

    return effect_flags_ret;
}

static u32 hack_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_index = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_last_retriggered_index = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED_END:
            // Works the same way as Dusk, but check what rank the card is
            switch (scored_card->rank)
            {
                case TWO:
                case THREE:
                case FOUR:
                case FIVE:
                    *joker_effect = &shared_joker_effect;

                    (*joker_effect)->retrigger =
                        (*p_last_retriggered_index < get_scored_card_index());
                    if ((*joker_effect)->retrigger)
                    {
                        *p_last_retriggered_index = get_scored_card_index();
                        (*joker_effect)->message = "Again!";
                        effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
                    }
                    break;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 seltzer_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_idx = &(joker->scoring_state);
    s32* p_hands_left_until_exp = &(joker->persistent_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_JOKER_CREATED:
            *p_hands_left_until_exp = 10; // remaining retriggered hands
            break;

        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_last_retriggered_idx = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED_END:
            // Works the same way as Dusk
            // No need to check for p_hands_left_until_exp because the Joker
            // will be destroyed the moment we hit 0
            *joker_effect = &shared_joker_effect;

            (*joker_effect)->retrigger = ((*p_last_retriggered_idx) < get_scored_card_index());
            if ((*joker_effect)->retrigger)
            {
                *p_last_retriggered_idx = get_scored_card_index();
                (*joker_effect)->message = "Again!";
                effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
            }
            break;

        case JOKER_EVENT_ON_HAND_SCORED_END:
            *joker_effect = &shared_joker_effect;
            effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;

            (*p_hands_left_until_exp)--;
            if (*p_hands_left_until_exp > 0)
            {
                // Need to do this for now because the message's memory can't really be allocated
                // So we can't use snprintf to craft a message depending on the number of hands left
                static const char* seltzer_messages[] =
                    {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
                (*joker_effect)->message = (char*)seltzer_messages[(*p_hands_left_until_exp) - 1];
            }
            else
            {
                (*joker_effect)->message = "Drank!";
                (*joker_effect)->expire = true;
                effect_flags_ret |= JOKER_EFFECT_FLAG_EXPIRE;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}

static u32 sock_and_buskin_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_face_index = &(joker->scoring_state);

    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            *p_last_retriggered_face_index = UNDEFINED;
            break;

        case JOKER_EVENT_ON_CARD_SCORED_END:
            *joker_effect = &shared_joker_effect;

            // Works the same way as Dusk, but for face cards
            (*joker_effect)->retrigger =
                ((*p_last_retriggered_face_index < get_scored_card_index()) &&
                 card_is_face(scored_card));
            if ((*joker_effect)->retrigger)
            {
                *p_last_retriggered_face_index = get_scored_card_index();
                (*joker_effect)->message = "Again!";
                effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
            }
            break;

        default:
            break;
    }

    return effect_flags_ret;
}
