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
 *         or NULL if @p type is out of range or has no implemented functions.
 */
ItemFuncs* get_item_type_funcs(enum ItemType type);

#endif // ITEM_FUNCS_H
