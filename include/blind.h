/**
 * @file blind.h
 * @brief Data structures and functions relative to the behaviour and graphics of Blinds.
 */

#ifndef BLIND_H
#define BLIND_H

#include "sprite.h"

// The GBA's max uint value is around 4 billion, so we're going to not add endless mode for
// simplicity's sake
#define MAX_ANTE 8

/**
 * @enum BlindTokens
 * @brief The sprites that display the blinds when in "GAME_BLIND_SELECT" state.
 *         There are only 3 blinds per Ante, so we don't need more sprites than that
 */
enum BlindTokens
{
    SMALL_BLIND,
    BIG_BLIND,
    BOSS_BLIND,
    NUM_BLINDS_PER_ANTE
};

/**
 * @enum BlindTokenLayers
 * @brief Sprite IDs of the various Blind Tokens used in the game, expressed as an offset relative
 *         to `BLIND_STARTING_LAYER`
 *
 * @sa SpriteStartingLayers
 */
enum BlindTokenLayers
{
    PLAYING_BLIND_TOKEN_LAYER,
    ROUND_END_BLIND_TOKEN_LAYER,
    SMALL_BLIND_TOKEN_LAYER,
    BIG_BLIND_TOKEN_LAYER,
    BOSS_BLIND_TOKEN_LAYER
};

/**
 * @enum BlindColorIndex
 * @brief Indices of the Blind sprites' colors as encoded in the files' palettes with Aseprite
 */
enum BlindColorIndex
{
    BLIND_TEXT_COLOR_INDEX = 1,
    BLIND_SHADOW_COLOR_INDEX,
    BLIND_HIGHLIGHT_COLOR_INDEX,
    BLIND_MAIN_COLOR_INDEX,
    BLIND_BACKGROUND_MAIN_COLOR_INDEX,
    BLIND_BACKGROUND_SECONDARY_COLOR_INDEX,
    BLIND_BACKGROUND_SHADOW_COLOR_INDEX,
};

// clang-format off
/**
 * @enum BlindType
 * @brief All Blind types in the game are listed here.
 *
 * Boss Blinds range from `BLIND_TYPE_BOSS` to `BLIND_TYPE_SHOWDOWN - 1`.
 *
 * Showdown Blinds range from `BLIND_TYPE_SHOWDOWN` to `BLIND_TYPE_MAX - 1`
 */
enum BlindType
{
    // Normal Blinds
    BLIND_TYPE_SMALL,
    BLIND_TYPE_BIG,

    // Boss Blinds
    BLIND_TYPE_BOSS,

    BLIND_TYPE_HOOK = BLIND_TYPE_BOSS,
    BLIND_TYPE_OX,
    BLIND_TYPE_HOUSE,
    BLIND_TYPE_WALL,
    BLIND_TYPE_WHEEL,
    BLIND_TYPE_ARM,
    BLIND_TYPE_CLUB,
    BLIND_TYPE_FISH,
    BLIND_TYPE_PSYCHIC,
    BLIND_TYPE_GOAD,
    BLIND_TYPE_WATER,
    BLIND_TYPE_WINDOW,
    BLIND_TYPE_MANACLE,
    BLIND_TYPE_EYE,
    BLIND_TYPE_MOUTH,
    BLIND_TYPE_PLANT,
    BLIND_TYPE_SERPENT,
    BLIND_TYPE_PILLAR,
    BLIND_TYPE_NEEDLE,
    BLIND_TYPE_HEAD,
    BLIND_TYPE_TOOTH,
    BLIND_TYPE_FLINT,
    BLIND_TYPE_MARK,

    // Showdown Blinds
    BLIND_TYPE_SHOWDOWN,

    BLIND_TYPE_ACORN = BLIND_TYPE_SHOWDOWN,
    BLIND_TYPE_LEAF,
    BLIND_TYPE_VESSEL,
    BLIND_TYPE_HEART,
    BLIND_TYPE_BELL,

    // End of Blinds' list
    BLIND_TYPE_MAX
};
// clang-format on

/**
 * @enum BlindState
 * @brief All the possible states an Ante's Blinds can be in when viewed in the "Blind Select"
 *         screen.
 */
enum BlindState
{
    BLIND_STATE_CURRENT,
    BLIND_STATE_UPCOMING,
    BLIND_STATE_DEFEATED,
    BLIND_STATE_SKIPPED,
    BLIND_STATE_MAX,
};

/**
 * @struct Blind
 * @brief Data structure containing data about a BlindType.
 *
 * Only contains the score requirement multiplier for now, but will contain info about the Blind's
 * effect when implemented.
 */
typedef struct
{
    u8 type;
    FIXED score_req_multiplier;
} Blind;

void blind_init();

/**
 * @brief Get the score required to beat a certain blind (either the Small, Big, or any
 *         Boss/Showdown blind) at a certain Ante.
 *
 * @param type the `BlindType` value of the blind we need the score requirement of
 * @param ante the Ante at which the blind `type` is encountered
 *
 * @return the required score to beat the requested blind
 */
u32 blind_get_requirement(enum BlindType type, int ante);

/**
 * @brief Get the amount of money gained from beating a certain Blind.
 *
 * @param type the BlindType of the beaten Blind
 *
 * @return 3 for the Small Blind, 4 for the Big one, 5 for Bosses and 8 for Showdowns
 */
int blind_get_reward(enum BlindType type);

/**
 * @brief Get the color associated with a given Blind.
 *
 * @param type the BlindType whose palette will be used
 * @param index of the color within the Blind's palette
 *
 * @return the BGR555 value of the requested color
 */
u16 blind_get_color(enum BlindType type, enum BlindColorIndex index);

/**
 * @brief Choose a random Blind among the ones that haven't been beaten yet.
 *
 * @param showdown whether we want to roll a Showdown or a regular Boss Blind.
 *
 * @return a BlindType value chosen at random
 */
enum BlindType roll_blind_type(bool showdown);

/**
 * @brief Initialize Boss and Showdown Blinds lists to roll from
 */
void init_unbeaten_blinds_lists(void);

/**
 * @brief Remove the given Blind from the corresponding List so that we don't roll it again in the
 *         future.
 *
 * @param type the BlindType of the Boss/Showdown Blind we've just beaten.
 */
void set_blind_beaten(enum BlindType type);

/**
 * @brief Copy the palette associated with the given Blind and copy it in the right spot in the
 *         palette memory
 *
 * @param type of the Blind whose palette we need
 */
void apply_blind_colors(enum BlindType type);

/**
 * @brief Create a new BlindToken sprite.
 *
 * @param type of the Blind the token needs to represent
 * @param x initial position of the new Sprite on the horizontal axis
 * @param y initial position of the new Sprite on the vertical axis
 * @param sprite_index layer of the new Sprite
 *
 * @sa BlindTokenLayers
 */
Sprite* blind_token_new(enum BlindType type, int x, int y, enum BlindTokenLayers sprite_index);

#endif // BLIND_H
