/**
 * @file pause_menu.h
 *
 * @brief Pause Menu substate funstions
 */

#ifndef PAUSE_MENU_H
#define PAUSE_MENU_H

/**
 * @brief Open the Pause Menu. Will erase and then take the whole screen for itself.
 */
void pause_menu_show(void);

/**
 * @brief Hide the Pause Menu.
 */
void pause_menu_hide(void);

#endif // PAUSE_MENU_H