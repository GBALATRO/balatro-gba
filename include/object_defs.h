struct SpriteObject;

typedef struct object_funcs
{
    int (*get_buy_price)(struct SpriteObject* sprite_object);
    void (*add_to_inventory)(struct SpriteObject* sprite_object);
} ObjectFuncs;

enum ObjectType
{
    OBJ_TYPE_JOKER,
    OBJ_TYPE_PLAYING_CARD,
    OBJ_TYPE_CONSUMABLE, // TODO: Expand to PLANET, TAROT, and SPECTRAL?

    OBJ_NUM_TYPES
};