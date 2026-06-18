// clang-format off
// (stateEnum, on_init, on_update, on_exit)
DEF_STATE_INFO(GAME_STATE_SPLASH_SCREEN, splash_screen_on_init,     splash_screen_on_update,     splash_screen_on_exit    )
DEF_STATE_INFO(GAME_STATE_MAIN_MENU,     game_main_menu_on_init,    game_main_menu_on_update,    game_main_menu_on_exit   )
DEF_STATE_INFO(GAME_STATE_OPTIONS_MENU,  game_options_menu_on_init, game_options_menu_on_update, game_options_menu_on_exit)
DEF_STATE_INFO(GAME_STATE_RUN_SETUP ,    game_run_setup_on_init,    game_run_setup_on_update,    game_run_setup_on_exit   )
DEF_STATE_INFO(GAME_STATE_GAME_START,    noop,                      game_start,                  noop                     )
DEF_STATE_INFO(GAME_STATE_ROUND,         game_round_on_init,        game_round_on_update,        noop                     )
DEF_STATE_INFO(GAME_STATE_ROUND_END,     game_round_end_on_init,    game_round_end_on_update,    game_round_end_on_exit   )
DEF_STATE_INFO(GAME_STATE_SHOP,          game_shop_on_init,         game_shop_on_update,         game_shop_on_exit        )
DEF_STATE_INFO(GAME_STATE_BLIND_SELECT,  game_blind_select_on_init, game_blind_select_on_update, game_blind_select_on_exit)
DEF_STATE_INFO(GAME_STATE_LOSE,          game_lose_on_init,         game_over_on_update,         game_over_on_exit        )
DEF_STATE_INFO(GAME_STATE_WIN,           game_win_on_init,          game_over_on_update,         game_over_on_exit        )
// clang-format on
