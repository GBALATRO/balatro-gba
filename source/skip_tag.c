/**
 * @file skip_tag.c
 * @brief Implementation of functions related to the handling of Skip Tags
 */
#include "skip_tag.h"

#include "audio_utils.h"
#include "game/blind_select.h"
#include "game_variables.h"
#include "joker.h"
#include "mgba_logger.h"
#include "pool.h"
#include "skip_tags_gfx.h"
#include "soundbank.h"
#include "state_machine.h"
#include "timer.h"
#include "util.h"

#include <stdlib.h> //#include "random.h"
#include <tonc.h>

#define MAX_NON_OVERLAPING_TAG_SPRITES 5
#define SKIP_TAG_HIDE_X_OFFSET         20

#define SKIP_TAG_GET_BOUNCE_STRENGTH     (float2fx(-0.3f))
#define SKIP_TAG_TRIGGER_BOUNCE_STRENGTH (float2fx(-0.4f))

// clang-format off
// Points                                         x    y
static const BG_POINT OWNED_SKIP_TAGS_BASE_POS = {219, 97};
// Sizes
static const int OWNED_SKIP_TAGS_STACK_HEIGHT = 64;
static const int OWNED_SKIP_TAGS_SPACING      = 12;
// clang-format on

/**
 * @def NB_ANTE1_SKIP_TAGS
 * @brief Number of implemented Skip Tags available from Ante 1
 */
#define NB_ANTE1_SKIP_TAGS 7

/**
 * @def NB_SKIP_TAG_TYPES
 * @brief Number of implemented Skip Tags
 */
#define NB_SKIP_TAG_TYPES 10

// Not all Skip Tags can be drawn on Ante 1, so we need 2 different roll tables
// one for Ante 1 only, and one for all other Antes.
static const u8 ante1_skip_tags_roll_table[NB_ANTE1_SKIP_TAGS] = {
    SKIP_TAG_TYPE_INVESTMENT,
    SKIP_TAG_TYPE_BOSS,
    SKIP_TAG_TYPE_DOUBLE,
    SKIP_TAG_TYPE_JUGGLE,
    SKIP_TAG_TYPE_D6,
    SKIP_TAG_TYPE_SPEED,
    SKIP_TAG_TYPE_ECONOMY
};

// Won't be needed if all tags are implemented, but likely won't ever be the case
static const u8 all_skip_tags_roll_table[NB_SKIP_TAG_TYPES] = {
    SKIP_TAG_TYPE_INVESTMENT,
    SKIP_TAG_TYPE_BOSS,
    SKIP_TAG_TYPE_HANDY,
    SKIP_TAG_TYPE_GARBAGE,
    SKIP_TAG_TYPE_DOUBLE,
    SKIP_TAG_TYPE_JUGGLE,
    SKIP_TAG_TYPE_D6,
    SKIP_TAG_TYPE_TOP_UP,
    SKIP_TAG_TYPE_SPEED,
    SKIP_TAG_TYPE_ECONOMY
};

static List s_owned_skip_tags;

List* get_owned_skip_tags(void)
{
    return &s_owned_skip_tags;
}

// SkipTag new/destroy functions

SkipTag* skip_tag_new(enum SkipTagTypes tag_type)
{
    SkipTag* tag = POOL_GET(SkipTag);
    GBAL_RETURN_IF_NULL_RET(tag, NULL);

    tag->type = tag_type;
    sprite_object_init((SpriteObject*)tag);

    return tag;
}

void skip_tag_set_sprite(SkipTag* tag, s16 layer)
{
    GBAL_RETURN_IF_NULL_VOID(tag);

    // Set tags palette the first time we ask for a sprite
    static bool pb_init = false;
    if (!pb_init)
    {
        GRIT_CPY(&pal_obj_mem[PAL_ROW_LEN * SKIP_TAGS_PB], skip_tags_gfxPal);
        pb_init = true;
    }

    int tile_index = sprite_get_tid(SKIP_TAG_SPRITE, layer);
    memcpy32(
        &tile_mem[TILE_MEM_OBJ_CHARBLOCK0_IDX][tile_index],
        &skip_tags_gfxTiles[tag->type * SKIP_TAG_SPRITE_TILES * TILE_SIZE],
        SKIP_TAG_SPRITE_TILES * TILE_SIZE
    );

    // Recreate sprite
    if (tag->sprite)
    {
        sprite_destroy(&tag->sprite);
    }
    Sprite* sprite = sprite_new(
        ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF,
        ATTR1_SIZE_16,
        tile_index,
        SKIP_TAGS_PB,
        sprite_get_starting_layer(SKIP_TAG_SPRITE) + layer
    );
    sprite_object_set_sprite((SpriteObject*)tag, sprite);
}

