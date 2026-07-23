#include "sprite.h"

#include "audio_utils.h"
#include "game.h"
#include "game_variables.h"
#include "graphic_utils.h"
#include "item.h"
#include "list.h"
#include "mgba_logger.h"
#include "pool.h"
#include "random.h"
#include "soundbank.h"
#include "util.h"

#include <maxmod.h>
#include <stdlib.h>
#include <string.h>
#include <tonc.h>
#include <tonc_oam.h>

// Damping Constants: SPRING_DAMP_NUMERATOR/2^SPRING_DAMP_DENOM_SHIFT ~ 0.699 damping factor
//
// SPRING_DAMP_ROUNDING = 2^(SPRING_DAMP_DENOM_SHIFT - 1) rounding for fixed-point arithmetic by
// adding half the denominator to round instead of truncating
#define SPRING_DAMP_NUMERATOR   179
#define SPRING_DAMP_DENOM_SHIFT 8
#define SPRING_DAMP_ROUNDING    (1 << (SPRING_DAMP_DENOM_SHIFT - 1))

/**
 * @brief Starting TIDs LUT for all sprite types. Filled at init by calling `common_ui_init`
 *
 * @sa common_ui_init
 */
static u32 s_sprite_starting_tids[MAX_SPRITE_TYPE] = {0};

static Bitset s_sprite_tids_slots[MAX_SPRITE_TYPE];

/**
 * @brief Starting layer LUT for all sprite types. Filled at init by calling `common_ui_init`
 *
 * @sa common_ui_init
 */
static int s_sprite_starting_layers[MAX_SPRITE_TYPE] = {0};

// clang-format off
static const int s_sprite_counts[MAX_SPRITE_TYPE] = {
    [CARD_SPRITE]        = MAX_HAND_SIZE,
    [CARD_PLAYED_SPRITE] = MAX_SELECTION_SIZE,
    [BLIND_TOKEN_SPRITE] = MAX_BLIND_TOKEN,
    [JOKER_SPRITE]       = MAX_ACTIVE_JOKERS,
    [SKIP_TAG_SPRITE]    = MAX_SKIP_TAGS,
    [DECK_SPRITE]        = 1
};
static const u8 s_sprite_sizes[MAX_SPRITE_TYPE] = {
    [CARD_SPRITE]        = CARD_SPRITE_SIZE,
    [CARD_PLAYED_SPRITE] = CARD_SPRITE_SIZE,
    [BLIND_TOKEN_SPRITE] = BLIND_SPRITE_SIZE,
    [JOKER_SPRITE]       = JOKER_SPRITE_SIZE,
    [SKIP_TAG_SPRITE]    = SKIP_TAG_SPRITE_SIZE,
    [DECK_SPRITE]        = CARD_SPRITE_SIZE
};
// clang-format on

OBJ_ATTR obj_buffer[MAX_SPRITES];
OBJ_AFFINE* obj_aff_buffer = (OBJ_AFFINE*)obj_buffer;

static Sprite* s_free_sprites[MAX_SPRITES] = {NULL};
static bool s_free_affines[MAX_AFFINES] = {false};

static List sprite_objects_list = LIST_DEFAULT;

// Sprite methods
Sprite* sprite_new(enum SpriteType sprite_type, u16 a0, u16 a1, u32 tid, u32 pb, int layer)
{
    Sprite* sprite = POOL_GET(Sprite);
    int sprite_index = sprite_type_get_starting_layer(sprite_type) + layer;

    sprite->type = sprite_type;
    sprite->obj = NULL;
    sprite->aff = NULL;
    sprite->tid_slot = (tid - s_sprite_starting_tids[sprite_type]) / s_sprite_sizes[sprite_type];
    sprite->idx = sprite_index;

    if (!s_free_sprites[sprite_index])
    {
        s_free_sprites[sprite_index] = sprite;
    }
    else
    {
        POOL_FREE(Sprite, sprite);
        return NULL;
    }

    if (a0 & ATTR0_AFF)
    {
        int aff_index = MAX_AFFINES;

        for (int i = 0; i < MAX_AFFINES; i++)
        {
            if (!s_free_affines[i])
            {
                s_free_affines[i] = true;
                aff_index = i;
                break;
            }
        }

        if (aff_index == MAX_AFFINES)
        {
            POOL_FREE(Sprite, sprite);
            return NULL;
        }

        a1 = a1 | ATTR1_AFF_ID(aff_index);

        sprite->obj = &obj_buffer[sprite_index];
        sprite->aff = &obj_aff_buffer[aff_index];
        obj_set_attr(sprite->obj, a0, a1, ATTR2_PALBANK(pb) | tid);
        obj_aff_identity(&obj_aff_buffer[aff_index]);
    }
    else
    {
        sprite->obj = &obj_buffer[sprite_index];
        obj_set_attr(sprite->obj, a0, a1, ATTR2_PALBANK(pb) | tid);
    }

    return sprite;
}

