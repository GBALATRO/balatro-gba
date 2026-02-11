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
     * @brief Focused status (card specific, raise and lower card)
     */
    bool focused;
} SpriteObject;

/**
 * @brief Allocate and retrieve a pointer to a valid Sprite
 *
 * @param a0 attribute 0 of OBJ_ATTR
 * @param a1 attribute 1 of OBJ_ATTR
 * @param tid base tile index of sprite, part of attribute 2
 * @param pb Palette-bank
 * @param sprite_index index in memory
 *
 * @return Valid Sprite if allocations are successful.
 *         Otherwise, return **NULL**.
 */
Sprite* sprite_new(u16 a0, u16 a1, u32 tid, u32 pb, int sprite_index);

/**
 * @brief Destroy Sprite
 *
 * @param sprite pointer to a pointer of Sprite to destroy. No action if **NULL**.
 */
void sprite_destroy(Sprite** sprite);

/**
 * @brief Get index of Sprite in the GBA object buffer
 *
 * @param sprite pointer to Sprite, cannot be **NULL**
 *
 * @return Index of sprite in object buffer if `sprite` is valid, otherwise **UNDEFINED**.
 */
int sprite_get_layer(Sprite* sprite);

/**
 * @brief Get a Sprite's width and height
 *
 * @param sprite pointer to Sprite, cannot be **NULL**
 * @param width pointer to variable to be set, cannot be **NULL**
 * @param height pointer to variable to be set, cannot be **NULL**
 *
 * @return **true** if successful, **false** if otherwise. Upon success,
 *         `width` and `height` contain valid data, otherwise, the
 *         variables are unchanged.
 */
bool sprite_get_dimensions(Sprite* sprite, int* width, int* height);

/**
 * @brief Get a Sprites's height
 *
 * @param sprite pointer to Sprite, cannot be **NULL**
 * @param height pointer to variable to be set, cannot be **NULL**
 *
 * @return **true** is successful, **false** if otherwise. Upon success,
 *         `height` contains valid data, otherwise, the variable is unchanged.
 */
bool sprite_get_height(Sprite* sprite, int* height);

/**
 * @brief Get a Sprite's width
 *
 * @param sprite pointer to Sprite, cannot be **NULL**
 * @param width pointer to variable to be set, cannot be **NULL**
 *
 * @return **true** is successful, **false** if otherwise. Upon success,
 *         `width` contains valid data, otherwise, the variable is unchanged.
 */
bool sprite_get_width(Sprite* sprite, int* width);

/**
 * @brief Get the palette bank of a Sprite
 *
 * @param sprite pointer to extract associated palette bank. Cannot be **NULL**.
 *
 * @return The palette bank of the Sprite if successful, otherwise return **UNDEFINED**.
 */
int sprite_get_pb(const Sprite* sprite);

/**
 * @brief Initialize GBAlatro sprite system
 */
void sprite_init(void);

/**
 * @brief Draw Sprites to screen, to be called once per frame
 */
void sprite_draw(void);

/**
 * @brief Allocate and retrieve a pointer to a valid SpriteObject
 *
 * @return A valid pointer to an newly allocated SpriteObject
 *         if successful, othewise return **NULL**.
 */
SpriteObject* sprite_object_new();

/**
 * @brief Destroy SpriteObject
 *
 * Destroy a SpriteObject by freeing it back to the pool and releasing its
 * associated resources
 *
 * @param sprite_object pointer to a pointer of SpriteObject to destroy.
 *        Cannot be **NULL**.
 */
void sprite_object_destroy(SpriteObject** sprite_object);

/**
 * @brief Register a Sprite to an associated SpriteObject
 *
 * @param sprite_object pointer to SpriteObject to associate Sprite with.
 *                      Cannot be **NULL**.
 *
 * @param sprite pointer to Sprite to associate SpriteObject with.
 *                      Cannot be **NULL**.
 */
void sprite_object_set_sprite(SpriteObject* sprite_object, Sprite* sprite);