void skip_tag_destroy(SkipTag** tag)
{
    // Nothing to do
    if (tag == NULL || *tag == NULL)
        return;

    sprite_object_destroy((SpriteObject*)(*tag));
    POOL_FREE(SkipTag, *tag);
    *tag = NULL;
}

// Misc functions

void set_owned_skip_tags_moved_offscreen(bool move_offscreen)
{
    SkipTag* tag;
    ListItr tag_itr = list_itr_create(&s_owned_skip_tags);
    int tag_xpos =
        int2fx(OWNED_SKIP_TAGS_BASE_POS.x + (move_offscreen ? SKIP_TAG_HIDE_X_OFFSET : 0));

    while ((tag = list_itr_next(&tag_itr)))
    {
        tag->tx = tag_xpos;
    }
}

SkipTag* roll_skip_tag(void)
{
    enum SkipTagTypes tag_type = 0;
    if (g_game_vars.ante == 1)
    {
        tag_type = ante1_skip_tags_roll_table[rand() % NB_ANTE1_SKIP_TAGS];
    }
    else
    {
        tag_type = all_skip_tags_roll_table[rand() % NB_SKIP_TAG_TYPES];
    }

    return skip_tag_new(tag_type);
}

static inline int get_skip_tag_sprites_spacing(void)
{
    int nb_owned_tags = list_get_len(&s_owned_skip_tags);
    return (nb_owned_tags <= MAX_NON_OVERLAPING_TAG_SPRITES)
             ? OWNED_SKIP_TAGS_SPACING
             : OWNED_SKIP_TAGS_STACK_HEIGHT / nb_owned_tags;
}

static void rearrange_skip_tag_sprites(int nb_owned_tags, int tag_spacing)
{
    for (int idx = 0; idx < nb_owned_tags; idx++)
    {
        SkipTag* tmp_tag = list_get_at_idx(&s_owned_skip_tags, idx);

        if (tmp_tag == NULL)
            continue;

        BG_POINT dest_pos = {
            OWNED_SKIP_TAGS_BASE_POS.x,
            OWNED_SKIP_TAGS_BASE_POS.y - idx * tag_spacing
        };

        sprite_object_set_target((SpriteObject*)tmp_tag, dest_pos);
    }
}

// Unused for now, may be useful for upcoming tags
GBAL_UNUSED
bool skip_tag_is_owned(enum SkipTagTypes tag_type)
{
    SkipTag* tag;
    ListItr tag_itr = list_itr_create(&s_owned_skip_tags);

    while ((tag = list_itr_next(&tag_itr)))
    {
        if (tag->type == tag_type)
        {
            return true;
        }
    }

    return false;
}

int skip_tag_count(enum SkipTagTypes tag_type)
{
    int count = 0;
    SkipTag* tag;
    ListItr tag_itr = list_itr_create(&s_owned_skip_tags);

    while ((tag = list_itr_next(&tag_itr)))
    {
        if (tag->type == tag_type)
        {
            count++;
        }
    }

    return count;
}

void add_skip_tag(SkipTag** blind_tag)
{
    GBAL_RETURN_IF_NULL_VOID(blind_tag);
    GBAL_RETURN_IF_NULL_VOID(*blind_tag);

    // Add to the back, so that the oldest (at the bottom) has the lowest sprite
    // index and is thus shown on top of the others.
    // Limit number to MAX_SKIP_TAGS - NB_SKIPPABLE_BLINDS so we always have room in the memory
    // pool to allocate the two tags in the Blind Select menu. Don't do anything, you just won't
    // get the tag.
    int nb_owned_tags = list_get_len(&s_owned_skip_tags);
    if (nb_owned_tags >= MAX_SKIP_TAGS - NB_SKIPPABLE_BLINDS)
    {
        MGBA_FUNC_WARN("Memory pool limit reached, cannot obtain more Skip Tags");
        return;
    }

    list_push_back(&s_owned_skip_tags, *blind_tag);
    nb_owned_tags++;

    BG_POINT new_tag_pos = OWNED_SKIP_TAGS_BASE_POS;

    // If all the tags fix on the screen without issue,
    // just move the new tag to the top of the stack and that's it
    if (nb_owned_tags <= MAX_NON_OVERLAPING_TAG_SPRITES)
    {
        new_tag_pos.y -= (nb_owned_tags - 1) * OWNED_SKIP_TAGS_SPACING;
    }
    // If it's going to overlap the consumables' frame, then we must move all
    // existing tags down a bit so we can put the new one at the very top
    else
    {
        int even_spacing = get_skip_tag_sprites_spacing();

        // Exclude the new one from the loop here, as it will be snapped into place directly
        rearrange_skip_tag_sprites(nb_owned_tags - 1, even_spacing);
        new_tag_pos.y -= even_spacing * (nb_owned_tags - 1);
    }

    // Create new sprite at the old position to avoid flickering
    skip_tag_set_sprite(*blind_tag, OWNED_SKIP_TAG_STARTING_LAYER + nb_owned_tags - 1);
    sprite_object_position((SpriteObject*)(*blind_tag), new_tag_pos.x, new_tag_pos.y);
    sprite_object_bounce((SpriteObject*)(*blind_tag), SKIP_TAG_GET_BOUNCE_STRENGTH);
    sprite_object_sway((SpriteObject*)(*blind_tag));
    play_sfx(SFX_CARD_DRAW, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);

    *blind_tag = NULL;
}