void sprite_destroy(Sprite** sprite)
{
    if (*sprite == NULL)
        return;

    obj_hide((*sprite)->obj);

    if ((*sprite)->aff != NULL)
    {
        s_free_affines[(*sprite)->aff - obj_aff_buffer] = false;
    }

    s_free_sprites[(*sprite)->idx] = NULL;

    bitset_set_idx(&s_sprite_tids_slots[(*sprite)->type], (*sprite)->tid_slot, false);

    POOL_FREE(Sprite, *sprite);

    *sprite = NULL;
}

u32 sprite_get_tid_from_layer(int sprite_index)
{
    Sprite* sprite = s_free_sprites[sprite_index];
    if (!sprite)
        return UNDEFINED;

    return s_sprite_starting_tids[sprite->type] + sprite->tid_slot * s_sprite_sizes[sprite->type];
}

u32 sprite_type_get_avail_tid(enum SpriteType sprite_type)
{
    // Get first available free tid slot
    int tid_slot = bitset_set_next_free_idx(&s_sprite_tids_slots[sprite_type]);

    if (tid_slot == UNDEFINED)
    {
        MGBA_FUNC_ERROR("Maximum sprite count for type %d exceeded", sprite_type);
        return tid_slot;
    }

    return s_sprite_starting_tids[sprite_type] + tid_slot * s_sprite_sizes[sprite_type];
}

int sprite_type_get_starting_layer(enum SpriteType sprite_type)
{
    return s_sprite_starting_layers[sprite_type];
}

int sprite_get_layer(Sprite* sprite)
{
    if (sprite == NULL || sprite->obj == NULL)
        return UNDEFINED;
    return sprite->obj - obj_buffer;
}

void sprite_swap_layers(int sprite_index1, int sprite_index2)
{
    Sprite* sprite1 = s_free_sprites[sprite_index1];
    Sprite* sprite2 = s_free_sprites[sprite_index2];

    // swap pointers

    OBJ_ATTR* obj1 = &obj_buffer[sprite_index1];
    OBJ_ATTR* obj2 = &obj_buffer[sprite_index2];

    if (sprite1)
    {
        sprite1->obj = obj2;
        sprite1->idx = sprite_index2;
    }
    if (sprite2)
    {
        sprite2->obj = obj1;
        sprite2->idx = sprite_index1;
    }

    s_free_sprites[sprite_index1] = sprite2;
    s_free_sprites[sprite_index2] = sprite1;

    // swap the OBJ_ATTR themselves, so that the sprites still point to the same data

    OBJ_ATTR tmp_obj = {0};
    obj_copy(&tmp_obj, obj1, 1);
    obj_copy(obj1, obj2, 1);
    obj_copy(obj2, &tmp_obj, 1);
}

bool sprite_get_width(Sprite* sprite, int* width)
{
    if (sprite == NULL || sprite->obj == NULL || width == NULL)
    {
        return false;
    }

    *width = obj_get_width(sprite->obj);
    return true;
}

bool sprite_get_height(Sprite* sprite, int* height)
{
    if (sprite == NULL || sprite->obj == NULL || height == NULL)
    {
        return false;
    }

    *height = obj_get_height(sprite->obj);
    return true;
}

bool sprite_get_dimensions(Sprite* sprite, int* width, int* height)
{
    if (sprite == NULL || sprite->obj == NULL || width == NULL || height == NULL)
    {
        return false;
    }

    const u8* size = obj_get_size(sprite->obj);
    *width = size[0];
    *height = size[1];
    return true;
}

