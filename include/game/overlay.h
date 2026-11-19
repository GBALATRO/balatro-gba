/**
 * @file overlay.h
 *
 * @brief Dimmed full-screen layer for text-only menus shown on top of any screen.
 */
#ifndef GAME_OVERLAY_H
#define GAME_OVERLAY_H

#include <stdbool.h>

/**
 * @brief Dim the screen and clear the on-screen text, which is kept to be restored on close.
 *
 * Does nothing if the overlay is already open.
 */
void overlay_open(void);

/**
 * @brief Restore the screen and its text as they were when the overlay was opened.
 *
 * Does nothing if the overlay is not open.
 */
void overlay_close(void);

/**
 * @brief Tell whether every key is released, to call before closing a menu shown on the overlay.
 *
 * WHY: a key still held would reach the game as a held or released input once it resumes
 * (releasing A selects the focused card).
 *
 * @return true if no key is down
 */
bool overlay_are_keys_released(void);

#endif // GAME_OVERLAY_H
