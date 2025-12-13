/**
 * @file font.h
 *
 * @brief Utility file to interact with custom font
 */
#ifndef FONT_H
#define FONT_H

/** @name Decimal Point Fonts
 *  @brief A set of macros to map the fonts decimal-point values (e.g. ".1")
 *  to their replaced characters.
 *
 *  "FP#" -> Font "Point" "number"
 *
 *  This is used at compile time directly with the help of libtonc's XSTR() macro
 *
 *  For example:
 *  ```c
 *  tte_printf("Testing " XSTR(FP0) " Something!");
 *  ```
 *  prints "Testing .0 Something!"
 *
 *  The map is the following:
 *
 *  ```c
 *  '&' == '.0'
 *  '^' == '.1'
 *  '{' == '.2'
 *  '}' == '.3'
 *  '|' == '.4'
 *  '`' == '.5'
 *  '<' == '.6'
 *  '>' == '.7'
 *  '_' == '.8'
 *  ';' == '.9'
 *  ```
 *
 * @{
 */
#define FP0 & // .0
#define FP1 ^ // .1
#define FP2 { // .2
#define FP3 } // .3
#define FP4 | // .4
#define FP5 ` // .5
#define FP6 < // .6
#define FP7 > // .7
#define FP8 _ // .8
#define FP9 ; // .9
/** @} */

/**
 * @brief Get the decimal point string equivalent of 0-9 from integer param.
 *
 * @param val Value between 0-9 to get ".0" to ".9" equivalents. Note that
 *            if a value outside of range is passed it will perform '% 9'
 *            on that value.
 *
 * @return A char* to the string values of '.0' to '.9'
 */
const char* get_font_point_str(int val);

#endif // FONT_H
