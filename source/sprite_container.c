#include "sprite_container.h"

#include "util.h"

static void container_adjust_pos_and_spacing(
    SpriteContainer* container,
    int max_length,
    int overrun,
    int nb_sprites,
    int sprite_size,
    int* spacing,
    int* start_pos
);
static void container_update_set_sprite_pos(
    SpriteContainer* container,
    POINT pos_size,
    int spacing,
    int start_pos
);
static int container_compute_ortho_sprite_pos(SpriteContainer* container);

/**
 * @brief Update the position of the contents of a given Container
 *
 * @param container pointer to SpriteContainer to update the contents of. Cannot be **NULL**
 */
static inline void container_update(SpriteContainer* container)
{
    // position and size of the sprite along the main `direction`
    POINT pos_size;
    // Sprites' starting position along the main direction
    int start_pos;
    // size of the container along the main direction
    int max_length;

    if (container->orientation == LAYOUT_ORI_HORIZONTAL)
    {
        pos_size.x = container->sprite_local_visible_rect.left;
        pos_size.y = rect_width(&(container->sprite_local_visible_rect)) - 1;

        start_pos = container->pos.left;

        max_length = rect_width(&(container->pos));
    }
    else
    {
        pos_size.x = container->sprite_local_visible_rect.top;
        pos_size.y = rect_height(&(container->sprite_local_visible_rect)) - 1;

        start_pos = container->pos.top;

        max_length = rect_height(&(container->pos));
    }

    int nb_sprites = list_get_len(&container->contents_list);
    int spacing = container->maximum_spacing;
    int naive_length = nb_sprites * pos_size.y + (nb_sprites - 1) * spacing;

    int overrun = naive_length - max_length;

    container_adjust_pos_and_spacing(
        container,
        max_length,
        overrun,
        nb_sprites,
        pos_size.y,
        &spacing,
        &start_pos
    );

    container_update_set_sprite_pos(container, pos_size, spacing, start_pos);

    // Invoke custom callback if defined
    if (container->custom_update)
        container->custom_update(container);
}

/**
 * @brief Take all container info and determine at what position and with which spacing all
 *         container contents should be displayed
 *
 * @param[in]  container   SpriteContainer object to update
 * @param[in]  max_length  maximum size of the Container, in pixels, along the orientation its
 *                         Sprites are arranged in
 * @param[in]  overrun     How many pixels the naively arranged Sprites overflow from the
 *                         @p max_length
 * @param[in]  nb_sprites  Number of elements in the @p container
 * @param[in]  sprite_size Size of the Sprites, in pixels, along the orientation they are arranged
 *                         in
 * @param[out] spacing     pointer to the base spacing value, will be updated to reflect how the
 *                         Sprites will be arranged
 * @param[out] start_pos   pointer to the position the Sprites start at initially, will be
 *                         updated to reflect how the Sprites will be arranged
 */
static inline void container_adjust_pos_and_spacing(
    SpriteContainer* container,
    int max_length,
    int overrun,
    int nb_sprites,
    int sprite_size,
    int* spacing,
    int* start_pos
)
{
    // If sprites take too much space, correct the spacing
    if (overrun > 0 && nb_sprites > 1)
    {
        // Ceil the reduction so the corrected layout does not still overflow.
        int reduce = (overrun + (nb_sprites - 2)) / (nb_sprites - 1);
        *spacing -= reduce;

        // If the sprites need to be centered, and depending on the numer of them, the reduction to
        // the spacing may cause an imbalance that can be solved by shifting the sprites slightly
        // to the right
        if (container->justification == LAYOUT_JUST_CENTER)
        {
            int new_length = nb_sprites * sprite_size + (nb_sprites - 1) * (*spacing);
            *start_pos += (max_length - new_length) / 2;
        }
    }
    // When they fit inside the container, correct the starting point if need be
    else if (overrun < 0)
    {
        switch (container->justification)
        {
            case LAYOUT_JUST_CENTER:
                *start_pos -= overrun / 2;
                break;

            case LAYOUT_JUST_END:
                *start_pos -= overrun;
                break;

            default:
                // Don't need to correct anything for the default alignment
                break;
        }
    }
    // And don't do anything if the fit is perfect (overrun == 0)
}

/**
 * @brief Set the positions for all Sprites inside of a Container
 *
 * @param container SpriteContainer object to update
 * @param pos_size  Actual position/size of the Sprite on the axis along which they are
 *                      organized on
 * @param spacing   How many pixels are put in between each Sprite. Can be negative if
 *                      Sprites need to overlap
 * @param start_pos Position on the main axis the Sprites start at initially.
 */
static inline void container_update_set_sprite_pos(
    SpriteContainer* container,
    POINT pos_size,
    int spacing,
    int start_pos
)
{
    SpriteObject* sprite_object = NULL;
    ListItr itr = list_itr_create(&container->contents_list);

    while ((sprite_object = list_itr_next(&itr)))
    {
        FIXED* coord;
        if (container->orientation == LAYOUT_ORI_HORIZONTAL)
            coord = &sprite_object->tx;
        else
            coord = &sprite_object->ty;
        *coord = int2fx(start_pos - pos_size.x);

        start_pos += pos_size.y + spacing;
    }
}

/**
 * @brief Take all container info and apply the appropriate position along the axis orthogonal to
 *        the main orientation to the new element
 *
 * @note This function will be called only once per @ref SpriteObject when they are added to the
 *       @SpriteContainer 's contents
 *
 * @param container the @ref SpriteContainer in which the @p new_item has been added in
 * @param new_item  pointer to the new @ref SpriteObject added to the @p container
 */
