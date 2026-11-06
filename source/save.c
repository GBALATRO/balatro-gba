/**
 * @file save.c
 */
#include "save.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "bitset.h"
#include "card.h"
#include "game.h"
#include "game_variables.h"
#include "item.h"
#include "joker.h"
#include "list.h"
#include "util.h"
#include "version.h"

#include <stdlib.h>
#include <string.h>

// See https://gbadev.net/gbadoc/memory.html for more details on SRAM
// A few important pieces of info:
//   - Read/Writes are limited to 8-bits words so they are done byte per byte
//   - Memory is filled with 1s by default
//     (at least in mgba, not sure about real HW)

#define HEADER_ADDRESS  0x0
#define OPTIONS_ADDRESS 0x10
#define GAME_ADDRESS    0x30

// This could be expanded to add more persistent data sections like unlocks
#define SAVE_SECTION_FLAG_NONE    0
#define SAVE_SECTION_FLAG_OPTIONS (1 << 0)
#define SAVE_SECTION_FLAG_GAME    (1 << 1)

#define CHECK_MAGIC     0x4C414247 // Spells GBAL, used to determine if the save data is junk
#define CHECK_HASH_SIZE 7
#define GIT_HASH_START  17 // starts after "GBALATRO-VERSION:" in the gbalatro_version var

// clang-format off
/**
 * @brief SaveHeader for validation checks to be packed and written to SRAM for validation.
 *         Defined in this discussion as follows: https://github.com/GBALATRO/balatro-gba/discussions/450
 *
 * word | Byte 0 | Byte 1 | Byte 2 | Byte 3 | name         | purpose
 * -----|--------|--------|--------|--------|--------------|------------------------------------------------------------------
 * 0    | 0x47   | 0x42   | 0x41   | 0x4C   | MAGIC        | Identify if proceeding data is valid and not junk, spells "GBAL"
 * 1    | Dirty  | H[0]   | H[1]   | H[2]   | GITHASH_LOW  | Dirty flag, followed by the first 3 bytes of shortened git hash H
 * 2    | H[3]   | H[4]   | H[5]   | H[6]   | GITHASH_HIGH | Last 4 bytes of shortened git hash H, with a dirty flag
 * 3    | SEC[0] | SEC[1] | SEC[2] | SEC[3] | VALID_SCTNS  | Identifies whether each section of the save data is valid or not
 */
// clang-format on
typedef struct SaveHeader
{
    u32 magic;
    bool dirty;
    char githash[CHECK_HASH_SIZE];
    u32 valid_sections;
} SaveHeader;

/**
 * @brief Default value for the SaveHeader struct.
 */
static const SaveHeader SaveHeader_default = {
    .magic = CHECK_MAGIC,
    .dirty = false,
    .githash = "fffffff",
    .valid_sections = SAVE_SECTION_FLAG_NONE
};

// clang-format off
/**
 * @brief SaveOptions will only contain options data set in the Options Menu
 *
 * word | Byte 0 | Byte 1 | Byte 2 | Byte 3 | name         | purpose
 * -----|--------|--------|--------|--------|--------------|------------------------------------------------------------------
 * 0    | '-'    | ' '    | 'O'    | 'P'    | TAG          | Pretty tag to clearly visualize the Options section in a hex viewer
 * 1    | 'T'    | 'I'    | 'O'    | 'N'    | -            | Spells "- OPTIONS DATA -"
 * 2    | 'S'    | ' '    | 'D'    | 'A'    | -            | -
 * 3    | 'T'    | 'A'    | ' '    | '-'    | -            | -
 * 4    | SPEED  | CNTRST | READBL | MUSIC  | OPTN_VALUES  | All 5 option values, followed by some padding,
 * 5    | SOUND  | UNDEF  | UNDEF  | UNDEF  | -            | so that the next section starts at the beginning of the
 * 6    | UNDEF  | UNDEF  | UNDEF  | UNDEF  | -            | next 4-word row in a hex viewer
 * 7    | UNDEF  | UNDEF  | UNDEF  | UNDEF  | -            | -
 */
// clang-format on
typedef struct SaveOptions
{
    char tag_options[SAVE_LABEL_SIZE];
    u8 game_speed;
    bool cards_high_contrast;
    bool cards_more_readable;
    u8 music_volume;
    u8 sound_volume;
    s8 padding[11];
} SaveOptions;

/**
 * @brief Default value for the SaveOptions struct, with tags already set.
 */
