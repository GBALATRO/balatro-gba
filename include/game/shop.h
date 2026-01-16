#ifndef GAME_SHOP_H
#define GAME_SHOP_H

void game_shop_on_update(void);
void game_shop_on_exit(void);

void reset_shop_jokers(void);
void set_shop_joker_avail(int joker_id, bool avail);

void game_shop_change_background();

#endif // GAME_SHOP_H