// Sprite functions
void sprite_init()
{
    oam_init(obj_buffer, MAX_SPRITES);

    for (enum SpriteType sprite_type = 0; sprite_type < MAX_SPRITE_TYPE; sprite_type++)
    {
        // both the TID and starting layer stay at 0 for the first Sprite type
        if (sprite_type > 0)
        {
            s_sprite_starting_tids[sprite_type] =
                s_sprite_starting_tids[sprite_type - 1] +
                s_sprite_counts[sprite_type - 1] * s_sprite_sizes[sprite_type - 1];

            s_sprite_starting_layers[sprite_type] =
                s_sprite_starting_layers[sprite_type - 1] + s_sprite_counts[sprite_type - 1];
        }

        s_sprite_tids_slots[sprite_type].w = (u32*)malloc(BITSET_ARRAY_SIZE * sizeof(u32));
        s_sprite_tids_slots[sprite_type].nbits = BITSET_BITS_PER_WORD;
        s_sprite_tids_slots[sprite_type].nwords = BITSET_ARRAY_SIZE;
        s_sprite_tids_slots[sprite_type].cap = s_sprite_sizes[sprite_type];
    }
}

void sprite_draw()
{
    obj_aff_copy(obj_aff_mem, obj_aff_buffer, MAX_AFFINES);
    oam_copy(oam_mem, obj_buffer, MAX_SPRITES);
}

int sprite_get_pb(const Sprite* sprite)
{
    if (sprite == NULL)
    {
        return UNDEFINED;
    }
    return (sprite->obj->attr2 & ATTR2_PALBANK_MASK) >> ATTR2_PALBANK_SHIFT;
}

// SpriteObject methods
void sprite_object_init(SpriteObject* sprite_object)
{
    GBAL_RETURN_IF_NULL_VOID(sprite_object);

    sprite_object->sprite = NULL;
    sprite_object_reset_transform(sprite_object);
    sprite_object->focused = false;

    list_push_back(&sprite_objects_list, sprite_object);
}

void sprite_object_destroy(SpriteObject* sprite_object)
{
    if (sprite_object == NULL)
        return;

    list_remove_data(&sprite_objects_list, sprite_object);

    sprite_destroy(&sprite_object->sprite);
}

void sprite_object_set_sprite(SpriteObject* sprite_object, Sprite* sprite)
{
    if (sprite_object == NULL)
        return;
    sprite_destroy(&sprite_object->sprite); // Destroy the old sprite if it exists
    sprite_object->sprite = sprite;
}

void sprite_object_reset_transform(SpriteObject* sprite_object)
{
    sprite_object_position(sprite_object, 0, 0); // Target position
    sprite_object->vx = 0;
    sprite_object->vy = 0;
    sprite_object->tscale = FIX_ONE; // Target scale
    sprite_object->scale = FIX_ONE;
    sprite_object->vscale = 0;
    sprite_object->trotation = 0; // Target rotation
    sprite_object->rotation = 0;
    sprite_object->vrotation = 0;
}

static inline bool sprite_object_has_velocity(const SpriteObject* sprite_object)
{
    return sprite_object->vx != 0 || sprite_object->vy != 0 || sprite_object->vscale != 0 ||
           sprite_object->vrotation != 0;
}

static inline bool sprite_object_at_target(const SpriteObject* s)
{
    return s->x == s->tx && s->y == s->ty && s->scale == s->tscale && s->rotation == s->trotation;
}

static inline bool is_sprite_object_static(const SpriteObject* sprite_object)
{
    return !sprite_object_has_velocity(sprite_object) && sprite_object_at_target(sprite_object);
}

