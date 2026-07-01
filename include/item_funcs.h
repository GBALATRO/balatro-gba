/**
 * @file item_funcs.h
 *
 * @brief An API for the item functions defined in item.h
 * Separated module from item.c/.h so item_funcs.c can include joker.h and card.h etc.
 * so the item.h doesn't depend on implementations such as jokers and cards.
 */

#ifndef ITEM_FUNCS_H
#define ITEM_FUNCS_H

#include "item.h"

/**
 * @brief Returns the function table for a given item type.
 *
 * Looks up the @ref ItemFuncs dispatch table corresponding to @p type.
 *
 * @param type The item type whose function table to retrieve.
 *
 * @return Pointer to the @ref ItemFuncs table for @p type,
 *         or NULL if @p type is invalid or has no implemented functions.
 */
ItemFuncs* get_item_type_funcs(enum ItemType type);

#endif // ITEM_FUNCS_H