void remove_skip_tag(int tag_idx)
{
    // Remove the tag itself
    SkipTag* tag = list_get_at_idx(&s_owned_skip_tags, tag_idx);
    (void)list_remove_at_idx(&s_owned_skip_tags, tag_idx);
    skip_tag_destroy(&tag);

    rearrange_skip_tag_sprites(list_get_len(&s_owned_skip_tags), get_skip_tag_sprites_spacing());
}

void remove_all_skip_tags(void)
{
    ListItr itr = rev_list_itr_create(&s_owned_skip_tags);
    SkipTag* tag = NULL;

    while ((tag = list_itr_next(&itr)))
    {
        skip_tag_destroy(&tag);
    }

    list_clear(&s_owned_skip_tags);
}

enum SkipTagProcessState
{
    SKIP_TAG_PROCESS_STATE_SEARCH,
    SKIP_TAG_PROCESS_STATE_TRIGGER,
    SKIP_TAG_PROCESS_STATE_REMOVE,
    SKIP_TAG_PROCESS_STATE_MAX
};

static void skip_tag_search_for_event(void);
static void skip_tag_trigger_for_event(void);
static void skip_tag_remove_for_event(void);

static StateInfo tag_process_state_fn[SKIP_TAG_PROCESS_STATE_MAX] = {
    STATE_INFO_UPDATE_FN_ONLY(skip_tag_search_for_event),
    STATE_INFO_UPDATE_FN_ONLY(skip_tag_trigger_for_event),
    STATE_INFO_UPDATE_FN_ONLY(skip_tag_remove_for_event)
};

static StateMachine tag_process_sm =
    STATE_MACHINE_DEFINE(tag_process_state_fn, SKIP_TAG_PROCESS_STATE_MAX);

typedef struct
{
    // General info about the processing of tags
    s32 tags_timer;
    enum SkipTagProcessState tag_process_state;
    enum SkipTagEvent tag_event;
    enum SkipTagProcStage tag_proc_stage;
} TagProcessInfo;

static TagProcessInfo s_tag_process_info = {
    .tags_timer = TM_ZERO,
    .tag_process_state = SKIP_TAG_PROCESS_STATE_MAX,
    .tag_event = SKIP_TAG_EVENT_NONE,
    .tag_proc_stage = SKIP_TAG_PROCESS_STAGE_NONE,
};

enum SkipTagProcStage skip_tag_process_get_proc_stage(void)
{
    return s_tag_process_info.tag_proc_stage;
}

void skip_tag_process_start(enum SkipTagEvent checked_tag_event)
{
    if (tag_process_sm.registered)
        return;

    s_tag_process_info.tags_timer = TM_ZERO;
    s_tag_process_info.tag_process_state = SKIP_TAG_PROCESS_STATE_SEARCH;
    s_tag_process_info.tag_event = checked_tag_event;
    s_tag_process_info.tag_proc_stage = SKIP_TAG_PROCESS_STAGE_NONE;

    skip_tag_process_resume();
}

void skip_tag_process_pause(void)
{
    // We're keeping track of the state separately so that we can restore it when
    // we resume the state machine processing
    s_tag_process_info.tag_process_state = tag_process_sm.state;
    s_tag_process_info.tags_timer = TM_ZERO;
    state_machine_remove(&tag_process_sm);
}