static inline IWRAM_CODE void update_sprite_position(SpriteObject* sprite_object)
{
    sprite_object->vx += ((sprite_object->tx - sprite_object->x) * g_game_vars.game_speed) / 8;
    sprite_object->vy += ((sprite_object->ty - sprite_object->y) * g_game_vars.game_speed) / 8;

    // Scale up the card when it's played
    sprite_object->vscale += (sprite_object->tscale - sprite_object->scale) / 8;

    // Rotate the card when it's played
    sprite_object->vrotation += (sprite_object->trotation - sprite_object->rotation) / 8;

    const FIXED epsilon = (FIX_ONE >> 6); // = 1/2^6 = 0.015625

    // Snap to target position when velocity is negligible to avoid infinite approach
    if (abs(sprite_object->vx) < epsilon && abs(sprite_object->vy) < epsilon)
    {
        sprite_object->vx = 0;
        sprite_object->vy = 0;

        sprite_object->x = sprite_object->tx;
        sprite_object->y = sprite_object->ty;
    }
    else
    {
        sprite_object->vx = (sprite_object->vx * SPRING_DAMP_NUMERATOR + SPRING_DAMP_ROUNDING) >>
                            SPRING_DAMP_DENOM_SHIFT;
        sprite_object->vy = (sprite_object->vy * SPRING_DAMP_NUMERATOR + SPRING_DAMP_ROUNDING) >>
                            SPRING_DAMP_DENOM_SHIFT;

        sprite_object->x += sprite_object->vx;
        sprite_object->y += sprite_object->vy;
    }

    // Set scale to 0 if it's close enough to the target
    if (abs(sprite_object->vscale) < epsilon)
    {
        sprite_object->vscale = 0;
        sprite_object->scale = sprite_object->tscale;
    }
    else
    {
        sprite_object->vscale =
            (sprite_object->vscale * SPRING_DAMP_NUMERATOR + SPRING_DAMP_ROUNDING) >>
            SPRING_DAMP_DENOM_SHIFT;
        sprite_object->scale += sprite_object->vscale;
    }

    // For rotation, prioritize snapping to target if close enough, then zero velocity.
    if (abs(sprite_object->vrotation) < epsilon)
    {
        sprite_object->vrotation = 0;
        sprite_object->rotation = sprite_object->trotation;
    }
    else // Apply damping and update rotation if not yet settled
    {
        sprite_object->vrotation =
            (sprite_object->vrotation * SPRING_DAMP_NUMERATOR + SPRING_DAMP_ROUNDING) >>
            SPRING_DAMP_DENOM_SHIFT;
        sprite_object->rotation += sprite_object->vrotation;
    }

    // Apply rotation and scale to the sprite
    obj_aff_rotscale(
        sprite_object->sprite->aff,
        sprite_object->scale,
        sprite_object->scale,
        -sprite_object->vx + sprite_object->rotation
    );
}

IWRAM_CODE void sprite_object_update(SpriteObject* sprite_object)
{
    if (!is_sprite_object_static(sprite_object))
        update_sprite_position(sprite_object);

    sprite_position(sprite_object->sprite, fx2int(sprite_object->x), fx2int(sprite_object->y));
}

void sprite_object_update_all(void)
{
    SpriteObject* sprite_object = NULL;
    ListItr itr = list_itr_create(&sprite_objects_list);
    while ((sprite_object = list_itr_next(&itr)))
    {
        sprite_object_update(sprite_object);
    }
}