// clang-format off
static const SaveOptions SaveOptions_default = {
    .tag_options = "- OPTIONS DATA -",
    .game_speed = GAME_SPEED_MIN,
    .cards_high_contrast = DEFAULT_HIGH_CONTRAST,
    .cards_more_readable = DEFAULT_MORE_READABLE,
    .music_volume = VOLUME_OPTION_MAX,
    .sound_volume = VOLUME_OPTION_MAX,
    .padding = {
        UNDEFINED, UNDEFINED, UNDEFINED,
        UNDEFINED, UNDEFINED, UNDEFINED,
        UNDEFINED, UNDEFINED, UNDEFINED,
        UNDEFINED, UNDEFINED
    }
};
// clang-format on

/**
 * @brief Default value for the SaveGame struct, with tags already set.
 */
static const SaveGame SaveGame_default = {
    .tag_internal = "-INTERNAL DATA -",

    .game_state = GAME_STATE_BLIND_SELECT,

    .timer = 0,
    .rng_info = {0, {0}},

    .money = 0,
    .hand_size = 0,
    .ante = 0,
    .round = 0,
    .deck = DECK_TYPE_RED,
    .nb_played_hands = {0},

    .best_hand_score = 0,
    .nb_skipped_rounds = 0,
    .nb_unused_discards = 0,

    .current_blind = BLIND_TYPE_SMALL,
    .next_boss_blind = BLIND_TYPE_BIG,
    .blinds_states = {0},

    .padding0 = {UNDEFINED},

    .tag_cards = "-PLAYING CARDS -",
    .nb_playing_cards = UNDEFINED,
    .playing_cards = {},

    .padding1 = {UNDEFINED},

    .tag_jokers = "- OWNED JOKERS -",
    .nb_jokers = 0,
    .jokers_data = {},

    .padding2 = {UNDEFINED},

    .tag_skip_tags = "-  SKIP TAGS   -",
    .blind_skip_tags = {0},
    .nb_owned_skip_tags = 0,
    .owned_skip_tags = {0},

    .tag_end = "_END"
};

/**
 * @brief Write raw binary data to SRAM
 *
 * @param sram_base address written to in the SRAM
 * @param bytes pointer to the written data
 * @param size number of bytes written
 */
static inline void write_sram(u32 sram_base, const u8* bytes, u32 size)
{
    if (sram_base + size > SRAM_SIZE)
        return;

    for (u32 i = 0; i < size; i++)
    {
        sram_mem[sram_base + i] = bytes[i];
    }
}

/**
 * @brief Read raw binary data from SRAM
 *
 * @sa write_sram
 */
static inline void read_sram(u32 sram_base, u8* bytes, u32 size)
{
    if (sram_base + size > SRAM_SIZE)
        return;

    for (u32 i = 0; i < size; i++)
    {
        bytes[i] = sram_mem[sram_base + i];
    }
}

/**
 * @brief Checks the 7 chars of gbalatro_version after the "GBALATRO_VERSION" prefix
 *         representing the git hash of the code the build is based on.
 *
 * @returns true if the git hash of the ROM is equal to the hash saved in SRAM.
 *          false if they are different.
 */
