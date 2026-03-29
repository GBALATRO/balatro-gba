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
 *
 *  For example:
 *  ```c
 *  tte_printf("Testing " FP0_STR " Something!");
 *  ```
 *  prints "Testing .0 Something!"
 *
 *  The map is the following:
 *
 *  ```c
 *  '&' == '.0'
 *  '^' == '.1'
 *  '}' == '.2'
 *  '{' == '.3'
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
#define FP0_STR "&" // .0
#define FP1_STR "^" // .1
#define FP2_STR "}" // .2
#define FP3_STR "{" // .3
#define FP4_STR "|" // .4
#define FP5_STR "`" // .5
#define FP6_STR "<" // .6
#define FP7_STR ">" // .7
#define FP8_STR "_" // .8
#define FP9_STR ";" // .9
/** @} */

/**
 * @brief Get the decimal point string equivalent of 0-9 from integer param.
 *
 * @param val Value between 0-9 to get ".0" to ".9" equivalents. Note that
 *            if a value outside of range is passed it will perform '% 10'
 *            on that value and the last decimal digit will be used.
 *
 * @return A char* to the string values of '.0' to '.9'
 */
const char* get_font_point_str(int val);

/**
 * @brief Get the decimal point char equivalent of 0-9 from char param.
 *
 * @param digit_char A char of a digit between '0'-'9' to get '.0' to '.9' equivalents. Note that
 *                   if a char outside of range is passed it the return value will be within limits
 *                   but is not defined with any relationship to the input.
 *
 * @return A char representing a value in the font within the range of '.0' to '.9'
 */
char digit_char_to_font_point(char digit_char);

#endif // FONT_H