void skip_tag_process_resume(void)
{
    if (s_tag_process_info.tag_process_state >= SKIP_TAG_PROCESS_STATE_MAX)
        return;

    state_machine_register(&tag_process_sm);
    state_machine_change_state(&tag_process_sm, s_tag_process_info.tag_process_state);
}

// Skip Tags will be fully processed over 61 frames (1 + 2 * TM_SKIP_TAG_ANIM_DURATION) so we have
// time to process what's happening:
//   - Detect triggered Tag, executed instantly but will take a full frame given how state machines
//     work
//   - Starting the little bouncy animation as the Tag triggers
//   - Delete triggered Tag, serves as a little pause

static SkipTag* s_consumed_tag = NULL;
static int s_consumed_tag_idx = 0;
static SkipTagCallback s_consumed_tag_effect_func = NULL;

static void skip_tag_search_for_event(void)
{
    s_tag_process_info.tag_proc_stage = SKIP_TAG_PROCESS_STAGE_NONE;
    s_tag_process_info.tags_timer++;

    int nb_owned_tags = list_get_len(&s_owned_skip_tags);

    for (; s_consumed_tag_idx < nb_owned_tags; s_consumed_tag_idx++)
    {
        s_consumed_tag = list_get_at_idx(&s_owned_skip_tags, s_consumed_tag_idx);

        if (s_consumed_tag == NULL)
            continue;

        const SkipTagInfo* info = get_skip_tag_registry_entry(s_consumed_tag->type);

        if (info == NULL || info->event_type != s_tag_process_info.tag_event)
            continue;

        // Return early in case the tag can trigger, will apply effect later
        if (info->tag_condition_func())
        {
            s_consumed_tag_effect_func = info->tag_effect_func;
            s_tag_process_info.tag_proc_stage = SKIP_TAG_PROCESS_STAGE_NONE;
            state_machine_change_state(&tag_process_sm, SKIP_TAG_PROCESS_STATE_TRIGGER);
            return;
        }
    }

    s_consumed_tag_idx = 0;
    s_tag_process_info.tag_proc_stage = SKIP_TAG_PROCESS_STAGE_END;
    s_tag_process_info.tag_process_state = SKIP_TAG_PROCESS_STATE_MAX;
    skip_tag_process_pause();
}

static void skip_tag_trigger_for_event(void)
{
    s_tag_process_info.tag_proc_stage = SKIP_TAG_PROCESS_STAGE_NONE;
    if (s_tag_process_info.tags_timer++ % FRAMES(TM_SKIP_TAG_ANIM_DURATION) != TM_ZERO)
        return;

    BG_POINT tag_pos = {fx2int(s_consumed_tag->x), fx2int(s_consumed_tag->y)};

    // Set tiles to the "activated" ones, each with colors that correspond to their tag type
    s_consumed_tag->type += MAX_SKIP_TAG_TYPES;
    skip_tag_set_sprite(s_consumed_tag, OWNED_SKIP_TAG_STARTING_LAYER + s_consumed_tag_idx);
    sprite_object_position((SpriteObject*)s_consumed_tag, tag_pos.x, tag_pos.y);

    // Reset rotation so that the sprite isn't too distorted when bouncing if
    // it's still moving from a previous animation
    s_consumed_tag->rotation = 0;
    s_consumed_tag->vrotation = 0;
    sprite_object_bounce((SpriteObject*)s_consumed_tag, SKIP_TAG_TRIGGER_BOUNCE_STRENGTH);
    play_sfx(SFX_REDEEM_TAG, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);

    s_tag_process_info.tag_proc_stage = SKIP_TAG_PROCESS_STAGE_TRIGGER;
    state_machine_change_state(&tag_process_sm, SKIP_TAG_PROCESS_STATE_REMOVE);

    // Apply tag here so it matches the animation, but AFTER changing the processing state,
    // else, with the Boss tag, the trigger state will be restored and `s_consumed_tag_effect_func`
    // will be dereferenced after being set to NULL
    (*s_consumed_tag_effect_func)();
    s_consumed_tag = NULL;
    s_consumed_tag_effect_func = NULL;
}

static void skip_tag_remove_for_event(void)
{
    s_tag_process_info.tag_proc_stage = SKIP_TAG_PROCESS_STAGE_NONE;
    if (s_tag_process_info.tags_timer++ % FRAMES(TM_SKIP_TAG_ANIM_DURATION) != TM_ZERO)
        return;

    remove_skip_tag(s_consumed_tag_idx);
    state_machine_change_state(&tag_process_sm, SKIP_TAG_PROCESS_STATE_SEARCH);
}
