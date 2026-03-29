#include "util.h"

#include "font.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int int_arr_max(int int_arr[], int size)
{
    int max = INT_MIN;
    for (int i = 0; i < size; i++)
    {
        if (int_arr[i] > max)
        {
            max = int_arr[i];
        }
    }

    return max;
}

/**
 * @brief Remove trailing zeros from a string.
 *        Currently static single use, may be unstaticed if needed but use with caution.
 *
 * @param num_str The string to remove trailing zeros from, modified in-place.
 * @param size The size of the string - strlen is not used and no checks are performed,
 *             the function relies on the caller to provide the correct size,
 *             if it's larger than the actual string length or negative it will result in
 *             an invalid write.
 */
static inline void num_str_truncate_trailing_zeros(char* num_str, int size)
{
    while (size > 0 && num_str[size - 1] == '0')
    {
        size--;
    }
    num_str[size] = '\0';
}

/**
 * @brief Build a truncated decimal remainder string.
 *        Helper function for truncate_uint_to_suffixed_str()
 *
 * @param decimal_remainder Integer remainder (the `num % divisor`) used to
 *        produce the fractional digits after the decimal point; formatted and
 *        padded before truncation.
 * @param truncated_num Integer part reduced by the divisor (`num / divisor`);
 *        Used to compute how many fractional characters may be kept.
 * @param num_req_chars Total character budget for the final string (truncated
 *        number, fractional digits, and suffix).
 *        Used to compute how many fractional characters may be kept.
 * @param suffix_char One of 'K', 'M', or 'B' used for selecting the suffix scale and
 *        padding width. If not one of the expected, the string may be incorrectly formatted.
 * @param remainder_str Output buffer (size >= UINT_MAX_DIGITS + 1) where the
 *        formatted fractional digits (including leading '.<digit>' special character) are written
 *        as a NULL-terminated string; may be empty if nothing remains.
 */
static inline void truncate_num_get_remainder_string(
    uint32_t decimal_remainder,
    uint32_t truncated_num,
    int num_req_chars,
    char suffix_char,
    char remainder_str[UINT_MAX_DIGITS + 1]
)
{
    // Truncating the remainder in string form rather than number to avoid divisions
    char* remainder_str_format;

    switch (suffix_char)
    {
        // Pad with 0s to not lose leading zeros after decimal point
        case 'B':
            remainder_str_format = "%09lu";
            break;
        case 'M':
            remainder_str_format = "%06lu";
            break;
        case 'K':
            remainder_str_format = "%03lu";
            break;
        default:
            // Should not reach here
            remainder_str_format = "%lu";
    }

    snprintf(remainder_str, UINT_MAX_DIGITS + 1, remainder_str_format, decimal_remainder);

    // Truncate overflow
    int remaining_chars = num_req_chars - u32_get_digits(truncated_num) - 1; // - 1 for suffix

    // If there is no room for any fractional characters, leave the remainder string empty.
    if (remaining_chars <= 0)
    {
        remainder_str[0] = '\0';
        return;
    }

    // Ensure we never write past the end of the buffer.
    if (remaining_chars > UINT_MAX_DIGITS)
    {
        remaining_chars = UINT_MAX_DIGITS;
    }
    remainder_str[remaining_chars] = '\0';

    num_str_truncate_trailing_zeros(remainder_str, remaining_chars);

    if (remainder_str[0] != '\0')
    {
        remainder_str[0] = digit_char_to_font_point(remainder_str[0]);
    }
}

void truncate_uint_to_suffixed_str(
    uint32_t num,
    int num_req_chars,
    char out_str_buff[UINT_MAX_DIGITS + 1]
)
{
    uint32_t truncated_num = num;
    int num_digits = u32_get_digits(num);
    uint32_t decimal_remainder = 0;
    bool overflow = num_digits > num_req_chars;
    char* suffix = "";
    char remainder_str[UINT_MAX_DIGITS + 1];
    remainder_str[0] = '\0';

    if (overflow)
    {
        /* If there is overflow, divide by the next suffixed power of 10
         * to truncate the number back within num_req_chars.
         * UINT32_MAX is in the billions so no need to check larger numbers
         * or perform complex mathematical operations.
         */
        uint32_t divisor = 1;
        if (num >= ONE_B)
        {
            divisor = ONE_B;
            suffix = "B";
        }
        else if (num >= ONE_M)
        {
            divisor = ONE_M;
            suffix = "M";
        }
        else if (num >= ONE_K)
        {
            divisor = ONE_K;
            suffix = "K";
        }

        // The compiler optimizes these into a single operation
        truncated_num = num / divisor;
        decimal_remainder = num % divisor;
    }

    if (suffix[0] != '\0' && decimal_remainder != 0)
    {
        truncate_num_get_remainder_string(
            decimal_remainder,
            truncated_num,
            num_req_chars,
            suffix[0],
            remainder_str
        );
    }

    snprintf(out_str_buff, UINT_MAX_DIGITS + 1, "%lu%s%s", truncated_num, remainder_str, suffix);
}

// Avoid uint overflow when add/multiplying score

uint32_t u32_protected_add(uint32_t a, uint32_t b)
{
    return (a > (UINT32_MAX - b)) ? UINT32_MAX : (a + b);
}

uint16_t u16_protected_add(uint16_t a, uint16_t b)
{
    return (a > (UINT16_MAX - b)) ? UINT16_MAX : (a + b);
}

uint32_t u32_protected_mult(uint32_t a, uint32_t b)
{
    return (a == 0 || b == 0) ? 0 : (a > (UINT32_MAX / b) ? UINT32_MAX : a * b);
}

uint16_t u16_protected_mult(uint16_t a, uint16_t b)
{
    return (a == 0 || b == 0) ? 0 : (a > (UINT16_MAX / b) ? UINT16_MAX : a * b);
}
