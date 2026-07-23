#include "card.h"
#include "game/common_ui.h"
#include "joker.h"
#include "list.h"
#include "skip_tag.h"
#include "sprite.h"

POOL_ENTRY(Sprite, MAX_SPRITES, IWRAM);
POOL_ENTRY(Joker, MAX_ACTIVE_JOKERS, EWRAM);
POOL_ENTRY(JokerObject, MAX_ACTIVE_JOKERS, EWRAM);
POOL_ENTRY(Card, MAX_CARDS, EWRAM);
POOL_ENTRY(CardObject, MAX_CARDS_ON_SCREEN, EWRAM);
POOL_ENTRY(SkipTag, MAX_SKIP_TAGS, EWRAM);
POOL_ENTRY(ListNode, MAX_LIST_NODES, IWRAM);
