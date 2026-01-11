#ifndef BUTTON_H
#define BUTTON_H

#include <tonc_types.h>

#define BTN_HIGHLIGHT_COLOR 0xFFFF

#define BUTTON_SFX_VOLUME 154 // 60% of MM_FULL_VOLUME

/**
 * @brief The function to be called when the button is pressed
 */
typedef void (*ButtonOnPressedFunc)(void);

/**
 * @brief Returns true if the button should be activated when pressed.
 */
typedef bool (*ButtonCanBePressedFunc)(void);

/**
 * @brief A button representation
 */
typedef struct
{
    // Using u8 for the palette indices to make sure they don't overflow pal_bg_mem

    /**
     * @brief Palette index of the button border whose color is updated depending on highlight.
     */
    u8 border_pal_idx;

    /**
     * @brief Palette index of the color of the button itself, used for the border when not
     * highlighted.
     */
    u8 button_pal_idx;

    /**
     * @brief Called when the button is pressed if @ref can_be_pressed, don't call this directly,
     * call @ref button_press() instead.
     * Should not be set to NULL.
     */
    ButtonOnPressedFunc on_pressed;

    /**
     * @brief Returns true if the button should be activated when pressed.
     * Can be NULL if button can always be pressed.
     */
    ButtonCanBePressedFunc can_be_pressed;
} Button;

/**
 * @brief Set the button highlight on or off.
 * @param button The button to modify. No-op on NULL.
 * @param highlight true to highlight, false to unhighlight.
 */
void button_set_highlight(Button* button, bool highlight);

/**
 * @brief Execute a button press, by calling the button's @ref on_pressed function.
 * This checks the button's @ref can_be_pressed function and will do nothing if
 * returns false.
 *
 * Button presses should go through this function rather than directly call @ref on_pressed.
 *
 * @param button The button being pressed. No-op on NULL.
 */
void button_press(Button* button);

#endif // BUTTON_H