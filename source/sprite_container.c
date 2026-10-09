#include "sprite_container.h"

#include "util.h"

void container_update(SpriteContainer* container)
{
    // position and size of the sprite along the main `direction`
    POINT pos_size;
    // same, but with direction orthogonal to the main one
    POINT pos_size_ortho;
    // Sprites' starting position along the main and orthogonal directions respectively
    POINT start_pos;
    // size of the container along the main and orthogonal directions respectively
    POINT max_length;

    if (container->direction == LAYOUT_DIR_HORIZONTAL)
    {
        pos_size.x = container->sprite_local_aabb.left;
        pos_size.y = rect_width(&(container->sprite_local_aabb)) - 1;

        pos_size_ortho.x = container->sprite_local_aabb.top;
        pos_size_ortho.y = rect_height(&(container->sprite_local_aabb)) - 1;

        start_pos.x = container->pos.left;
        start_pos.y = container->pos.top;

        max_length.x = rect_width(&(container->pos));
        max_length.y = rect_height(&(container->pos));
    }
    else
    {
        pos_size.x = container->sprite_local_aabb.top;
        pos_size.y = rect_height(&(container->sprite_local_aabb)) - 1;

        pos_size_ortho.x = container->sprite_local_aabb.left;
        pos_size_ortho.y = rect_width(&(container->sprite_local_aabb)) - 1;

        start_pos.x = container->pos.top;
        start_pos.y = container->pos.left;

        max_length.x = rect_height(&(container->pos));
        max_length.y = rect_width(&(container->pos));
    }

    int nb_sprites = list_get_len(container->contents);
    int spacing = container->maximum_spacing;
    int naive_length = nb_sprites * pos_size.y + (nb_sprites - 1) * spacing;

    int overrun = naive_length - max_length.x;
    int overrun_ortho = pos_size_ortho.y - max_length.y;

    // If sprites take too much space, correct the spacing
    if (overrun > 0 && nb_sprites > 1)
    {
        // Ceil the reduction so the corrected layout does not still overflow.
        int reduce = (overrun + (nb_sprites - 2)) / (nb_sprites - 1);
        spacing -= reduce;

        // If the sprites need to be centered, and depending on the numer of them, the reduction to
        // the spacing may cause an imbalance that can be solved by shifting the sprites slightly
        // to the right
        if (container->justification == LAYOUT_JUST_CENTER)
        {
            int new_length = nb_sprites * pos_size.y + (nb_sprites - 1) * spacing;
            start_pos.x += (max_length.x - new_length) / 2;
        }
    }
    // When they fit inside the container, correct the starting point if need be
    else if (overrun < 0)
    {
        switch (container->justification)
        {
            case LAYOUT_JUST_CENTER:
                start_pos.x -= overrun / 2;
                break;

            case LAYOUT_JUST_END:
                start_pos.x -= overrun;
                break;

            default:
                // Don't need to correct anything for the default alignment
                break;
        }
    }
    // And don't do anything if the fit is perfect (overrun == 0)

    // Always center along the direction orthogonal to the main one if Sprite takes too much space
    if (overrun_ortho > 0)
        start_pos.y -= overrun_ortho / 2;
    // If there's enough room, align Sprite accoring to `container->justification_ortho`
    else if (overrun_ortho < 0)
    {
        switch (container->justification_ortho)
        {
            case LAYOUT_JUST_CENTER:
                start_pos.y -= overrun_ortho / 2;
                break;

            case LAYOUT_JUST_END:
                start_pos.y -= overrun_ortho;
                break;

            default:
                // Still don't need to correct anything for the default alignment
                break;
        }
    }

    // Set sprite positions
    SpriteObject* sprite_object = NULL;
    ListItr itr = list_itr_create(container->contents);

    while ((sprite_object = list_itr_next(&itr)))
    {
        FIXED* coord;
        FIXED* coord_ortho;

        if (container->direction == LAYOUT_DIR_HORIZONTAL)
        {
            coord = &sprite_object->tx;
            coord_ortho = &sprite_object->ty;
        }
        else
        {
            coord = &sprite_object->ty;
            coord_ortho = &sprite_object->tx;
        }

        *coord = int2fx(start_pos.x - pos_size.x);
        *coord_ortho = int2fx(start_pos.y);

        start_pos.x += pos_size.y + spacing;
    }
}

void container_push_front(SpriteContainer* container, SpriteObject* sprite_object, bool do_update)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);
    list_push_front(container->contents, (void*)sprite_object);
    if (do_update)
        container_update(container);
}

void container_push_back(SpriteContainer* container, SpriteObject* sprite_object, bool do_update)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);
    list_push_back(container->contents, (void*)sprite_object);
    if (do_update)
        container_update(container);
}

void container_insert(
    SpriteContainer* container,
    SpriteObject* sprite_object,
    unsigned int idx,
    bool do_update
)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);
    list_insert(container->contents, (void*)sprite_object, idx);
    if (do_update)
        container_update(container);
}

bool container_swap(
    SpriteContainer* container,
    unsigned int idx_a,
    unsigned int idx_b,
    bool do_update
)
{
    GBAL_RETURN_IF_NULL(container, false);
    bool res = list_swap(container->contents, idx_a, idx_b);
    if (do_update)
        container_update(container);
    return res;
}

bool container_remove_at_idx(SpriteContainer* container, unsigned int idx, bool do_update)
{
    GBAL_RETURN_IF_NULL(container, false);
    bool res = list_remove_at_idx(container->contents, idx);
    if (do_update)
        container_update(container);
    return res;
}

bool container_remove_data(SpriteContainer* container, SpriteObject* sprite_object, bool do_update)
{
    GBAL_RETURN_IF_NULL(container, false);
    bool res = list_remove_data(container->contents, (void*)sprite_object);
    if (do_update)
        container_update(container);
    return res;
}

void container_itr_remove_current_node(SpriteContainer* container, ListItr* itr, bool do_update)
{
    GBAL_RETURN_IF_NULL(container, RET_NONE);
    list_itr_remove_current_node(itr);
    if (do_update)
        container_update(container);
}
