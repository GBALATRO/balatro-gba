/**
 * @file util.h
 *
 * @brief Utilities relating around number string representation and protected arithmatic helper
 * functions
 */
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

/**
 * @def GBAL_UNUSED
 * @brief A friendly wrapper around the not so friendly looking __attribute__ syntax for ((unused))
 */
#define GBAL_UNUSED __attribute__((unused))

#define UNDEFINED -1

/**
 * @def NUM_ELEM_IN_ARR
 * @brief Get the number of elements in an array
 *
 * @param arr input array
 */
#define NUM_ELEM_IN_ARR(arr) (sizeof(arr) / sizeof((arr)[0]))

#define INT_MAX_DIGITS   11 // strlen(str(INT_MAX)) = strlen("-2147483647")
#define UINT_MAX_DIGITS  10 // strlen(str(UINT32_MAX)) = strlen("4294967295")
#define UINT8_MAX_DIGITS 3  // strlen(str(UINT8_MAX)) = strlen("255")

#define ONE_K 1000
#define ONE_M 1000000
#define ONE_B 1000000000

#define ONE_K_ZEROS 3
#define ONE_M_ZEROS 6
#define ONE_B_ZEROS 9

// The suffix replaces everything past the third digit, e.g. "999K" -> "1M"
// so it needs at least this number of chars to be able to display any suffixed number
#define SUFFIXED_NUM_MIN_REQ_CHARS 4

/**
 * @brief Avoid overflow when adding two u32 integers
 *
 * @param a left operator **a + b**
 * @param b left operator **a + b**
 *
 * @return the result of **a + b** or **UINT32_MAX** in case of overflow
 */
uint32_t u32_protected_add(uint32_t a, uint32_t b);

/**
 * @brief Avoid overflow when adding two u16 integers
 *
 * @param a left operator **a + b**
 * @param b left operator **a + b**
 *
 * @return the result of **a + b** or **UINT16_MAX** in case of overflow
 */
uint16_t u16_protected_add(uint16_t a, uint16_t b);

/**
 * @brief Avoid overflow when multiplying two u32 integers
 *
 * @param a left operator **a * b**
 * @param b left operator **a * b**
 *
 * @return the result of **a * b** or **UINT32_MAX** in case of overflow
 */
uint32_t u32_protected_mult(uint32_t a, uint32_t b);

/**
 * @brief Avoid overflow when multiplying two u16 integers
 *
 * @param a left operator **a * b**
 * @param b left operator **a * b**
 *
 * @return the result of **a * b** or **UINT16_MAX** in case of overflow
 */
uint16_t u16_protected_mult(uint16_t a, uint16_t b);

/**
 * @brief   Truncate an unsigned number into a suffixed string representation e.g. 12000 -> "12K"
 *          The least significant digits are rounded down e.g. 12345 -> "12K", 12987 -> "12K"
 *
 * @param num           The number to truncate, can be anything from 0 to UINT32_MAX.
 *
 * @param num_req_chars The number of characters to constrain the string to.
 *                      The function will use up as much characters as it can
 *                      in order to maintain as much accuracy as possible.
 *                      So numbers are not fully truncated if not necessary,
 *                      e.g. 123123000 -> "123123K" for example value 7,
 *                      and if num_req_chars > u32_get_digits(num) the number will not
 *                      be truncated at all.
 *                      Passing less than SUFFIXED_NUM_MIN_REQ_CHARS may result in an
 *                      output string longer than num_req_chars but
 *                      can be done to truncate 1000s -> "1K", 2000 -> "2K" etc.
 *                      which wouldn't be otherwise.
 *
 * @param out_str       An output buffer to write the resulting string to.
 *                      Must be of size UINT_MAX_DIGITS + 1. + 1 for null-terminator.
 *                      At that size the suffix character will always be accounted for since
 *                      a number with more digits than UINT_MAX_DIGITS will not be handled nor
 *                      truncated.
 */
void truncate_uint_to_suffixed_str(
    uint32_t num,
    int num_req_chars,
    char out_str_buff[UINT_MAX_DIGITS + 1]
);

/**
 * @brief Get the number of digits in a 32-bit unsigned number
 * https://stackoverflow.com/questions/1068849/how-do-i-determine-the-number-of-digits-of-an-integer-in-c
 *
 * @param n 32-bit unsigned value to find the number of decimal digits of
 *
 * @return the number of digits in a number
 */
static inline int u32_get_digits(uint32_t n)
{
    if (n < 10)
        return 1;
    if (n < 100)
        return 2;
    if (n < 1000)
        return 3;
    if (n < 10000)
        return 4;
    if (n < 100000)
        return 5;
    if (n < 1000000)
        return 6;
    if (n < 10000000)
        return 7;
    if (n < 100000000)
        return 8;
    if (n < 1000000000)
        return 9;
    return 10;
}

#endif // UTIL_H