static inline void container_update_item_on_add(SpriteContainer* container, SpriteObject* new_item)
{
    FIXED* coord_ortho =
        (container->orientation == LAYOUT_ORI_HORIZONTAL) ? &new_item->ty : &new_item->tx;
    *coord_ortho = int2fx(container_compute_ortho_sprite_pos(container));
}

/**
 * @brief Take all container info and apply the appropriate position along the axis orthogonal to
 *        the main orientation to all new elements
 *
 * @note This function will be called only once upon adding the new elements
 *
 * @param container the @ref SpriteContainer in which the @p new_items has been added in
 * @param new_items pointer to a @ref List of elements added to the @p container
 *
 * @sa container_update_item_on_add
 */
static inline void container_update_item_list_on_add(SpriteContainer* container, List* new_items)
{
    int pos_ortho = container_compute_ortho_sprite_pos(container);
    FIXED* coord_ortho;

    ListItr itr = list_itr_create(new_items);
    SpriteObject* item = NULL;
    while ((item = list_itr_next(&itr)))
    {
        coord_ortho = (container->orientation == LAYOUT_ORI_HORIZONTAL) ? &item->ty : &item->tx;
        *coord_ortho = int2fx(pos_ortho);
    }
}

/**
 * @brief Compute the position along the exis orthogonal to the main orientation a SpriteObject
 *        should be placed at by default
 *
 * @param container the @ref SpriteContainer for which the position is computed
 * @return a position along the axis orthogonal to the main orientation
 */
static inline int container_compute_ortho_sprite_pos(SpriteContainer* container)
{
    // These positions and dimentions are along the `direction` orthogonal to the main one
    int size_ortho;       // size of the sprite
    int pos_ortho;        // Sprites' starting position
    int max_length_ortho; // size of the container

    if (container->orientation == LAYOUT_ORI_HORIZONTAL)
    {
        size_ortho = rect_height(&(container->sprite_local_visible_rect)) - 1;
        pos_ortho = container->pos.top;
        max_length_ortho = rect_height(&(container->pos));
    }
    else
    {
        size_ortho = rect_width(&(container->sprite_local_visible_rect)) - 1;
        pos_ortho = container->pos.left;
        max_length_ortho = rect_width(&(container->pos));
    }

    int overrun_ortho = size_ortho - max_length_ortho;

    // Always center along the direction orthogonal to the main one if Sprite takes too much
    // space
    if (overrun_ortho > 0)
        pos_ortho -= overrun_ortho / 2;
    // If there's enough room, align Sprite accoring to `container->justification_ortho`
    else if (overrun_ortho < 0)
    {
        switch (container->justification_ortho)
        {
            case LAYOUT_JUST_CENTER:
                pos_ortho -= overrun_ortho / 2;
                break;

            case LAYOUT_JUST_END:
                pos_ortho -= overrun_ortho;
                break;

            default:
                // Still don't need to correct anything for the default alignment
                break;
        }
    }

    return pos_ortho;
}

void container_push_front(SpriteContainer* container, SpriteObject* sprite_object)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);
    GBAL_RETURN_IF_NULL(sprite_object, RET_NONE);

    list_push_front(&container->contents_list, (void*)sprite_object);

    container_update_item_on_add(container, sprite_object);
    container_update(container);
}

void container_push_back(SpriteContainer* container, SpriteObject* sprite_object)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);
    GBAL_RETURN_IF_NULL(sprite_object, RET_NONE);

    list_push_back(&container->contents_list, (void*)sprite_object);

    container_update_item_on_add(container, sprite_object);
    container_update(container);
}

void container_append_list(SpriteContainer* container, List* added_list)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);
    GBAL_RETURN_IF_NULL(added_list, RET_NONE);

    list_append_other_list(&container->contents_list, added_list);

    container_update_item_list_on_add(container, added_list);
    container_update(container);
}

void container_insert(SpriteContainer* container, SpriteObject* sprite_object, unsigned int idx)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);
    GBAL_RETURN_IF_NULL(sprite_object, RET_NONE);

    list_insert(&container->contents_list, (void*)sprite_object, idx);

    container_update_item_on_add(container, sprite_object);
    container_update(container);
}

bool container_swap(SpriteContainer* container, unsigned int idx_a, unsigned int idx_b)
{
    GBAL_RETURN_IF_NULL(container, false);

    bool res = list_swap(&container->contents_list, idx_a, idx_b);

    container_update(container);
    return res;
}

void container_clear_contents(SpriteContainer* container)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);

    list_clear(&container->contents_list);
}

bool container_remove_at_idx(SpriteContainer* container, unsigned int idx)
{
    GBAL_RETURN_IF_NULL(container, false);

    bool res = list_remove_at_idx(&container->contents_list, idx);

    container_update(container);
    return res;
}

bool container_remove_data(SpriteContainer* container, SpriteObject* sprite_object)
{
    GBAL_RETURN_IF_NULL(container, false);
    GBAL_RETURN_IF_NULL(sprite_object, false);

    bool res = list_remove_data(&container->contents_list, (void*)sprite_object);

    container_update(container);
    return res;
}

void container_itr_remove_current_node(SpriteContainer* container, ListItr* itr)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);

    list_itr_remove_current_node(itr);

    container_update(container);
}
