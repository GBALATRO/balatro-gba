// clang-format off
// (stateEnum, on_init, on_update, on_exit)
DEF_STATE_INFO(GAME_STATE_SPLASH_SCREEN, splash_screen_on_init,     splash_screen_on_update,     splash_screen_on_exit    )
DEF_STATE_INFO(GAME_STATE_MAIN_MENU,     main_menu_on_init,          main_menu_on_update,          main_menu_on_exit         )
DEF_STATE_INFO(GAME_STATE_OPTIONS_MENU,  options_menu_on_init,       options_menu_on_update,       options_menu_on_exit      )
DEF_STATE_INFO(GAME_STATE_RUN_SETUP ,    run_setup_on_init,          run_setup_on_update,          run_setup_on_exit         )
DEF_STATE_INFO(GAME_STATE_GAME_START,    noop,                      game_start,                  noop                     )
DEF_STATE_INFO(GAME_STATE_ROUND,         round_on_init,              round_on_update,              noop                      )
DEF_STATE_INFO(GAME_STATE_ROUND_END,     round_end_on_init,          round_end_on_update,          round_end_on_exit         )
DEF_STATE_INFO(GAME_STATE_SHOP,          shop_on_init,               shop_on_update,               shop_on_exit              )
DEF_STATE_INFO(GAME_STATE_BLIND_SELECT,  blind_select_on_init,       blind_select_on_update,       blind_select_on_exit      )
DEF_STATE_INFO(GAME_STATE_LOSE,          game_lose_on_init,         game_over_on_update,         game_over_on_exit        )
DEF_STATE_INFO(GAME_STATE_WIN,           game_win_on_init,          game_over_on_update,         game_over_on_exit        )
// clang-format on
