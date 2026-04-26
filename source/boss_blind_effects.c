/**
 * @file boss_blind_effects.c
 * @brief Boss blind effect implementations.
 */

#include boss_blind_effects.h

static bool s_eye_played_types[BOSS_BLIND_MAX_HAND_TYPES];

void boss_blind_apply_round_start(enum BlindType id, int* hands, int* discards, int* hand_size)
{
    (void)hand_size;
    switch (id)
    {
        case BLIND_TYPE_NEEDLE:
            *hands = 1;
            break;
        case BLIND_TYPE_WATER:
            *discards = 0;
            break;
        default:
            break;
    }
}

bool boss_blind_validate_play(enum BlindType id, int hand_selections, int hand_type)
{
    switch (id)
    {
        case BLIND_TYPE_PSYCHIC:
            return hand_selections == 5;
        case BLIND_TYPE_EYE:
            if (hand_type >= 0 && hand_type < BOSS_BLIND_MAX_HAND_TYPES)
                return !s_eye_played_types[hand_type];
            return true;
        default:
            return true;
    }
}

int boss_blind_get_hook_count(enum BlindType id)
{
    return (id == BLIND_TYPE_HOOK) ? 2 : 0;
}

int boss_blind_get_tooth_penalty(enum BlindType id, int cards_played)
{
    if (id == BLIND_TYPE_TOOTH && cards_played > 0)
        return cards_played;
    return 0;
}

bool boss_blind_is_ox_active(enum BlindType id)
{
    return id == BLIND_TYPE_OX;
}

void boss_blind_register_hand(enum BlindType id, int hand_type)
{
    if (id == BLIND_TYPE_EYE && hand_type >= 0 && hand_type < BOSS_BLIND_MAX_HAND_TYPES)
        s_eye_played_types[hand_type] = true;
}

void boss_blind_reset(void)
{
    for (int i = 0; i < BOSS_BLIND_MAX_HAND_TYPES; i++)
        s_eye_played_types[i] = false;
}