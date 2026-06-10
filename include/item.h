#ifndef ITEM_H
#define ITEM_H

#include "item_defs.h"
#include "sprite.h"

typedef struct Item
{
    SpriteObject;
    enum ItemType type;
} Item;

int item_get_buy_price(Item* item);
void item_add_to_inventory(Item* item);

#endif // ITEM_H
