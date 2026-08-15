/**
 * @file sprite_container.h
 * @brief Definitions for the SpriteContainer data structure and its handling functions
 */

#ifndef SPRITE_CONTAINER_H
#define SPRITE_CONTAINER_H

#include "graphic_utils.h"
#include "list.h"
#include "sprite.h"

const static Rect CARD_SPRITE_LOCAL_AABB = {4, 0, 28, 32};

enum LayoutDirection
{
    LAYOUT_DIR_HORIZONTAL,
    LAYOUT_DIR_VERTICAL,
    LAYOUT_DIR_MAX
};

enum LayoutJustification
{
    LAYOUT_JUST_BEGIN,
    LAYOUT_JUST_CENTER
};

/**
 * @brief A structure organizing a List of SpriteObjects
 */
typedef struct SpriteContainer
{
    /**
     * @brief List of SpriteObjects to organize
     */
    List* contents;

    /**
     * @brief A Rect holding the position and size of the Container
     */
    Rect pos;

    /**
     * @brief Whether the sprites are to be organized horizontally or vertically
     */
    enum LayoutDirection direction;

    /**
     * @brief Determines the alignment of the sprites
     */
    enum LayoutJustification justification;

    /**
     * @brief A Rect representing the actual, local position/size of the art within the Sprites.
     *
     * Examples:
     *   - For the Skip Tags, the sprite is 16x16 px, but the actual art always fits within
     *     a centered 10x10 px square, which gives `{3, 3, 13, 13}`
     *   - For the Jokers, Cards, Planets etc, they are generally rectangles of 24x32 px centered
     *     in a 32x32 px sprite, which gives `{4, 0, 28, 32}`
     *
     * The Sprites should all be uniform within a single Container, so this is stored here instead
     * of per-sprite.
     */
    Rect sprite_local_aabb;

    /**
     * @brief By how much do we need to space the Sprites, in case there are not enough to fill the
     *         whole Container.
     */
    int minimum_spacing;
} SpriteContainer;

// clang-format off
const static Rect DEFAULT_POS_SIZE_RECT = {0, 0, 1, 1};
#define SPRITE_CONTAINER_DEFAULT {              \
    .contents          = NULL,                  \
    .pos               = DEFAULT_POS_SIZE_RECT, \
    .direction         = LAYOUT_DIR_HORIZONTAL, \
    .justification     = LAYOUT_JUST_CENTER,    \
    .sprite_local_aabb = DEFAULT_POS_SIZE_RECT, \
    .minimum_spacing   = 0                      \
}
// clang-format on

/**
 * Prepend an entry to the `head` of a @ref SpriteContainer contents
 *
 * @param container pointer to a @ref SpriteContainer
 * @param sprite_object pointer to a SpriteObject to put into the @ref SpriteContainer
 *
 * @sa list_push_front
 */
void container_push_front(SpriteContainer* container, SpriteObject* sprite_object);

/**
 * Append an entry to the `tail` of a @ref SpriteContainer contents
 *
 * @param container pointer to a @ref SpriteContainer
 * @param sprite_object pointer to a SpriteObject to put into the @ref SpriteContainer
 *
 * @sa list_push_back
 */
void container_push_back(SpriteContainer* container, SpriteObject* sprite_object);

/** Insert a SpriteObject into a @ref SpriteContainer a specific index
 *
 * @param container pointer to a @ref SpriteContainer
 * @param sprite_object pointer to a SpriteObject to put into the @ref SpriteContainer
 * @param idx desired index to insert
 *
 * @sa list_insert
 */
void container_insert(SpriteContainer* container, SpriteObject* sprite_object, unsigned int idx);

/**
 * Swap the data pointers at the specified indices of a @ref SpriteContainer
 *
 * If either indices are larger than the length of the underlying list, return false.
 *
 * @param container pointer to a @ref SpriteContainer
 * @param idx_a desired index to swap with idx_b
 * @param idx_b desired index to swap with idx_a
 *
 * @return true if successful, false otherwise
 *
 * @sa list_swap
 */
bool container_swap(SpriteContainer* container, unsigned int idx_a, unsigned int idx_b);

/**
 * Remove a SpriteContainer's underlying List node at the specified index
 *
 * @param container pointer to a @ref SpriteContainer
 * @param idx index of the desired @ref ListNode in the container
 *
 * @return `true` if successfully removed, `false` if out-of-bounds
 */
bool container_remove_at_idx(SpriteContainer* container, unsigned int idx);

/**
 * Remove a SpriteContainer's underlying List's node with the matching pointer
 *
 * @param container pointer to a @ref SpriteContainer
 * @param sprite_object pointer to a SpriteObject in node in container
 *
 * @return `true` if successfully removed, `false` otherwise
 *
 * @note When working with @ref ListItr, use @ref list_itr_remove_current_node()
 */
bool container_remove_data(SpriteContainer* container, SpriteObject* sprite_object);

/**
 * @brief Remove the current @ref ListNode from the iterator from the @ref SpriteContainer's
 *         underlying @ref List.
 *
 * @param container pointer to the @ref SpriteContainer to which the iterator's @ref List belongs to
 * @param itr pointer to the @ref ListItr
 *
 * @note When working with @ref ListItr, use this and not @ref container_remove_at_idx() as it will
 * "break" the iterator.
 *
 * @sa list_itr_remove_current_node
 */
void container_itr_remove_current_node(SpriteContainer* container, ListItr* itr);

#endif // SPRITE_CONTAINER_H
