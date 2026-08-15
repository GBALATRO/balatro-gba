#include "sprite_container.h"

#include "util.h"

static inline void container_update(SpriteContainer* container)
{
    POINT pos_size;
    int max_length;
    int start_pos;

    if (container->direction == LAYOUT_DIR_HORIZONTAL)
    {
        pos_size.x = container->sprite_local_aabb.left;
        pos_size.y = rect_width(&(container->sprite_local_aabb));

        max_length = rect_width(&(container->pos));
        start_pos = container->pos.left;
    }
    else
    {
        pos_size.x = container->sprite_local_aabb.top;
        pos_size.y = rect_height(&(container->sprite_local_aabb));

        max_length = rect_height(&(container->pos));
        start_pos = container->pos.top;
    }

    int nb_sprites = list_get_len(container->contents);
    int spacing = container->minimum_spacing;
    int naive_length = nb_sprites * pos_size.y + (nb_sprites - 1) * spacing;

    int overrun = naive_length - max_length;

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
            start_pos += (max_length - new_length) / 2;
        }
    }
    // If they fit inside the container, correct the starting point if they need to be centered
    else if (overrun < 0 && container->justification == LAYOUT_JUST_CENTER)
    {
        start_pos -= overrun / 2;
    }
    // And don't do anything if the fit is perfect (overrun == 0)

    // Set sprite positions
    SpriteObject* sprite_object = NULL;
    ListItr itr = list_itr_create(container->contents);

    while ((sprite_object = list_itr_next(&itr)))
    {
        FIXED* coord = (container->direction == LAYOUT_DIR_HORIZONTAL) ? &sprite_object->tx
                                                                       : &sprite_object->ty;
        *coord = int2fx(start_pos - pos_size.x);

        start_pos += pos_size.y + spacing;
    }
}

void container_push_front(SpriteContainer* container, SpriteObject* sprite_object)
{
    GBAL_RETURN_IF_NULL_VOID(container);
    list_push_front(container->contents, (void*)sprite_object);
    container_update(container);
}

void container_push_back(SpriteContainer* container, SpriteObject* sprite_object)
{
    GBAL_RETURN_IF_NULL_VOID(container);
    list_push_back(container->contents, (void*)sprite_object);
    container_update(container);
}

void container_insert(SpriteContainer* container, SpriteObject* sprite_object, unsigned int idx)
{
    GBAL_RETURN_IF_NULL_VOID(container);
    list_insert(container->contents, (void*)sprite_object, idx);
    container_update(container);
}

bool container_swap(SpriteContainer* container, unsigned int idx_a, unsigned int idx_b)
{
    GBAL_RETURN_IF_NULL_RET(container, false);
    bool res = list_swap(container->contents, idx_a, idx_b);
    container_update(container);
    return res;
}

bool container_remove_at_idx(SpriteContainer* container, unsigned int idx)
{
    GBAL_RETURN_IF_NULL_RET(container, false);
    bool res = list_remove_at_idx(container->contents, idx);
    container_update(container);
    return res;
}

bool container_remove_data(SpriteContainer* container, SpriteObject* sprite_object)
{
    GBAL_RETURN_IF_NULL_RET(container, false);
    bool res = list_remove_data(container->contents, (void*)sprite_object);
    container_update(container);
    return res;
}

void container_itr_remove_current_node(SpriteContainer* container, ListItr* itr)
{
    GBAL_RETURN_IF_NULL_VOID(container);
    list_itr_remove_current_node(itr);
    container_update(container);
}
