/**
 * @file save.c
 */
#include "save.h"

#include "audio_utils.h"
#include "bitset.h"
#include "game.h"
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

#define SAVE_SECTION_FLAG_NONE    0
#define SAVE_SECTION_FLAG_OPTIONS (1 << 0)
#define SAVE_SECTION_FLAG_GAME    (1 << 1)

#define CHECK_MAGIC     0x4C414247 // Spells GBAL, used to determine if the save data is junk
#define CHECK_HASH_SIZE 7
#define GIT_HASH_START  17 // starts after "GBALATRO-VERSION:" in the gbalatro_version var

#define SAVE_LABEL_SIZE 16

// clang-format off
/**
 * @brief SaveHeader for validation checks
 *
 * Structure holding save data header info to be packed and written to SRAM for validation.
 * Defined in this discussion as follows: https://github.com/GBALATRO/balatro-gba/discussions/450
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
 * @brief JokerObjectSaveData will hold the minimal amount of data necessary to reconstruct a Joker.
 *         The `id` is a u8 in the base Joker struct, but I made it a u32 here to keep
 *         a better aligment when looking at the save file in a hex viewer.
 */
typedef struct JokerObjectSaveData
{
    u32 id;
    u32 persistent_state;
} JokerObjectSaveData;

/**
 * @brief SaveGame will contain the data about the current run to be saved to SRAM.
 *
 * GameVariables was used for this purpose at first, but some data needed to be shared but
 * not saved, so it couldn't be dumped "as is" anymore and this struct had to be created.
 *
 * Some data will be taken directly from GameVariables like the RNG seed/step and timers, some
 * will be encoded/decoded to/from minimal necessary data like the Jokers.
 *
 * Some magic numbers will also be mixed in, these are there only to make sections more
 * obvious to the eye when opening the data in an hex viewer/editor.
 * I recommend using `xxd -l 512 -e -g 4 <gbalatro.sav>`
 */
typedef struct SaveGame
{
    char tag_internal[SAVE_LABEL_SIZE];
    s32 timer;
    u32 rng_seed;
    u32 rng_step;
    int round;
    int ante;
    int money;
    u32 padding[2];

    char tag_jokers[SAVE_LABEL_SIZE];
    JokerObjectSaveData jokers_data[MAX_JOKERS_HELD_SIZE];

    char tag_end[4];
} SaveGame;

/**
 * @brief SaveOptions will only contain options data set in the Options Menu.
 */
typedef struct SaveOptions
{
    char tag_options[SAVE_LABEL_SIZE];
    u8 game_speed;
    bool high_contrast;
    u8 music_volume;
    u8 sound_volume;
    u32 padding[3];
} SaveOptions;

/**
 * @brief Default value for the SaveHeader struct.
 */
static const SaveHeader SaveHeader_default = {
    .magic = CHECK_MAGIC,
    .dirty = false,
    .githash = "fffffff",
    .valid_sections = SAVE_SECTION_FLAG_NONE
};

/**
 * @brief Default value for the SaveGame struct, with tags already set.
 */
static const SaveGame SaveGame_default = {
    .tag_internal = "-INTERNAL DATA -",
    .timer = 0,
    .rng_seed = 0,
    .rng_step = 0,
    .round = 0,
    .ante = 0,
    .money = 0,
    .padding = {UNDEFINED, UNDEFINED},

    .tag_jokers = "- OWNED JOKERS -",
    .jokers_data = {},

    .tag_end = "_END"
};

/**
 * @brief Default value for the SaveOptions struct, with tags already set.
 */
static const SaveOptions SaveOptions_default = {
    .tag_options = "- OPTIONS DATA -",
    .game_speed = 0,
    .high_contrast = false,
    .music_volume = 0,
    .sound_volume = 0,
    .padding = {UNDEFINED, UNDEFINED, UNDEFINED},
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
    options.high_contrast = g_game_vars.high_contrast;
    options.music_volume = g_game_vars.music_volume;
    options.sound_volume = g_game_vars.sound_volume;

    write_sram(OPTIONS_ADDRESS, (const u8*)&options, sizeof(options));
    set_save_header(SAVE_SECTION_FLAG_OPTIONS);
}

void load_options(void)
{
    SaveHeader header;
    if (!get_save_header(&header) || !(header.valid_sections & SAVE_SECTION_FLAG_OPTIONS))
        return;

    SaveOptions options = SaveOptions_default;

    read_sram(OPTIONS_ADDRESS, (u8*)&options, sizeof(options));

    g_game_vars.game_speed = options.game_speed;
    g_game_vars.high_contrast = options.high_contrast;
    g_game_vars.music_volume = options.music_volume;
    g_game_vars.sound_volume = options.sound_volume;

    mmSetModuleVolume(MM_MODULE_FULL_VOLUME * g_game_vars.music_volume / VOLUME_OPTION_MAX);
}

void save_game(void)
{
    SaveGame game = SaveGame_default;

    // Fixed data

    game.timer = g_game_vars.timer;
    game.rng_seed = g_game_vars.rng_seed;
    game.rng_step = g_game_vars.rng_step;
    game.round = g_game_vars.round;
    game.ante = g_game_vars.ante;
    game.money = g_game_vars.money;

    // Lists

    List* jokers_list = get_jokers_list();
    int nb_jokers = list_get_len(jokers_list);

    int i = 0;
    for (; i < nb_jokers; i++)
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

    write_sram(GAME_ADDRESS, (const u8*)&game, sizeof(game));
    set_save_header(SAVE_SECTION_FLAG_GAME);
}

void load_game(void)
{
    SaveHeader header;
    if (!get_save_header(&header) || !(header.valid_sections & SAVE_SECTION_FLAG_GAME))
        return;

    SaveGame game = SaveGame_default;

    read_sram(GAME_ADDRESS, (u8*)&game, sizeof(game));

    g_game_vars.timer = game.timer;
    g_game_vars.rng_seed = game.rng_seed;
    g_game_vars.rng_step = game.rng_step;
    g_game_vars.round = game.round;
    g_game_vars.ante = game.ante;
    g_game_vars.money = game.money;

    // TODO: load Jokers from stored minimal data

    // return to where we were in the random sequence so that the run stays reproducible
    for (u32 i = 0; i < g_game_vars.rng_step; i++)
    {
        (void)rand();
    }
}