void sprite_object_shake(SpriteObject* sprite_object, mm_word sound_id)
{
    GBAL_RETURN_IF_NULL_VOID(sprite_object);

    sprite_object->vscale = float2fx(0.3f);
    sprite_object->vrotation = float2fx(8.0f); // Rotate the card when it's scored

    if (sound_id == UNDEFINED)
        return; // If no sound ID is provided, do nothing

    play_sfx(sound_id, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
}

#define SPRITE_OBJECT_BOUNCE_VSCALE (-0.4f)
void sprite_object_bounce_grow(SpriteObject* sprite_object, float strength, mm_word sound_id)
{
    GBAL_RETURN_IF_NULL_VOID(sprite_object);

    sprite_object->vscale = float2fx(SPRITE_OBJECT_BOUNCE_VSCALE * strength);

    if (sound_id == UNDEFINED)
        return;

    play_sfx(sound_id, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
}

#define SPRITE_OBJECT_BOUNCE_SWAY_VSCALE    (-0.3f)
#define SPRITE_OBJECT_BOUNCE_SWAY_VROTATION (-10.f)
void sprite_object_bounce_sway(SpriteObject* sprite_object)
{
    GBAL_RETURN_IF_NULL_VOID(sprite_object);

    // set scale and rotation velocity to grow the card and rotate it slightly
    sprite_object->vscale = float2fx(SPRITE_OBJECT_BOUNCE_SWAY_VSCALE);
    sprite_object->vrotation = float2fx(SPRITE_OBJECT_BOUNCE_SWAY_VROTATION);
}

void sprite_object_slide_to(SpriteObject* sprite_object, BG_POINT to)
{
    GBAL_RETURN_IF_NULL_VOID(sprite_object);
    if (to.x == UNDEFINED || to.y == UNDEFINED)
        return;

    sprite_object->tx = int2fx(to.x);
    sprite_object->ty = int2fx(to.y);
}

Sprite* sprite_object_get_sprite(SpriteObject* sprite_object)
{
    if (sprite_object == NULL)
        return NULL;
    return sprite_object->sprite;
}

void sprite_object_swap_layers(SpriteObject* sprite_object1, SpriteObject* sprite_object2)
{
    if ((sprite_object1 == NULL || sprite_object1->sprite == NULL) ||
        (sprite_object2 == NULL || sprite_object2->sprite == NULL))
        return;

    if (sprite_object1->sprite->type != sprite_object2->sprite->type)
        return;

    sprite_swap_layers(sprite_object1->sprite->idx, sprite_object2->sprite->idx);
}

/**
 * @brief Get the layer of the Sprite associated with the given SpriteObject
 *
 * @param sprite_object pointer to a SpriteObject
 * @return the layer of its Sprite if it exists, UNDEFINED if it doesn't
 */
static inline int sprite_object_get_layer(SpriteObject* sprite_object)
{
    if (sprite_object == NULL)
        return UNDEFINED;
    return sprite_object->sprite->idx;
}

void sprite_object_sort_list(void* sprite_object_list, bool ascending)
{
    if (sprite_object_list == NULL)
        return;

    ListNode* start = ((List*)sprite_object_list)->head;

    if (start == NULL)
        return;

    bool swapped = false;
    ListNode* node;
    ListNode* node_prev = NULL;

    do
    {
        swapped = false;
        node = start;

        while (node->next != node_prev)
        {
            int curr_layer = sprite_object_get_layer((SpriteObject*)node->data);
            int next_layer = sprite_object_get_layer((SpriteObject*)node->next->data);
            if ((ascending && (curr_layer < next_layer)) ||
                (!ascending && (curr_layer > next_layer)))
            {
                // list_swap(node, node->next);
                sprite_object_swap_layers(
                    (SpriteObject*)node->data,
                    (SpriteObject*)node->next->data
                );
                swapped = true;
            }
            node = node->next;
        }
        node_prev = node;
    } while (swapped);
}

void sprite_object_set_focus(SpriteObject* sprite_object, bool focus)
{
    if (sprite_object->focused == focus)
    {
        return;
    }
    sprite_object->focused = focus;

    play_sfx(
        SFX_CARD_FOCUS,
        MM_BASE_PITCH_RATE + rng_get_u32() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
        SFX_DEFAULT_VOLUME
    );
    sprite_object->ty = sprite_object->ty + int2fx((focus ? -1 : 1) * SPRITE_FOCUS_RAISE_PX);
}

bool sprite_object_get_width(SpriteObject* sprite_object, int* width)
{
    if (sprite_object == NULL)
    {
        return false;
    }

    return sprite_get_width(sprite_object->sprite, width);
}

bool sprite_object_get_height(SpriteObject* sprite_object, int* height)
{
    if (sprite_object == NULL)
    {
        return false;
    }

    return sprite_get_height(sprite_object->sprite, height);
}

bool sprite_object_get_dimensions(SpriteObject* sprite_object, int* width, int* height)
{
    if (sprite_object == NULL)
    {
        return false;
    }

    return sprite_get_dimensions(sprite_object->sprite, width, height);
}

bool sprite_object_is_focused(SpriteObject* sprite_object)
{
    return sprite_object->focused;
}

static Rect sprite_object_get_text_rect_under(SpriteObject* sprite_object)
{
    int height = 0;
    int width = 0;

    if (sprite_object_get_dimensions(sprite_object, &width, &height) == false)
    {
        // fallback
        height = CARD_SPRITE_SIZE_PX;
        width = CARD_SPRITE_SIZE_PX;
    }

    Rect ret_rect = {0};

    ret_rect.left = fx2int(sprite_object->tx);
    ret_rect.top = fx2int(sprite_object->ty) + height + TILE_SIZE;
    ret_rect.right = ret_rect.left + width;
    ret_rect.bottom = ret_rect.top + TTE_CHAR_SIZE;

    return ret_rect;
}

void sprite_object_print_text_under(SpriteObject* sprite_object, const char text[])
{
    Rect text_rect = sprite_object_get_text_rect_under(sprite_object);

    update_text_rect_to_center_str(&text_rect, text, SCREEN_LEFT);

    tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_rect.left, text_rect.top, TTE_YELLOW_PB, text);
}

void sprite_object_print_price_under(SpriteObject* sprite_object, int price)
{
    // + 2 for null-terminator and "$"
    char price_str_buff[INT_MAX_DIGITS + 2];
    snprintf(price_str_buff, sizeof(price_str_buff), "$%d", price);
    sprite_object_print_text_under(sprite_object, price_str_buff);
}

void sprite_object_erase_text_under(SpriteObject* sprite_object)
{
    Rect text_rect = sprite_object_get_text_rect_under(sprite_object);

    // Add SPRITE_FOCUS_RAISE_PX to cover the focused case
    text_rect.bottom = text_rect.bottom + SPRITE_FOCUS_RAISE_PX;

    tte_erase_rect_wrapper(text_rect);
}
