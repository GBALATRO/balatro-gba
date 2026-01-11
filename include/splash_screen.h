/**
 * @file splash_screen.h
 *
 * @brief Functions and constants for the splash screen intro
 *
 */
#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include "game.h"

#include <tonc.h>

/** @name Splash screen timing variables
 *
 *  Constants for timing the duration of the splash screen.
 *
 * @{
 */
#define SPLASH_FPS              60
#define SPLASH_DURATION_SECONDS 10
#define SPLASH_DURATION_FRAMES  (SPLASH_FPS * SPLASH_DURATION_SECONDS)
/** @} */

/**
 * @brief Initialize the splash screen by printing the splash screen text.
 */
void splash_screen_on_init(GameVariables* vars);

/**
 * @brief Update splash screen timers and print the remaining time accordingly.
 */
void splash_screen_on_update(GameVariables* vars);

/**
 * @brief Exit the splash screen
 */
void splash_screen_on_exit(GameVariables* vars);

#endif // SPLASH_SCREEN_H