/**
 * @brief Reset SpriteObject's transform back to default values.
 *
 * @param sprite_object pointer to SpriteObject to reset transform.
 *                      Cannot be **NULL**.
 */
void sprite_object_reset_transform(SpriteObject* sprite_object);

/**
 * @brief Update a SpriteObject, to be called once per frame per active SpriteObject
 *
 * @param sprite_object pointer to SpriteObject to update. Cannot be **NULL**.
 */
void sprite_object_update(SpriteObject* sprite_object);

/**
 * @brief Shake SpriteObject on screen and play a sound
 *
 * @param SpriteObject pointer to SpriteObject to shake. Cannot be **NULL**.
 * @param sound_id ID of sound from maxmod to play on executing shake. If **UNDEFINED**
 *        no sound will play.
 */
void sprite_object_shake(SpriteObject* sprite_object, mm_word sound_id);

/**
 * @brief Get a SpriteObject's registered Sprite
 *
 * @param sprite_object pointer to SpriteObject's registered Sprite. Cannot be **NULL**.
 *
 * @return Sprite pointer registered to `sprite_object` if successful,
 *         otherwise return **NULL**. May be successful and **NULL** if there is no
 *         Sprite registered to the SpriteObject.
 */
Sprite* sprite_object_get_sprite(SpriteObject* sprite_object);

/**
 * @brief Set the focus for SpriteObject
 * Raises the object by SPRITE_FOCUS_RAISE_PX.
 *
 * Note: This is currently unused by CardObject as their focus is handled in
 * cards_in_hand_update_loop() but we may want to extract it from there and refactor them use this
 * instead.
 *
 * @param sprite_object pointer to SpriteObject to set the focus of. Cannot be **NULL**.
 * @param focus **true** to focus, **false** to unfocus
 */
void sprite_object_set_focus(SpriteObject* sprite_object, bool focus);

/**
 * @brief Get the width and height of SpriteObject's registered Sprite
 *
 * @param sprite_object pointer to SpriteObject to get the dimensions of. Cannot be **NULL**.
 * @param width pointer to variable to be set, cannot be **NULL**
 * @param height pointer to variable to be set, cannot be **NULL**
 *
 * @return **true** is successful, **false** if otherwise. Upon success,
 *         `width` and `height` contain valid data, otherwise, the
 *         variables are unchanged.
 */
bool sprite_object_get_dimensions(SpriteObject* sprite_object, int* width, int* height);

/**
 * @brief Get a SpriteObject's height
 *
 * @param sprite_object pointer to SpriteObject to get the height of. Cannot be **NULL**.
 * @param height pointer to variable to be set, cannot be **NULL**
 *
 * @return **true** is successful, **false** if otherwise. Upon success,
 *         `height` contains valid data, otherwise, the
 *         variable is unchanged.
 */
bool sprite_object_get_height(SpriteObject* sprite_object, int* height);

/**
 * @brief Get a SpriteObject's width
 *
 * @param sprite_object pointer to SpriteObject to get the width of. Cannot be **NULL**.
 * @param width pointer to variable to be set, cannot be **NULL**
 *
 * @return **true** is successful, **false** if otherwise. Upon success,
 *         `width` contains valid data, otherwise, the
 *         variable is unchanged.
 */
bool sprite_object_get_width(SpriteObject* sprite_object, int* width);

/**
 * @brief Get the `focused` variable from a SpriteObject
 *
 * @param sprite_object valid pointer to SpriteObject to check
 *
 * @return `true` if the SpriteObject is focused, `false` otherwise
 */
bool sprite_object_is_focused(SpriteObject* sprite_object);

/**
 * @brief Set sprite position. Inlined for efficiency
 *
 * @param sprite poitner to Sprite to adjust the position of. A **NULL** check is
 *        not performed, though the value cannot be **NULL**.
 *
 * @param x horizontal position in pixels
 * @param y vertical position in pixels
 */
INLINE void sprite_position(Sprite* sprite, int x, int y)
{
    sprite->pos.x = x;
    sprite->pos.y = y;

    obj_set_pos(sprite->obj, x, y);
}

#endif // SPRITE_H
