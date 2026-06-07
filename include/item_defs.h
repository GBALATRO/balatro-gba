struct SpriteObject;

typedef struct item_funcs
{
    /*
     * @brief Returns the buy price of the item
     */
    int (*get_buy_price)(struct SpriteObject* sprite_object);

    /**
     * @brief Adds the object to the inventory. 
     * Called when it is purchased from the shop.
     * Note that for packs this could just be to open the pack.
     */
    void (*add_to_inventory)(struct SpriteObject* sprite_object);
} ItemFuncs;

enum ItemType
{
    ITEM_TYPE_JOKER,
    ITEM_TYPE_PLAYING_CARD,
    ITEM_TYPE_CONSUMABLE, // TODO: Expand to PLANET, TAROT, and SPECTRAL?

    ITEM_NUM_TYPES
};