/**
 * @file selection_grid.h
 *
 * @brief An implementation for a selection grid that handles directional selection.
 *
 * The selection grid is not the same vertically and horizontally.
 * It is divided into rows and each row defines its own callbacks for size, directional changes,
 * and selection input.
 * The idea is that it would be a more fitting simple solution for this game since it tends
 * to be more dynamic horizontally than vertically with differing sizes of hands, jokers, etc.
 */

#ifndef SELECTION_GRID_H
#define SELECTION_GRID_H

#include <tonc.h>

typedef POINT Selection;

struct SelectionGridRow;
struct SelectionGrid;
typedef struct SelectionGridRow SelectionGridRow;
typedef struct SelectionGrid SelectionGrid;

/**
 * @brief Callback function type for handling selection changes in a SelectionGrid.
 *
 * This function is invoked whenever the selection cursor changes position within
 * the grid. Used to perform custom actions based on selection changes,
 * such as updating UI elements.
 *
 * @param selection_grid Pointer to the SelectionGrid instance where the change occurred
 * @param row_idx Index of the row associated with this callback. This can be used to
 *                determine whether this is the previously selected row or the newly
 *                selected row
 * @param prev_selection Pointer to the Selection state before the change occurred.
 *                       Contains the previous cursor position
 * @param new_selection Pointer to the Selection state after the change occurred.
 *                      Contains the new cursor position
 * @return false if the selection change needs to be aborted, true if can proceed.
 *
 */
typedef bool (*RowOnSelectionChangedFunc)(
    SelectionGrid* selection_grid,
    int row_idx,
    const Selection* prev_selection,
    const Selection* new_selection
);

/**
 * @brief Function pointer type for retrieving the size of a row in a selection grid.
 *
 * This is useful to allow generic row lengths e.g. when it can depend on the number of cards
 * in hand, items in the shop, etc.
 *
 * @return int The number of elements in the row.
 */
typedef int (*RowGetSizeFunc)();

/**
 * @brief Callback function type for handling non-directional key presses in a selection grid row.
 *
 * This function is invoked whenever a non-directional key transitions
 * (either hit down or release up).
 * The specific key and the type of transition
 * (press/release) are not passed as parameters to the callback. Instead, the implementation must
 * query the key state using functions like key_hit(), key_released(), etc. to determine which key
 * triggered the event and its current state.
 *
 * @param selection_grid Pointer to the SelectionGrid that contains the row receiving the key event
 * @param selection Pointer to the Selection (row) that is handling the key transition.
 */
typedef void (*RowOnKeyTransitFunc)(SelectionGrid* selection_grid, Selection* selection);

/**
 * @brief A set of attributes to the selection grid row affecting selection grid behavior.
 */
typedef struct
{

    /**
     * @brief Whether to wrap selection when it passes the end of the row.
     */
    bool wrap;
} SelGridRowAttributes;

/**
 * @brief A single row in the selection grid, defined by its callback functions.
 *
 * @var row_idx is used to easily identify the row within the callbacks.
 */
struct SelectionGridRow
{
    int row_idx;
    RowGetSizeFunc get_size;
    RowOnSelectionChangedFunc on_selection_changed;
    RowOnKeyTransitFunc on_key_transit;
    SelGridRowAttributes attributes;
};

/**
 * @brief The core selection grid struct, represents the grid itself and its state.
 *
 * It can be statically defined with an array of @ref SelectionGridRow to define its
 * contents.
 */
struct SelectionGrid
{
    const SelectionGridRow* rows;
    const int num_rows;
    Selection selection;
};

/**
 * @brief Processes user input for the selection grid.
 *
 * This function handles input events (such as directional controls and button presses)
 * and updates the selection grid's state accordingly. It should be called each frame
 * to respond to user interactions with the grid.
 *
 * @param selection_grid Pointer to the SelectionGrid structure to process input for.
 *                       Must not be NULL. NULL-checks are in place and will return early.
 */
void selection_grid_process_input(SelectionGrid* selection_grid);

/**
 * @brief Moves the selection horizontally within the selection grid.
 *
 * This function updates the current selection position by moving it horizontally
 * based on the specified direction. This can be useful if some non-press event
 * should update the selection grid.
 *
 * @param selection_grid Pointer to the SelectionGrid structure to operate on.
 *                       Must not be NULL. NULL-checks are in place and will return early.
 * @param direction_tribool Direction indicator for horizontal movement behaving like tonc's
 *                          tribools:
 *                          - Negative value: move left
 *                          - Zero: no movement
 *                          - Positive value: move right
 */
void selection_grid_move_selection_horz(SelectionGrid* selection_grid, int direction_tribool);

/**
 * @brief Moves the selection vertically within the selection grid.
 *
 * This function updates the current selection position by moving it vertically
 * based on the specified direction. This can be useful if some non-press event
 * should update the selection grid.
 *
 * @param selection_grid Pointer to the SelectionGrid structure to operate on.
 *                       Must not be NULL. NULL-checks are in place and will return early.
 * @param direction_tribool Direction indicator for vertical movement behaving like tonc's tribools:
 *                          - Negative value: move up
 *                          - Zero: no movement
 *                          - Positive value: move down
 */
void selection_grid_move_selection_vert(SelectionGrid* selection_grid, int direction_tribool);

#endif