static inline bool check_hash(const char* prefix)
{
    for (u32 i = 0; i < CHECK_HASH_SIZE; i++)
    {
        if (gbalatro_version[GIT_HASH_START + i] != prefix[i])
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Determines if the current build is considered "dirty" aka has uncommitted changes.
 *         This works because the gbalatro_version string has "-dirty" added at the end if it's
 *         dirty.
 *
 * @returns true if version is dirty, false otherwise.
 */
static inline bool is_version_dirty(void)
{
    return strlen(gbalatro_version) > GIT_HASH_START + CHECK_HASH_SIZE;
}

/**
 * @brief Reads whether the save data exists and is valid.
 *
 * @param header pointer to the SaveHeader struct to fill
 * @returns true if the save data is valid, false if not
 */
static inline bool get_save_header(SaveHeader* header)
{
    read_sram(HEADER_ADDRESS, (u8*)header, sizeof(*header));
    return (header->magic == CHECK_MAGIC) && header->dirty == is_version_dirty() &&
           check_hash(header->githash);
}

/**
 * @brief Writes a magic number and ROM version info to SRAM to signal that the
 *         save data exists and allow the game to determine if it is compatible.
 *
 * This will read the SaveHeader first and check if the data is valid. If yes, the
 * `valid_sections` will be updated, if not, it will be overwritten and start from
 * `SAVE_SECTION_FLAG_NONE`.
 *
 * @param section_flag The section flag to be set to 1, corresponds to the
 *                      section we're writing to SRAM.
 */
static inline void set_save_header(u32 section_flag)
{
    SaveHeader header;

    // Check for valid data. If it's junk, set all sections as invalid, else keep the flags.
    // Then add the requested flag.
    if (!get_save_header(&header))
    {
        memcpy(&header, &SaveHeader_default, sizeof(SaveHeader_default));
    }

    header.valid_sections |= section_flag;

    header.dirty = is_version_dirty();
    memcpy(&(header.githash), gbalatro_version + GIT_HASH_START, CHECK_HASH_SIZE);

    write_sram(HEADER_ADDRESS, (const u8*)&header, sizeof(header));
}

void save_options(void)
{
    SaveOptions options = SaveOptions_default;

    options.game_speed = g_game_vars.game_speed;
    options.cards_high_contrast = get_cards_high_contrast();
    options.cards_more_readable = get_cards_more_readable();
    options.music_volume = g_game_vars.music_volume;
    options.sound_volume = g_game_vars.sound_volume;

    write_sram(OPTIONS_ADDRESS, (const u8*)&options, sizeof(options));
    set_save_header(SAVE_SECTION_FLAG_OPTIONS);
}

void load_options(void)
{
    SaveHeader header;
    SaveOptions options = SaveOptions_default;

    // If options data doesn't exist or is invalid, just don't read from
    // SRAM and apply the default values
    if (get_save_header(&header) && (header.valid_sections & SAVE_SECTION_FLAG_OPTIONS))
        read_sram(OPTIONS_ADDRESS, (u8*)&options, sizeof(options));

    g_game_vars.game_speed = options.game_speed;
    g_game_vars.music_volume = options.music_volume;
    g_game_vars.sound_volume = options.sound_volume;

    set_volume(volume_module_step_to_val(g_game_vars.music_volume));
    set_cards_high_contrast(options.cards_high_contrast);
    set_cards_more_readable(options.cards_more_readable);
}

bool is_game_data_valid(void)
{
    SaveHeader header;
    return get_save_header(&header) && (header.valid_sections & SAVE_SECTION_FLAG_GAME);
}

void erase_save_data(void)
{
    SaveHeader header;
    if (!get_save_header(&header))
        return;

    header.valid_sections &= ~SAVE_SECTION_FLAG_GAME;
    write_sram(HEADER_ADDRESS, (const u8*)&header, sizeof(header));
}

void save_game(enum GameState state)
{
    SaveGame game = SaveGame_default;

    // Fixed data

    game.game_state = state;

    game.timer = g_game_vars.timer;
    game.rng_info = g_game_vars.rng_info;

    game.money = g_game_vars.money;
    game.hand_size = g_game_vars.hand_size;
    game.ante = g_game_vars.ante;
    game.round = g_game_vars.round;
    game.deck = g_game_vars.deck;
    for (enum HandType hand_type = 0; hand_type < HAND_TYPE_MAX; hand_type++)
        game.nb_played_hands[hand_type] = g_game_vars.nb_played_hands[hand_type];

    game.best_hand_score = g_game_vars.best_hand_score;
    game.nb_skipped_rounds = g_game_vars.nb_skipped_rounds;
    game.nb_unused_discards = g_game_vars.nb_unused_discards;

    game.current_blind = g_game_vars.current_blind;
    game.next_boss_blind = g_game_vars.next_boss_blind;
    for (enum BlindTokens blind = 0; blind < NUM_BLINDS_PER_ANTE; blind++)
        game.blinds_states[blind] = g_game_vars.blinds_states[blind];

    // Cards

    for (int idx = 0; idx < MAX_DECK_SIZE; idx++)
    {
        Card* card = deck_get_at_idx(idx);

        // No more valid cards mean we've arrived at the end of the Deck
        if (card == NULL)
        {
            game.nb_playing_cards = idx;
            break;
        }

        game.playing_cards[idx].suit = card->suit;
        game.playing_cards[idx].rank = card->rank;
    }

    if (game.nb_playing_cards == UNDEFINED)
        game.nb_playing_cards = MAX_DECK_SIZE;

    // Owned Jokers

    List* jokers_list = get_jokers_list();
    game.nb_jokers = list_get_len(jokers_list);

    int i = 0;
    for (; i < game.nb_jokers; i++)
    {
        JokerObject* joker_object = list_get_at_idx(jokers_list, (u32)i);
        JokerObjectSaveData data = {
            (u32)joker_object->joker->id,
            joker_object->joker->persistent_state
        };
        game.jokers_data[i] = data;
    }
    for (; i < MAX_JOKERS_HELD_SIZE; i++)
    {
        JokerObjectSaveData data = {UNDEFINED, UNDEFINED};
        game.jokers_data[i] = data;
    }

    // Skip Tags

    i = 0;
    for (; i < NB_SKIPPABLE_BLINDS; i++)
        game.blind_skip_tags[i] = get_blind_skip_tag_type(i);

    List* owned_skip_tags = get_owned_skip_tags();
    game.nb_owned_skip_tags = list_get_len(owned_skip_tags);
    i = 0;
    for (; i < game.nb_owned_skip_tags; i++)
    {
        SkipTag* skip_tag = list_get_at_idx(owned_skip_tags, (u32)i);
        GBAL_RETURN_IF_NULL(skip_tag, RET_NONE);

        game.owned_skip_tags[i] = skip_tag->type;
    }
    for (; i < MAX_SKIP_TAGS - NB_SKIPPABLE_BLINDS; i++)
        game.owned_skip_tags[i] = UNDEFINED;

    write_sram(GAME_ADDRESS, (const u8*)&game, sizeof(game));
    set_save_header(SAVE_SECTION_FLAG_GAME);
}

void get_game_saved_data(SaveGame* game_data)
{
    GBAL_RETURN_IF_NULL(game_data, RET_NONE);

    SaveHeader header;

    if (!get_save_header(&header) || !(header.valid_sections & SAVE_SECTION_FLAG_GAME))
        return;

    read_sram(GAME_ADDRESS, (u8*)game_data, sizeof(*game_data));
}

void load_game(SaveGame* game_data)
{
    GBAL_RETURN_IF_NULL(game_data, RET_NONE);

    game_reset();

    g_game_vars.timer = game_data->timer;
    rng_restore(game_data->rng_info);

    g_game_vars.money = game_data->money;
    g_game_vars.hand_size = game_data->hand_size;
    g_game_vars.ante = game_data->ante;
    g_game_vars.round = game_data->round;
    g_game_vars.deck = game_data->deck;
    for (enum HandType hand_type = 0; hand_type < HAND_TYPE_MAX; hand_type++)
        g_game_vars.nb_played_hands[hand_type] = game_data->nb_played_hands[hand_type];

    g_game_vars.best_hand_score = game_data->best_hand_score;
    g_game_vars.nb_skipped_rounds = game_data->nb_skipped_rounds;
    g_game_vars.nb_unused_discards = game_data->nb_unused_discards;

    g_game_vars.current_blind = game_data->current_blind;
    g_game_vars.next_boss_blind = game_data->next_boss_blind;
    for (enum BlindTokens blind = 0; blind < NUM_BLINDS_PER_ANTE; blind++)
        g_game_vars.blinds_states[blind] = game_data->blinds_states[blind];

    // Jokers

    int i = 0;
    for (; i < game_data->nb_jokers; i++)
    {
        Joker* joker = joker_new(game_data->jokers_data[i].id);
        GBAL_RETURN_IF_NULL(joker, RET_NONE);
        joker->persistent_state = game_data->jokers_data[i].persistent_state;

        JokerObject* joker_object = joker_object_new(joker);
        GBAL_RETURN_IF_NULL(joker_object, RET_NONE);
        sprite_object_position((SpriteObject*)joker_object, 260, 0);

        joker_set_rollable(joker->id, false);
        item_acquire((Item*)joker_object);
    }

    // Skip Tags

    i = 0;
    for (; i < NB_SKIPPABLE_BLINDS; i++)
        set_blind_skip_tag(i, game_data->blind_skip_tags[i]);

    i = 0;
    for (; i < game_data->nb_owned_skip_tags; i++)
    {
        SkipTag* skip_tag = skip_tag_new(game_data->owned_skip_tags[i]);
        GBAL_RETURN_IF_NULL(skip_tag, RET_NONE);

        add_skip_tag(&skip_tag);
    }

    // Deck

    for (int i = 0; i < game_data->nb_playing_cards; i++)
    {
        Card saved_card = game_data->playing_cards[i];
        Card* card = card_new(saved_card.suit, saved_card.rank);
        deck_push(card);
    }

    display_score(0);
    display_chips();
    display_mult();
    display_hands();
    display_discards();
    display_money();
    display_ante();
    display_round();
    display_deck_size_max();

    // Quirk of the Blind panel animation: the background color for Boss Blinds is set during an
    // animation of the Blind Select screen.
    // For now, set color here if needed, but when round.c is refactored to use a full-on state
    // machine, move that animation to it so we don't have to set the color here
    if (game_data->game_state == GAME_STATE_ROUND && g_game_vars.current_blind >= BLIND_TYPE_BOSS)
    {
        affine_background_set_color(
            blind_get_color(g_game_vars.current_blind, BLIND_SHADOW_COLOR_INDEX)
        );
    }

    game_change_state(game_data->game_state);
}
