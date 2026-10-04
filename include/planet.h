/**
 * @file planet.h
 *
 * @brief Planet cards: shop items that are used on purchase and level up one poker hand.
 */
#ifndef PLANET_H
#define PLANET_H

#include "hand.h"
#include "item.h"

#define PLANET_BUY_PRICE   3
#define HAND_LEVEL_MAX     99
#define PLANET_PB          (NUM_PALETTES - 1) // OBJ palette bank reserved for Planet sprites

typedef struct PlanetObject
{
    Item; // First member struct inheritance
    u8 hand_type;
    s8 layer;
    bool in_use;
} PlanetObject;

/** @brief Set every hand back to level 1, to be called when a run starts. */
void hand_levels_reset(void);

/** @brief Current level of a hand type (Royal Flush shares the Straight Flush level). */
int hand_get_level(enum HandType hand_type);

/** @brief Raise a hand type by one level (capped at HAND_LEVEL_MAX). */
void hand_level_up(enum HandType hand_type);

/** @brief Extra chips/mult given by the current level of a hand type. */
u32 hand_get_level_bonus_chips(enum HandType hand_type);
u32 hand_get_level_bonus_mult(enum HandType hand_type);

const char* planet_get_name(enum HandType hand_type);

/** @brief Print the planet description in @p dest_rect, returns the number of lines used. */
int planet_object_print_desc(PlanetObject* planet, Rect dest_rect);

// ItemFuncs implementations
Item* planet_object_roll_new(enum RngSequence key);
int planet_object_get_buy_price(Item* item);
bool planet_object_can_acquire(Item* item);
void planet_object_acquire(Item* item);
void planet_object_dispose(Item** item);

#endif // PLANET_H
