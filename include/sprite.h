/**
 * @file sprite.h
 *
 * @brief Sprite system for Gbalatro
 */
#ifndef SPRITE_H
#define SPRITE_H

#include <maxmod.h>
#include <tonc.h>

/**
 * @name Sprite system constants
 * @{
 */
#define CARD_SPRITE_SIZE                  32
#define MAX_AFFINES                       32
#define MAX_SPRITES                       128
#define MAX_SPRITE_OBJECTS                16
#define SPRITE_FOCUS_RAISE_PX             10
#define CARD_FOCUS_SFX_PITCH_OFFSET_RANGE 512

/** @} */

/**
 * @brief Sprite struct for GBA hardware specifics
 */
typedef struct
{
    /**
     * @brief GBA sprite attribute registers info (A0-A2)
     */
    OBJ_ATTR* obj;

    /**
     * @brief GBA sprite affine matrices registers info
     */
    OBJ_AFFINE* aff;

    /**
     * @brief Sprite position on screen in pixels
     */
    POINT pos;

    /**
     * @brief Sprite index in memory managed by GBAlatro
     */
    int idx;
} Sprite;

/**
 * @brief A sprite object is a sprite that is focusable and movable in animation
 */
typedef struct
{
    /**
     * @brief Sprite configuration info
     */
    Sprite* sprite;

    /**
     * @brief Target position
     */
    FIXED tx, ty;

    /**
     * @brief Current position
     */
    FIXED x, y;

    /**
     * @brief Velocity
     */
    FIXED vx, vy;

    /**
     * @brief Target Scale
     */
    FIXED tscale;

    /**
     * @brief Current Scale, in units for tonc's `obj_aff_rotscale`
     */
    FIXED scale;

    /**
     * @brief Scale velocity AKA the rate of change of scaling ops
     */
    FIXED vscale;

    /**
     * @brief Target rotation
     */
    FIXED trotation;

    /**
     * @brief Actual rotation, in units for tonc's `obj_aff_rotscale`
     */
    FIXED rotation;

    /**
     * @brief Rotation velocity
     */
    FIXED vrotation;

    /**
     * @brief Flag to focus on particular sprite
     */
    bool focused;

} SpriteObject;

/**
 * @brief Create a pointer to a valid Sprite
 *
 * @param a0 requested attribute 0 of OBJ_ATTR
 * @param a1 requested attribute 1 of OBJ_ATTR
 * @param tid base tile index of sprite, part of attribute 2
 * @param pb Palette-bank
 * @param sprite_index index in memory
 *
 * @return Valid @ref SpriteInfo if allocations are successful.
 *         Otherwise, return NULL.
 */
Sprite* sprite_new(u16 a0, u16 a1, u32 tid, u32 pb, int sprite_index);

/**
 * @brief Destroy sprite
 *
 * @param sprite pointer to a pointer of @ref Sprite
 */
void sprite_destroy(Sprite** sprite);

/**
 * @brief Get index in the object buffer
 *
 * @param sprite pointer to @Sprite, cannot be NULL
 *
 * @return Index in object buffer if `sprite` is valid, otherwise UNDEFINED.
 */
int sprite_get_layer(Sprite* sprite);

/**
 * @brief Get width and height of a sprite
 *
 * @param sprite pointer to @Sprite, cannot be NULL
 * @param width pointer to variable to be set, cannot be NULL
 * @param height pointer to variable to be set, cannot be NULL
 *
 * @return true is successful, false if otherwise. Upon success,
 *         width and height contain valid data, otherwise, the
 *         variables are invalid.
 */
bool sprite_get_dimensions(Sprite* sprite, int* width, int* height);

/**
 * @brief Get height of a sprite
 *
 * @param sprite pointer to @Sprite, cannot be NULL
 * @param height pointer to variable to be set, cannot be NULL
 *
 * @return true is successful, false if otherwise. Upon success,
 *         height contain valid data, otherwise, the variables are invalid.
 */
bool sprite_get_height(Sprite* sprite, int* height);

/**
 * @brief Get width of a sprite
 *
 * @param sprite pointer to @Sprite, cannot be NULL
 * @param width pointer to variable to be set, cannot be NULL
 *
 * @return true is successful, false if otherwise. Upon success,
 *         width contain valid data, otherwise, the variables are invalid.
 */
bool sprite_get_width(Sprite* sprite, int* width);

// Sprite functions
void sprite_init(void);
void sprite_draw(void);
int sprite_get_pb(const Sprite* sprite);

// SpriteObject methods
SpriteObject* sprite_object_new();
void sprite_object_destroy(SpriteObject** sprite_object);
void sprite_object_set_sprite(SpriteObject* sprite_object, Sprite* sprite);
void sprite_object_reset_transform(SpriteObject* sprite_object);
void sprite_object_update(SpriteObject* sprite_object);
void sprite_object_shake(SpriteObject* sprite_object, mm_word sound_id);

Sprite* sprite_object_get_sprite(SpriteObject* sprite_object);
void sprite_object_set_focus(SpriteObject* sprite_object, bool focus);
bool sprite_object_get_dimensions(SpriteObject* sprite_object, int* width, int* height);
bool sprite_object_get_height(SpriteObject* sprite_object, int* height);
bool sprite_object_get_width(SpriteObject* sprite_object, int* width);
bool sprite_object_is_focused(SpriteObject* sprite_object);

INLINE void sprite_position(Sprite* sprite, int x, int y)
{
    sprite->pos.x = x;
    sprite->pos.y = y;

    obj_set_pos(sprite->obj, x, y);
}

#endif // SPRITE_H
