#include "game.h"
#include "game/round.h"
#include "game_variables.h"
#include "hand.h"
#include "joker.h"
#include "list.h"
#include "pool.h"
#include "random.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MISPRINT_MAX_MULT 23

#define REGISTER_JOKER_DESC_FUNC(joker_desc_name) \
    static int joker_desc_name(Joker* joker, Rect dest_rect);

#define REGISTER_JOKER_EFFECT_FUNC(joker_effect_name) \
    static u32 joker_effect_name(                     \
        Joker* joker,                                 \
        Card* scored_card,                            \
        enum JokerEvent joker_event,                  \
        JokerEffect** joker_effect                    \
    );

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

static JokerEffect s_shared_joker_effect = {0};

// Joker Descriptions

REGISTER_JOKER_DESC_FUNC(default_joker_desc)
REGISTER_JOKER_DESC_FUNC(greedy_joker_desc)
REGISTER_JOKER_DESC_FUNC(lusty_joker_desc)
REGISTER_JOKER_DESC_FUNC(wrathful_joker_desc)
REGISTER_JOKER_DESC_FUNC(gluttonous_joker_desc)
REGISTER_JOKER_DESC_FUNC(jolly_joker_desc)
REGISTER_JOKER_DESC_FUNC(zany_joker_desc)
REGISTER_JOKER_DESC_FUNC(mad_joker_desc)
REGISTER_JOKER_DESC_FUNC(crazy_joker_desc)
REGISTER_JOKER_DESC_FUNC(droll_joker_desc)
REGISTER_JOKER_DESC_FUNC(sly_joker_desc)
REGISTER_JOKER_DESC_FUNC(wily_joker_desc)
REGISTER_JOKER_DESC_FUNC(clever_joker_desc)
REGISTER_JOKER_DESC_FUNC(devious_joker_desc)
REGISTER_JOKER_DESC_FUNC(crafty_joker_desc)
REGISTER_JOKER_DESC_FUNC(half_joker_desc)
REGISTER_JOKER_DESC_FUNC(stencil_joker_desc)
REGISTER_JOKER_DESC_FUNC(misprint_joker_desc)
REGISTER_JOKER_DESC_FUNC(walkie_talkie_joker_desc)
REGISTER_JOKER_DESC_FUNC(fibonnaci_joker_desc)
REGISTER_JOKER_DESC_FUNC(banner_joker_desc)
REGISTER_JOKER_DESC_FUNC(mystic_summit_joker_desc)
REGISTER_JOKER_DESC_FUNC(blackboard_joker_desc)
REGISTER_JOKER_DESC_FUNC(blue_joker_desc)
REGISTER_JOKER_DESC_FUNC(raised_fist_joker_desc)
REGISTER_JOKER_DESC_FUNC(reserved_parking_joker_desc)
REGISTER_JOKER_DESC_FUNC(business_card_joker_desc)
REGISTER_JOKER_DESC_FUNC(scholar_joker_desc)
REGISTER_JOKER_DESC_FUNC(scary_face_joker_desc)
REGISTER_JOKER_DESC_FUNC(abstract_joker_desc)
REGISTER_JOKER_DESC_FUNC(bull_joker_desc)
REGISTER_JOKER_DESC_FUNC(smiley_face_joker_desc)
REGISTER_JOKER_DESC_FUNC(even_steven_joker_desc)
REGISTER_JOKER_DESC_FUNC(odd_todd_joker_desc)
REGISTER_JOKER_DESC_FUNC(acrobat_joker_desc)
REGISTER_JOKER_DESC_FUNC(hanging_chad_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_duo_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_trio_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_family_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_order_joker_desc)
REGISTER_JOKER_DESC_FUNC(the_tribe_joker_desc)
REGISTER_JOKER_DESC_FUNC(bootstraps_joker_desc)
REGISTER_JOKER_DESC_FUNC(shoot_the_moon_joker_desc)
REGISTER_JOKER_DESC_FUNC(pareidolia_joker_desc)
REGISTER_JOKER_DESC_FUNC(photograph_joker_desc)
REGISTER_JOKER_DESC_FUNC(dusk_joker_desc)
REGISTER_JOKER_DESC_FUNC(shortcut_joker_desc)
REGISTER_JOKER_DESC_FUNC(blueprint_joker_desc)
REGISTER_JOKER_DESC_FUNC(brainstorm_joker_desc)
REGISTER_JOKER_DESC_FUNC(hack_joker_desc)
REGISTER_JOKER_DESC_FUNC(four_fingers_joker_desc)
REGISTER_JOKER_DESC_FUNC(seltzer_joker_desc)
REGISTER_JOKER_DESC_FUNC(sock_and_buskin_joker_desc)

