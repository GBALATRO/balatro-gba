#ifndef SPRITE_H
#define SPRITE_H

#include <maxmod.h>
#include <tonc.h>

#define CARD_SPRITE_SIZE                  32
#define MAX_AFFINES                       32
#define MAX_SPRITES                       128
#define MAX_SPRITE_OBJECTS                16
#define SPRITE_FOCUS_RAISE_PX             10
#define CARD_FOCUS_SFX_PITCH_OFFSET_RANGE 512

typedef struct
{
    OBJ_ATTR* obj;
    OBJ_AFFINE* aff;
    POINT pos;
    int idx;
} SpriteInfo;

// A sprite object is a sprite that is focusable and movable in animation
typedef struct
{
    Sprite* sprite;
    FIXED tx, ty; // target position
    FIXED x, y;   // position
    FIXED vx, vy; // velocity
    FIXED tscale;
    FIXED scale;
    FIXED vscale;
    FIXED trotation; // this never gets used so i might remove it later
    FIXED rotation;
    FIXED vrotation;
    bool focused;

} SpriteObject;

// Sprite methods
SpriteInfo* sprite_new(u16 a0, u16 a1, u32 tid, u32 pb, int sprite_index);
SpriteInfo* affine_sprite_new(u16 a0, u16 a1, u32 tid, u32 pb);
void sprite_destroy(SpriteInfo** sprite);
int sprite_get_layer(SpriteInfo* sprite);
bool sprite_get_dimensions(SpriteInfo* sprite, int* width, int* height);
bool sprite_get_height(SpriteInfo* sprite, int* height);
bool sprite_get_width(SpriteInfo* sprite, int* width);

// Sprite functions
void sprite_init();
void sprite_draw();
int sprite_get_pb(const SpriteInfo* sprite);

// SpriteObject methods
SpriteObject* sprite_object_new();
void sprite_object_destroy(SpriteObject** sprite_object);
void sprite_object_set_sprite(SpriteObject* sprite_object, SpriteInfo* sprite);
void sprite_object_reset_transform(SpriteObject* sprite_object);
void sprite_object_update(SpriteObject* sprite_object);
void sprite_object_shake(SpriteObject* sprite_object, mm_word sound_id);

SpriteInfo* sprite_object_get_sprite(SpriteObject* sprite_object);
void sprite_object_set_focus(SpriteObject* sprite_object, bool focus);
bool sprite_object_get_dimensions(SpriteObject* sprite_object, int* width, int* height);
bool sprite_object_get_height(SpriteObject* sprite_object, int* height);
bool sprite_object_get_width(SpriteObject* sprite_object, int* width);
bool sprite_object_is_focused(SpriteObject* sprite_object);

INLINE void sprite_position(SpriteInfo* sprite, int x, int y)
{
    sprite->pos.x = x;
    sprite->pos.y = y;

    obj_set_pos(sprite->obj, x, y);
}

#endif // SPRITE_H