// Joker Effect functions

static u32 sinful_joker_effect(
    Card* scored_card,
    u8 sinful_suit,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
);

REGISTER_JOKER_EFFECT_FUNC(joker_effect_noop)
REGISTER_JOKER_EFFECT_FUNC(default_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(greedy_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(lusty_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(wrathful_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(gluttonous_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(jolly_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(zany_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(mad_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(crazy_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(droll_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(sly_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(wily_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(clever_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(devious_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(crafty_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(half_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(stencil_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(misprint_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(walkie_talkie_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(fibonnaci_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(banner_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(mystic_summit_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(blackboard_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(blue_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(raised_fist_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(reserved_parking_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(business_card_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(scholar_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(scary_face_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(abstract_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(bull_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(smiley_face_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(even_steven_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(odd_todd_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(acrobat_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(hanging_chad_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_duo_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_trio_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_family_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_order_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(the_tribe_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(bootstraps_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(shoot_the_moon_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(photograph_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(dusk_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(blueprint_brainstorm_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(hack_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(seltzer_joker_effect)
REGISTER_JOKER_EFFECT_FUNC(sock_and_buskin_joker_effect)

/* The index of a joker in the registry matches its ID.
 *
 * The joker sprites are matched by ID so the position in the registry
 * determines the joker's sprite.
 *
 * Each consecutive NUM_JOKERS_PER_SPRITESHEET (defined in joker.c) jokers
 * share a spritesheet and thus a color palette.
 *
 * To make better use of color palettes jokers may be rearranged here
 * (and put together in the matching spritesheet) to share a color palette.
 * Otherwise the order is similar to the wiki.
 *
 * TODO: move Name and Description printing out of this when the CardInstance is implemented.
 */
// clang-format off
const JokerInfo joker_registry[] = 
{
    // Spritesheet 0
    { "Joker",            COMMON_JOKER,    2, false, default_joker_desc,          default_joker_effect              }, // DEFAULT_JOKER_ID = 0
    { "Abstract Joker",   COMMON_JOKER,    4, false, abstract_joker_desc,         abstract_joker_effect             }, // 1
    { "Half Joker",       COMMON_JOKER,    5, false, half_joker_desc,             half_joker_effect                 }, // 2
    { "Misprint",         COMMON_JOKER,    4, true,  misprint_joker_desc,         misprint_joker_effect             }, // 3
    { "Scary Face",       COMMON_JOKER,    4, false, scary_face_joker_desc,       scary_face_joker_effect           }, // 4
    { "Sock and Buskin",  UNCOMMON_JOKER,  6, false, sock_and_buskin_joker_desc,  sock_and_buskin_joker_effect      }, // 5
    { "Acrobat",          UNCOMMON_JOKER,  6, false, acrobat_joker_desc,          acrobat_joker_effect              }, // 6
    { "Fibonacci",        UNCOMMON_JOKER,  8, false, fibonnaci_joker_desc,        fibonnaci_joker_effect            }, // 7
    { "Scholar",          COMMON_JOKER,    4, false, scholar_joker_desc,          scholar_joker_effect              }, // 8
    { "Crafty Joker",     COMMON_JOKER,    4, false, crafty_joker_desc,           crafty_joker_effect               }, // 9
    { "Droll Joker",      COMMON_JOKER,    4, false, droll_joker_desc,            droll_joker_effect                }, // 10
    { "Raised Fist",      COMMON_JOKER,    5, false, raised_fist_joker_desc,      raised_fist_joker_effect          }, // 11
    { "Reserved Parking", COMMON_JOKER,    6, false, reserved_parking_joker_desc, reserved_parking_joker_effect     }, // 12
    { "Business Card",    COMMON_JOKER,    4, false, business_card_joker_desc,    business_card_joker_effect        }, // 13
    { "Hanging Chad",     COMMON_JOKER,    4, false, hanging_chad_joker_desc,     hanging_chad_joker_effect         }, // 14
    { "Joker Stencil",    UNCOMMON_JOKER,  8, false, stencil_joker_desc,          stencil_joker_effect              }, // 15
    { "Banner",           COMMON_JOKER,    5, false, banner_joker_desc,           banner_joker_effect               }, // 16
    { "Shoot the Moon",   COMMON_JOKER,    5, false, shoot_the_moon_joker_desc,   shoot_the_moon_joker_effect,      }, // 17
    // Spritesheet 1 
    { "Greedy Joker",     COMMON_JOKER,    5, false, greedy_joker_desc,           greedy_joker_effect               }, // 18
    { "Lusty Joker",      COMMON_JOKER,    5, false, lusty_joker_desc,            lusty_joker_effect                }, // 19
    // Spritesheet 2
    { "Wrathful Joker",   COMMON_JOKER,    5, false, wrathful_joker_desc,         wrathful_joker_effect             }, // 20
    { "Gluttonous Joker", COMMON_JOKER,    5, false, gluttonous_joker_desc,       gluttonous_joker_effect           }, // 21
    // Spritesheet 3
    { "Crazy Joker",      COMMON_JOKER,    4, false, crazy_joker_desc,            crazy_joker_effect                }, // 22
    { "Mad Joker",        COMMON_JOKER,    4, false, mad_joker_desc,              mad_joker_effect                  }, // 23
    { "Clever Joker",     COMMON_JOKER,    4, false, clever_joker_desc,           clever_joker_effect               }, // 24
    { "Devious Joker",    COMMON_JOKER,    4, false, devious_joker_desc,          devious_joker_effect              }, // 25
    { "Even Steven",      COMMON_JOKER,    4, false, even_steven_joker_desc,      even_steven_joker_effect          }, // 26
    // Spritesheet 4
    { "Blackboard",       UNCOMMON_JOKER,  6, false, blackboard_joker_desc,       blackboard_joker_effect           }, // 27
    { "Mystic Summit",    COMMON_JOKER,    5, false, mystic_summit_joker_desc,    mystic_summit_joker_effect        }, // 28
    { "Walkie Talkie",    COMMON_JOKER,    4, false, walkie_talkie_joker_desc,    walkie_talkie_joker_effect        }, // 29
    { "Zany Joker",       COMMON_JOKER,    4, false, zany_joker_desc,             zany_joker_effect                 }, // 30
    { "Wily Joker",       COMMON_JOKER,    4, false, wily_joker_desc,             wily_joker_effect                 }, // 31
    // Spritesheet 5
    { "Sly Joker",        COMMON_JOKER,    3, false, sly_joker_desc,              sly_joker_effect                  }, // 32
    { "Jolly Joker",      COMMON_JOKER,    3, false, jolly_joker_desc,            jolly_joker_effect                }, // 33
    { "Blue Joker",       COMMON_JOKER,    5, false, blue_joker_desc,             blue_joker_effect                 }, // 34
    { "Odd Todd",         COMMON_JOKER,    4, false, odd_todd_joker_desc,         odd_todd_joker_effect             }, // 35
    // Spritesheet 6
    { "The Duo",          RARE_JOKER,      8, false, the_duo_joker_desc,          the_duo_joker_effect              }, // 36
    { "The Trio",         RARE_JOKER,      8, false, the_trio_joker_desc,         the_trio_joker_effect             }, // 37
    { "The Order",        RARE_JOKER,      8, false, the_order_joker_desc,        the_order_joker_effect            }, // 38
    { "The Tribe",        RARE_JOKER,      8, false, the_tribe_joker_desc,        the_tribe_joker_effect            }, // 39
    // Spritesheet 7
    { "The Family",       RARE_JOKER,      8, false, the_family_joker_desc,       the_family_joker_effect           }, // 40
    { "Brainstorm",       RARE_JOKER,     10, false, brainstorm_joker_desc,       blueprint_brainstorm_joker_effect }, // 41 Brainstorm
    // Spritesheet 8
    { "Smiley Face",      COMMON_JOKER,    4, false, smiley_face_joker_desc,      smiley_face_joker_effect          }, // 42
    { "Bull",             UNCOMMON_JOKER,  6, false, bull_joker_desc,             bull_joker_effect                 }, // 43
    // Individual Jokers (for now :3)
    { "Photograph",       COMMON_JOKER,    5, false, photograph_joker_desc,       photograph_joker_effect,          }, // 44
    { "Hack",             UNCOMMON_JOKER,  6, false, hack_joker_desc,             hack_joker_effect                 }, // 45
    { "Pareidolia",       UNCOMMON_JOKER,  5, false, pareidolia_joker_desc,       joker_effect_noop                 }, // 46 Pareidolia
    { "Bootstraps",       UNCOMMON_JOKER,  7, false, bootstraps_joker_desc,       bootstraps_joker_effect           }, // 47
    { "Shortcut",         UNCOMMON_JOKER,  7, false, shortcut_joker_desc,         joker_effect_noop,                }, // 48 Shortcut
    { "Dusk",             UNCOMMON_JOKER,  5, false, dusk_joker_desc,             dusk_joker_effect                 }, // 49
    { "Four Fingers",     UNCOMMON_JOKER,  7, false, four_fingers_joker_desc,     joker_effect_noop,                }, // 50 Four Fingers
    { "Seltzer",          UNCOMMON_JOKER,  6, false, seltzer_joker_desc,          seltzer_joker_effect,             }, // 51
    { "Blueprint",        RARE_JOKER,     10, false, blueprint_joker_desc,        blueprint_brainstorm_joker_effect }, // 52 Blueprint

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

#pragma region JOKER DESCRIPTIONS

static int default_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG "+4 " TTE_BLACK_TAG "Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int greedy_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_DIAMOND_TAG TTE_BLACK_TAG "suit give " TTE_RED_TAG
                      "+3 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int lusty_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_HEART_TAG TTE_BLACK_TAG "suit give " TTE_RED_TAG
                      "+3 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int wrathful_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_SPADE_TAG TTE_BLACK_TAG "suit give " TTE_RED_TAG
                      "+3 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int gluttonous_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_CLUB_TAG TTE_BLACK_TAG "suit give " TTE_RED_TAG
                      "+3 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int jolly_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "+8 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int zany_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "+12 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Three of a Kind";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int mad_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "+10 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Two Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int crazy_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "+12 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Straight";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int droll_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "+10 " TTE_BLACK_TAG "Mult if played hand contains a " TTE_YELLOW_TAG "Flush";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int sly_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLUE_TAG "+50 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int wily_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLUE_TAG
        "+100 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Three of a Kind";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int clever_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLUE_TAG
        "+80 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Two Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int devious_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLUE_TAG
        "+100 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Straight";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int crafty_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLUE_TAG "+80 " TTE_BLACK_TAG "Chips if played hand contains a " TTE_YELLOW_TAG "Flush";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int half_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "+20 " TTE_BLACK_TAG "Mult if played hand contains " TTE_YELLOW_TAG
                    "3 " TTE_BLACK_TAG "or fewer cards";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int stencil_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_RED_TAG "X1 " TTE_BLACK_TAG
                    "Mult for each empty Joker slot Joker Stencil included\n\n(Now " TTE_RED_TAG
                    "X%ld " TTE_BLACK_TAG "Mult)";
    const u32 desc_max_size = 130;

    List* jokers = get_jokers_list();
    u32 stencil_bonus = MAX_JOKERS_HELD_SIZE - list_get_len(jokers);

    ListItr itr = list_itr_create(jokers);
    JokerObject* joker_object;
    while ((joker_object = list_itr_next(&itr)))
    {
        if (joker_object->joker->id == STENCIL_JOKER_ID)
            stencil_bonus++;
    }

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, stencil_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int misprint_joker_desc(Joker* joker, Rect dest_rect)
{
    // TODO: print glitchy desc with occasional next card reveal
    char desc[] = TTE_YELLOW_TAG "Random" TTE_BLACK_TAG " Mult between " TTE_RED_TAG
                                 "+0" TTE_BLACK_TAG " and " TTE_RED_TAG "+23";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int walkie_talkie_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Each played " TTE_YELLOW_TAG "10 " TTE_BLACK_TAG "or " TTE_YELLOW_TAG
                      "4 " TTE_BLACK_TAG "gives " TTE_BLUE_TAG "+10 " TTE_BLACK_TAG
                      "Chips and " TTE_RED_TAG "+4 " TTE_BLACK_TAG "Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int fibonnaci_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Each played\n" TTE_YELLOW_TAG "Ace" TTE_BLACK_TAG ", " TTE_YELLOW_TAG
                      "2" TTE_BLACK_TAG ", " TTE_YELLOW_TAG "3" TTE_BLACK_TAG ", " TTE_YELLOW_TAG
                      "5" TTE_BLACK_TAG ", " TTE_YELLOW_TAG "8\n" TTE_BLACK_TAG "gives " TTE_RED_TAG
                      "+8 " TTE_BLACK_TAG "Mult when scored";

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int banner_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLUE_TAG "+30 " TTE_BLACK_TAG "Chips for each remaining " TTE_YELLOW_TAG "discard";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int mystic_summit_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG "+15 " TTE_BLACK_TAG "Mult when " TTE_YELLOW_TAG
                                           "0 " TTE_BLACK_TAG "discards remaining ";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int blackboard_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "X3 " TTE_BLACK_TAG "Mult if all cards held in hand are " TTE_SPADE_TAG TTE_BLACK_TAG
        "or " TTE_CLUB_TAG;
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int blue_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_BLUE_TAG "+2 " TTE_BLACK_TAG "Chips for each remaining card in " TTE_YELLOW_TAG
                     "deck" TTE_BLACK_TAG "\n\n(Now " TTE_BLUE_TAG "+%ld" TTE_BLACK_TAG " Chips)";
    const u32 desc_max_size = 139;

    u32 blue_bonus = (get_deck_top() + 1) * 2;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, blue_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int raised_fist_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Adds " TTE_YELLOW_TAG "double" TTE_BLACK_TAG " the rank of " TTE_YELLOW_TAG
                      "lowest" TTE_BLACK_TAG " ranked card held in hand to Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int reserved_parking_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG
        "Each " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " card held in hand has a " TTE_GREEN_TAG
        "1 in 2" TTE_BLACK_TAG " chance to give " TTE_YELLOW_TAG "$1";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int business_card_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG
        "Played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards have a " TTE_GREEN_TAG
        "1 in 2" TTE_BLACK_TAG " chance to give " TTE_YELLOW_TAG "$2" TTE_BLACK_TAG " when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int scholar_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG
        "Played " TTE_YELLOW_TAG "Aces" TTE_BLACK_TAG " give " TTE_BLUE_TAG "+20" TTE_BLACK_TAG
        " Chips and " TTE_RED_TAG "+4" TTE_BLACK_TAG " Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int scary_face_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards give " TTE_BLUE_TAG
                      "+30" TTE_BLACK_TAG " Chips when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int abstract_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_RED_TAG "+3" TTE_BLACK_TAG " Mult for each " TTE_YELLOW_TAG "Joker" TTE_BLACK_TAG
                    " card\n\n(Now " TTE_RED_TAG "+%ld" TTE_BLACK_TAG " Mult)";
    const u32 desc_max_size = 125;

    u32 abstract_bonus = list_get_len(get_jokers_list()) * 3;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, abstract_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int bull_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_BLUE_TAG "+2" TTE_BLACK_TAG " Chips for each " TTE_YELLOW_TAG "$1" TTE_BLACK_TAG
                     " you have\n\n(Now " TTE_BLUE_TAG "+%ld" TTE_BLACK_TAG " Chips)";
    const u32 desc_max_size = 127;

    u32 bull_bonus = (g_game_vars.money > 0) ? g_game_vars.money * 2 : 0;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, bull_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int smiley_face_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards give " TTE_RED_TAG
                      "+5" TTE_BLACK_TAG " Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int even_steven_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_YELLOW_TAG "even rank give " TTE_RED_TAG
                      "+4" TTE_BLACK_TAG " Mult when scored\n(10, 8, 6, 4, 2)";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int odd_todd_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Played cards with " TTE_YELLOW_TAG "odd rank give " TTE_BLUE_TAG
                      "+31" TTE_BLACK_TAG " Chips when scored\n(A, 9, 7, 5, 3)";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int acrobat_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG "X3" TTE_BLACK_TAG " Mult on " TTE_YELLOW_TAG
                                           "final hand" TTE_BLACK_TAG " of round";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int hanging_chad_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG "Retrigger " TTE_YELLOW_TAG "first" TTE_BLACK_TAG
                                             " played card used in scoring " TTE_YELLOW_TAG
                                             "2" TTE_BLACK_TAG " additional times ";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_duo_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "X2" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Pair";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_trio_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "X3" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Three of a Kind";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_family_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_RED_TAG
        "X4" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Four of a Kind";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_order_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "X3" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Straight";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int the_tribe_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_RED_TAG "X2" TTE_BLACK_TAG " Mult if played hand contains a " TTE_YELLOW_TAG "Flush";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int bootstraps_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] =
        TTE_RED_TAG "+2" TTE_BLACK_TAG " Mult for every " TTE_YELLOW_TAG "$5" TTE_BLACK_TAG
                    " you have\n\n(Now " TTE_RED_TAG "+%ld" TTE_BLACK_TAG " Mult)";
    const u32 desc_max_size = 125;

    u32 bootstrap_bonus = (g_game_vars.money > 0) ? (g_game_vars.money / 5) * 2 : 0;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, bootstrap_bonus);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int shoot_the_moon_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Each " TTE_YELLOW_TAG "Queen" TTE_BLACK_TAG
                      " held in hand gives " TTE_RED_TAG "+13" TTE_BLACK_TAG " Mult";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int photograph_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "First played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " card gives " TTE_RED_TAG
                      "X2" TTE_BLACK_TAG " Mult when scored";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int dusk_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG "Retrigger all played cards in " TTE_YELLOW_TAG
                                             "final hand" TTE_BLACK_TAG " of the round";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int brainstorm_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Copies ability of the leftmost " TTE_YELLOW_TAG "Joker";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int blueprint_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Copies ability of " TTE_YELLOW_TAG "Joker" TTE_BLACK_TAG " to the right";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int hack_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] = TTE_BLACK_TAG
        "Retrigger each played " TTE_YELLOW_TAG "2" TTE_BLACK_TAG ", " TTE_YELLOW_TAG
        "3" TTE_BLACK_TAG ", " TTE_YELLOW_TAG "4" TTE_BLACK_TAG ", or " TTE_YELLOW_TAG "5";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int seltzer_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc_format[] = TTE_BLACK_TAG
        "Retrigger all cards played for the next " TTE_YELLOW_TAG "%ld" TTE_BLACK_TAG " hands";
    const u32 desc_max_size = 94;

    char desc[desc_max_size];
    snprintf(desc, desc_max_size, desc_format, joker->persistent_state);

    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int sock_and_buskin_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Retrigger all played " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int pareidolia_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "All cards are considered " TTE_YELLOW_TAG "face" TTE_BLACK_TAG " cards";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int shortcut_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "Allows " TTE_YELLOW_TAG "Straights" TTE_BLACK_TAG
                      " to be made with gaps of " TTE_YELLOW_TAG "1 rank" TTE_BLACK_TAG
                      "\n\n(ex: " TTE_YELLOW_TAG "10 8 6 5 3" TTE_BLACK_TAG ")";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

static int four_fingers_joker_desc(Joker* joker, Rect dest_rect)
{
    static const char desc[] =
        TTE_BLACK_TAG "All " TTE_YELLOW_TAG "Flushes" TTE_BLACK_TAG " and " TTE_YELLOW_TAG
                      "Straights" TTE_BLACK_TAG " can be made with 4 cards";
    return tte_printf_justified_in_rect(desc, dest_rect, JUSTIFY_CENTER, SCREEN_LEFT, true);
}

#pragma endregion

#pragma region JOKER EFFECTS

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
    *joker_effect = &s_shared_joker_effect;

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
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->THREE_OF_A_KIND)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->TWO_PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->STRAIGHT)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->FLUSH)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->THREE_OF_A_KIND)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->TWO_PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->STRAIGHT)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->FLUSH)
    {
        *joker_effect = &s_shared_joker_effect;

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

    int played_size = get_played_size();
    if (played_size <= 3)
    {
        *joker_effect = &s_shared_joker_effect;

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

    *joker_effect = &s_shared_joker_effect;

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

    *joker_effect = &s_shared_joker_effect;

    (*joker_effect)->mult = rng_get_u32() % (MISPRINT_MAX_MULT + 1);

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
        *joker_effect = &s_shared_joker_effect;

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
            *joker_effect = &s_shared_joker_effect;
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
        *joker_effect = &s_shared_joker_effect;

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
        *joker_effect = &s_shared_joker_effect;

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
    for (int i = 0; i < g_game_vars.hand_size; i++)
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
        *joker_effect = &s_shared_joker_effect;

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

    *joker_effect = &s_shared_joker_effect;

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
            for (int i = 0; i < g_game_vars.hand_size; i++)
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
                *joker_effect = &s_shared_joker_effect;

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

    if ((rng_get_u32() % 2 == 0) && card_is_face(scored_card))
    {
        *joker_effect = &s_shared_joker_effect;

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

    if ((rng_get_u32() % 2 == 0) && card_is_face(scored_card))
    {
        *joker_effect = &s_shared_joker_effect;

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
        *joker_effect = &s_shared_joker_effect;

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
        *joker_effect = &s_shared_joker_effect;

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

    *joker_effect = &s_shared_joker_effect;

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
    if (g_game_vars.money > 0)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->chips = g_game_vars.money * 2;
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
        *joker_effect = &s_shared_joker_effect;

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
                *joker_effect = &s_shared_joker_effect;

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
        *joker_effect = &s_shared_joker_effect;

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
        *joker_effect = &s_shared_joker_effect;

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
            *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->PAIR)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->THREE_OF_A_KIND)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->FOUR_OF_A_KIND)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->STRAIGHT)
    {
        *joker_effect = &s_shared_joker_effect;

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

    if (get_contained_hands()->FLUSH)
    {
        *joker_effect = &s_shared_joker_effect;

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
    if (g_game_vars.money > 0)
    {
        *joker_effect = &s_shared_joker_effect;

        (*joker_effect)->mult = (g_game_vars.money / 5) * 2;
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
        *joker_effect = &s_shared_joker_effect;

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
                *joker_effect = &s_shared_joker_effect;

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
                *joker_effect = &s_shared_joker_effect;

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
                    *joker_effect = &s_shared_joker_effect;

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
            *joker_effect = &s_shared_joker_effect;

            (*joker_effect)->retrigger = ((*p_last_retriggered_idx) < get_scored_card_index());
            if ((*joker_effect)->retrigger)
            {
                *p_last_retriggered_idx = get_scored_card_index();
                (*joker_effect)->message = "Again!";
                effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
            }
            break;

        case JOKER_EVENT_ON_HAND_SCORED_END:
            *joker_effect = &s_shared_joker_effect;
            effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;

            (*p_hands_left_until_exp)--;
            if (*p_hands_left_until_exp > 0)
            {
                // Need to do this for now because the message's memory can't really be allocated
                // So we can't use snprintf to craft a message depending on the number of hands left
                static const char* SELTZER_MESSAGES[] =
                    {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
                (*joker_effect)->message = (char*)SELTZER_MESSAGES[(*p_hands_left_until_exp) - 1];
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
            *joker_effect = &s_shared_joker_effect;

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

#pragma endregion
